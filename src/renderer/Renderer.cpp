#include "renderer/Renderer.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <array>

static std::vector<char> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::ate | std::ios::binary);
    if (!f) return {};
    size_t size = (size_t)f.tellg();
    std::vector<char> buf(size);
    f.seekg(0);
    f.read(buf.data(), size);
    return buf;
}

static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    if (code.empty()) return VK_NULL_HANDLE;

    VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule m{};
    if (vkCreateShaderModule(device, &ci, nullptr, &m) != VK_SUCCESS)
        return VK_NULL_HANDLE;
    return m;
}

static uint32_t findMemoryType(VkPhysicalDevice phys, uint32_t typeFilter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(phys, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    return UINT32_MAX;
}

static bool createBuffer(
    VulkanContext& vk,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memProps,
    VkBuffer& outBuf,
    VkDeviceMemory& outMem
) {
    VkBufferCreateInfo bi{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bi.size = size;
    bi.usage = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vk.device(), &bi, nullptr, &outBuf) != VK_SUCCESS) return false;

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(vk.device(), outBuf, &req);

    uint32_t memType = findMemoryType(vk.physicalDevice(), req.memoryTypeBits, memProps);
    if (memType == UINT32_MAX) return false;

    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = memType;

    if (vkAllocateMemory(vk.device(), &ai, nullptr, &outMem) != VK_SUCCESS) return false;
    if (vkBindBufferMemory(vk.device(), outBuf, outMem, 0) != VK_SUCCESS) return false;

    return true;
}

static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo iv{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    iv.image = image;
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = format;
    iv.subresourceRange.aspectMask = aspect;
    iv.subresourceRange.baseMipLevel = 0;
    iv.subresourceRange.levelCount = 1;
    iv.subresourceRange.baseArrayLayer = 0;
    iv.subresourceRange.layerCount = 1;

    VkImageView view = VK_NULL_HANDLE;
    if (vkCreateImageView(device, &iv, nullptr, &view) != VK_SUCCESS) return VK_NULL_HANDLE;
    return view;
}


static bool vk_ok(VkResult r, const char* msg) {
    if (r != VK_SUCCESS) {
        std::cerr << msg << " VkResult=" << r << "\n";
        return false;
    }
    return true;
}

VkVertexInputBindingDescription Renderer::bindingDesc() {
    VkVertexInputBindingDescription b{};
    b.binding = 0;
    b.stride = sizeof(Vertex);
    b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return b;
}

