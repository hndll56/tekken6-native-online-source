#pragma once
// The launcher and game use numeric Xbox/Win32 key identifiers as a configuration format.
// These are data values, not calls to the Windows input system.
#include <cstdint>
using UINT = unsigned;
using DWORD = uint32_t;
using WORD = uint16_t;
using BYTE = uint8_t;
using SHORT = int16_t;
constexpr int VK_ESCAPE=27, VK_RETURN=13, VK_BACK=8, VK_LEFT=37, VK_UP=38, VK_RIGHT=39, VK_DOWN=40;
constexpr int VK_PRIOR=33, VK_NEXT=34, VK_END=35, VK_HOME=36, VK_INSERT=45, VK_DELETE=46;
constexpr int VK_DIVIDE=111, VK_NUMLOCK=144, VK_RCONTROL=163, VK_RMENU=165;
constexpr unsigned JOY_RETURNALL=0xff, JOY_RETURNBUTTONS=0x80, JOYERR_NOERROR=0;
constexpr unsigned JOY_POVCENTERED=0xffff, JOY_POVFORWARD=0, JOY_POVRIGHT=9000, JOY_POVBACKWARD=18000, JOY_POVLEFT=27000;
struct JOYINFOEX { unsigned dwSize, dwFlags; DWORD dwXpos{},dwYpos{},dwZpos{},dwRpos{},dwUpos{},dwVpos{},dwButtons{},dwPOV{JOY_POVCENTERED}; };
struct JOYCAPSW { wchar_t szPname[256]{}; DWORD wXmin{},wXmax{65535},wYmin{},wYmax{65535},wZmin{},wZmax{65535},wRmin{},wRmax{65535},wUmin{},wUmax{65535},wVmin{},wVmax{65535}; };
unsigned joyGetNumDevs();
unsigned joyGetPosEx(unsigned, JOYINFOEX*);
unsigned joyGetDevCapsW(unsigned, JOYCAPSW*, unsigned);
bool linuxGamepadDown(int kind);
