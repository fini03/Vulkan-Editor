#include "imgui/imgui.h"
#include "tinyfiledialogs.h"
#include "vulkan_editor/vulkan_base.h"
#include <SDL3/SDL_video.h>
#include <map>
#include "vulkan_generator.h"
#include "libs/tinyxml2.h"
#include <fstream>

static bool runOnLinux = true;
static bool runOnMacOS = false;
static bool runOnWindows = false;
static bool showDebug = true;
static char appName[256] = "";

std::vector<std::string> gpuNames;
std::vector<VkPhysicalDevice> physicalDevices;
int selectedGPU = 0;

std::vector<std::string> availableExtensions;
std::map<std::string, bool> extensionSelection;

static bool graphicsQueue = true;
static bool computeQueue = false;

static float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
static int framesInFlight = 2;
static int swapchainImageCount = 3;
static int imageUsage = 0;
const char* imageUsageOptions[] = {
        "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT",
        "VK_IMAGE_USAGE_TRANSFER_SRC_BIT",
        "VK_IMAGE_USAGE_TRANSFER_DST_BIT"
    };
static int presentMode = 1;
const char* presentModeOptions[] = {
        "VK_PRESENT_MODE_IMMEDIATE_KHR",
        "VK_PRESENT_MODE_MAILBOX_KHR",
        "VK_PRESENT_MODE_FIFO_KHR",
        "VK_PRESENT_MODE_FIFO_RELAXED_KHR"
    };
static int imageFormat = 0;
const char* imageFormatOptions[] = {
    "VK_FORMAT_B8G8R8A8_UNORM",
    "VK_FORMAT_B8G8R8A8_SRGB",
    "VK_FORMAT_R8G8B8A8_SRGB"
};
static int imageColorSpace = 0;
const char* imageColorSpaceOptions[] = {
    "VK_COLORSPACE_SRGB_NONLINEAR_KHR"
};

static int vulkanVersionIndex = 0; // Default index (corresponds to Vulkan 1.0)
const char* vulkanVersions[] = { "VK_API_VERSION_1_0", "VK_API_VERSION_1_1", "VK_API_VERSION_1_2", "VK_API_VERSION_1_3", "VK_API_VERSION_1_4" };

void savePhysicalDeviceSettings(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* root, int selectedGPU, const std::vector<std::string>& gpuNames) {
    tinyxml2::XMLElement* deviceElement = doc.NewElement("PhysicalDevice");
    deviceElement->SetAttribute("SelectedGPU", selectedGPU);
    deviceElement->SetText(gpuNames[selectedGPU].c_str());
    root->InsertEndChild(deviceElement);
}

void saveDeviceExtensions(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* root, const std::map<std::string, bool>& extensionSelection) {
    tinyxml2::XMLElement* extensionsElement = doc.NewElement("DeviceExtensions");
    for (const auto& ext : extensionSelection) {
        tinyxml2::XMLElement* extElement = doc.NewElement("Extension");
        extElement->SetAttribute("name", ext.first.c_str());
        extElement->SetAttribute("enabled", ext.second);
        extensionsElement->InsertEndChild(extElement);
    }
    root->InsertEndChild(extensionsElement);
}

void saveLogicalDeviceSettings(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* root, bool graphicsQueue, bool computeQueue) {
    tinyxml2::XMLElement* logicalDeviceElement = doc.NewElement("LogicalDevice");

    tinyxml2::XMLElement* graphicsElement = doc.NewElement("GraphicsQueue");
    graphicsElement->SetText(graphicsQueue);
    logicalDeviceElement->InsertEndChild(graphicsElement);

    tinyxml2::XMLElement* computeElement = doc.NewElement("ComputeQueue");
    computeElement->SetText(computeQueue);
    logicalDeviceElement->InsertEndChild(computeElement);

    root->InsertEndChild(logicalDeviceElement);
}