std::array<VkVertexInputAttributeDescription, 2> Renderer::attrDescs() {
    std::array<VkVertexInputAttributeDescription, 2> a{};

    // location 0: vec3 position
    a[0].location = 0;
    a[0].binding = 0;
    a[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    a[0].offset = offsetof(Vertex, pos);

    // location 1: vec3 color
    a[1].location = 1;
    a[1].binding = 0;
    a[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    a[1].offset = offsetof(Vertex, color);

    return a;
}

bool Renderer::createMeshBuffers(VulkanContext& vk) {
    destroyMeshBuffers(vk);

    // ---------- 1) Cube ----------
    std::vector<Vertex> cubeVerts = {
        {{-0.5f,-0.5f,-0.5f},{1,0,0}}, // 0
        {{ 0.5f,-0.5f,-0.5f},{0,1,0}}, // 1
        {{ 0.5f, 0.5f,-0.5f},{0,0,1}}, // 2
        {{-0.5f, 0.5f,-0.5f},{1,1,0}}, // 3
        {{-0.5f,-0.5f, 0.5f},{0,1,1}}, // 4
        {{ 0.5f,-0.5f, 0.5f},{1,0,1}}, // 5
        {{ 0.5f, 0.5f, 0.5f},{1,1,1}}, // 6
        {{-0.5f, 0.5f, 0.5f},{0.7f,0.7f,0.7f}}, // 7
    };

    std::vector<uint32_t> cubeIdx = {
        // back (-Z)
        0,1,2,  0,2,3,
        // front (+Z)
        4,6,5,  4,7,6,
        // left (-X)
        0,3,7,  0,7,4,
        // right (+X)
        1,5,6,  1,6,2,
        // bottom (-Y)
        0,4,5,  0,5,1,
        // top (+Y)
        3,2,6,  3,6,7
    };

    cubeIndexCount_ = (uint32_t)cubeIdx.size();

    VkDeviceSize cubeVbSize = sizeof(Vertex) * cubeVerts.size();
    VkDeviceSize cubeIbSize = sizeof(uint32_t) * cubeIdx.size();

    if (!createBuffer(vk, cubeVbSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        cubeVb_, cubeVbMem_)) return false;

    if (!createBuffer(vk, cubeIbSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        cubeIb_, cubeIbMem_)) return false;

    void* p = nullptr;
    vkMapMemory(vk.device(), cubeVbMem_, 0, cubeVbSize, 0, &p);
    std::memcpy(p, cubeVerts.data(), (size_t)cubeVbSize);
    vkUnmapMemory(vk.device(), cubeVbMem_);

    vkMapMemory(vk.device(), cubeIbMem_, 0, cubeIbSize, 0, &p);
    std::memcpy(p, cubeIdx.data(), (size_t)cubeIbSize);
    vkUnmapMemory(vk.device(), cubeIbMem_);

    // ---------- 2) Grid lines (XZ plane, LINE_LIST) ----------
    const int half = 20;      // от -20 до +20
    const float step = 1.0f;
    const float y = 0.0f;

    std::vector<Vertex> gridVerts;
    std::vector<uint32_t> gridIdx;
    gridVerts.reserve((half * 2 + 1) * 4);
    gridIdx.reserve((half * 2 + 1) * 4);

    uint32_t idx = 0;
    for (int i = -half; i <= half; ++i) {
        float x = i * step;
        // линия вдоль Z при фиксированном X
        gridVerts.push_back({ { x, y, -half * step }, {0.35f, 0.35f, 0.35f} });
        gridVerts.push_back({ { x, y,  half * step }, {0.35f, 0.35f, 0.35f} });
        gridIdx.push_back(idx++);
        gridIdx.push_back(idx++);
    }
    for (int i = -half; i <= half; ++i) {
        float z = i * step;
        // линия вдоль X при фиксированном Z
        gridVerts.push_back({ { -half * step, y, z }, {0.35f, 0.35f, 0.35f} });
        gridVerts.push_back({ {  half * step, y, z }, {0.35f, 0.35f, 0.35f} });
        gridIdx.push_back(idx++);
        gridIdx.push_back(idx++);
    }

    gridIndexCount_ = (uint32_t)gridIdx.size();

    VkDeviceSize gridVbSize = sizeof(Vertex) * gridVerts.size();
    VkDeviceSize gridIbSize = sizeof(uint32_t) * gridIdx.size();

    if (!createBuffer(vk, gridVbSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        gridVb_, gridVbMem_)) return false;

    if (!createBuffer(vk, gridIbSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        gridIb_, gridIbMem_)) return false;

    vkMapMemory(vk.device(), gridVbMem_, 0, gridVbSize, 0, &p);
    std::memcpy(p, gridVerts.data(), (size_t)gridVbSize);
    vkUnmapMemory(vk.device(), gridVbMem_);

    vkMapMemory(vk.device(), gridIbMem_, 0, gridIbSize, 0, &p);
    std::memcpy(p, gridIdx.data(), (size_t)gridIbSize);
    vkUnmapMemory(vk.device(), gridIbMem_);

    return true;
}


void Renderer::destroyMeshBuffers(VulkanContext& vk) {
    // Cube
    if (cubeVb_) { vkDestroyBuffer(vk.device(), cubeVb_, nullptr); cubeVb_ = VK_NULL_HANDLE; }
    if (cubeVbMem_) { vkFreeMemory(vk.device(), cubeVbMem_, nullptr); cubeVbMem_ = VK_NULL_HANDLE; }
    if (cubeIb_) { vkDestroyBuffer(vk.device(), cubeIb_, nullptr); cubeIb_ = VK_NULL_HANDLE; }
    if (cubeIbMem_) { vkFreeMemory(vk.device(), cubeIbMem_, nullptr); cubeIbMem_ = VK_NULL_HANDLE; }
    cubeIndexCount_ = 0;

    // Grid
    if (gridVb_) { vkDestroyBuffer(vk.device(), gridVb_, nullptr); gridVb_ = VK_NULL_HANDLE; }
    if (gridVbMem_) { vkFreeMemory(vk.device(), gridVbMem_, nullptr); gridVbMem_ = VK_NULL_HANDLE; }
    if (gridIb_) { vkDestroyBuffer(vk.device(), gridIb_, nullptr); gridIb_ = VK_NULL_HANDLE; }
    if (gridIbMem_) { vkFreeMemory(vk.device(), gridIbMem_, nullptr); gridIbMem_ = VK_NULL_HANDLE; }
    gridIndexCount_ = 0;
}


VkFormat Renderer::chooseDepthFormat(VulkanContext& vk) const {
    VkFormat candidates[] = {
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat f : candidates) {
        VkFormatProperties props{};
        vkGetPhysicalDeviceFormatProperties(vk.physicalDevice(), f, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return f;
        }
    }
    return VK_FORMAT_UNDEFINED;
}

bool Renderer::createDepthResources(VulkanContext& vk) {
    destroyDepthResources(vk);

    depthFormat_ = chooseDepthFormat(vk);
    if (depthFormat_ == VK_FORMAT_UNDEFINED) return false;

    VkExtent2D ext = swapchain_.extent();

    VkImageCreateInfo img{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    img.imageType = VK_IMAGE_TYPE_2D;
    img.extent.width = ext.width;
    img.extent.height = ext.height;
    img.extent.depth = 1;
    img.mipLevels = 1;
    img.arrayLayers = 1;
    img.format = depthFormat_;
    img.tiling = VK_IMAGE_TILING_OPTIMAL;
    img.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    img.samples = VK_SAMPLE_COUNT_1_BIT;
    img.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(vk.device(), &img, nullptr, &depthImage_) != VK_SUCCESS) return false;

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(vk.device(), depthImage_, &req);

    uint32_t memType = findMemoryType(vk.physicalDevice(), req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memType == UINT32_MAX) return false;

    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = memType;

    if (vkAllocateMemory(vk.device(), &ai, nullptr, &depthMemory_) != VK_SUCCESS) return false;
    if (vkBindImageMemory(vk.device(), depthImage_, depthMemory_, 0) != VK_SUCCESS) return false;

    depthView_ = createImageView(vk.device(), depthImage_, depthFormat_, VK_IMAGE_ASPECT_DEPTH_BIT);
    return depthView_ != VK_NULL_HANDLE;
}

bool Renderer::createUniform(VulkanContext& vk) {
    VkDeviceSize size = sizeof(UBO);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (!createBuffer(vk, size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uboBuffer_[i], uboMemory_[i])) return false;

        if (vkMapMemory(vk.device(), uboMemory_[i], 0, size, 0, &uboMapped_[i]) != VK_SUCCESS) return false;
    }
    return true;
}

bool Renderer::createDescriptors(VulkanContext& vk) {
    VkDescriptorSetLayoutBinding b{};
    b.binding = 0;
    b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    b.descriptorCount = 1;
    b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo li{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    li.bindingCount = 1;
    li.pBindings = &b;

    if (vkCreateDescriptorSetLayout(vk.device(), &li, nullptr, &descSetLayout_) != VK_SUCCESS) return false;

    VkDescriptorPoolSize ps{};
    ps.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ps.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo pi{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pi.poolSizeCount = 1;
    pi.pPoolSizes = &ps;
    pi.maxSets = MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(vk.device(), &pi, nullptr, &descPool_) != VK_SUCCESS) return false;

    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts{};
    layouts.fill(descSetLayout_);

    VkDescriptorSetAllocateInfo ai{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    ai.descriptorPool = descPool_;
    ai.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    ai.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(vk.device(), &ai, descSet_.data()) != VK_SUCCESS) return false;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo dbi{};
        dbi.buffer = uboBuffer_[i];
        dbi.offset = 0;
        dbi.range = sizeof(UBO);

        VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        w.dstSet = descSet_[i];
        w.dstBinding = 0;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w.pBufferInfo = &dbi;

        vkUpdateDescriptorSets(vk.device(), 1, &w, 0, nullptr);
    }
    return true;
}


void Renderer::destroyDescriptors(VulkanContext& vk) {
    if (descPool_) {
        vkDestroyDescriptorPool(vk.device(), descPool_, nullptr);
        descPool_ = VK_NULL_HANDLE;
    }
    if (descSetLayout_) {
        vkDestroyDescriptorSetLayout(vk.device(), descSetLayout_, nullptr);
        descSetLayout_ = VK_NULL_HANDLE;
    }
    //descSet_ = VK_NULL_HANDLE;
}


void Renderer::destroyUniform(VulkanContext& vk) {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (uboMapped_[i]) {
            vkUnmapMemory(vk.device(), uboMemory_[i]);
            uboMapped_[i] = nullptr;
        }
        if (uboBuffer_[i]) {
            vkDestroyBuffer(vk.device(), uboBuffer_[i], nullptr);
            uboBuffer_[i] = VK_NULL_HANDLE;
        }
        if (uboMemory_[i]) {
            vkFreeMemory(vk.device(), uboMemory_[i], nullptr);
            uboMemory_[i] = VK_NULL_HANDLE;
        }
    }
}

