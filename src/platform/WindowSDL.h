#pragma once
#include <SDL.h>
#include <string>
#include <functional>

class WindowSDL {
public:
	bool create(const std::string& title, int w, int h, bool fullscreen);
	void pollEvents(bool& running);
	void destroy();

	SDL_Window* sdl() const { return window_; }

	int width()  const { return w_; }
	int height() const { return h_; }

	bool wasResized() const { return resized_; }
	void resetResizedFlag() { resized_ = false; }

	bool consumeToggleFullscreenRequested();
	bool toggleFullscreen();
	bool isFullscreen() const { return fullscreen_; }

	void pollEvents(bool& running, const std::function<void(const SDL_Event&)>& onEvent);

private:
	void updateSizeFromWindow();

	SDL_Window* window_{ nullptr };
	int w_{ 0 }, h_{ 0 };
	bool resized_{ false };

	bool fullscreen_{ false };
	bool toggleFsRequested_{ false };

	int windowedW_{ 1280 }, windowedH_{ 720 };
};
