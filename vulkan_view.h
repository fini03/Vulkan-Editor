#include "imgui/imgui.h"
#include "tinyfiledialogs.h"
#include "vulkan_editor/vulkan_base.h"
#include <SDL3/SDL_video.h>
#include <map>
#include "vulkan_generator.h"
#include "vulkan_config.h"
#include "libs/tinyxml2.h"
#include <fstream>

VulkanConfig config = {};

void saveFile() {
	std::string generatedCode = generateVulkanCode(config);

    // Save to file
    std::ofstream outFile("myvulkanapp.cpp");
    if (outFile.is_open()) {
        outFile << generatedCode;
        outFile.close();
        std::cout << "Vulkan code successfully saved to myvulkanapp.cpp" << std::endl;
    } else {
        std::cerr << "Error: Could not open file for writing!" << std::endl;
    }
}

void showInstanceView() {
	ImGui::Text("Application Name:");

    ImGui::InputText("##appName", config.appName, IM_ARRAYSIZE(config.appName));

    ImGui::Text("Vulkan API Version:");
    ImGui::Combo("##vulkanVersion", &config.vulkanVersionIndex, config.vulkanVersions.data(), (int)config.vulkanVersions.size());

    ImGui::Checkbox("Show Validation Layer Debug Info", &config.showDebug);

    ImGui::Checkbox("Run on Linux", &config.runOnLinux);
    ImGui::Checkbox("Run on MacOS", &config.runOnMacOS);
    ImGui::Checkbox("Run on Windows", &config.runOnWindows);

    ImGui::EndTabItem();
}

void enumerateGPUs(VulkanContext* context) {
    uint32_t numDevices = 0;
    vkEnumeratePhysicalDevices(context->instance, &numDevices, nullptr);

    if (numDevices == 0) {
        return;
    }

    config.physicalDevices.resize(numDevices);
    config.gpuNames.clear();

    vkEnumeratePhysicalDevices(context->instance, &numDevices, config.physicalDevices.data());

    for (uint32_t i = 0; i < numDevices; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(config.physicalDevices[i], &properties);
        config.gpuNames.push_back(properties.deviceName);
    }

    // Set default selection
    config.selectedGPU = 0;
    context->physicalDevice = config.physicalDevices[config.selectedGPU];
}

void showPhysicalDeviceView(VulkanContext* context) {
	ImGui::Text("Select a Physical Device:");

    // Convert vector<string> to vector<const char*>
    std::vector<const char*> gpuNamesCStr;
    for (const auto& name : config.gpuNames)
        gpuNamesCStr.push_back(name.c_str());

    if (ImGui::Combo("##gpuSelector", &config.selectedGPU, gpuNamesCStr.data(), gpuNamesCStr.size())) {
        context->physicalDevice = config.physicalDevices[config.selectedGPU];
        vkGetPhysicalDeviceProperties(context->physicalDevice, &context->physicalDeviceProperties);
    }

    ImGui::Text("Device Properties:");
    ImGui::Text("Memory: %u MB", context->physicalDeviceProperties.limits.maxMemoryAllocationCount);
    ImGui::Text("Compute Units: %u", context->physicalDeviceProperties.limits.maxComputeSharedMemorySize);

    ImGui::EndTabItem();
}

void queryDeviceExtensions(VkPhysicalDevice physicalDevice) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    if (extensionCount == 0) {
        return;
    }

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

    config.availableExtensions.clear();
    config.extensionSelection.clear();

    // Only required extensions
    config.availableExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    config.extensionSelection[VK_KHR_SWAPCHAIN_EXTENSION_NAME] = true;

    if (config.runOnMacOS) {
        config.availableExtensions.push_back("VK_KHR_portability_subset");
        config.extensionSelection["VK_KHR_portability_subset"] = true;
    }
}


void showLogicalDeviceView() {
	ImGui::Text("Queue Families:");
    ImGui::Checkbox("Graphics Queue", &config.graphicsQueue);
    ImGui::Checkbox("Compute Queue", &config.computeQueue);

    ImGui::Text("Extensions:");

    for (const auto& ext : config.availableExtensions) {
        if (ext == VK_KHR_SWAPCHAIN_EXTENSION_NAME || (config.runOnMacOS && ext == "VK_KHR_portability_subset")) {
        	// Always enable required extensions
         	config.extensionSelection[ext] = true;
          	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));  // Yellow for required
           	ImGui::Text("** %s ** (Required)", ext.c_str());
            ImGui::PopStyleColor();
        }
    }

    ImGui::EndTabItem();
}

