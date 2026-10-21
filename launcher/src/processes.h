#pragma once
// Tekken 6 Native Online launcher - Game processes: ended together with the launcher, leftovers of an earlier launcher.

#include "common.h"
#ifndef _WIN32
#include <sys/prctl.h>
#include <signal.h>
#include <unistd.h>
inline void prepareLinuxChild(QProcess* p) {
    const pid_t parent = getpid();
    p->setChildProcessModifier([parent] {
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || getppid() != parent) _exit(127);
    });
}
#endif

void endWithLauncher(qint64 pid);

// At start (one launcher per PC): a game of this package's tekken6.exe that still runs was left by a launcher that went
// away (before endWithLauncher) - it would hold the game's UDP port. Returns how many were ended.
inline int g_leftoverGames = 0;
int endLeftoverGames();
