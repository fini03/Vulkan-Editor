#include "swapchain.h"
#include "vulkan_editor/logicalDevice.h"

SwapchainNode::SwapchainNode(int id) : Node(id) {}
SwapchainNode::~SwapchainNode() { }

void SwapchainNode::render() const {}

void SwapchainNode::generateSwapchain() {
    outFile.open("Vertex.h", std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << R"(
	void createSyncObjects(VkDevice device, SyncObjects& syncObjects) {
	    syncObjects.m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	    syncObjects.m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	    syncObjects.m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	    VkSemaphoreCreateInfo semaphoreInfo{};
	    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	    VkFenceCreateInfo fenceInfo{};
	    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
	        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects.m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
	            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects.m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
	            vkCreateFence(device, &fenceInfo, nullptr, &syncObjects.m_inFlightFences[i]) != VK_SUCCESS) {
	            throw std::runtime_error("failed to create synchronization objects for a frame!");
	        }
	    }
	}

	void cleanupSwapChain(VkDevice device, VmaAllocator vmaAllocator, SwapChain& swapChain, DepthImage& depthImage) {
	    vkDestroyImageView(device, depthImage.m_depthImageView, nullptr);

	    destroyImage(device, vmaAllocator, depthImage.m_depthImage, depthImage.m_depthImageAllocation);

	    for (auto framebuffer : swapChain.m_swapChainFramebuffers) {
	        vkDestroyFramebuffer(device, framebuffer, nullptr);
	    }

	    for (auto imageView : swapChain.m_swapChainImageViews) {
	        vkDestroyImageView(device, imageView, nullptr);
	    }

	    vkDestroySwapchainKHR(device, swapChain.m_swapChain, nullptr);
	}

	void createFramebuffers(VkDevice device, SwapChain& swapChain, DepthImage& depthImage, VkRenderPass renderPass) {
	    swapChain.m_swapChainFramebuffers.resize(swapChain.m_swapChainImageViews.size());

	    for (size_t i = 0; i < swapChain.m_swapChainImageViews.size(); i++) {
	        std::array<VkImageView, 2> attachments = {
	            swapChain.m_swapChainImageViews[i],
	            depthImage.m_depthImageView
	        };

	        VkFramebufferCreateInfo framebufferInfo{};
	        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	        framebufferInfo.renderPass = renderPass;
	        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	        framebufferInfo.pAttachments = attachments.data();
	        framebufferInfo.width = swapChain.m_swapChainExtent.width;
	        framebufferInfo.height = swapChain.m_swapChainExtent.height;
	        framebufferInfo.layers = 1;

	        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChain.m_swapChainFramebuffers[i]) != VK_SUCCESS) {
	            throw std::runtime_error("failed to create framebuffer!");
	        }
	    }
	}

	VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	    for (VkFormat format : candidates) {
	        VkFormatProperties props;
	        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

	        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
	            return format;
	        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
	            return format;
	        }
	    }

	    throw std::runtime_error("failed to find supported format!");
	}

	VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
	    return findSupportedFormat(physicalDevice,
	        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
	        VK_IMAGE_TILING_OPTIMAL,
	        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	    );
	}

	void createDepthResources(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator, SwapChain& swapChain, DepthImage& depthImage) {
	    VkFormat depthFormat = findDepthFormat(physicalDevice);

	    createImage(physicalDevice, device, vmaAllocator, swapChain.m_swapChainExtent.width
	        , swapChain.m_swapChainExtent.height, depthFormat
	        , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
	        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage.m_depthImage, depthImage.m_depthImageAllocation);
	    depthImage.m_depthImageView = createImageView(device, depthImage.m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}


	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	    for (const auto& availableFormat : availableFormats) {
	        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
	            return availableFormat;
	        }
	    }

	    return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	    for (const auto& availablePresentMode : availablePresentModes) {
	        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
	            return availablePresentMode;
	        }
	    }

	    return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
	        return capabilities.currentExtent;
	    } else {
	        int width, height;
	        SDL_GetWindowSize(m_sdlWindow, &width, &height);

	        VkExtent2D actualExtent = {
	            static_cast<uint32_t>(width),
	            static_cast<uint32_t>(height)
	        };

	        actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width
	            , capabilities.maxImageExtent.width);
	        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height
	            , capabilities.maxImageExtent.height);

	        return actualExtent;
	    }
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
	    SwapChainSupportDetails details;

	    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	    uint32_t formatCount;
	    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	    if (formatCount != 0) {
	        details.formats.resize(formatCount);
	        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	    }

	    uint32_t presentModeCount;
	    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	    if (presentModeCount != 0) {
	        details.presentModes.resize(presentModeCount);
	        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	    }

	    return details;
	}

	void recreateSwapChain(
	    SDL_Window* window,
	    VkSurfaceKHR surface,
	    VkPhysicalDevice physicalDevice,
	    VkDevice device,
	    VmaAllocator vmaAllocator,
	    SwapChain& swapChain,
	    DepthImage& depthImage,
	    VkRenderPass renderPass
	) {

	    int width = 0, height = 0;

	    SDL_GetWindowSize(window, &width, &height);
	    while (width == 0 || height == 0) {
	        SDL_Event event;
	        SDL_WaitEvent(&event);
	        SDL_GetWindowSize(window, &width, &height);
	    }

	    vkDeviceWaitIdle(device);

	    cleanupSwapChain(device, vmaAllocator, swapChain, depthImage);

	    createSwapChain(surface, physicalDevice, device, swapChain);
	    createImageViews(device, swapChain);
	    createDepthResources(physicalDevice, device, vmaAllocator, swapChain, depthImage);
	    createFramebuffers(device, swapChain, depthImage, renderPass);
	}

	void createSwapChain(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkDevice device, SwapChain& swapChain) {
	    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

	    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
	        imageCount = swapChainSupport.capabilities.maxImageCount;
	    }

	    VkSwapchainCreateInfoKHR createInfo{};
	    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	    createInfo.surface = surface;

	    createInfo.minImageCount = imageCount;
	    createInfo.imageFormat = surfaceFormat.format;
	    createInfo.imageColorSpace = surfaceFormat.colorSpace;
	    createInfo.imageExtent = extent;
	    createInfo.imageArrayLayers = 1;
	    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

	    if (indices.graphicsFamily != indices.presentFamily) {
	        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	        createInfo.queueFamilyIndexCount = 2;
	        createInfo.pQueueFamilyIndices = queueFamilyIndices;
	    } else {
	        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	    }

	    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	    createInfo.presentMode = presentMode;
	    createInfo.clipped = VK_TRUE;

	    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain.m_swapChain) != VK_SUCCESS) {
	        throw std::runtime_error("failed to create swap chain!");
	    }

	    vkGetSwapchainImagesKHR(device, swapChain.m_swapChain, &imageCount, nullptr);
	    swapChain.m_swapChainImages.resize(imageCount);
	    vkGetSwapchainImagesKHR(device, swapChain.m_swapChain, &imageCount, swapChain.m_swapChainImages.data());

	    swapChain.m_swapChainImageFormat = surfaceFormat.format;
	    swapChain.m_swapChainExtent = extent;
	}
)";

    outFile.close();

    LogicalDeviceNode logicalDevice{id};
	logicalDevice.generateLogicalDevice();
}
