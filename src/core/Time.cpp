#include "core/Time.h"
#include <SDL.h>
#include <algorithm>

void Time::start() {
	freq_ = (uint64_t)SDL_GetPerformanceFrequency();
	last_ = (uint64_t)SDL_GetPerformanceCounter();
	dt_ = 0.0f;
	total_ = 0.0;
}

void Time::tick() {
	const uint64_t now = (uint64_t)SDL_GetPerformanceCounter();
	const uint64_t diff = now - last_;
	last_ = now;

	double dt = (double)diff / (double)freq_;
	// защита от скачков (alt-tab, breakpoint)
	dt = std::clamp(dt, 0.0, 0.1);
	dt_ = (float)dt;
	total_ += dt;
}
