#include "t6_host.h"
#include <sys/mman.h>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <unistd.h>
int main() {
    LARGE_INTEGER frequency,a,b; QueryPerformanceFrequency(&frequency); QueryPerformanceCounter(&a); Sleep(2); QueryPerformanceCounter(&b);
    if(frequency.QuadPart!=1000000000 || b.QuadPart<=a.QuadPart) return 1;
    auto* page=static_cast<char*>(mmap(nullptr,8192,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    if(page==MAP_FAILED) return 2;
    MEMORY_BASIC_INFORMATION info{}; if(!VirtualQuery(page,&info,sizeof(info)) || info.State!=MEM_COMMIT || info.Protect!=PAGE_READWRITE) return 3;
    DWORD old; if(!VirtualProtect(page,4096,PAGE_READONLY,&old) || old!=PAGE_READWRITE) return 4;
    if(!VirtualQuery(page,&info,sizeof(info)) || info.Protect!=PAGE_READONLY) return 5;
    if(!VirtualProtect(page,4096,PAGE_READWRITE,&old) || old!=PAGE_READONLY) return 6;
    page[0]=42;
    if(!VirtualProtect(page+4096,4096,PAGE_NOACCESS,&old)) return 7;
    if(!VirtualQuery(page+4096,&info,sizeof(info)) || info.Protect!=PAGE_NOACCESS) return 8;
    munmap(page,8192);
    char directory[]="/tmp/t6-host-XXXXXX"; if(!mkdtemp(directory)) return 9;
    std::filesystem::path dir(directory); auto from=dir/"unicode-試験.tmp",to=dir/"status.txt";
    std::ofstream(from)<<"new"; std::ofstream(to)<<"old";
    if(!MoveFileExA(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING)) return 10;
    std::string value; std::ifstream(to)>>value; if(value!="new") return 11;
    if(CopyFileA(to.c_str(),to.c_str(),true)) return 12;
    if(!CopyFileA(to.c_str(),from.c_str(),true)) return 13;
    if(!GetProcAddress(GetModuleHandleW(nullptr),"malloc")) return 14;
    std::filesystem::remove_all(dir);
    puts("native timers, mmap protection/query, atomic UTF-8 rename, exclusive copy, dlsym: PASS");
}
