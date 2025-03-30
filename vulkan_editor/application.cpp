#include "application.h"

ApplicationNode::ApplicationNode(int id) : Node(id) {}
ApplicationNode::~ApplicationNode() { }

void ApplicationNode::render() const {}

void ApplicationNode::generateApplication(std::ofstream& outFile) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << R"(

	void createSurface(VkInstance instance, VkSurfaceKHR& surface) {
	    if (SDL_Vulkan_CreateSurface(m_sdlWindow, instance, &surface) == 0) {
	        printf("Failed to create Vulkan surface.\n");
	    }
	}

	void drawFrame(SDL_Window* window,
	    VkSurfaceKHR surface,
	    VkPhysicalDevice physicalDevice,
	    VkDevice device,
	    VmaAllocator vmaAllocator,
	    VkQueue graphicsQueue,
	    VkQueue presentQueue,
	    SwapChain& swapChain,
	    DepthImage& depthImage,
	    VkRenderPass renderPass,
	    Pipeline& graphicsPipeline,
	    std::vector<Object>& objects,
	    std::vector<VkCommandBuffer>& commandBuffers,
	    SyncObjects& syncObjects,
	    uint32_t& currentFrame,
	    bool& framebufferResized
	) {
	    vkWaitForFences(device, 1, &syncObjects.m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	    uint32_t imageIndex;
	    VkResult result = vkAcquireNextImageKHR(device, swapChain.m_swapChain, UINT64_MAX
	                        , syncObjects.m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	    if (result == VK_ERROR_OUT_OF_DATE_KHR ) {
	        recreateSwapChain(window, surface, physicalDevice, device, vmaAllocator, swapChain, depthImage, renderPass);
	        return;
	    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
	        throw std::runtime_error("failed to acquire swap chain image!");
	    }

	    updateUniformBuffer(currentFrame, swapChain, objects);

	    vkResetFences(device, 1, &syncObjects.m_inFlightFences[currentFrame]);

	    vkResetCommandBuffer(commandBuffers[currentFrame],  0);
	    recordCommandBuffer(commandBuffers[currentFrame], imageIndex, swapChain, renderPass, graphicsPipeline, objects, currentFrame);

	    VkSubmitInfo submitInfo{};
	    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	    VkSemaphore waitSemaphores[] = {syncObjects.m_imageAvailableSemaphores[currentFrame]};
	    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	    submitInfo.waitSemaphoreCount = 1;
	    submitInfo.pWaitSemaphores = waitSemaphores;
	    submitInfo.pWaitDstStageMask = waitStages;

	    submitInfo.commandBufferCount = 1;
	    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

	    VkSemaphore signalSemaphores[] = {syncObjects.m_renderFinishedSemaphores[currentFrame]};
	    submitInfo.signalSemaphoreCount = 1;
	    submitInfo.pSignalSemaphores = signalSemaphores;

	    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, syncObjects.m_inFlightFences[currentFrame]) != VK_SUCCESS) {
	        throw std::runtime_error("failed to submit draw command buffer!");
	    }

	    VkPresentInfoKHR presentInfo{};
	    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	    presentInfo.waitSemaphoreCount = 1;
	    presentInfo.pWaitSemaphores = signalSemaphores;

	    VkSwapchainKHR swapChains[] = {swapChain.m_swapChain};
	    presentInfo.swapchainCount = 1;
	    presentInfo.pSwapchains = swapChains;

	    presentInfo.pImageIndices = &imageIndex;

	    result = vkQueuePresentKHR(presentQueue, &presentInfo);

	    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
	        framebufferResized = false;
	        recreateSwapChain(window, surface, physicalDevice, device, vmaAllocator, swapChain, depthImage, renderPass);
	    }
	    else if (result != VK_SUCCESS) {
	        throw std::runtime_error("failed to present swap chain image!");
	    }
	    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void cleanup() {
	    ImGui_ImplVulkan_Shutdown();
	    ImGui_ImplSDL2_Shutdown();
	    ImGui::DestroyContext();

	    cleanupSwapChain(m_device, m_vmaAllocator, m_swapChain, m_depthImage);

	    vkDestroyPipeline(m_device, m_graphicsPipeline.m_pipeline, nullptr);
	    vkDestroyPipelineLayout(m_device, m_graphicsPipeline.m_pipelineLayout, nullptr);
	    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

	    for( auto& object : m_objects) {
	        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
	            destroyBuffer(m_device, m_vmaAllocator, object.m_uniformBuffers.m_uniformBuffers[i], object.m_uniformBuffers.m_uniformBuffersAllocation[i]);
	        }

	        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

	        vkDestroySampler(m_device, object.m_texture.m_textureSampler, nullptr);
	        vkDestroyImageView(m_device, object.m_texture.m_textureImageView, nullptr);

	        destroyImage(m_device, m_vmaAllocator, object.m_texture.m_textureImage, object.m_texture.m_textureImageAllocation);

	        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

	        destroyBuffer(m_device, m_vmaAllocator, object.m_geometry.m_indexBuffer, object.m_geometry.m_indexBufferAllocation);

	        destroyBuffer(m_device, m_vmaAllocator, object.m_geometry.m_vertexBuffer, object.m_geometry.m_vertexBufferAllocation);
	    }

	    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
	        vkDestroySemaphore(m_device, m_syncObjects.m_renderFinishedSemaphores[i], nullptr);
	        vkDestroySemaphore(m_device, m_syncObjects.m_imageAvailableSemaphores[i], nullptr);
	        vkDestroyFence(m_device, m_syncObjects.m_inFlightFences[i], nullptr);
	    }

	    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

	    vmaDestroyAllocator(m_vmaAllocator);

	    vkDestroyDevice(m_device, nullptr);

	    if (enableValidationLayers) {
	        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	    }

	    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	    vkDestroyInstance(m_instance, nullptr);

	    SDL_DestroyWindow(m_sdlWindow);
	    SDL_Quit();
	}

	void mainLoop() {
	    SDL_Event event;
	    SDL_PollEvent(&event);

	    while (!m_quit) {
	        while (SDL_PollEvent(&event)) {
	            ImGui_ImplSDL2_ProcessEvent(&event); // Forward your event to backend

	            if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE )
	                m_quit = true;

	            if (event.type == SDL_WINDOWEVENT) {
	                switch (event.window.event) {
	                case SDL_WINDOWEVENT_MINIMIZED:
	                    m_isMinimized = true;
	                    break;

	                case SDL_WINDOWEVENT_MAXIMIZED:
	                    m_isMinimized = false;
	                    break;

	                case SDL_WINDOWEVENT_RESTORED:
	                    m_isMinimized = false;
	                    break;
	                }
	            }
	        }

	        if(!m_isMinimized) {
	            ImGui_ImplVulkan_NewFrame();
	            ImGui_ImplSDL2_NewFrame();
	            ImGui::NewFrame();

	            ImGui::ShowDemoWindow(); // Show demo window! :)

	            drawFrame(m_sdlWindow, m_surface, m_physicalDevice, m_device, m_vmaAllocator
	                , m_graphicsQueue, m_presentQueue, m_swapChain, m_depthImage
	                , m_renderPass, m_graphicsPipeline, m_objects, m_commandBuffers
				, m_syncObjects, m_currentFrame, m_framebufferResized);
	        }
	    }
	    vkDeviceWaitIdle(m_device);
	}

	void initWindow() {
	    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
	    #ifdef SDL_HINT_IME_SHOW_UI
	        SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
	    #endif

	    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	    m_sdlWindow = SDL_CreateWindow("Tutorial", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);
	}

	void initVMA(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator& allocator) {
	    VmaVulkanFunctions vulkanFunctions = {};
	    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	    VmaAllocatorCreateInfo allocatorCreateInfo = {};
	    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	    allocatorCreateInfo.physicalDevice = physicalDevice;
	    allocatorCreateInfo.device = device;
	    allocatorCreateInfo.instance = instance;
	    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
	    vmaCreateAllocator(&allocatorCreateInfo, &allocator);
	}

	void initVulkan() {
	    createInstance(&m_instance, m_validationLayers);
	    setupDebugMessenger(m_instance);
	    createSurface(m_instance, m_surface);
	    pickPhysicalDevice(m_instance, m_deviceExtensions, m_surface, m_physicalDevice);
	    createLogicalDevice(m_surface, m_physicalDevice, m_queueFamilies, m_validationLayers, m_deviceExtensions, m_device, m_graphicsQueue, m_presentQueue);
	    initVMA(m_instance, m_physicalDevice, m_device, m_vmaAllocator);
	    createSwapChain(m_surface, m_physicalDevice, m_device, m_swapChain);
	    createImageViews(m_device, m_swapChain);
	    createRenderPass(m_physicalDevice, m_device, m_swapChain, m_renderPass);
	    createDescriptorSetLayout(m_device, m_descriptorSetLayout);
	    createGraphicsPipeline(m_device, m_renderPass, m_descriptorSetLayout, m_graphicsPipeline);
	    createCommandPool(m_surface, m_physicalDevice, m_device, m_commandPool);
	    createDepthResources(m_physicalDevice, m_device, m_vmaAllocator, m_swapChain, m_depthImage);
	    createFramebuffers(m_device, m_swapChain, m_depthImage, m_renderPass);
	    createDescriptorPool(m_device, m_descriptorPool);

	    createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::mat4{1.0f}, m_MODEL_PATH, m_TEXTURE_PATH, m_objects);

	    createCommandBuffers(m_device, m_commandPool, m_commandBuffers);
	    createSyncObjects(m_device, m_syncObjects);
	    setupImgui(m_instance, m_physicalDevice, m_queueFamilies, m_device, m_graphicsQueue, m_commandPool, m_descriptorPool, m_renderPass);
	}

	void createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout) {
	    VkDescriptorSetLayoutBinding uboLayoutBinding{};
	    uboLayoutBinding.binding = 0;
	    uboLayoutBinding.descriptorCount = 1;
	    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	    uboLayoutBinding.pImmutableSamplers = nullptr;
	    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	    samplerLayoutBinding.binding = 1;
	    samplerLayoutBinding.descriptorCount = 1;
	    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    samplerLayoutBinding.pImmutableSamplers = nullptr;
	    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
	    VkDescriptorSetLayoutCreateInfo layoutInfo{};
	    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	    layoutInfo.pBindings = bindings.data();

	    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
	        throw std::runtime_error("failed to create descriptor set layout!");
	    }
	}

	void createCommandPool(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool& commandPool) {
	    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	    VkCommandPoolCreateInfo poolInfo{};
	    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
	        throw std::runtime_error("failed to create graphics command pool!");
	    }
	}

	void createDescriptorPool(VkDevice device, VkDescriptorPool& descriptorPool) {
	    /*std::array<VkDescriptorPoolSize, 2> poolSizes{};
	    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	    VkDescriptorPoolCreateInfo poolInfo{};
	    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	    poolInfo.pPoolSizes = poolSizes.data();
	    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
	        throw std::runtime_error("failed to create descriptor pool!");
	    }*/

	    VkDescriptorPoolSize pool_sizes[] =
	    {
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

	    VkDescriptorPool imguiPool;
	    vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);
	}

	void createCommandBuffers(VkDevice device, VkCommandPool commandPool, std::vector<VkCommandBuffer>& commandBuffers) {
	    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	    VkCommandBufferAllocateInfo allocInfo{};
	    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	    allocInfo.commandPool = commandPool;
	    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

	    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
	        throw std::runtime_error("failed to allocate command buffers!");
	    }
	}

	void run() {
	    initWindow();
	    initVulkan();
	    mainLoop();
	    cleanup();
	}

	void setupImgui(
	    VkInstance instance,
	    VkPhysicalDevice physicalDevice,
	    QueueFamilyIndices queueFamilies,
	    VkDevice device,
	    VkQueue graphicsQueue,
	    VkCommandPool commandPool,
	    VkDescriptorPool descriptorPool,
	    VkRenderPass renderPass
	) {
	    // Setup Dear ImGui context
	    IMGUI_CHECKVERSION();
	    ImGui::CreateContext();
	    ImGuiIO& io = ImGui::GetIO();
	    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

	    // Setup Platform/Renderer backends
	    ImGui_ImplSDL2_InitForVulkan(m_sdlWindow);

	    ImGui_ImplVulkan_InitInfo init_info = {};
	    init_info.Instance = instance;
	    init_info.PhysicalDevice = physicalDevice;
	    init_info.Device = device;
	    init_info.QueueFamily = queueFamilies.graphicsFamily.value();
	    init_info.Queue = graphicsQueue;
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
	    // (this gets a bit more complicated, see example app for full reference)
	    //ImGui_ImplVulkan_CreateFontsTexture(YOUR_COMMAND_BUFFER);
	    // (your code submit a queue)
	    //ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
)";
}
