#include "common.h"
#include <SDL.h>
namespace {
struct Inputs {
    Inputs() { SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1"); SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER); }
    ~Inputs() { SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER); }
};
void update() { static Inputs inputs; SDL_PumpEvents(); SDL_JoystickUpdate(); }
}
unsigned joyGetNumDevs() { update(); return std::max(0, SDL_NumJoysticks()); }
unsigned joyGetPosEx(unsigned id, JOYINFOEX* p) {
    update();
    SDL_Joystick* joy = SDL_JoystickOpen(int(id));
    if (!joy) return 1;
    DWORD* axes[] = {&p->dwXpos,&p->dwYpos,&p->dwZpos,&p->dwRpos,&p->dwUpos,&p->dwVpos};
    for (int i=0; i<6; ++i) *axes[i] = i < SDL_JoystickNumAxes(joy) ? int(SDL_JoystickGetAxis(joy,i))+32768 : 32768;
    p->dwButtons = 0;
    for (int i=0; i<std::min(32, SDL_JoystickNumButtons(joy)); ++i) if (SDL_JoystickGetButton(joy,i)) p->dwButtons |= 1u << i;
    auto hat = SDL_JoystickGetHat(joy,0);
    switch(hat) {
    case SDL_HAT_UP: p->dwPOV=0; break; case SDL_HAT_RIGHTUP: p->dwPOV=4500; break;
    case SDL_HAT_RIGHT: p->dwPOV=9000; break; case SDL_HAT_RIGHTDOWN: p->dwPOV=13500; break;
    case SDL_HAT_DOWN: p->dwPOV=18000; break; case SDL_HAT_LEFTDOWN: p->dwPOV=22500; break;
    case SDL_HAT_LEFT: p->dwPOV=27000; break; case SDL_HAT_LEFTUP: p->dwPOV=31500; break;
    default: p->dwPOV=JOY_POVCENTERED;
    }
    SDL_JoystickClose(joy); return 0;
}
unsigned joyGetDevCapsW(unsigned id, JOYCAPSW* p, unsigned) {
    update(); const char* name = SDL_JoystickNameForIndex(int(id));
    if (!name) return 1;
    QString::fromUtf8(name).left(255).toWCharArray(p->szPname); return 0;
}
bool linuxGamepadDown(int kind) {
    update();
    for (int i=0; i<SDL_NumJoysticks(); ++i) {
        auto* pad = SDL_GameControllerOpen(i); if (!pad) continue;
        bool down = SDL_GameControllerGetButton(pad, kind==1 ? SDL_CONTROLLER_BUTTON_B : kind==2 ? SDL_CONTROLLER_BUTTON_Y : SDL_CONTROLLER_BUTTON_A);
        if (kind==0) down |= SDL_GameControllerGetButton(pad,SDL_CONTROLLER_BUTTON_START);
        SDL_GameControllerClose(pad); if (down) return true;
    }
    return false;
}
