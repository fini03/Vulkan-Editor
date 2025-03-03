#include <vulkan/vulkan_core.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_video.h>

#define IMGUI_IMPL_VULKAN
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl3.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "vulkan_editor/vulkan_base.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <array>

constexpr int MAX_FRAMES_IN_FLIGHT  = 2;

#ifdef __APPLE__
const std::vector<const char*> enabledDeviceExtensions = {
    "VK_KHR_swapchain",
    "VK_KHR_portability_subset"
};
#else
const std::vector<const char*> enabledDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
#endif

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class VulkanTutorial {
public:
    void run() {
        initWindow();
        initVulkan();
        while (handleMessage()) {
        	//TODO: Render with Vulkan
         	ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            //ImGui::ShowDemoWindow();

        	render();
        }
        cleanup();
    }

private:
    SDL_Window* window = {};

    VulkanContext* context = new VulkanContext();

    VkSurfaceKHR surface = {};

    VulkanSwapchain swapchain = {};

    VulkanRenderPass renderPass = {};
    std::vector<VkCommandPool> commandPools{ MAX_FRAMES_IN_FLIGHT };
    std::vector<VkCommandBuffer> commandBuffers{ MAX_FRAMES_IN_FLIGHT };

    VkDescriptorPool descriptorPool = {};

    std::vector<VkSemaphore> acquireSemaphores{ MAX_FRAMES_IN_FLIGHT };
    std::vector<VkSemaphore> releaseSemaphores{ MAX_FRAMES_IN_FLIGHT };
    std::vector<VkFence> fences{ MAX_FRAMES_IN_FLIGHT };
    uint32_t currentFrame = 0;

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
        std::vector<const char*> enabledInstanceExtensions = getRequiredExtensions();
        context = context->initVulkan(
        	static_cast<uint32_t>(enabledInstanceExtensions.size()),
         	enabledInstanceExtensions,
          	static_cast<uint32_t>(enabledDeviceExtensions.size()),
           	enabledDeviceExtensions,
            enableValidationLayers
        );
        createSurface();

        int width = 0;
        int height = 0;
		if (!SDL_GetWindowSize(window, &width, &height)) {
			std::cerr << "SDL_GetWindowSize Error: " << SDL_GetError() << std::endl;
		}

		VkExtent2D extent = {
           	static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        swapchain.createSwapchain(
        	context,
         	surface,
          	VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
           	extent
        );

        renderPass.createRenderPass(context, swapchain.format);

        createSyncObjects();

        createCommandPool();
        createCommandBuffers();

        swapchain.createFrameBuffers(context, renderPass);

        createDescriptorPool();

        setupImgui(
        	context,
            commandPools[currentFrame],
            descriptorPool,
            renderPass.renderPass
        );
    }

    bool handleMessage() {
    	SDL_Event event = {};
     	while (SDL_PollEvent(&event)) {
      		ImGui_ImplSDL3_ProcessEvent(&event);
      		switch (event.type) {
        	case SDL_EVENT_QUIT:
         		return false;
        	}
      	}

        return true;
    }

    void setupImgui(VulkanContext* context, VkCommandPool commandPool, VkDescriptorPool descriptorPool, VkRenderPass renderPass) {
            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
            //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

            // Setup Platform/Renderer backends
            ImGui_ImplSDL3_InitForVulkan(window);

            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = context->instance;
            init_info.PhysicalDevice = context->physicalDevice;
            init_info.Device = context->device;
            init_info.QueueFamily = context->graphicsQueue.familyIndex;
            init_info.Queue = context->graphicsQueue.queue;
            init_info.PipelineCache = VK_NULL_HANDLE;
            init_info.DescriptorPool = descriptorPool;
            init_info.RenderPass = renderPass;
            init_info.Subpass = 0;
            init_info.MinImageCount = 3;
            init_info.ImageCount = 3;
            init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            init_info.Allocator = nullptr;
            init_info.CheckVkResultFn = nullptr;
            ImGui_ImplVulkan_Init(&init_info);
        }

    void cleanup() {
    	vkDeviceWaitIdle(context->device);

     	ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

     	swapchain.destroySwapchain(context);

        renderPass.destroyRenderpass(context);

        vkDestroyDescriptorPool(context->device, descriptorPool, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(context->device, releaseSemaphores[i], nullptr);
            vkDestroySemaphore(context->device, acquireSemaphores[i], nullptr);
            vkDestroyFence(context->device, fences[i], nullptr);
        }

        for (uint32_t i = 0; i < commandPools.size(); i++) {
        	vkDestroyCommandPool(context->device, commandPools[i], 0);
        }

        vkDestroySurfaceKHR(context->instance, surface, nullptr);

        context->exitVulkan();

        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        SDL_GetWindowSize(window, &width, &height);
        while (width == 0 || height == 0) {
        	SDL_Event event;
            SDL_WaitEvent(&event);
            SDL_GetWindowSize(window, &width, &height);
        }

        vkDeviceWaitIdle(context->device);

        swapchain.destroySwapchain(context);

        swapchain.createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        //swapchain.createDepthResources(context);
        swapchain.createFrameBuffers(context, renderPass);
    }

    void createSurface() {
    	if (!SDL_Vulkan_CreateSurface(window, context->instance, 0, &surface)) {
     		std::cerr << "SDL_CreateSurface Error: " << SDL_GetError() << std::endl;
     	}
    }

    void createSyncObjects() {
	    for(uint32_t i = 0; i < fences.size(); ++i) {
	    	VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	     	createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	      	if (vkCreateFence(context->device, &createInfo, 0, &fences[i]) != VK_SUCCESS) {
	       		throw std::runtime_error("Failed to create fence");
	       	}
	    }

	    for(uint32_t i = 0; i < acquireSemaphores.size(); ++i) {
	    	VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	     	if (vkCreateSemaphore(context->device, &createInfo, 0, &acquireSemaphores[i]) != VK_SUCCESS) {
	      		throw std::runtime_error("Failed to create Semaphore");
	      	}
	       	if (vkCreateSemaphore(context->device, &createInfo, 0, &releaseSemaphores[i]) != VK_SUCCESS) {
	        	throw std::runtime_error("Failed to create Semaphore");
	        }
	    }
    }

    void createCommandPool() {
	    for (uint32_t i = 0; i < commandPools.size(); i++) {
	    	VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	     	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	      	createInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;
	       	if (vkCreateCommandPool(context->device, &createInfo, 0, &commandPools[i]) != VK_SUCCESS) {
	        	throw std::runtime_error("Failed to create command pool");
	        }
	    }
    }

    void createCommandBuffers() {
	    for (uint32_t i = 0; i < commandPools.size(); i++) {
	    	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	     	allocateInfo.commandPool = commandPools[i];
	      	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	       	allocateInfo.commandBufferCount = 1;
	        if (vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffers[i]) != VK_SUCCESS) {
	        	throw std::runtime_error("Failed to allocate command buffers");
	        }
	    }
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize pool_sizes[] = {
        	{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        vkCreateDescriptorPool(context->device, &pool_info, nullptr, &descriptorPool);
    }

    void createImguiWindow() {
        int width, height;
        SDL_GetWindowSize(window, &width, &height); // Get real-time window size

        ImGui::SetNextWindowSize(ImVec2((float)width, (float)height), ImGuiCond_Always); // Always resize
        ImGui::SetNextWindowPos(ImVec2(0, 0)); // Lock to top-left corner

        ImGui::Begin("Graphical Vulkan Editor", nullptr,
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize);

        if (ImGui::BeginTabBar("MainTabBar")) {
            if (ImGui::BeginTabItem("Instance")) {
                ImGui::Text("Application Name:");
                static char appName[256] = "";
                ImGui::InputText("##appName", appName, IM_ARRAYSIZE(appName));

                static bool showDebug = true;
                ImGui::Checkbox("Show Validation Layer Debug Info", &showDebug);

                static bool runOnMacOS = false;
                ImGui::Checkbox("Run on MacOS", &runOnMacOS);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Physical Device")) {
                ImGui::Text("Select a Physical Device:");
                static int selectedGPU = 0;
                const char* devices[] = { "GPU 1", "GPU 2", "GPU 3" }; // Placeholder
                ImGui::Combo("##gpuSelector", &selectedGPU, devices, IM_ARRAYSIZE(devices));

                ImGui::Text("Device Properties:");
                ImGui::Text("Memory: 8GB");
                ImGui::Text("Compute Units: 32");

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Logical Device")) {
                ImGui::Text("Queue Families:");
                static bool graphicsQueue = true;
                static bool computeQueue = false;
                ImGui::Checkbox("Graphics Queue", &graphicsQueue);
                ImGui::Checkbox("Compute Queue", &computeQueue);

                ImGui::Text("Extensions:");
                static bool enableRayTracing = false;
                ImGui::Checkbox("Enable Ray Tracing", &enableRayTracing);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Swapchain")) {
                ImGui::Text("Choose Swapchain Settings:");

                static int swapchainImageCount = 3;
                ImGui::SliderInt("Image Count", &swapchainImageCount, 2, 8);

                static int vsyncMode = 0;
                const char* vsyncOptions[] = { "Immediate", "Mailbox", "Fifo", "Fifo Relaxed" };
                ImGui::Combo("VSync Mode", &vsyncMode, vsyncOptions, IM_ARRAYSIZE(vsyncOptions));

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Model")) {
                static char modelPath[256] = "models/viking_room.obj";
                static char texturePath[256] = "textures/viking_room.png";

                ImGui::Text("Model File:");
                ImGui::InputText("##modelFile", modelPath, IM_ARRAYSIZE(modelPath));
                ImGui::SameLine();
                if (ImGui::Button("...")) {
                    // Open file dialog
                }

                ImGui::Text("Texture File:");
                ImGui::InputText("##textureFile", texturePath, IM_ARRAYSIZE(texturePath));
                ImGui::SameLine();
                if (ImGui::Button("...")) {
                    // Open file dialog
                }

                if (ImGui::Button("Load Model")) {
                    // Load model logic
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Graphics Pipeline")) {
                static bool enableWireframe = false;
                static int renderMode = 0;
                const char* renderModes[] = { "Lit", "Wireframe", "Normals" };

                ImGui::Checkbox("Enable Wireframe", &enableWireframe);
                ImGui::Combo("Render Mode", &renderMode, renderModes, IM_ARRAYSIZE(renderModes));

                if (ImGui::Button("Rebuild Pipeline")) {
                    // Rebuild pipeline logic
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // Bottom Button: Generate GVE Project Header
        ImGui::SetCursorPosY(height - 60); // Align near bottom
        ImGui::Separator();
        if (ImGui::Button("Generate GVE Project Header", ImVec2(ImGui::GetContentRegionAvail().x, 40))) {
            // Handle button press
        }

        ImGui::End();
    }


    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
     	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassInfo.renderPass = renderPass.renderPass;
        renderPassInfo.framebuffer = swapchain.framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchain.extent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        	//----------------------------------------------------------------------------------
         	createImguiWindow();

            ImGui::Render();

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
            //----------------------------------------------------------------------------------

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void render() {
        vkWaitForFences(context->device, 1, &fences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(
        	context->device,
         	swapchain.swapchain,
          	UINT64_MAX,
           	acquireSemaphores[currentFrame],
            VK_NULL_HANDLE,
            &imageIndex
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(context->device, 1, &fences[currentFrame]);
        vkResetCommandPool(context->device, commandPools[currentFrame], 0);

        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &acquireSemaphores[currentFrame];
        VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submitInfo.pWaitDstStageMask = &waitMask;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &releaseSemaphores[currentFrame];


        if (vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, fences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &releaseSemaphores[currentFrame];
        result = vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo);


        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t instanceExtensionCount = 0;
        const char* const* instanceExtensions = SDL_Vulkan_GetInstanceExtensions(&instanceExtensionCount);
        std::vector<const char*> enabledInstanceExtensions(instanceExtensions, instanceExtensions + instanceExtensionCount);

        if (enableValidationLayers) {
            enabledInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        #ifdef __APPLE__
            enabledInstanceExtensions.push_back("VK_MVK_macos_surface");
            enabledInstanceExtensions.push_back("VK_KHR_get_physical_device_properties2");
            enabledInstanceExtensions.push_back("VK_KHR_portability_enumeration");
        #endif

        return enabledInstanceExtensions;
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