void Renderer::destroyDepthResources(VulkanContext& vk) {
    if (depthView_) {
        vkDestroyImageView(vk.device(), depthView_, nullptr);
        depthView_ = VK_NULL_HANDLE;
    }
    if (depthImage_) {
        vkDestroyImage(vk.device(), depthImage_, nullptr);
        depthImage_ = VK_NULL_HANDLE;
    }
    if (depthMemory_) {
        vkFreeMemory(vk.device(), depthMemory_, nullptr);
        depthMemory_ = VK_NULL_HANDLE;
    }
    depthFormat_ = VK_FORMAT_UNDEFINED;
}


bool Renderer::init(VulkanContext& vk, uint32_t width, uint32_t height) {
    if (!swapchain_.init(
        vk.physicalDevice(), vk.device(), vk.surface(),
        vk.graphicsQueueFamily(), vk.presentQueueFamily(),
        width, height
    )) return false;

    if (!createDepthResources(vk)) return false;
    if (!createRenderPass(vk)) return false;
    if (!createFramebuffers(vk)) return false;
    if (!createUniform(vk)) return false;
    if (!createDescriptors(vk)) return false;
    if (!createMeshBuffers(vk)) return false;
    if (!createPipeline(vk)) return false;
    if (!createCommandResources(vk)) return false;
    if (!createSync(vk)) return false;

    imagesInFlight_.assign(swapchain_.imageViews().size(), VK_NULL_HANDLE);
    currentFrame_ = 0;

    return true;
}

