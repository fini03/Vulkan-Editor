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
