#include "core/Input.h"
#include <cstring>

void Input::beginFrame() {
    mouse_.dx = 0;
    mouse_.dy = 0;

    // Обновляем состояния клавиатуры для pressed/released
    const Uint8* cur = SDL_GetKeyboardState(nullptr);

    if (!kbNow_) {
        kbNow_ = cur;
        std::memcpy(kbPrev_, kbNow_, SDL_NUM_SCANCODES);
    }
    else {
        std::memcpy(kbPrev_, kbNow_, SDL_NUM_SCANCODES);
        kbNow_ = cur;
    }
}

void Input::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEMOTION) {
        mouse_.dx += e.motion.xrel;
        mouse_.dy += e.motion.yrel;
    }
    if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
        bool down = (e.type == SDL_MOUSEBUTTONDOWN);
        if (e.button.button == SDL_BUTTON_LEFT)  mouse_.left = down;
        if (e.button.button == SDL_BUTTON_RIGHT) mouse_.right = down;
    }
}

bool Input::keyDown(SDL_Scancode sc) const {
    const Uint8* k = kbNow_ ? kbNow_ : SDL_GetKeyboardState(nullptr);
    return k && (k[sc] != 0);
}

void Input::setRelativeMouse(bool enable) {
    SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE);
    SDL_ShowCursor(enable ? SDL_DISABLE : SDL_ENABLE);
}

bool Input::isRelativeMouse() const {
    return SDL_GetRelativeMouseMode() == SDL_TRUE;
}

bool Input::keyPressed(SDL_Scancode sc) const {
    if (!kbNow_) return false;
    return (kbNow_[sc] != 0) && (kbPrev_[sc] == 0);
}

bool Input::keyReleased(SDL_Scancode sc) const {
    if (!kbNow_) return false;
    return (kbNow_[sc] == 0) && (kbPrev_[sc] != 0);
}