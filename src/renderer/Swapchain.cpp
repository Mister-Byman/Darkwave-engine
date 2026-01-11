#include "renderer/Swapchain.h"
#include <vector>
#include <algorithm>
#include <iostream>

static VkPresentModeKHR pickPresentMode(
    VkPhysicalDevice phys,
    VkSurfaceKHR surface,
    Swapchain::PresentMode preferred
) {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &count, modes.data());

    auto has = [&](VkPresentModeKHR m) {
        for (auto x : modes) if (x == m) return true;
        return false;
        };

    // FIFO гарантирован Vulkan-спекой
    if (preferred == Swapchain::PresentMode::MAILBOX && has(VK_PRESENT_MODE_MAILBOX_KHR))
        return VK_PRESENT_MODE_MAILBOX_KHR;
    if (preferred == Swapchain::PresentMode::IMMEDIATE && has(VK_PRESENT_MODE_IMMEDIATE_KHR))
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    return VK_PRESENT_MODE_FIFO_KHR;
}


bool Swapchain::init(
    VkPhysicalDevice phys,
    VkDevice device,
    VkSurfaceKHR surface,
    uint32_t graphicsQF,
    uint32_t presentQF,
    uint32_t width,
    uint32_t height
) {
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);

    uint32_t fmtCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &fmtCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(fmtCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &fmtCount, formats.data());

    VkSurfaceFormatKHR chosen = formats[0];
    for (auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM) {
            chosen = f;
            break;
        }
    }

    format_ = chosen.format;

    extent_.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
    extent_.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR ci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    ci.surface = surface;
    ci.minImageCount = imageCount;
    ci.imageFormat = chosen.format;
    ci.imageColorSpace = chosen.colorSpace;
    ci.imageExtent = extent_;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.preTransform = caps.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    chosenPresentMode_ = pickPresentMode(phys, surface, preferredPresentMode_);
    ci.presentMode = chosenPresentMode_;
    ci.clipped = VK_TRUE;

    // пример (на время)
    if (chosenPresentMode_ == VK_PRESENT_MODE_MAILBOX_KHR) std::cout << "PresentMode: MAILBOX" << "\n";
    else if (chosenPresentMode_ == VK_PRESENT_MODE_IMMEDIATE_KHR) std::cout << "PresentMode: IMMEDIATE" << "\n";
    else std::cout << ("PresentMode: FIFO") << "\n";


    uint32_t qfs[] = { graphicsQF, presentQF };
    if (graphicsQF != presentQF) {
        ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices = qfs;
    }
    else {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    if (vkCreateSwapchainKHR(device, &ci, nullptr, &swapchain_) != VK_SUCCESS)
        return false;

    uint32_t imgCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain_, &imgCount, nullptr);
    images_.resize(imgCount);
    vkGetSwapchainImagesKHR(device, swapchain_, &imgCount, images_.data());

    imageViews_.resize(imgCount);
    for (size_t i = 0; i < imgCount; i++) {
        VkImageViewCreateInfo iv{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        iv.image = images_[i];
        iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv.format = format_;
        iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.levelCount = 1;
        iv.subresourceRange.layerCount = 1;

        vkCreateImageView(device, &iv, nullptr, &imageViews_[i]);
    }

    return true;
}

void Swapchain::cleanup(VkDevice device) {
    for (auto iv : imageViews_)
        vkDestroyImageView(device, iv, nullptr);
    imageViews_.clear();

    if (swapchain_)
        vkDestroySwapchainKHR(device, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
}