void showSwapchainView(SDL_Window* window) {
	ImGui::Text("Swapchain Settings:");

	// Fetch SDL Window Size
    int windowWidth = 0, windowHeight = 0;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // Display Image Dimensions (Non-Editable)
    ImGui::Text("Image Dimensions:");
    ImGui::SameLine();
    ImGui::Text("Height: %d  Width: %d", windowHeight, windowWidth);

    // Image Clear Color
    ImGui::Text("Image Clear Color:");
    ImGui::ColorEdit4("Clear Color", config.clearColor);

    // Frames in Flight
    ImGui::SliderInt("Frames in Flight", &config.framesInFlight, 1, 3);

    // Swapchain Image Count
    ImGui::SliderInt("Swapchain Image Count", &config.swapchainImageCount, 2, 8);

    // Image Usage Selection
    ImGui::Combo("Image Usage", &config.imageUsage, config.imageUsageOptions.data(), (int)config.imageUsageOptions.size());

    // Presentation Mode Selection
    ImGui::Combo("Presentation Mode", &config.presentMode, config.presentModeOptions.data(), (int)config.presentModeOptions.size());

    // Image Format Selection
    ImGui::Combo("Image Format", &config.imageFormat, config.imageFormatOptions.data(), (int)config.imageFormatOptions.size());

    // Image Color Space Selection
    ImGui::Combo("Image Color Space", &config.imageColorSpace, config.imageColorSpaceOptions.data(), (int)config.imageColorSpaceOptions.size());

    ImGui::EndTabItem();
}

void showModelView() {
    static char modelPath[256] = "models/viking_room.obj";
    static char texturePath[256] = "textures/viking_room.png";

    ImGui::Text("Model File:");
    ImGui::InputText("##modelFile", modelPath, IM_ARRAYSIZE(modelPath));
    ImGui::SameLine();
    if (ImGui::Button("...##Model")) {
        const char* filter[] = { "*.obj", "*.fbx", "*.gltf", "*.glb", "*.dae", "*.*" }; // Model file filters
        const char* selectedPath = tinyfd_openFileDialog("Select Model File", "", 6, filter, "Model Files", 0);
        if (selectedPath) {
            strncpy(modelPath, selectedPath, IM_ARRAYSIZE(modelPath));
        }
    }

    ImGui::Text("Texture File:");
    ImGui::InputText("##textureFile", texturePath, IM_ARRAYSIZE(texturePath));
    ImGui::SameLine();
    if (ImGui::Button("...##Texture")) {
        const char* filter[] = { "*.png", "*.jpg", "*.jpeg", "*.tga", "*.bmp", "*.dds", "*.*" }; // Texture file filters
        const char* selectedPath = tinyfd_openFileDialog("Select Texture File", "", 7, filter, "Texture Files", 0);
        if (selectedPath) {
            strncpy(texturePath, selectedPath, IM_ARRAYSIZE(texturePath));
        }
    }

    if (ImGui::Button("Load Model")) {
        // Load model logic here
    }
    ImGui::EndTabItem();
}

void showShaderFileSelector() {
    const char* filter[] = { "*.vert", "*.spv", "*.glsl", "*.txt", "*.*" }; // Shader file filters
    static char vertexShaderPath[256] = "path/to/shader.vert";
    ImGui::InputText("Vertex Shader File", vertexShaderPath, IM_ARRAYSIZE(vertexShaderPath));
    ImGui::SameLine();
    if (ImGui::Button("...##Vertex")) {
        const char* selectedPath = tinyfd_openFileDialog("Select Vertex Shader", "", 5, filter, "Shader Files", 0);
        if (selectedPath) {
            strncpy(vertexShaderPath, selectedPath, IM_ARRAYSIZE(vertexShaderPath));
        }
    }

    static char vertexEntryName[64] = "main";
    ImGui::InputText("Vertex Shader Entry Function", vertexEntryName, IM_ARRAYSIZE(vertexEntryName));

    static char fragmentShaderPath[256] = "path/to/shader.frag";
    ImGui::InputText("Fragment Shader File", fragmentShaderPath, IM_ARRAYSIZE(fragmentShaderPath));
    ImGui::SameLine();
    if (ImGui::Button("...##Fragment")) {
        const char* selectedPath = tinyfd_openFileDialog("Select Fragment Shader", "", 5, filter, "Shader Files", 0);
        if (selectedPath) {
            strncpy(fragmentShaderPath, selectedPath, IM_ARRAYSIZE(fragmentShaderPath));
        }
    }

    static char fragmentEntryName[64] = "main";
    ImGui::InputText("Fragment Shader Entry Function", fragmentEntryName, IM_ARRAYSIZE(fragmentEntryName));
}

