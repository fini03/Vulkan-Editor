#pragma once
#include <limits>
#include <vulkan/vulkan_core.h>
#include <cassert>
#include <cstdint>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <array>


struct VulkanQueue {
	VkQueue queue;
	uint32_t familyIndex;
};

class VulkanContext {
public:
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkDevice device;
	VulkanQueue graphicsQueue;
	VkDebugUtilsMessengerEXT debugCallback = nullptr;

	// Methods
	VulkanContext* initVulkan(
		uint32_t instanceExtensionCount,
		const std::vector<const char*> instanceExtensions,
		uint32_t deviceExtensionCount,
		const std::vector<const char*> deviceExtensions,
		bool enableValidationLayers
	);
	void exitVulkan();
};

class VulkanRenderPass {
public:
	VkRenderPass renderPass;

	// Methods
	void createRenderPass(VulkanContext* context, VkFormat format);
	void destroyRenderpass(VulkanContext* context);
};

class VulkanSwapchain {
public:
	VkSwapchainKHR swapchain;
	VkExtent2D extent;
	VkFormat format;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	std::vector<VkFramebuffer> framebuffers;

	void createSwapchain(
		VulkanContext* context,
		VkSurfaceKHR surface,
		VkImageUsageFlags usage,
		VkExtent2D extent = { 0, 0 }
	);
	void createFrameBuffers(
		VulkanContext* context,
		VulkanRenderPass renderPass
	);
	void destroySwapchain(VulkanContext* context);
};

class VulkanPipeline {
public:
	std::string name;
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	// Methods
	void createPipeline(
		VulkanContext* context,
		const char* vertexShaderFilename,
		const char* fragmentShaderFilename,
		VkRenderPass renderPass,
		uint32_t numSetLayouts,
		VkDescriptorSetLayout* setLayouts
	);
	void destroyPipeline(VulkanContext* context);
};
