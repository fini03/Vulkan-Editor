#include "vulkan_base.h"

void VulkanSwapchain::createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage, VkExtent2D extent) {
	VkBool32 supportsPresent = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(context->physicalDevice, context->graphicsQueue.familyIndex, surface, &supportsPresent);
	if (!supportsPresent) {
		std::cerr << "Graphics queue does not support present" << std::endl;
		return;
	}

	uint32_t numFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, 0);
	std::vector<VkSurfaceFormatKHR> availableFormats(numFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, availableFormats.data());

	if (numFormats <= 0) {
		std::cerr << "No surface formats available" << std::endl;
		return;
	}

	// First available format should be a sensible default in most cases
	VkFormat format = availableFormats[0].format;
	VkColorSpaceKHR colorSpace = availableFormats[0].colorSpace;

	for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        	format = availableFormat.format;
         	colorSpace = availableFormat.colorSpace;
        }
    }

	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, surface, &surfaceCapabilities);

	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    	extent = surfaceCapabilities.currentExtent;
    } else {
        extent.width = std::clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }

	if (surfaceCapabilities.maxImageCount == 0) {
		surfaceCapabilities.maxImageCount = 8;
	}

	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
		imageCount = surfaceCapabilities.maxImageCount;
    }

	VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = format,
		.imageColorSpace = colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = usage,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR
	};

	if (vkCreateSwapchainKHR(context->device, &createInfo, 0, &this->swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}

	this->format = format;
	this->extent = extent;

	// Acquire swapchain images
	uint32_t numImages = 0;
	vkGetSwapchainImagesKHR(context->device, this->swapchain, &numImages, 0);
	this->images.resize(numImages);
	vkGetSwapchainImagesKHR(context->device, this->swapchain, &numImages, this->images.data());

	// Create image views
	this->imageViews.resize(numImages);
	for (uint32_t i = 0; i < numImages; ++i) {
		VkImageViewCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = this->images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.components = {},
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};

		if (vkCreateImageView(context->device, &createInfo, 0, &this->imageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image views");
		}
	}
}

void VulkanSwapchain::createFrameBuffers(VulkanContext* context, VulkanRenderPass renderPass) {
	framebuffers.resize(images.size());
	for (uint32_t i = 0; i < images.size(); ++i) {
		VkImageView attachments[] = {
            imageViews[i]
        };
		VkFramebufferCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass.renderPass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = extent.width,
			.height = extent.height,
			.layers = 1
		};

		if (vkCreateFramebuffer(context->device, &createInfo, 0, &framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create frame buffer");
		}
	}
}

void VulkanSwapchain::destroySwapchain(VulkanContext* context) {
	for (uint32_t i = 0; i < this->imageViews.size(); ++i) {
		vkDestroyImageView(context->device, this->imageViews[i], 0);
	}

	for (uint32_t i = 0; i < framebuffers.size(); ++i) {
		vkDestroyFramebuffer(context->device, framebuffers[i], 0);
	}
	framebuffers.clear();

	vkDestroySwapchainKHR(context->device, this->swapchain, 0);
}
