#include "t6_host.h"
#include "t6_common.h"
#include "t6_fault_linux.h"
#include <signal.h>
#include <ucontext.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <array>
namespace {
std::atomic<T6FaultCallback> research{nullptr};
struct sigaction oldSegv{},oldTrap{};
int crashFd=-1;
void (*guestContext)()=nullptr;
void appendHex(char*& to,uintptr_t value) {
    for(int bit=60;bit>=0;bit-=4) *to++="0123456789abcdef"[(value>>bit)&15];
}
void dispatch(int sig,siginfo_t* info,void* raw) {
    auto* c=static_cast<ucontext_t*>(raw);
    uintptr_t flags=c->uc_mcontext.gregs[REG_EFL];
    uintptr_t pc=c->uc_mcontext.gregs[REG_RIP];
    uintptr_t addr=reinterpret_cast<uintptr_t>(info->si_addr);
    bool writeAccess=(c->uc_mcontext.gregs[REG_ERR]&2)!=0;
    if(auto callback=research.load(std::memory_order_acquire); callback && callback(sig,pc,addr,flags,writeAccess)) {
        c->uc_mcontext.gregs[REG_EFL]=static_cast<greg_t>(flags); return;
    }
    // Keep the SDK's memory-fault/signal handling chain intact.
    auto& previous=sig==SIGSEGV?oldSegv:oldTrap;
    if(previous.sa_handler!=SIG_DFL && previous.sa_handler!=SIG_IGN) {
        if(previous.sa_flags&SA_SIGINFO) previous.sa_sigaction(sig,info,raw);
        else previous.sa_handler(sig);
        return;
    }
    if(crashFd>=0) {
        char message[128]="Linux fault: host pc="; char* end=message+21;
        appendHex(end,pc); *end++=' '; *end++='a'; *end++='d'; *end++='d'; *end++='r'; *end++='='; appendHex(end,addr); *end++='\n';
        ::write(crashFd,message,size_t(end-message));
    }
    sigaction(sig,&previous,nullptr);
    // Returning retries the fault and preserves normal Linux core dumps/debugger behavior.
}
}
void T6SetLinuxResearchFault(T6FaultCallback callback) { research.store(callback,std::memory_order_release); }
void T6InstallLinuxFaults() {
    static bool installed=false; if(installed) return; installed=true;
    crashFd=open(T6DumpPath("crash_linux.txt").c_str(),O_WRONLY|O_CREAT|O_APPEND|O_CLOEXEC,0600);
    struct sigaction action{}; action.sa_flags=SA_SIGINFO|SA_RESTART; action.sa_sigaction=dispatch; sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV,&action,&oldSegv); sigaction(SIGTRAP,&action,&oldTrap);
}
