#pragma once
#include "renderer/VulkanContext.h"
#include "renderer/Swapchain.h"
#include <vulkan/vulkan.h>
#include "math/Mat4.h"
#include <vector>
#include <string>
#include <array>

class Renderer {
public:
	bool init(VulkanContext& vk, uint32_t width, uint32_t height);
	void shutdown(VulkanContext& vk);

	// вызвать при ресайзе (когда окно изменило размер)
	bool recreateSwapchain(VulkanContext& vk, uint32_t width, uint32_t height);

	// рисуем 1 кадр: clear + present
	bool drawFrame(VulkanContext& vk);
	void setViewProj(const Mat4& view, const Mat4& proj);
	void setCubeModel(const Mat4& m) { cubeModel_ = m; }

	void setPreferredPresentMode(Swapchain::PresentMode m) { swapchain_.setPreferredPresentMode(m); }
	Swapchain::PresentMode preferredPresentMode() const { return swapchain_.preferredPresentMode(); }
	VkPresentModeKHR chosenVkPresentMode() const { return swapchain_.chosenVkPresentMode(); }

private:
	bool createRenderPass(VulkanContext& vk);
	bool createFramebuffers(VulkanContext& vk);
	bool createCommandResources(VulkanContext& vk);
	bool createSync(VulkanContext& vk);

	void cleanupSwapchainDependent(VulkanContext& vk);

	bool createPipeline(VulkanContext& vk);
	void destroyPipeline(VulkanContext& vk);

	bool createDepthResources(VulkanContext& vk);
	void destroyDepthResources(VulkanContext& vk);
	VkFormat chooseDepthFormat(VulkanContext& vk) const;

	bool createUniform(VulkanContext& vk);
	void destroyUniform(VulkanContext& vk);

	bool createDescriptors(VulkanContext& vk);
	void destroyDescriptors(VulkanContext& vk);

	VkFormat depthFormat_{ VK_FORMAT_UNDEFINED };
	VkImage depthImage_{ VK_NULL_HANDLE };
	VkDeviceMemory depthMemory_{ VK_NULL_HANDLE };
	VkImageView depthView_{ VK_NULL_HANDLE };

	VkPipelineLayout pipelineLayout_{ VK_NULL_HANDLE };
	VkPipeline pipelineTriangles_{ VK_NULL_HANDLE };
	VkPipeline pipelineLines_{ VK_NULL_HANDLE };

	Swapchain swapchain_;

	VkRenderPass renderPass_{ VK_NULL_HANDLE };
	std::vector<VkFramebuffer> framebuffers_;

	VkCommandPool cmdPool_{ VK_NULL_HANDLE };

	static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	uint32_t currentFrame_ = 0;

	// per-frame sync
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailable_{};
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinished_{};
	std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences_{};

	// per-frame command buffers
	std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> cmd_{};

	// per-frame UBO
	std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> uboBuffer_{};
	std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> uboMemory_{};
	std::array<void*, MAX_FRAMES_IN_FLIGHT> uboMapped_{};

	// descriptors
	VkDescriptorSetLayout descSetLayout_{ VK_NULL_HANDLE };
	VkDescriptorPool descPool_{ VK_NULL_HANDLE };
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descSet_{};

	// track swapchain images
	std::vector<VkFence> imagesInFlight_;

	Mat4 cubeModel_ = Mat4::identity();

	struct UBO {
		Mat4 view;
		Mat4 proj;
	} uboCpu_;

	struct Vertex {
		float pos[3];
		float color[3];
	};

	// Cube (triangles)
	VkBuffer cubeVb_{ VK_NULL_HANDLE };
	VkDeviceMemory cubeVbMem_{ VK_NULL_HANDLE };
	VkBuffer cubeIb_{ VK_NULL_HANDLE };
	VkDeviceMemory cubeIbMem_{ VK_NULL_HANDLE };
	uint32_t cubeIndexCount_{ 0 };

	// Grid (lines)
	VkBuffer gridVb_{ VK_NULL_HANDLE };
	VkDeviceMemory gridVbMem_{ VK_NULL_HANDLE };
	VkBuffer gridIb_{ VK_NULL_HANDLE };
	VkDeviceMemory gridIbMem_{ VK_NULL_HANDLE };
	uint32_t gridIndexCount_{ 0 };

	bool createMeshBuffers(VulkanContext& vk);
	void destroyMeshBuffers(VulkanContext& vk);

	static VkVertexInputBindingDescription bindingDesc();
	static std::array<VkVertexInputAttributeDescription, 2> attrDescs();
};