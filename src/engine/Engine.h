#pragma once
#include "platform/WindowSDL.h"
#include "renderer/VulkanContext.h"
#include "renderer/Renderer.h"

#include "core/Time.h"
#include "core/Input.h"

#include "game/CameraFPS.h"
#include "game/Player.h"

class Engine {
public:
	bool init();
	void run();
	void shutdown();

private:
	bool running_{ false };

	WindowSDL window_;
	VulkanContext vk_;
	Renderer renderer_;


	Time time_;
	Input input_;
	CameraFPS cam_;
	Player player_;
};