void saveSwapchainSettings(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* root) {
    // Create SwapchainSettings element
    tinyxml2::XMLElement* swapchainElement = doc.NewElement("SwapchainSettings");
    root->InsertEndChild(swapchainElement);

    // Clear Color
    tinyxml2::XMLElement* clearColorElement = doc.NewElement("ClearColor");
    clearColorElement->SetAttribute("name", "ClearColor");

    tinyxml2::XMLElement* rElement = doc.NewElement("clearColorRInput");
    rElement->SetAttribute("name", "clearColorRInput");
    rElement->SetText(clearColor[0]);
    clearColorElement->InsertEndChild(rElement);

    tinyxml2::XMLElement* gElement = doc.NewElement("clearColorGInput");
    gElement->SetAttribute("name", "clearColorGInput");
    gElement->SetText(clearColor[1]);
    clearColorElement->InsertEndChild(gElement);

    tinyxml2::XMLElement* bElement = doc.NewElement("clearColorBInput");
    bElement->SetAttribute("name", "clearColorBInput");
    bElement->SetText(clearColor[2]);
    clearColorElement->InsertEndChild(bElement);

    tinyxml2::XMLElement* aElement = doc.NewElement("clearColorAInput");
    aElement->SetAttribute("name", "clearColorAInput");
    aElement->SetText(clearColor[3]);
    clearColorElement->InsertEndChild(aElement);

    swapchainElement->InsertEndChild(clearColorElement);

    // Frames in Flight
    tinyxml2::XMLElement* framesElement = doc.NewElement("FramesInFlight");
    framesElement->SetText(framesInFlight);
    swapchainElement->InsertEndChild(framesElement);

    // Swapchain Image Count
    tinyxml2::XMLElement* imageCountElement = doc.NewElement("SwapchainImageCount");
    imageCountElement->SetText(swapchainImageCount);
    swapchainElement->InsertEndChild(imageCountElement);

    // Image Usage
    tinyxml2::XMLElement* usageElement = doc.NewElement("ImageUsage");
    usageElement->SetText(imageUsageOptions[imageUsage]);
    swapchainElement->InsertEndChild(usageElement);

    // Presentation Mode
    tinyxml2::XMLElement* presentElement = doc.NewElement("PresentationMode");
    presentElement->SetText(presentModeOptions[presentMode]);
    swapchainElement->InsertEndChild(presentElement);

    // Image Format
    tinyxml2::XMLElement* formatElement = doc.NewElement("ImageFormat");
    formatElement->SetText(imageFormatOptions[imageFormat]);
    swapchainElement->InsertEndChild(formatElement);

    // Image Color Space
    tinyxml2::XMLElement* colorSpaceElement = doc.NewElement("ImageColorSpace");
    colorSpaceElement->SetText(imageColorSpaceOptions[imageColorSpace]);
    swapchainElement->InsertEndChild(colorSpaceElement);
}


void saveToXML() {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* root = doc.NewElement("VulkanSettings");
    doc.InsertFirstChild(root);

    // General settings
    tinyxml2::XMLElement* nameElement = doc.NewElement("AppName");
    nameElement->SetText(appName);
    root->InsertEndChild(nameElement);

    tinyxml2::XMLElement* debugElement = doc.NewElement("ShowDebug");
    debugElement->SetText(showDebug);
    root->InsertEndChild(debugElement);

    tinyxml2::XMLElement* osElement = doc.NewElement("OperatingSystems");
    osElement->SetAttribute("Linux", runOnLinux);
    osElement->SetAttribute("MacOS", runOnMacOS);
    osElement->SetAttribute("Windows", runOnWindows);
    root->InsertEndChild(osElement);

    // Save Vulkan settings
    savePhysicalDeviceSettings(doc, root, selectedGPU, gpuNames);
    saveDeviceExtensions(doc, root, extensionSelection);
    saveLogicalDeviceSettings(doc, root, graphicsQueue, computeQueue);
    saveSwapchainSettings(doc, root);

    // Save to file
    doc.SaveFile("vulkan.xml");
}

