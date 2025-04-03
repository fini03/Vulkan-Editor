#define IMGUI_IMPL_VULKAN
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include "vulkan_base/vulkan_base.h"
#include "vulkan_editor/vulkan_view.h"

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

// Data
VkAllocationCallbacks* g_Allocator = nullptr;
VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

SDL_Window* window;
ImGui_ImplVulkanH_Window* wd;
ImGui_ImplVulkanH_Window g_MainWindowData;
uint32_t g_MinImageCount = 2;
bool g_SwapChainRebuild = false;

ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
bool done = false;

VulkanContext* context = {};

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

const std::vector<std::string> templateFileNames = {
	"vulkan_templates/class.txt",
	"vulkan_templates/application.txt",
	"vulkan_templates/buffer.txt",
	"vulkan_templates/globalVariables.txt",
	"vulkan_templates/header.txt",
	"vulkan_templates/image.txt",
	"vulkan_templates/instance.txt",
	"vulkan_templates/logicalDevice.txt",
	"vulkan_templates/model.txt",
	"vulkan_templates/physicalDevice.txt",
	"vulkan_templates/pipeline.txt",
	"vulkan_templates/renderpass.txt",
	"vulkan_templates/swapchain.txt",
	"vulkan_templates/utils.txt",
};

Editor editor{templateFileNames};

static void check_vk_result(VkResult err) {
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
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

void createDescriptorPool() {
	VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
    };

    VkDescriptorPoolCreateInfo pool_info = {
    	.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
     	.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      	.maxSets = 0
    };

    for (VkDescriptorPoolSize& pool_size : pool_sizes) {
        pool_info.maxSets += pool_size.descriptorCount;
    }

    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    vkCreateDescriptorPool(context->device, &pool_info, g_Allocator, &g_DescriptorPool);
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
    // Create Descriptor Pool
    createDescriptorPool();
}

void initWindow() {
	if (!SDL_Init(SDL_INIT_VIDEO| SDL_INIT_GAMEPAD)) {
 		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
   		return;
 	}

  	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);
	window = SDL_CreateWindow("Vulkan Tutorial", 1920, 1080, window_flags);
 	if (!window) {
  		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
  	}
}

void createVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height) {
    wd->Surface = surface;

    // Check for WSI support
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(context->physicalDevice, context->graphicsQueue.familyIndex, wd->Surface, &presentSupport);
    if (!presentSupport) {
        std::cerr << "Error: No WSI support on physical device 0" << std::endl;
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(context->physicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
	#ifdef APP_USE_UNLIMITED_FRAME_RATE
	    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
	#else
	    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
	#endif

    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(context->physicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));

    // Create SwapChain, RenderPass, Framebuffer, etc.
    ImGui_ImplVulkanH_CreateOrResizeWindow(context->instance, context->physicalDevice, context->device, wd, context->graphicsQueue.familyIndex, g_Allocator, width, height, g_MinImageCount);
}

void cleanup() {
 	vkDeviceWaitIdle(context->device);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    ImGui_ImplVulkanH_DestroyWindow(context->instance, context->device, &g_MainWindowData, g_Allocator);
    vkDestroyDescriptorPool(context->device, g_DescriptorPool, g_Allocator);
    context->exitVulkan();

    SDL_DestroyWindow(window);
    SDL_Quit();
}

void render(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data) {
    VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkResult err = vkAcquireNextImageKHR(context->device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex]; {
        err = vkWaitForFences(context->device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
        check_vk_result(err);

        err = vkResetFences(context->device, 1, &fd->Fence);
        check_vk_result(err);
    }

    {
        err = vkResetCommandPool(context->device, fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
    }

    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);
        err = vkQueueSubmit(context->graphicsQueue.queue, 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

void present(ImGui_ImplVulkanH_Window* wd) {
    if (g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(context->graphicsQueue.queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount; // Now we can use the next set of semaphores
}

void initSurface() {
 	// Create Window Surface
    VkSurfaceKHR surface;

    if (SDL_Vulkan_CreateSurface(window, context->instance, g_Allocator, &surface) == 0) {
        printf("Failed to create Vulkan surface.\n");
    }

    // Create Framebuffers
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    wd = &g_MainWindowData;
    createVulkanWindow(wd, surface, width, height);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);
}

void initImgui() {
	// Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    //init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance = context->instance;
    init_info.PhysicalDevice = context->physicalDevice;
    init_info.Device = context->device;
    init_info.QueueFamily = context->graphicsQueue.familyIndex;
    init_info.Queue = context->graphicsQueue.queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.RenderPass = wd->RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = g_Allocator;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);
}

void runEditor() {
	int width, height;
	SDL_GetWindowSize(window, &width, &height); // Get real-time window size

	ImGui::SetNextWindowSize(ImVec2((float)width, (float)height), ImGuiCond_Always); // Always resize
	ImGui::SetNextWindowPos(ImVec2(0, 0)); // Lock to top-left corner

	ImGui::Begin("Graphical Vulkan Editor", nullptr,
	    ImGuiWindowFlags_NoCollapse |
	    ImGuiWindowFlags_NoMove |
	    ImGuiWindowFlags_NoTitleBar
	);

	editor.startEditor();

	ImGui::End();
}

void handleMessage() {
	SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            return;
        }

        // Resize swap chain?
        int fb_width, fb_height;
        SDL_GetWindowSize(window, &fb_width, &fb_height);
        if (fb_width > 0 && fb_height > 0 && (g_SwapChainRebuild || g_MainWindowData.Width != fb_width || g_MainWindowData.Height != fb_height)) {
            ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
            ImGui_ImplVulkanH_CreateOrResizeWindow(context->instance, context->physicalDevice, context->device, &g_MainWindowData, context->graphicsQueue.familyIndex, g_Allocator, fb_width, fb_height, g_MinImageCount);
            g_MainWindowData.FrameIndex = 0;
            g_SwapChainRebuild = false;
        }
}

void renderFrame() {
    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized)
    {
        wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
        wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
        wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
        wd->ClearValue.color.float32[3] = clear_color.w;
        render(wd, draw_data);
        present(wd);
    }
}

// Main code
int main() {
	initWindow();
    initVulkan();
    initSurface();
    initImgui();

    while (!done) {
    	handleMessage();

     	// Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        runEditor();
        renderFrame();
    }

    cleanup();

    return 0;
}
