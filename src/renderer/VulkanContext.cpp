#include "renderer/VulkanContext.h"
#include <SDL_vulkan.h>
#include <vector>
#include <iostream>

static bool check(VkResult r, const char* msg) {
    if (r != VK_SUCCESS) {
        std::cerr << msg << " VkResult=" << r << "\n";
        return false;
    }
    return true;
}

bool VulkanContext::init(SDL_Window* window) {
    if (!createInstance(window)) return false;
    if (!createSurface(window)) return false;
    if (!pickPhysicalDevice()) return false;
    if (!createDevice()) return false;
    return true;
}

bool VulkanContext::createInstance(SDL_Window* window) {
    unsigned extCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr)) {
        std::cerr << "SDL_Vulkan_GetInstanceExtensions(count) failed: " << SDL_GetError() << "\n";
        return false;
    }
    std::vector<const char*> exts(extCount);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, exts.data())) {
        std::cerr << "SDL_Vulkan_GetInstanceExtensions(list) failed: " << SDL_GetError() << "\n";
        return false;
    }

    VkApplicationInfo app{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app.pApplicationName = "cs_like";
    app.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app.pEngineName = "mini_engine";
    app.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo ci{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = (uint32_t)exts.size();
    ci.ppEnabledExtensionNames = exts.data();

    return check(vkCreateInstance(&ci, nullptr, &instance_), "vkCreateInstance failed");
}

bool VulkanContext::createSurface(SDL_Window* window) {
    if (!SDL_Vulkan_CreateSurface(window, instance_, &surface_)) {
        std::cerr << "SDL_Vulkan_CreateSurface failed: " << SDL_GetError() << "\n";
        return false;
    }
    return true;
}

bool VulkanContext::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    if (count == 0) {
        std::cerr << "No Vulkan physical devices found\n";
        return false;
    }
    std::vector<VkPhysicalDevice> devs(count);
    vkEnumeratePhysicalDevices(instance_, &count, devs.data());

    // Самый простой выбор: первый, который имеет graphics+present
    for (auto d : devs) {
        uint32_t qCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qCount, nullptr);
        std::vector<VkQueueFamilyProperties> qprops(qCount);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qCount, qprops.data());

        uint32_t g = UINT32_MAX, p = UINT32_MAX;
        for (uint32_t i = 0; i < qCount; i++) {
            if (qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) g = i;

            VkBool32 supportsPresent = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(d, i, surface_, &supportsPresent);
            if (supportsPresent) p = i;
        }

        if (g != UINT32_MAX && p != UINT32_MAX) {
            phys_ = d;
            graphicsQF_ = g;
            presentQF_ = p;
            return true;
        }
    }

    std::cerr << "No suitable physical device (graphics+present) found\n";
    return false;
}

bool VulkanContext::createDevice() {
    float prio = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> qcis;
    auto addQ = [&](uint32_t family) {
        VkDeviceQueueCreateInfo qci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        qci.queueFamilyIndex = family;
        qci.queueCount = 1;
        qci.pQueuePriorities = &prio;
        qcis.push_back(qci);
        };

    addQ(graphicsQF_);
    if (presentQF_ != graphicsQF_) addQ(presentQF_);

    const char* devExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo ci{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    ci.queueCreateInfoCount = (uint32_t)qcis.size();
    ci.pQueueCreateInfos = qcis.data();
    ci.enabledExtensionCount = 1;
    ci.ppEnabledExtensionNames = devExts;

    if (!check(vkCreateDevice(phys_, &ci, nullptr, &device_), "vkCreateDevice failed"))
        return false;

    vkGetDeviceQueue(device_, graphicsQF_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, presentQF_, 0, &presentQueue_);

    return true;
}

void VulkanContext::shutdown() {
    if (device_) vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;

    if (surface_) vkDestroySurfaceKHR(instance_, surface_, nullptr);
    surface_ = VK_NULL_HANDLE;

    if (instance_) vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
}
