#include "t6_netplay_internal.h"
#include <signal.h>
#include <pthread.h>
#include <ucontext.h>
#include <dlfcn.h>
namespace t6np {
std::atomic<HANDLE> g_prof_main{nullptr};
namespace {
std::atomic<uintptr_t> sampledPc{0};
void onSample(int, siginfo_t*,void* raw) {
    auto* context=static_cast<ucontext_t*>(raw);
    sampledPc.store(static_cast<uintptr_t>(context->uc_mcontext.gregs[REG_RIP]), std::memory_order_release);
}
}
HANDLE T6LinuxProfileTarget() {
    struct sigaction action{}, old{}; action.sa_flags=SA_SIGINFO|SA_RESTART; action.sa_sigaction=onSample;
    sigemptyset(&action.sa_mask);
    if(sigaction(SIGRTMIN+6,nullptr,&old) || old.sa_handler!=SIG_DFL) throw std::runtime_error("profiling signal is already in use");
    if(sigaction(SIGRTMIN+6,&action,nullptr)) throw std::runtime_error("cannot install profiling signal");
    return reinterpret_cast<HANDLE>(pthread_self());
}
void ProfileThread() {
    std::vector<std::pair<uintptr_t,uint32_t>> functions;
    for(const PPCFuncMapping* m=PPCFuncMappings;m->guest;++m) functions.emplace_back(reinterpret_cast<uintptr_t>(m->host),static_cast<uint32_t>(m->guest));
    std::sort(functions.begin(),functions.end());
    std::map<std::string,uint64_t> counts[2]; uint64_t totals[2]{};
    double next=T6NowMs()+30000;
    for(;;) {
        Sleep(1); sampledPc=0;
        auto target=reinterpret_cast<pthread_t>(g_prof_main.load()); if(!target) continue;
        if(pthread_kill(target,SIGRTMIN+6)) return;
        for(int wait=0;wait<100 && !sampledPc.load(std::memory_order_acquire);++wait) std::this_thread::yield();
        uintptr_t pc=sampledPc.load(std::memory_order_acquire); if(!pc) continue;
        auto it=std::upper_bound(functions.begin(),functions.end(),std::pair{pc,UINT32_MAX});
        std::string name;
        if(it!=functions.begin() && pc-std::prev(it)->first<0x40000) name=fmt::format("sub_{:08X}",std::prev(it)->second);
        else { Dl_info info{}; name=dladdr(reinterpret_cast<void*>(pc),&info) && info.dli_fname ? info.dli_fname : "unknown"; }
        int mode=g_net_resim ? 1 : 0; counts[mode][name]++; totals[mode]++;
        if(T6NowMs()<next) continue;
        next=T6NowMs()+30000;
        for(int k=0;k<2;++k) {
            std::vector<std::pair<uint64_t,std::string>> rows;
            for(auto& [name,count]:counts[k]) rows.emplace_back(count,name);
            std::sort(rows.rbegin(),rows.rend()); std::string text;
            for(size_t i=0;i<rows.size() && i<30;++i) text+=fmt::format(" {}={:.1f}%",rows[i].second,100.0*rows[i].first/totals[k]);
            REXLOG_INFO("T6 profile {} ({} samples):{}",k?"re-simulated":"live",totals[k],text);
        }
    }
}
}