void Renderer::cleanupSwapchainDependent(VulkanContext& vk) {
    destroyPipeline(vk); // ← ВАЖНО: сначала pipeline, потом renderpass/framebuffers

    for (auto fb : framebuffers_) vkDestroyFramebuffer(vk.device(), fb, nullptr);
    framebuffers_.clear();

    if (renderPass_) vkDestroyRenderPass(vk.device(), renderPass_, nullptr);
    renderPass_ = VK_NULL_HANDLE;

    destroyDepthResources(vk);
    swapchain_.cleanup(vk.device());
}


bool Renderer::recreateSwapchain(VulkanContext& vk, uint32_t width, uint32_t height) {
    // если окно свернули → width/height могут стать 0, ждём пока станет >0
    if (width == 0 || height == 0) return true;

    vkDeviceWaitIdle(vk.device());
    cleanupSwapchainDependent(vk);

    if (!swapchain_.init(
        vk.physicalDevice(), vk.device(), vk.surface(),
        vk.graphicsQueueFamily(), vk.presentQueueFamily(),
        width, height
    )) return false;

    if (!createDepthResources(vk)) return false;
    if (!createRenderPass(vk)) return false;
    if (!createFramebuffers(vk)) return false;
    if (!createPipeline(vk)) return false;

    imagesInFlight_.assign(swapchain_.imageViews().size(), VK_NULL_HANDLE);
    currentFrame_ = 0;

    return true;
}

