#include "t6_watch_internal.h"
#include "t6_fault_linux.h"
#include <signal.h>
#include <sys/mman.h>
namespace t6w { uint32_t CwGuestFunc(uintptr_t); }
namespace {
thread_local void* rearm=nullptr;
struct Record { std::atomic<int> state{0}; uintptr_t pc{},addr{}; bool write{}; };
Record records[2048];
std::atomic<uintptr_t> watchLo{0}, watchHi{0};
bool researchFault(int sig,uintptr_t pc,uintptr_t addr,uintptr_t& flags,bool write) {
    if(sig==SIGTRAP && rearm) {
        if(mprotect(rearm,4096,PROT_NONE)) return false;
        rearm=nullptr; flags&=~uintptr_t(0x100); return true;
    }
    uintptr_t lo=watchLo.load(std::memory_order_acquire),hi=watchHi.load(std::memory_order_acquire);
    if(sig!=SIGSEGV || !lo || addr<lo || addr>=hi) return false;
    void* page=reinterpret_cast<void*>(addr&~uintptr_t(4095));
    if(mprotect(page,4096,PROT_READ|PROT_WRITE)) return false;
    rearm=page; flags|=0x100;
    for(auto& record:records) { int empty=0;
        if(record.state.compare_exchange_strong(empty,1)) {
            record.pc=pc; record.addr=addr; record.write=write; record.state.store(2,std::memory_order_release); break;
        }
    }
    return true;
}
}
namespace t6w {
void T6LinuxWatchStart() {
    watchHi.store(reinterpret_cast<uintptr_t>(g_cw.hi));
    watchLo.store(reinterpret_cast<uintptr_t>(g_cw.lo),std::memory_order_release);
    T6SetLinuxResearchFault(researchFault);
}
void T6LinuxWatchStop() { watchLo.store(0,std::memory_order_release); }
}
void T6FlushLinuxResearch() {
    // Allocation, formatting and locks happen here on the ordinary guest thread, never in a signal handler.
    for(auto& record:records) if(record.state.load(std::memory_order_acquire)==2) {
        if(g_cw.rec && record.addr>=reinterpret_cast<uintptr_t>(g_cw.rec) && record.addr<reinterpret_cast<uintptr_t>(g_cw.rec)+g_cw.len) {
            uint32_t fn=CwGuestFunc(record.pc),off=uint32_t(record.addr-reinterpret_cast<uintptr_t>(g_cw.rec));
            std::lock_guard lock(g_cw.m);
            if(g_cw.seen.insert({fn,g_cw.len>80?off&~15u:off}).second && g_cw.logged++<4000)
                REXLOG_INFO("T6 custom watch: record {:08X} +{} {} by sub_{:08X} (host pc {:X})",g_cw.guest,off,record.write?"WRITE":"read",fn,record.pc);
        }
        record.state.store(0,std::memory_order_release);
    }
}
