#include "application.h"

#include <inja/inja.hpp>
using namespace inja;

static json data;

ApplicationNode::ApplicationNode(int id) : Node(id) {}
ApplicationNode::~ApplicationNode() { }

void ApplicationNode::render() const {}

void ApplicationNode::generateApplication(std::ofstream& outFile, TemplateLoader templateLoader) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << R"(

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

	outFile << templateLoader.renderTemplateFile("vulkan_templates/application.txt", data);
}
