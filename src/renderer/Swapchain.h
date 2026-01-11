#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Swapchain {
public:
    bool init(
        VkPhysicalDevice phys,
        VkDevice device,
        VkSurfaceKHR surface,
        uint32_t graphicsQF,
        uint32_t presentQF,
        uint32_t width,
        uint32_t height
    );

    void cleanup(VkDevice device);

    VkSwapchainKHR handle() const { return swapchain_; }
    VkFormat format() const { return format_; }
    VkExtent2D extent() const { return extent_; }
    const std::vector<VkImageView>& imageViews() const { return imageViews_; }

    enum class PresentMode { FIFO, MAILBOX, IMMEDIATE };

    void setPreferredPresentMode(PresentMode m) { preferredPresentMode_ = m; }
    PresentMode preferredPresentMode() const { return preferredPresentMode_; }
    VkPresentModeKHR chosenVkPresentMode() const { return chosenPresentMode_; }

    PresentMode preferredPresentMode_{ PresentMode::MAILBOX };
    VkPresentModeKHR chosenPresentMode_{ VK_PRESENT_MODE_FIFO_KHR };

private:
    VkSwapchainKHR swapchain_{ VK_NULL_HANDLE };
    VkFormat format_;
    VkExtent2D extent_;

    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
};
