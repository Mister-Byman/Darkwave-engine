#include "platform/WindowSDL.h"
#include <SDL_vulkan.h>
#include <iostream>

bool WindowSDL::create(const std::string& title, int w, int h, bool fullscreen) {
    w_ = w;
    h_ = h;
    fullscreen_ = fullscreen;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    Uint32 flags = SDL_WINDOW_VULKAN;
    flags |= fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE;

    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w, h,
        flags
    );

    if (!window_) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    updateSizeFromWindow();
    resized_ = true;
    return true;
}

void WindowSDL::pollEvents(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;

        if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
            if (e.key.keysym.sym == SDLK_F11) {
                toggleFsRequested_ = true;
            }
        }

        if (e.type == SDL_WINDOWEVENT) {
            if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                // размер окна мог измениться — обновим drawable size под Vulkan
                updateSizeFromWindow();
            }
            if (e.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
        }
    }
}

void WindowSDL::destroy() {
    if (window_) SDL_DestroyWindow(window_);
    window_ = nullptr;
    SDL_Quit();
}

bool WindowSDL::consumeToggleFullscreenRequested() {
    if (!toggleFsRequested_) return false;
    toggleFsRequested_ = false;
    return true;
}

void WindowSDL::updateSizeFromWindow() {
    if (!window_) return;

    int w = 0, h = 0;
    SDL_Vulkan_GetDrawableSize(window_, &w, &h);
    if (w > 0 && h > 0) {
        w_ = w;
        h_ = h;
    }
    else {
        SDL_GetWindowSize(window_, &w_, &h_);
    }

    resized_ = true;
}

bool WindowSDL::toggleFullscreen() {
    if (!window_) return false;

    if (!fullscreen_) {
        // запомнить текущий оконный размер
        SDL_GetWindowSize(window_, &windowedW_, &windowedH_);

        int display = SDL_GetWindowDisplayIndex(window_);
        if (display < 0) display = 0;

        SDL_DisplayMode mode{};
        if (SDL_GetDesktopDisplayMode(display, &mode) == 0) {
            SDL_SetWindowDisplayMode(window_, &mode);
        }

        if (SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN) != 0) {
            std::cerr << "SDL_SetWindowFullscreen ON failed: " << SDL_GetError() << "\n";
            return false;
        }

        fullscreen_ = true;
        updateSizeFromWindow();
        return true;
    }
    else {
        if (SDL_SetWindowFullscreen(window_, 0) != 0) {
            std::cerr << "SDL_SetWindowFullscreen OFF failed: " << SDL_GetError() << "\n";
            return false;
        }

        fullscreen_ = false;

        SDL_SetWindowSize(window_, windowedW_, windowedH_);
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        updateSizeFromWindow();
        return true;
    }
}

void WindowSDL::pollEvents(bool& running, const std::function<void(const SDL_Event&)>& onEvent) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        onEvent(e);

        if (e.type == SDL_QUIT) running = false;

        if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
            if (e.key.keysym.sym == SDLK_F11) toggleFsRequested_ = true;
            if (e.key.keysym.sym == SDLK_ESCAPE) running = false; // удобно для теста
        }

        if (e.type == SDL_WINDOWEVENT) {
            if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) updateSizeFromWindow();
            if (e.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
        }
    }
}
