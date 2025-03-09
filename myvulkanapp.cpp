
#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <cstring>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_video.h>

bool enableValidationLayers = true;

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


std::vector<const char*> getRequiredExtensions() {
	uint32_t instanceExtensionCount = 0;
    const char* const* instanceExtensions = SDL_Vulkan_GetInstanceExtensions(&instanceExtensionCount);
    std::vector<const char*> enabledInstanceExtensions(instanceExtensions, instanceExtensions + instanceExtensionCount);

    if (enableValidationLayers) {
    	enabledInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return enabledInstanceExtensions;
}

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
}

bool initVulkanInstance(VulkanContext* context, uint32_t instanceExtensionCount, const std::vector<const char*> instanceExtensions) {
    VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "",
        .apiVersion = VK_API_VERSION_1_0
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        
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