bool Renderer::createRenderPass(VulkanContext& vk) {
    // 1) Color attachment (swapchain)
    VkAttachmentDescription color{};
    color.format = swapchain_.format();
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 2) Depth attachment
    VkAttachmentDescription depth{};
    depth.format = depthFormat_;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[2] = { color, depth };

    VkRenderPassCreateInfo rp{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rp.attachmentCount = 2;
    rp.pAttachments = attachments;
    rp.subpassCount = 1;
    rp.pSubpasses = &subpass;
    rp.dependencyCount = 1;
    rp.pDependencies = &dep;

    return vkCreateRenderPass(vk.device(), &rp, nullptr, &renderPass_) == VK_SUCCESS;
}


bool Renderer::createFramebuffers(VulkanContext& vk) {
    const auto& views = swapchain_.imageViews();
    framebuffers_.resize(views.size());

    for (size_t i = 0; i < views.size(); i++) {
        VkImageView attachments[] = { views[i], depthView_ };

        VkFramebufferCreateInfo fb{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fb.renderPass = renderPass_;
        fb.attachmentCount = 2;
        fb.pAttachments = attachments;
        fb.width = swapchain_.extent().width;
        fb.height = swapchain_.extent().height;
        fb.layers = 1;

        if (!vk_ok(vkCreateFramebuffer(vk.device(), &fb, nullptr, &framebuffers_[i]),
            "vkCreateFramebuffer failed"))
            return false;
    }
    return true;
}

bool Renderer::createCommandResources(VulkanContext& vk) {
    VkCommandPoolCreateInfo pci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pci.queueFamilyIndex = vk.graphicsQueueFamily();

    if (!vk_ok(vkCreateCommandPool(vk.device(), &pci, nullptr, &cmdPool_), "vkCreateCommandPool failed"))
        return false;

    VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.commandPool = cmdPool_;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    return vk_ok(vkAllocateCommandBuffers(vk.device(), &ai, cmd_.data()), "vkAllocateCommandBuffers failed");
}


bool Renderer::createSync(VulkanContext& vk) {
    VkSemaphoreCreateInfo si{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VkFenceCreateInfo fi{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (!vk_ok(vkCreateSemaphore(vk.device(), &si, nullptr, &imageAvailable_[i]), "vkCreateSemaphore failed")) return false;
        if (!vk_ok(vkCreateSemaphore(vk.device(), &si, nullptr, &renderFinished_[i]), "vkCreateSemaphore failed")) return false;
        if (!vk_ok(vkCreateFence(vk.device(), &fi, nullptr, &inFlightFences_[i]), "vkCreateFence failed")) return false;
    }
    return true;
}


bool Renderer::drawFrame(VulkanContext& vk) {
    uint32_t frame = currentFrame_;

    // 1) ждём завершения GPU по этому frame-слоту
    vkWaitForFences(vk.device(), 1, &inFlightFences_[frame], VK_TRUE, UINT64_MAX);

    // 2) получить индекс изображения swapchain
    uint32_t imageIndex = 0;
    VkResult acq = vkAcquireNextImageKHR(
        vk.device(), swapchain_.handle(), UINT64_MAX,
        imageAvailable_[frame], VK_NULL_HANDLE, &imageIndex
    );

    if (acq == VK_ERROR_OUT_OF_DATE_KHR) { std::cout << "ACQ: OUT_OF_DATE\n"; return false; }
    if (acq == VK_SUBOPTIMAL_KHR) { std::cout << "ACQ: SUBOPTIMAL\n"; /* не return */ }

    if (acq == VK_ERROR_OUT_OF_DATE_KHR) return false;
    if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR) {
        std::cerr << "vkAcquireNextImageKHR failed: " << acq << "\n";
        return false;
    }

    // 3) если swapchain image уже в полёте — ждём fence того кадра
    if (imageIndex < imagesInFlight_.size() && imagesInFlight_[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(vk.device(), 1, &imagesInFlight_[imageIndex], VK_TRUE, UINT64_MAX);
    }
    if (imageIndex < imagesInFlight_.size()) {
        imagesInFlight_[imageIndex] = inFlightFences_[frame];
    }

    // 4) теперь можно ресетнуть fence нашего frame-слота
    vkResetFences(vk.device(), 1, &inFlightFences_[frame]);

    // 5) обновляем UBO для текущего frame-слота
    std::memcpy(uboMapped_[frame], &uboCpu_, sizeof(UBO));

    // 6) записываем командный буфер для frame-слота, но framebuffer берём по imageIndex
    VkCommandBuffer cmd = cmd_[frame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(cmd, &bi);

    VkClearValue clears[2]{};
    clears[0].color.float32[0] = 0.05f;
    clears[0].color.float32[1] = 0.07f;
    clears[0].color.float32[2] = 0.12f;
    clears[0].color.float32[3] = 1.0f;
    clears[1].depthStencil.depth = 1.0f;
    clears[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo rbi{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rbi.renderPass = renderPass_;
    rbi.framebuffer = framebuffers_[imageIndex];
    rbi.renderArea.offset = { 0, 0 };
    rbi.renderArea.extent = swapchain_.extent();
    rbi.clearValueCount = 2;
    rbi.pClearValues = clears;

    vkCmdBeginRenderPass(cmd, &rbi, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain_.extent().width;
    viewport.height = (float)swapchain_.extent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain_.extent();

    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Descriptor set — по frame-слоту (как было)
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout_,
        0, 1, &descSet_[frame],
        0, nullptr
    );

    // ----- 1) GRID (lines) -----
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLines_);

    VkDeviceSize off = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &gridVb_, &off);
    vkCmdBindIndexBuffer(cmd, gridIb_, 0, VK_INDEX_TYPE_UINT32);

    Mat4 gridModel = Mat4::identity();
    vkCmdPushConstants(cmd, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4), &gridModel);
    vkCmdDrawIndexed(cmd, gridIndexCount_, 1, 0, 0, 0);

    // ----- 2) CUBE (triangles) -----
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineTriangles_);

    vkCmdBindVertexBuffers(cmd, 0, 1, &cubeVb_, &off);
    vkCmdBindIndexBuffer(cmd, cubeIb_, 0, VK_INDEX_TYPE_UINT32);

    vkCmdPushConstants(cmd, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4), &cubeModel_);
    vkCmdDrawIndexed(cmd, cubeIndexCount_, 1, 0, 0, 0);


    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    // 7) submit
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &imageAvailable_[frame];
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &renderFinished_[frame];

    if (!vk_ok(vkQueueSubmit(vk.graphicsQueue(), 1, &submit, inFlightFences_[frame]), "vkQueueSubmit failed"))
        return false;

    // 8) present
    VkPresentInfoKHR pi{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &renderFinished_[frame];
    VkSwapchainKHR sc = swapchain_.handle();
    pi.swapchainCount = 1;
    pi.pSwapchains = &sc;
    pi.pImageIndices = &imageIndex;

    VkResult pres = vkQueuePresentKHR(vk.presentQueue(), &pi);

    if (pres == VK_ERROR_OUT_OF_DATE_KHR) { std::cout << "PRES: OUT_OF_DATE\n"; return false; }
    if (pres == VK_SUBOPTIMAL_KHR) { std::cout << "PRES: SUBOPTIMAL\n"; return false; } // или не возвращать — см. ниже

    // 9) следующий frame-слот
    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;

    if (pres == VK_ERROR_OUT_OF_DATE_KHR) return false;
    return pres == VK_SUCCESS;
}


void Renderer::shutdown(VulkanContext& vk) {
    vkDeviceWaitIdle(vk.device());

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (inFlightFences_[i]) vkDestroyFence(vk.device(), inFlightFences_[i], nullptr);
        if (renderFinished_[i]) vkDestroySemaphore(vk.device(), renderFinished_[i], nullptr);
        if (imageAvailable_[i]) vkDestroySemaphore(vk.device(), imageAvailable_[i], nullptr);
    }

    if (cmdPool_) vkDestroyCommandPool(vk.device(), cmdPool_, nullptr);

    destroyMeshBuffers(vk);
    destroyPipeline(vk);
    destroyDescriptors(vk);
    destroyUniform(vk);

    cleanupSwapchainDependent(vk);
}