void saveFile() {
	std::string generatedCode = generateVulkanCode(appName, vulkanVersionIndex, showDebug, runOnLinux, runOnMacOS, runOnWindows);

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

    ImGui::InputText("##appName", appName, IM_ARRAYSIZE(appName));

    ImGui::Text("Vulkan API Version:");
    ImGui::Combo("##vulkanVersion", &vulkanVersionIndex, vulkanVersions, IM_ARRAYSIZE(vulkanVersions));

    ImGui::Checkbox("Show Validation Layer Debug Info", &showDebug);

    ImGui::Checkbox("Run on Linux", &runOnLinux);
    ImGui::Checkbox("Run on MacOS", &runOnMacOS);
    ImGui::Checkbox("Run on Windows", &runOnWindows);

    ImGui::EndTabItem();
}

void enumerateGPUs(VulkanContext* context) {
    uint32_t numDevices = 0;
    vkEnumeratePhysicalDevices(context->instance, &numDevices, nullptr);

    if (numDevices == 0) {
        return;
    }

    physicalDevices.resize(numDevices);
    gpuNames.clear();

    vkEnumeratePhysicalDevices(context->instance, &numDevices, physicalDevices.data());

    for (uint32_t i = 0; i < numDevices; ++i) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);
        gpuNames.push_back(properties.deviceName);
    }

    // Set default selection
    selectedGPU = 0;
    context->physicalDevice = physicalDevices[selectedGPU];
}

void showPhysicalDeviceView(VulkanContext* context) {
	ImGui::Text("Select a Physical Device:");

    // Convert vector<string> to vector<const char*>
    std::vector<const char*> gpuNamesCStr;
    for (const auto& name : gpuNames)
        gpuNamesCStr.push_back(name.c_str());

    if (ImGui::Combo("##gpuSelector", &selectedGPU, gpuNamesCStr.data(), gpuNamesCStr.size())) {
        context->physicalDevice = physicalDevices[selectedGPU];
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

    availableExtensions.clear();
    extensionSelection.clear();

    // Only required extensions
    availableExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    extensionSelection[VK_KHR_SWAPCHAIN_EXTENSION_NAME] = true;

    if (runOnMacOS) {
        availableExtensions.push_back("VK_KHR_portability_subset");
        extensionSelection["VK_KHR_portability_subset"] = true;
    }
}


void showLogicalDeviceView() {
	ImGui::Text("Queue Families:");
    ImGui::Checkbox("Graphics Queue", &graphicsQueue);
    ImGui::Checkbox("Compute Queue", &computeQueue);

    ImGui::Text("Extensions:");

    for (const auto& ext : availableExtensions) {
        if (ext == VK_KHR_SWAPCHAIN_EXTENSION_NAME || (runOnMacOS && ext == "VK_KHR_portability_subset")) {
        	// Always enable required extensions
         	extensionSelection[ext] = true;
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
    ImGui::ColorEdit4("Clear Color", clearColor);

    // Frames in Flight
    ImGui::SliderInt("Frames in Flight", &framesInFlight, 1, 3);

    // Swapchain Image Count
    ImGui::SliderInt("Swapchain Image Count", &swapchainImageCount, 2, 8);

    // Image Usage Selection
    ImGui::Combo("Image Usage", &imageUsage, imageUsageOptions, IM_ARRAYSIZE(imageUsageOptions));

    // Presentation Mode Selection
    ImGui::Combo("Presentation Mode", &presentMode, presentModeOptions, IM_ARRAYSIZE(presentModeOptions));

    // Image Format Selection
    ImGui::Combo("Image Format", &imageFormat, imageFormatOptions, IM_ARRAYSIZE(imageFormatOptions));

    // Image Color Space Selection
    ImGui::Combo("Image Color Space", &imageColorSpace, imageColorSpaceOptions, IM_ARRAYSIZE(imageColorSpaceOptions));

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