void showPipelineList(int& selectedPipeline, std::vector<std::string>& pipelineNames) {
    if (ImGui::BeginListBox("##PipelineList", ImVec2(200, 200))) {
        for (int i = 0; i < pipelineNames.size(); ++i) {
            if (ImGui::Selectable(pipelineNames[i].c_str(), selectedPipeline == i)) {
                selectedPipeline = i;
            }
        }
        ImGui::EndListBox();
    }
}

void showPipelineButtons(int& selectedPipeline, std::vector<std::string>& pipelineNames) {
    if (ImGui::Button("Add Pipeline", ImVec2(200, 0))) {
        pipelineNames.push_back("Graphics Pipeline " + std::to_string(pipelineNames.size() + 1));
    }
    if (ImGui::Button("Edit Pipeline", ImVec2(200, 0)) && !pipelineNames.empty()) {
        ImGui::OpenPopup("Edit Graphics Pipeline");
    }
    if (ImGui::Button("Delete Pipeline", ImVec2(200, 0)) && !pipelineNames.empty()) {
        pipelineNames.erase(pipelineNames.begin() + selectedPipeline);
        selectedPipeline = std::max(0, selectedPipeline - 1);
    }
}

void showInputAssemblySettings() {
    ImGui::Text("Input Assembly");
    static int topology = 0;
    const char* topologyOptions[] = { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST", "VK_PRIMITIVE_TOPOLOGY_LINE_LIST" };
    ImGui::Combo("Vertex Topology", &topology, topologyOptions, IM_ARRAYSIZE(topologyOptions));

    static bool primitiveRestart = false;
    ImGui::Checkbox("Primitive Restart", &primitiveRestart);
}

void showRasterizerSettings() {
    ImGui::Separator();
    ImGui::Text("Rasterizer");
    static bool depthClamp = false, rasterizerDiscard = false;
    ImGui::Checkbox("Depth Clamp", &depthClamp);
    ImGui::Checkbox("Rasterizer Discard", &rasterizerDiscard);

    static int polygonMode = 0;
    const char* polygonModes[] = { "VK_POLYGON_MODE_FILL", "VK_POLYGON_MODE_LINE" };
    ImGui::Combo("Polygon Mode", &polygonMode, polygonModes, IM_ARRAYSIZE(polygonModes));

    static float lineWidth = 1.0f;
    ImGui::InputFloat("Line Width", &lineWidth);

    static int cullMode = 0;
    const char* cullModes[] = { "VK_CULL_MODE_NONE", "VK_CULL_MODE_BACK_BIT" };
    ImGui::Combo("Cull Mode", &cullMode, cullModes, IM_ARRAYSIZE(cullModes));

    static int frontFace = 0;
    const char* frontFaceOptions[] = { "VK_FRONT_FACE_CLOCKWISE", "VK_FRONT_FACE_COUNTER_CLOCKWISE" };
    ImGui::Combo("Front Face", &frontFace, frontFaceOptions, IM_ARRAYSIZE(frontFaceOptions));

    static bool depthBiasEnabled = false;
    ImGui::Checkbox("Depth Bias Enabled", &depthBiasEnabled);

    static float depthBiasConstantFactor = 0.0f, depthBiasClamp = 0.0f, depthBiasSlopeFactor = 0.0f;
    ImGui::InputFloat("Depth Bias Constant Factor", &depthBiasConstantFactor);
    ImGui::InputFloat("Depth Bias Clamp", &depthBiasClamp);
    ImGui::InputFloat("Depth Bias Slope Factor", &depthBiasSlopeFactor);
}

void showDepthStencilSettings() {
    ImGui::Separator();
    ImGui::Text("Depth & Stencil");
    static bool depthTest = true, depthWrite = true;
    ImGui::Checkbox("Depth Test", &depthTest);
    ImGui::Checkbox("Depth Write", &depthWrite);
    static int depthCompareOp = 0;
    const char* depthCompareOptions[] = { "VK_COMPARE_OP_LESS", "VK_COMPARE_OP_GREATER" };
    ImGui::Combo("Depth Compare Operation", &depthCompareOp, depthCompareOptions, IM_ARRAYSIZE(depthCompareOptions));

    static bool depthBoundsTest = false;
    ImGui::Checkbox("Depth Bounds Test", &depthBoundsTest);

    static float depthBoundsMin = 0.0f, depthBoundsMax = 0.0f;
    ImGui::InputFloat("Depth Bounds Min", &depthBoundsMin);
    ImGui::InputFloat("Depth Bounds Max", &depthBoundsMax);

    static bool stencilTest = false;
    ImGui::Checkbox("Stencil Test", &stencilTest);
}

void showMultisamplingSettings() {
    ImGui::Separator();
    ImGui::Text("Multisampling");
    static bool sampleShading = false;
    ImGui::Checkbox("Sample Shading", &sampleShading);

    static int rasterizationSamples = 0;
    const char* sampleCountOptions[] = { "VK_SAMPLE_COUNT_1_BIT", "VK_SAMPLE_COUNT_4_BIT" };
    ImGui::Combo("Rasterization Samples", &rasterizationSamples, sampleCountOptions, IM_ARRAYSIZE(sampleCountOptions));

    static float minSampleShading = 0.0f;
    ImGui::InputFloat("Min Sample Shading", &minSampleShading);

    static bool alphaToCoverage = false, alphaToOne = false;
    ImGui::Checkbox("Alpha to Coverage", &alphaToCoverage);
    ImGui::Checkbox("Alpha to One", &alphaToOne);
}

void showColorBlendingSettings() {
    ImGui::Separator();
    ImGui::Text("Color Blending");

    static int colorWriteMask = 0xF;
    const char* colorWriteMaskOptions[] = { "VK_COLOR_COMPONENT_R_BIT", "VK_COLOR_COMPONENT_G_BIT", "VK_COLOR_COMPONENT_B_BIT", "VK_COLOR_COMPONENT_A_BIT" };
    ImGui::Combo("Color Write Mask", &colorWriteMask, colorWriteMaskOptions, IM_ARRAYSIZE(colorWriteMaskOptions));

    static bool colorBlend = false;
    ImGui::Checkbox("Color Blend", &colorBlend);

    static int srcColorBlendFactor = 0, dstColorBlendFactor = 0, colorBlendOp = 0;
    static int srcAlphaBlendFactor = 0, dstAlphaBlendFactor = 0, alphaBlendOp = 0;

    const char* blendFactors[] = { "VK_BLEND_FACTOR_ONE", "VK_BLEND_FACTOR_ZERO" };
    const char* blendOps[] = { "VK_BLEND_OP_ADD", "VK_BLEND_OP_SUBTRACT" };

    ImGui::Combo("Source Color Blend Factor", &srcColorBlendFactor, blendFactors, IM_ARRAYSIZE(blendFactors));
    ImGui::Combo("Destination Color Blend Factor", &dstColorBlendFactor, blendFactors, IM_ARRAYSIZE(blendFactors));
    ImGui::Combo("Color Blend Operation", &colorBlendOp, blendOps, IM_ARRAYSIZE(blendOps));

    ImGui::Combo("Source Alpha Blend Factor", &srcAlphaBlendFactor, blendFactors, IM_ARRAYSIZE(blendFactors));
    ImGui::Combo("Destination Alpha Blend Factor", &dstAlphaBlendFactor, blendFactors, IM_ARRAYSIZE(blendFactors));
    ImGui::Combo("Alpha Blend Operation", &alphaBlendOp, blendOps, IM_ARRAYSIZE(blendOps));

    static bool logicOpEnable = false;
    ImGui::Checkbox("Logic Operation Enabled", &logicOpEnable);

    static int logicOp = 0;
    const char* logicOps[] = { "VK_LOGIC_OP_COPY", "VK_LOGIC_OP_XOR" };
    ImGui::Combo("Logic Operation", &logicOp, logicOps, IM_ARRAYSIZE(logicOps));

    static int attachmentCount = 1;
    ImGui::InputInt("Attachment Count", &attachmentCount);

    static float blendConstants[4] = { 0.0f, 1.0f, 2.0f, 3.0f };
    ImGui::InputFloat4("Color Blend Constants", blendConstants);
}

void showShaderSettings() {
    ImGui::Separator();
    ImGui::Text("Shaders");
    showShaderFileSelector();
}

void showEditPipelineModal() {
    if (ImGui::BeginPopupModal("Edit Graphics Pipeline", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        showInputAssemblySettings();
        showRasterizerSettings();
        showDepthStencilSettings();
        showMultisamplingSettings();
        showColorBlendingSettings();
        showShaderSettings();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            // Save pipeline settings here
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void showPipelineView() {
    ImGui::Text("Pipelines");
    static int selectedPipeline = 0;
    static std::vector<std::string> pipelineNames;

    showPipelineList(selectedPipeline, pipelineNames);
    showPipelineButtons(selectedPipeline, pipelineNames);
    showEditPipelineModal();

    ImGui::EndTabItem();
}
