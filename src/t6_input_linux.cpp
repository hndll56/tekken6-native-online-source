#include "t6_host.h"
#include <SDL3/SDL.h>
#include <array>
namespace {
std::array<std::atomic<unsigned>,256> keys{};
std::atomic<bool> focused{false};
std::array<bool,SDL_SCANCODE_COUNT> physical{};
std::array<unsigned,256> held{};
int virtualKey(SDL_Scancode s) {
    if (s>=SDL_SCANCODE_A && s<=SDL_SCANCODE_Z) return 'A'+s-SDL_SCANCODE_A;
    if (s>=SDL_SCANCODE_1 && s<=SDL_SCANCODE_9) return '1'+s-SDL_SCANCODE_1;
    if (s>=SDL_SCANCODE_KP_1 && s<=SDL_SCANCODE_KP_9) return 97+s-SDL_SCANCODE_KP_1;
    if (s>=SDL_SCANCODE_F1 && s<=SDL_SCANCODE_F12) return 112+s-SDL_SCANCODE_F1;
    if (s>=SDL_SCANCODE_F13 && s<=SDL_SCANCODE_F24) return 124+s-SDL_SCANCODE_F13;
    switch (s) {
    case SDL_SCANCODE_0:return '0';case SDL_SCANCODE_RETURN:case SDL_SCANCODE_KP_ENTER:return 13;
    case SDL_SCANCODE_BACKSPACE:return 8;case SDL_SCANCODE_SPACE:return 32;case SDL_SCANCODE_ESCAPE:return 27;
    case SDL_SCANCODE_TAB:return 9;case SDL_SCANCODE_UP:return 38;case SDL_SCANCODE_DOWN:return 40;
    case SDL_SCANCODE_LEFT:return 37;case SDL_SCANCODE_RIGHT:return 39;case SDL_SCANCODE_KP_PLUS:return 107;
    case SDL_SCANCODE_KP_MINUS:return 109;case SDL_SCANCODE_KP_0:return 96;case SDL_SCANCODE_HOME:return 36;
    case SDL_SCANCODE_END:return 35;case SDL_SCANCODE_PAGEUP:return 33;case SDL_SCANCODE_PAGEDOWN:return 34;
    case SDL_SCANCODE_INSERT:return 45;case SDL_SCANCODE_DELETE:return 46;
    case SDL_SCANCODE_LSHIFT:case SDL_SCANCODE_RSHIFT:return 16;
    case SDL_SCANCODE_LCTRL:case SDL_SCANCODE_RCTRL:return 17;
    case SDL_SCANCODE_LALT:case SDL_SCANCODE_RALT:return 18;
    case SDL_SCANCODE_CAPSLOCK:return 20;case SDL_SCANCODE_SEMICOLON:return 186;
    case SDL_SCANCODE_EQUALS:return 187;case SDL_SCANCODE_COMMA:return 188;
    case SDL_SCANCODE_MINUS:return 189;case SDL_SCANCODE_PERIOD:return 190;
    case SDL_SCANCODE_SLASH:return 191;case SDL_SCANCODE_GRAVE:return 192;
    case SDL_SCANCODE_LEFTBRACKET:return 219;case SDL_SCANCODE_BACKSLASH:return 220;
    case SDL_SCANCODE_RIGHTBRACKET:return 221;case SDL_SCANCODE_APOSTROPHE:return 222;
    case SDL_SCANCODE_KP_MULTIPLY:return 106;case SDL_SCANCODE_KP_DIVIDE:return 111;
    case SDL_SCANCODE_KP_PERIOD:return 110;default:return 0;
    }
}
bool eventWatch(void*, SDL_Event* e) {
    if(e->type==SDL_EVENT_WINDOW_FOCUS_GAINED) focused=true;
    if(e->type==SDL_EVENT_WINDOW_FOCUS_LOST) { focused=false; for(auto& key:keys) key.store(0); physical.fill(false); held.fill(0); }
    if(e->type==SDL_EVENT_KEY_DOWN || e->type==SDL_EVENT_KEY_UP) {
        int vk=virtualKey(e->key.scancode);
        if(vk && unsigned(e->key.scancode)<physical.size()) {
            bool& down=physical[e->key.scancode];
            if(e->key.down && !down) { down=true; ++held[vk]; keys[vk].fetch_or(0x8001); }
            else if(!e->key.down && down) { down=false; if(held[vk]) --held[vk]; if(!held[vk]) keys[vk].fetch_and(1); }
        }
    }
    return true;
}
}
void T6InitLinuxInput() { SDL_AddEventWatch(eventWatch,nullptr); focused=SDL_GetKeyboardFocus()!=nullptr; }
SHORT GetAsyncKeyState(int vk) { return static_cast<SHORT>(keys[unsigned(vk)&255].fetch_and(0x8000)); }
bool T6KeyboardFocused() { return focused.load(); }
