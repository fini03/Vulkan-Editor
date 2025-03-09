#pragma once
#include <array>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <vulkan/vulkan.h>

struct VulkanConfig {
	bool runOnLinux = true;
	bool runOnMacOS = false;
	bool runOnWindows = false;
	bool showDebug = true;
	char appName[256] = "";

	std::vector<std::string> gpuNames;
	std::vector<VkPhysicalDevice> physicalDevices;
	int selectedGPU = 0;

	std::vector<std::string> availableExtensions;
	std::map<std::string, bool> extensionSelection;

	bool graphicsQueue = true;
	bool computeQueue = false;

	float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	int framesInFlight = 2;
	int swapchainImageCount = 3;
	int imageUsage = 0;
	std::array<const char*, 3> imageUsageOptions = {
	        "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT",
	        "VK_IMAGE_USAGE_TRANSFER_SRC_BIT",
	        "VK_IMAGE_USAGE_TRANSFER_DST_BIT"
	    };
	int presentMode = 1;
	std::array<const char*, 4> presentModeOptions = {
	    "VK_PRESENT_MODE_IMMEDIATE_KHR",
	    "VK_PRESENT_MODE_MAILBOX_KHR",
	    "VK_PRESENT_MODE_FIFO_KHR",
	    "VK_PRESENT_MODE_FIFO_RELAXED_KHR"
	};
	int imageFormat = 0;
	std::array<const char*, 3> imageFormatOptions = {
	    "VK_FORMAT_B8G8R8A8_UNORM",
	    "VK_FORMAT_B8G8R8A8_SRGB",
	    "VK_FORMAT_R8G8B8A8_SRGB"
	};
	int imageColorSpace = 0;
	std::array<const char*, 1> imageColorSpaceOptions = {
	    "VK_COLORSPACE_SRGB_NONLINEAR_KHR"
	};

	int vulkanVersionIndex = 0; // Default index (corresponds to Vulkan 1.0)
	std::array<const char*, 5> vulkanVersions = { "VK_API_VERSION_1_0", "VK_API_VERSION_1_1", "VK_API_VERSION_1_2", "VK_API_VERSION_1_3", "VK_API_VERSION_1_4" };
};
