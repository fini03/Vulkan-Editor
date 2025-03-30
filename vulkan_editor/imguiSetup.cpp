#include "imguiSetup.h"
#include <fstream>
#include <iostream>
std::ofstream outFile;

void generateImguiCode() {
    outFile.open("Vertex.h", std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << R"(
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

    outFile.close();
}
