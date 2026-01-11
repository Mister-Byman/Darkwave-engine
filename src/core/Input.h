#pragma once
#include <SDL.h>

struct MouseState {
	int dx = 0;
	int dy = 0;
	bool right = false;
	bool left = false;
};

class Input {
public:
	void beginFrame();                 // сбросить dx/dy
	void handleEvent(const SDL_Event& e);

	bool keyDown(SDL_Scancode sc) const;
	const MouseState& mouse() const { return mouse_; }

	void setRelativeMouse(bool enable);
	bool isRelativeMouse() const;

	bool keyPressed(SDL_Scancode sc) const;   // нажато в этом кадре
	bool keyReleased(SDL_Scancode sc) const;  // отпущено в этом кадре (на будущее)

private:
	MouseState mouse_;
	const uint8_t* kbNow_ = nullptr;
	uint8_t kbPrev_[SDL_NUM_SCANCODES] = {};
};