void Renderer::setViewProj(const Mat4& view, const Mat4& proj) {
    uboCpu_.view = view;
    uboCpu_.proj = proj;
}

bool Renderer::createPipeline(VulkanContext& vk) {
    destroyPipeline(vk);

    // пути относительно папки exe: ./shaders/...
    auto vert = readFile("shaders/triangle.vert.spv");
    auto frag = readFile("shaders/triangle.frag.spv");

    VkShaderModule vertMod = createShaderModule(vk.device(), vert);
    VkShaderModule fragMod = createShaderModule(vk.device(), frag);
    if (!vertMod || !fragMod) {
        if (vertMod) vkDestroyShaderModule(vk.device(), vertMod, nullptr);
        if (fragMod) vkDestroyShaderModule(vk.device(), fragMod, nullptr);
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertMod;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragMod;
    stages[1].pName = "main";

    auto b = bindingDesc();
    auto a = attrDescs();

    VkPipelineVertexInputStateCreateInfo vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &b;
    vi.vertexAttributeDescriptionCount = (uint32_t)a.size();
    vi.pVertexAttributeDescriptions = a.data();

    VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // viewport/scissor сделаем DYNAMIC, чтобы не пересоздавать pipeline при resize
    VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE; //VK_CULL_MODE_BACK_BIT
    rs.frontFace = VK_FRONT_FACE_CLOCKWISE; // Vulkan NDC “перевёрнут” относительно OpenGL
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cbAtt{};
    cbAtt.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    cb.attachmentCount = 1;
    cb.pAttachments = &cbAtt;

    VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates = dynStates;

    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcr.offset = 0;
    pcr.size = sizeof(Mat4);

    VkPipelineLayoutCreateInfo pli{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &descSetLayout_;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &pcr;
    if (vkCreatePipelineLayout(vk.device(), &pli, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        vkDestroyShaderModule(vk.device(), vertMod, nullptr);
        vkDestroyShaderModule(vk.device(), fragMod, nullptr);
        return false;
    }

    VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pi{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pi.stageCount = 2;
    pi.pDepthStencilState = &ds;
    pi.pStages = stages;
    pi.pVertexInputState = &vi;
    pi.pInputAssemblyState = &ia;
    pi.pViewportState = &vp;
    pi.pRasterizationState = &rs;
    pi.pMultisampleState = &ms;
    pi.pColorBlendState = &cb;
    pi.pDynamicState = &dyn;
    pi.layout = pipelineLayout_;
    pi.renderPass = renderPass_;
    pi.subpass = 0;

    // --- triangles ---
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkResult r1 = vkCreateGraphicsPipelines(vk.device(), VK_NULL_HANDLE, 1, &pi, nullptr, &pipelineTriangles_);
    if (r1 != VK_SUCCESS) {
        vkDestroyShaderModule(vk.device(), vertMod, nullptr);
        vkDestroyShaderModule(vk.device(), fragMod, nullptr);
        return false;
    }

    // --- lines ---
    ia.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    VkResult r2 = vkCreateGraphicsPipelines(vk.device(), VK_NULL_HANDLE, 1, &pi, nullptr, &pipelineLines_);
    vkDestroyShaderModule(vk.device(), vertMod, nullptr);
    vkDestroyShaderModule(vk.device(), fragMod, nullptr);

    return r2 == VK_SUCCESS;
}

void Renderer::destroyPipeline(VulkanContext& vk) {
    if (pipelineTriangles_) {
        vkDestroyPipeline(vk.device(), pipelineTriangles_, nullptr);
        pipelineTriangles_ = VK_NULL_HANDLE;
    }
    if (pipelineLines_) {
        vkDestroyPipeline(vk.device(), pipelineLines_, nullptr);
        pipelineLines_ = VK_NULL_HANDLE;
    }
    if (pipelineLayout_) {
        vkDestroyPipelineLayout(vk.device(), pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
}
