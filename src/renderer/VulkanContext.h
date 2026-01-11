#pragma once
#include <vulkan/vulkan.h>
#include <SDL.h>

class VulkanContext {
public:
	bool init(SDL_Window* window);
	void shutdown();

	VkInstance instance() const { return instance_; }
	VkPhysicalDevice physicalDevice() const { return phys_; }
	VkDevice device() const { return device_; }
	VkSurfaceKHR surface() const { return surface_; }
	uint32_t graphicsQueueFamily() const { return graphicsQF_; }
	uint32_t presentQueueFamily() const { return presentQF_; }
	VkQueue graphicsQueue() const { return graphicsQueue_; }
	VkQueue presentQueue() const { return presentQueue_; }

private:
	bool createInstance(SDL_Window* window);
	bool createSurface(SDL_Window* window);
	bool pickPhysicalDevice();
	bool createDevice();

	VkInstance instance_{ VK_NULL_HANDLE };
	VkSurfaceKHR surface_{ VK_NULL_HANDLE };

	VkPhysicalDevice phys_{ VK_NULL_HANDLE };
	VkDevice device_{ VK_NULL_HANDLE };

	uint32_t graphicsQF_{ UINT32_MAX };
	uint32_t presentQF_{ UINT32_MAX };

	VkQueue graphicsQueue_{ VK_NULL_HANDLE };
	VkQueue presentQueue_{ VK_NULL_HANDLE };
};
