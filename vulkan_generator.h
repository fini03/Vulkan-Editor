#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "vulkan_config.h"

std::string generateHeader(bool showDebug, bool runOnMacOS) {
	std::cout << "habibi\n";
	std::string enableValidationLayers = showDebug ? "true" : "false";

	std::string output = R"(
#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <cstring>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_video.h>

bool enableValidationLayers = )" + enableValidationLayers;

if(runOnMacOS) {
	output += R"(
const std::vector<const char*> enabledDeviceExtensions = {
	"VK_KHR_swapchain",
	"VK_KHR_portability_subset"
};)";
} else {
	output += R"(
const std::vector<const char*> enabledDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};)";
}

output += R"(
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
)";
	return output;
}

std::string generateRequiredExtension(bool runOnMacOS) {
	std::string output = R"(
std::vector<const char*> getRequiredExtensions() {
	uint32_t instanceExtensionCount = 0;
    const char* const* instanceExtensions = SDL_Vulkan_GetInstanceExtensions(&instanceExtensionCount);
    std::vector<const char*> enabledInstanceExtensions(instanceExtensions, instanceExtensions + instanceExtensionCount);

    if (enableValidationLayers) {
    	enabledInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    })";

	if (runOnMacOS) {
	output += R"(
    #ifdef __APPLE__
    	enabledInstanceExtensions.push_back("VK_MVK_macos_surface");
        enabledInstanceExtensions.push_back("VK_KHR_get_physical_device_properties2");
        enabledInstanceExtensions.push_back("VK_KHR_portability_enumeration");
    #endif
	)";
	}
    return output + R"(
    return enabledInstanceExtensions;
})";
}

// Function to generate platform flags for MacOS
std::string generatePlatformFlags(bool runOnMacOS) {
    if (runOnMacOS) {
        return "#ifdef __APPLE__\n\t\t.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,\n#endif\n";
    }
    return "";
}

// Function to generate debug setup code
std::string generateDebugSetup() {
    return R"(
VkBool32 VKAPI_CALL debugReportCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		std::cerr << "ERROR: " << callbackData->pMessage << std::endl;
	} else {
		std::cout << "WARN: " << callbackData->pMessage << std::endl;
	}
	return VK_FALSE;
}

VkDebugUtilsMessengerEXT registerDebugCallback(VkInstance instance) {
	PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebutUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	VkDebugUtilsMessengerCreateInfoEXT callbackInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
		.pfnUserCallback = debugReportCallback
	};

	VkDebugUtilsMessengerEXT callback = 0;
	pfnCreateDebutUtilsMessengerEXT(instance, &callbackInfo, 0, &callback);

	return callback;
})";
}

// Function to generate the complete Vulkan instance creation code
std::string generateInstanceCreationCode(const std::string& appName, const std::string& vulkanVersion, const std::string& platformFlags) {
    return R"(
bool initVulkanInstance(VulkanContext* context, uint32_t instanceExtensionCount, const std::vector<const char*> instanceExtensions) {
	uint32_t layerPropertyCount = 0;
	vkEnumerateInstanceLayerProperties(&layerPropertyCount, 0);
	std::vector<VkLayerProperties> availableLayers(layerPropertyCount);
   vkEnumerateInstanceLayerProperties(&layerPropertyCount, availableLayers.data());

	const std::vector<const char*> enabledLayers = {
		"VK_LAYER_KHRONOS_validation",
	};

	std::vector<VkValidationFeatureEnableEXT> enableValidationFeatures = {
		VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
	};
	VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	validationFeatures.enabledValidationFeatureCount = static_cast<uint32_t>(enableValidationFeatures.size());
	validationFeatures.pEnabledValidationFeatures = enableValidationFeatures.data();

    VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = ")" + appName + R"(",
        .apiVersion = )" + vulkanVersion + R"(
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        )" + platformFlags + R"(
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = instanceExtensionCount,
        .ppEnabledExtensionNames = instanceExtensions.data()
    };

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = 1;
        const char* validationLayer = "VK_LAYER_KHRONOS_validation";
        createInfo.ppEnabledLayerNames = &validationLayer;
    }

    if (vkCreateInstance(&createInfo, nullptr, &context->instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }

    if (enableValidationLayers) {
        context->debugCallback = registerDebugCallback(context->instance);
    }

    return true;
}
)";
}

std::string generatePhysicalDeviceSelectionCode(const VkPhysicalDevice physicalDevice) {
	std::string output = R"(
bool selectPhysicalDevice(VulkanContext* context, const VkPhysicalDevice physicalDevice) {

	context->physicalDevice = physicalDevice;
	vkGetPhysicalDeviceProperties(context->physicalDevice, &context->physicalDeviceProperties);

	return true;
})";
	return output;
}

std::string generateLogicalDeviceCreation() {
	return R"(
		bool createLogicalDevice(VulkanContext* context, uint32_t deviceExtensionCount, const std::vector<const char*> deviceExtensions) {
			// Queues
			uint32_t numQueueFamilies = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, 0);

			std::vector<VkQueueFamilyProperties> queueFamilies(numQueueFamilies);
			vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, queueFamilies.data());

			uint32_t graphicsQueueIndex = 0;
			for (uint32_t i = 0; i < numQueueFamilies; ++i) {
				VkQueueFamilyProperties queueFamily = queueFamilies[i];
				if (queueFamily.queueCount > 0) {
					if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
						graphicsQueueIndex = i;
						break;
					}
				}
			}

			float priorities[] = { 1.0f };
			VkDeviceQueueCreateInfo queueCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = graphicsQueueIndex,
				.queueCount = 1,
				.pQueuePriorities = priorities
			};

			VkPhysicalDeviceFeatures enabledFeatures = {
				.samplerAnisotropy = VK_TRUE
			};

			VkDeviceCreateInfo createInfo = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.queueCreateInfoCount = 1,
				.pQueueCreateInfos = &queueCreateInfo,
				.enabledExtensionCount = deviceExtensionCount,
				.ppEnabledExtensionNames = deviceExtensions.data(),
				.pEnabledFeatures = &enabledFeatures
			};

			if (vkCreateDevice(context->physicalDevice, &createInfo, 0, &context->device) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create logical device");
			}

			// Acquire queues
			context->graphicsQueue.familyIndex = graphicsQueueIndex;
			vkGetDeviceQueue(context->device, graphicsQueueIndex, 0, &context->graphicsQueue.queue);
			return true;
}
	)";
}

// Function to generate Vulkan code based on the provided parameters
std::string generateVulkanCode(const VulkanConfig& config) {
    // OS-specific flags
    std::string platformFlags = generatePlatformFlags(config.runOnMacOS);

    // Vulkan instance creation code
    return generateHeader(config.showDebug, config.runOnMacOS) + "\n" + \
    generateRequiredExtension(config.runOnMacOS) + "\n" + \
    generateDebugSetup() + "\n" + \
    generateInstanceCreationCode(config.appName, config.vulkanVersions[config.vulkanVersionIndex], platformFlags) + "\n" + \
    generatePhysicalDeviceSelectionCode(config.physicalDevices[config.selectedGPU]) + "\n";
}
