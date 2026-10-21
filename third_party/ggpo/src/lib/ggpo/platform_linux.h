#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <chrono>
#include <thread>
using SOCKET = int;
using HANDLE = int; // descriptors for Poll::RegisterHandle
constexpr int INVALID_SOCKET = -1, SOCKET_ERROR = -1, INFINITE = -1;
constexpr int WSAEWOULDBLOCK = EWOULDBLOCK;
constexpr bool FALSE = false;
inline int WSAGetLastError() { return errno; }
inline int closesocket(int fd) { return close(fd); }
inline int ioctlsocket(int fd, unsigned long op, unsigned long* value) { return ioctl(fd, op, value); }
inline void Sleep(unsigned ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
inline void OutputDebugStringA(const char* s) { fputs(s, stderr); }
inline int CreateDirectoryA(const char* s, void*) { return mkdir(s, 0700) == 0; }
#define sprintf_s snprintf
#define vsprintf_s vsnprintf
inline int fopen_s(FILE** out, const char* path, const char* mode) { *out = fopen(path, mode); return *out ? 0 : errno; }
template<size_t N> int strcpy_s(char (&dst)[N], const char* src) {
    if (strlen(src) >= N) { dst[0] = 0; return ERANGE; }
    memcpy(dst, src, strlen(src) + 1); return 0;
}
class Platform {
public:
    using ProcessID = pid_t;
    static ProcessID GetProcessID() { return getpid(); }
    static uint32 GetCurrentTimeMS() {
        return static_cast<uint32>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }
    static void AssertFailed(char* msg) {
        fputs(msg, stderr);
        if (FILE* f = fopen("ggpo_assert.txt", "a")) { fputs(msg, f); fputc('\n', f); fclose(f); }
    }
    static int GetConfigInt(const char* name);
    static bool GetConfigBool(const char* name);
};
