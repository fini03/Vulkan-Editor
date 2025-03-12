
        #include <vector>
        #include <string>

        struct VulkanConfig {
            std::string appName;
            std::vector<std::string> vulkanVersions;
            int vulkanVersionIndex;
            int selectedGPU;
            bool runOnMacOS;
            bool showDebug;

            float clearColor[4];
            int framesInFlight;
            int swapchainImageCount;
            std::string imageUsage;
            std::string presentMode;
            std::string imageFormat;
            std::string imageColorSpace;
        };

        VulkanConfig config = {
    "",{"VK_API_VERSION_1_0", "VK_API_VERSION_1_1", "VK_API_VERSION_1_2", "VK_API_VERSION_1_3", "VK_API_VERSION_1_4"},
0,
0,
false,
true,
{ 0.000000f, 0.000000f, 0.000000f, 1.000000f },
2,
3,
"VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT",
"VK_PRESENT_MODE_MAILBOX_KHR",
"VK_FORMAT_B8G8R8A8_UNORM",
"VK_COLORSPACE_SRGB_NONLINEAR_KHR"
};

#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <limits>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_video.h>

bool enableValidationLayers = true;
const std::vector<const char*> enabledDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
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
	VulkanContext* initVulkan();
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

bool initVulkanInstance(VulkanContext* context) {
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
        .pApplicationName = "",
        .apiVersion = VK_API_VERSION_1_0
    };

    std::vector<const char*> enabledInstanceExtensions = getRequiredExtensions();

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensions.size()),
        .ppEnabledExtensionNames = enabledInstanceExtensions.data()
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


bool selectPhysicalDevice(VulkanContext* context, const int selectedGPU) {
	uint32_t numDevices = 0;
	vkEnumeratePhysicalDevices(context->instance, &numDevices, 0);
	if(!numDevices) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> physicalDevices(numDevices);
	vkEnumeratePhysicalDevices(context->instance, &numDevices, physicalDevices.data());

	// TODO: Is it okay to always pick the first one?
	// Picking first device should be fine for now
	// hopefully this doesn't bite us
	context->physicalDevice = physicalDevices[selectedGPU];
	vkGetPhysicalDeviceProperties(context->physicalDevice, &context->physicalDeviceProperties);

	return true;
}

		bool createLogicalDevice(VulkanContext* context) {
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
				.enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size()),
				.ppEnabledExtensionNames = enabledDeviceExtensions.data(),
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
		.minImageCount = 3,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
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

VulkanContext* VulkanContext::initVulkan() {
	VulkanContext* context = new VulkanContext;

	if (!initVulkanInstance(context)) {
		return 0;
	}

	if (!selectPhysicalDevice(context, config.selectedGPU)) {
		return 0;
	}

	if (!createLogicalDevice(context)) {
		return 0;
	}

	return context;
}

void VulkanContext::exitVulkan() {
	vkDeviceWaitIdle(this->device);
	vkDestroyDevice(this->device, 0);

	if (this->debugCallback != nullptr) {
		PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT;
		pfnDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->instance, "vkDestroyDebugUtilsMessengerEXT");
		pfnDestroyDebugUtilsMessengerEXT(this->instance, this->debugCallback, 0);
		this->debugCallback = 0;
	}

	vkDestroyInstance(this->instance, 0);
}

		class VulkanTutorial {
public:
    void run() {
        initWindow();
        initVulkan();
        while (handleMessage()) {
        	//TODO: Render with Vulkan

        	render();
        }
        cleanup();
    }

private:
    SDL_Window* window = {};

    VulkanContext* context = new VulkanContext();

    VkSurfaceKHR surface = {};

    void initWindow() {
    	if (!SDL_Init(SDL_INIT_VIDEO)) {
     		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
       		return;
     	}

    	window = SDL_CreateWindow("Vulkan Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
     	if (!window) {
      		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
      	}
    }

    void initVulkan() {
        context = context->initVulkan();

        createSurface();
    }

    bool handleMessage() {
    	SDL_Event event = {};
     	while (SDL_PollEvent(&event)) {
      		switch (event.type) {
        	case SDL_EVENT_QUIT:
         		return false;
        	}
      	}

        return true;
    }

    void cleanup() {
    	vkDeviceWaitIdle(context->device);

        vkDestroySurfaceKHR(context->instance, surface, nullptr);

        context->exitVulkan();

        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void createSurface() {
    	if (!SDL_Vulkan_CreateSurface(window, context->instance, 0, &surface)) {
     		std::cerr << "SDL_CreateSurface Error: " << SDL_GetError() << std::endl;
     	}
    }

    void render() {
    }
};

int main() {
    VulkanTutorial vulkanTutorial;

    try {
        vulkanTutorial.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
	