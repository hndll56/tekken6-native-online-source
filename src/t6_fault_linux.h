#pragma once
#ifndef _WIN32
#include <cstdint>
using T6FaultCallback = bool(*)(int signal, uintptr_t pc, uintptr_t address, uintptr_t& flags, bool write);
void T6InstallLinuxFaults();
void T6SetLinuxResearchFault(T6FaultCallback);
void T6FlushLinuxResearch();
#endif
