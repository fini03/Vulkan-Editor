#include "imgui/imgui.h"
#include "vulkan_editor/vulkan_base.h"
#include <SDL3/SDL_video.h>
#include <map>

void showInstanceView() {
	ImGui::Text("Application Name:");
    static char appName[256] = "";
    ImGui::InputText("##appName", appName, IM_ARRAYSIZE(appName));

    static bool showDebug = true;
    ImGui::Checkbox("Show Validation Layer Debug Info", &showDebug);

    static bool runOnLinux = true;
    static bool runOnMacOS = false;
    static bool runOnWindows = false;

    ImGui::Checkbox("Run on Linux", &runOnLinux);
    ImGui::Checkbox("Run on MacOS", &runOnMacOS);
    ImGui::Checkbox("Run on Windows", &runOnWindows);

    ImGui::EndTabItem();
}

std::vector<std::string> gpuNames;
std::vector<VkPhysicalDevice> physicalDevices;
int selectedGPU = 0;

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

std::vector<std::string> availableExtensions;
std::map<std::string, bool> extensionSelection;
bool runOnMacOS = false;

void queryDeviceExtensions(VkPhysicalDevice physicalDevice) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    if (extensionCount == 0) {
       // LOG_ERROR("No device extensions found!");
        return;
    }

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

    availableExtensions.clear();
    extensionSelection.clear();

    // Required extensions
    std::vector<std::string> requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    if (runOnMacOS) {
        requiredExtensions.push_back("VK_KHR_portability_subset");
    }

    // Populate available extensions and selection map
    for (const auto& ext : extensions) {
        std::string extName = ext.extensionName;
        availableExtensions.push_back(extName);
        extensionSelection[extName] = false;  // Default unchecked
    }

    // Ensure required extensions are always enabled and appear first
    for (const auto& required : requiredExtensions) {
        extensionSelection[required] = true;  // Always enabled
    }

    //LOG_INFO("Found %u available device extensions.", extensionCount);
}

void showLogicalDeviceView() {
	ImGui::Text("Queue Families:");
    static bool graphicsQueue = true;
    static bool computeQueue = false;
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
                static float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                ImGui::ColorEdit4("Clear Color", clearColor);

                // Frames in Flight
                static int framesInFlight = 2;
                ImGui::SliderInt("Frames in Flight", &framesInFlight, 1, 3);


                // Swapchain Image Count
                static int swapchainImageCount = 3;
                ImGui::SliderInt("Swapchain Image Count", &swapchainImageCount, 2, 8);

                // VSync Mode Selection
                static int vsyncMode = 1;
                const char* vsyncOptions[] = { "Immediate", "Mailbox", "Fifo", "Fifo Relaxed" };
                ImGui::Combo("VSync Mode", &vsyncMode, vsyncOptions, IM_ARRAYSIZE(vsyncOptions));

                // Image Usage Selection
                static int imageUsage = 0;
                const char* imageUsageOptions[] = {
                    "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT",
                    "VK_IMAGE_USAGE_TRANSFER_SRC_BIT",
                    "VK_IMAGE_USAGE_TRANSFER_DST_BIT"
                };
                ImGui::Combo("Image Usage", &imageUsage, imageUsageOptions, IM_ARRAYSIZE(imageUsageOptions));

                // Presentation Mode Selection
                static int presentMode = 1;
                const char* presentModeOptions[] = {
                    "VK_PRESENT_MODE_IMMEDIATE_KHR",
                    "VK_PRESENT_MODE_MAILBOX_KHR",
                    "VK_PRESENT_MODE_FIFO_KHR",
                    "VK_PRESENT_MODE_FIFO_RELAXED_KHR"
                };
                ImGui::Combo("Presentation Mode", &presentMode, presentModeOptions, IM_ARRAYSIZE(presentModeOptions));

                // Image Format Selection
                static int imageFormat = 0;
                const char* imageFormatOptions[] = {
                    "VK_FORMAT_B8G8R8A8_UNORM",
                    "VK_FORMAT_B8G8R8A8_SRGB",
                    "VK_FORMAT_R8G8B8A8_SRGB"
                };
                ImGui::Combo("Image Format", &imageFormat, imageFormatOptions, IM_ARRAYSIZE(imageFormatOptions));

                // Image Color Space Selection
                static int imageColorSpace = 0;
                const char* imageColorSpaceOptions[] = {
                    "VK_COLORSPACE_SRGB_NONLINEAR_KHR"
                };
                ImGui::Combo("Image Color Space", &imageColorSpace, imageColorSpaceOptions, IM_ARRAYSIZE(imageColorSpaceOptions));

                ImGui::EndTabItem();
}
#include "tinyfiledialogs.h"

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
    ImGui::Text("Shaders");
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

void showPipelineView() {
	ImGui::Text("Pipelines");

                // List of pipelines
                static int selectedPipeline = 0;
                static std::vector<std::string> pipelineNames;

                if (ImGui::BeginListBox("##PipelineList", ImVec2(200, 200))) {
                    for (int i = 0; i < pipelineNames.size(); ++i) {
                        if (ImGui::Selectable(pipelineNames[i].c_str(), selectedPipeline == i)) {
                            selectedPipeline = i;
                        }
                    }
                    ImGui::EndListBox();
                }

                // Buttons
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

                if (ImGui::BeginPopupModal("Edit Graphics Pipeline", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Input Assembly");
                    static int topology = 0;
                    const char* topologyOptions[] = { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST", "VK_PRIMITIVE_TOPOLOGY_LINE_LIST" };
                    ImGui::Combo("Vertex Topology", &topology, topologyOptions, IM_ARRAYSIZE(topologyOptions));

                    static bool primitiveRestart = false;
                    ImGui::Checkbox("Primitive Restart", &primitiveRestart);

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

                    ImGui::Separator();
                    ImGui::Text("Color Blending");
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

                    ImGui::Separator();
                    ImGui::Text("Shaders");
                    showShaderFileSelector();

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

                ImGui::EndTabItem();
}
