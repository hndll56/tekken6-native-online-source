#include "t6_host.h"
#include <sys/mman.h>
#include <sys/syscall.h>
#include <dlfcn.h>
#include <filesystem>
#include <cerrno>
#include <limits>
#include <pthread.h>

pid_t GetCurrentThreadId() { return static_cast<pid_t>(syscall(SYS_gettid)); }
void ExitThread(unsigned code) { pthread_exit(reinterpret_cast<void*>(uintptr_t(code))); __builtin_unreachable(); }
size_t VirtualQuery(const void* address, MEMORY_BASIC_INFORMATION* out, size_t size) {
    if (size < sizeof(*out)) return 0;
    FILE* f=fopen("/proc/self/maps","r"); if (!f) return 0;
    char line[4096], perms[5]; unsigned long lo,hi; uintptr_t p=reinterpret_cast<uintptr_t>(address);
    bool found=false;
    while (fgets(line,sizeof(line),f)) {
        if (sscanf(line,"%lx-%lx %4s",&lo,&hi,perms)!=3 || p>=hi) continue;
        if (p<lo) { *out={reinterpret_cast<void*>(p),lo-p,0,PAGE_NOACCESS}; }
        else {
            DWORD flags=perms[0]=='r' ? (perms[1]=='w' ? (perms[2]=='x' ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE) : (perms[2]=='x' ? PAGE_EXECUTE_READ : PAGE_READONLY)) : PAGE_NOACCESS;
            *out={reinterpret_cast<void*>(lo),hi-lo,flags==PAGE_NOACCESS ? 0u : MEM_COMMIT,flags};
        }
        found=true; break;
    }
    fclose(f); return found ? sizeof(*out) : 0;
}
bool VirtualProtect(void* ptr, size_t n, DWORD access, DWORD* old) {
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(ptr,&info,sizeof(info))) return false;
    if (old) *old=info.Protect;
    int prot=PROT_NONE;
    if (access==PAGE_READWRITE || access==PAGE_EXECUTE_READWRITE) prot=PROT_READ|PROT_WRITE;
    else if (access==PAGE_READONLY || access==PAGE_EXECUTE_READ) prot=PROT_READ;
    if (access==PAGE_EXECUTE_READ || access==PAGE_EXECUTE_READWRITE) prot|=PROT_EXEC;
    uintptr_t p=reinterpret_cast<uintptr_t>(ptr), page=uintptr_t(sysconf(_SC_PAGESIZE));
    uintptr_t first=p & ~(page-1), end=(p+n+page-1)&~(page-1);
    return mprotect(reinterpret_cast<void*>(first),end-first,prot)==0;
}
bool MoveFileExA(const char* from,const char* to,DWORD) { return rename(from,to)==0; }
bool CopyFileA(const char* from,const char* to,bool exclusive) {
    std::error_code ec; return std::filesystem::copy_file(from,to,exclusive ? std::filesystem::copy_options::none : std::filesystem::copy_options::overwrite_existing,ec);
}
HMODULE GetModuleHandleW(const wchar_t* name) {
    if (!name) { static void* self=dlopen(nullptr,RTLD_NOW); return self; }
    // dlopen(NOLOAD) never creates a second runtime. Hooks must come from the already loaded SDK.
    if (wcscmp(name,L"rexruntime.dll")==0) return dlopen("librexruntime.so",RTLD_NOW|RTLD_NOLOAD);
    if (wcscmp(name,L"rexgpu-xenos.dll")==0) return dlopen("librexgpu-xenos.so",RTLD_NOW|RTLD_NOLOAD);
    return nullptr;
}
void* GetProcAddress(HMODULE module,const char* name) { return module ? dlsym(module,name) : nullptr; }
