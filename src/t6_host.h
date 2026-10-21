#pragma once
// Host services used by the title. Windows retains its native implementation;
// Linux services are implemented in t6_host_linux.cpp using POSIX and SDL3.
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h>
#include <intrin.h>
#else
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <chrono>
#include <thread>
#include <sys/types.h>
#include <unistd.h>
#include <immintrin.h>
using DWORD = uint32_t;
using SHORT = int16_t;
using HMODULE = void*;
using HANDLE = void*;
struct LARGE_INTEGER { int64_t QuadPart; };
struct MEMORY_BASIC_INFORMATION { void* BaseAddress; size_t RegionSize; DWORD State, Protect, Type=0; };
constexpr DWORD MEM_COMMIT=0x1000, PAGE_NOACCESS=1, PAGE_READONLY=2, PAGE_READWRITE=4,
                PAGE_WRITECOPY=8, PAGE_EXECUTE_READ=0x20, PAGE_EXECUTE_READWRITE=0x40, PAGE_GUARD=0x100;
constexpr DWORD MOVEFILE_REPLACE_EXISTING=1;
constexpr int VK_RETURN=13,VK_BACK=8,VK_UP=38,VK_DOWN=40,VK_LEFT=37,VK_RIGHT=39,VK_ADD=107,VK_SUBTRACT=109,
    VK_NUMPAD1=97,VK_NUMPAD2=98,VK_NUMPAD4=100,VK_NUMPAD5=101;
inline uint32_t _byteswap_ulong(uint32_t v) { return __builtin_bswap32(v); }
inline uint16_t _byteswap_ushort(uint16_t v) { return __builtin_bswap16(v); }
inline uint64_t _byteswap_uint64(uint64_t v) { return __builtin_bswap64(v); }
template<size_t N> inline int strcpy_s(char (&dst)[N], const char* src) {
    if (strlen(src)>=N) { dst[0]=0; return 1; } memcpy(dst,src,strlen(src)+1); return 0;
}
inline void* _aligned_malloc(size_t bytes,size_t alignment) { void* p=nullptr; return posix_memalign(&p,alignment,bytes)==0 ? p : nullptr; }
void T6InitLinuxInput();
inline unsigned __popcnt(unsigned v) { return __builtin_popcount(v); }
inline void YieldProcessor() { _mm_pause(); }
inline int fopen_s(FILE** out, const char* path, const char* mode) { *out=std::fopen(path,mode); return *out ? 0 : 1; }
inline void QueryPerformanceFrequency(LARGE_INTEGER* v) { v->QuadPart=1000000000; }
inline void QueryPerformanceCounter(LARGE_INTEGER* v) {
    v->QuadPart=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline uint64_t GetTickCount64() { LARGE_INTEGER v; QueryPerformanceCounter(&v); return v.QuadPart/1000000; }
inline uint32_t GetTickCount() { return uint32_t(GetTickCount64()); }
inline void Sleep(unsigned ms) { if (ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms)); else std::this_thread::yield(); }
inline pid_t GetCurrentProcessId() { return getpid(); }
pid_t GetCurrentThreadId();
[[noreturn]] inline void ExitProcess(unsigned code) { std::_Exit(int(code)); }
[[noreturn]] void ExitThread(unsigned code);
size_t VirtualQuery(const void*, MEMORY_BASIC_INFORMATION*, size_t);
bool VirtualProtect(void*, size_t, DWORD, DWORD*);
bool MoveFileExA(const char*, const char*, DWORD);
bool CopyFileA(const char*, const char*, bool);
HMODULE GetModuleHandleW(const wchar_t* name);
void* GetProcAddress(HMODULE, const char*);
SHORT GetAsyncKeyState(int vk);
bool T6KeyboardFocused();
#endif
