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

bool enableValidationLayers = )" + enableValidationLayers + ";";

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
        .pApplicationName = ")" + appName + R"(",
        .apiVersion = )" + vulkanVersion + R"(
    };

    std::vector<const char*> enabledInstanceExtensions = getRequiredExtensions();

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        )" + platformFlags + R"(
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
)";
}

std::string generatePhysicalDeviceSelectionCode(const VkPhysicalDevice physicalDevice) {
	std::string output = R"(
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
})";
	return output;
}

std::string generateLogicalDeviceCreation() {
	return R"(
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
	)";
}

std::string generateVulkanInitialization() {
	return R"(
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
})";
}

std::string generateExitVulkanCode() {
	return R"(
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
})";
}

std::string generateMainCode() {
	return R"(
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
	)";
}

std::string generateVulkanCode(const VulkanConfig& config) {
    std::string output = R"(
        #include <vector>
        #include <string>

        struct VulkanConfig {
            std::string appName;
            std::vector<std::string> vulkanVersions;
            int vulkanVersionIndex;
            int selectedGPU;
            bool runOnMacOS;
            bool showDebug;
        };

        VulkanConfig config = {
    )";
	    output += "\"";
	    output += config.appName;
	    output += R"(",
        {)" +
    // Vulkan versions
    [&]() {
        std::string versions;
        for (size_t i = 0; i < config.vulkanVersions.size(); ++i) {
            versions += "\"";
            versions += config.vulkanVersions[i];
            versions += "\"";
            if (i < config.vulkanVersions.size() - 1) versions += ", ";
        }
        return versions;
    }() + R"(},
            )" + std::to_string(config.vulkanVersionIndex) + R"(,
            )" + std::to_string(config.selectedGPU) + R"(,
            )" + (config.runOnMacOS ? "true" : "false") + R"(,
            )" + (config.showDebug ? "true" : "false") + R"(
        };
    )" +
    generateHeader(config.showDebug, config.runOnMacOS) + "\n" +
    generateRequiredExtension(config.runOnMacOS) + "\n" +
    generateDebugSetup() + "\n" +
    generateInstanceCreationCode(config.appName, config.vulkanVersions[config.vulkanVersionIndex], generatePlatformFlags(config.runOnMacOS)) + "\n" +
    generatePhysicalDeviceSelectionCode(config.physicalDevices[config.selectedGPU]) + "\n" +
    generateLogicalDeviceCreation() + "\n" +
    generateVulkanInitialization() + "\n" +
    generateExitVulkanCode() + "\n" +
    generateMainCode();

    return output;
}
