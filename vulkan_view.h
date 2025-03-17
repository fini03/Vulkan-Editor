#include "imgui/imgui.h"
#include "imgui-node-editor/imgui_node_editor.h"
#include "model.h"
#include "tinyfiledialogs.h"
#include "vulkan_editor/vulkan_base.h"
#include <SDL3/SDL_video.h>
#include <map>
#include "vulkan_generator.h"
#include "vulkan_config.h"
#include "libs/tinyxml2.h"
#include <fstream>
#include <memory>
#include <optional>

namespace ed = ax::NodeEditor;
ed::EditorContext* g_Context = nullptr;

VulkanConfig config = {};

struct PipelineSettings {
    // Define settings for each category
    int inputAssembly = 0;
    bool primitiveRestart = false;

    bool depthClamp = false, rasterizerDiscard = false;
    int polygonMode = 0;
    float lineWidth = 0;
    int cullMode = 0;
    int frontFace = 0;
    bool depthBiasEnabled = false;
    float depthBiasConstantFactor = 0.0f, depthBiasClamp = 0.0f, depthBiasSlopeFactor = 0.0f;

    bool depthTest = true, depthWrite = true;
    int depthCompareOp = 0;
    bool depthBoundsTest = false;
    float depthBoundsMin = 0.0f, depthBoundsMax = 0.0f;
    bool stencilTest = false;

    bool sampleShading = false;
    int rasterizationSamples = 0;
    float minSampleShading = 0.0f;
    bool alphaToCoverage = false, alphaToOne = false;

    int colorWriteMask = 0xF;
    bool colorBlend = false;
    int srcColorBlendFactor = 0, dstColorBlendFactor = 0, colorBlendOp = 0;
    int srcAlphaBlendFactor = 0, dstAlphaBlendFactor = 0, alphaBlendOp = 0;
    bool logicOpEnable = false;
    int logicOp = 0;
    int attachmentCount = 1;
    float blendConstants[4] = { 0.0f, 1.0f, 2.0f, 3.0f };

    char vertexShaderPath[256] = "path/to/shader.vert";
    char vertexEntryName[64] = "main";
    char fragmentShaderPath[256] = "path/to/shader.frag";
    char fragmentEntryName[64] = "main";
};

struct Editor {
    ed::EditorContext* context = nullptr;
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<Link> links;
    int currentId = 1;
};

Editor editor = {};

void NodeEditorInitialize() {
    editor.context = ed::CreateEditor();
    //editor.nodes.emplace_back(std::make_unique<PipelineNode>(editor.currentId++));
    editor.nodes.emplace_back(std::make_unique<ModelNode>(editor.currentId++));
    editor.nodes.emplace_back(std::make_unique<ModelNode>(editor.currentId++));
}

void saveFile() {

}

void showModelView(VulkanContext* context, Model& model) {
    static char modelPath[256] = "data/models/viking_room.obj";
    static char texturePath[256] = "data/images/viking_room.png";

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
       model.createModel(context, modelPath, texturePath);
       std::cout << model.indices.size() << "quack\n";
    }
    ImGui::EndTabItem();
}

void showShaderFileSelector(PipelineSettings& settings) {
    const char* filter[] = { "*.vert", "*.spv", "*.glsl", "*.txt", "*.*" }; // Shader file filters

    ImGui::InputText("Vertex Shader File", settings.vertexShaderPath, IM_ARRAYSIZE(settings.vertexShaderPath));
    ImGui::SameLine();
    if (ImGui::Button("...##Vertex")) {
        const char* selectedPath = tinyfd_openFileDialog("Select Vertex Shader", "", 5, filter, "Shader Files", 0);
        if (selectedPath) {
            strncpy(settings.vertexShaderPath, selectedPath, IM_ARRAYSIZE(settings.vertexShaderPath));
        }
    }

    ImGui::InputText("Vertex Shader Entry Function", settings.vertexEntryName, IM_ARRAYSIZE(settings.vertexEntryName));

    ImGui::InputText("Fragment Shader File", settings.fragmentShaderPath, IM_ARRAYSIZE(settings.fragmentShaderPath));
    ImGui::SameLine();
    if (ImGui::Button("...##Fragment")) {
        const char* selectedPath = tinyfd_openFileDialog("Select Fragment Shader", "", 5, filter, "Shader Files", 0);
        if (selectedPath) {
            strncpy(settings.fragmentShaderPath, selectedPath, IM_ARRAYSIZE(settings.fragmentShaderPath));
        }
    }

    ImGui::InputText("Fragment Shader Entry Function", settings.fragmentEntryName, IM_ARRAYSIZE(settings.fragmentEntryName));
}

void showInputAssemblySettings(PipelineSettings& settings) {
    ImGui::Text("Input Assembly");
    const char* topologyOptions[] = { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST", "VK_PRIMITIVE_TOPOLOGY_LINE_LIST" };
    ImGui::Combo("Vertex Topology", &settings.inputAssembly, topologyOptions, IM_ARRAYSIZE(topologyOptions));

    ImGui::Checkbox("Primitive Restart", &settings.primitiveRestart);
}

void showRasterizerSettings(PipelineSettings& settings) {
    ImGui::Separator();
    ImGui::Text("Rasterizer");

    ImGui::Checkbox("Depth Clamp", &settings.depthClamp);
    ImGui::Checkbox("Rasterizer Discard", &settings.rasterizerDiscard);

    const char* polygonModes[] = { "VK_POLYGON_MODE_FILL", "VK_POLYGON_MODE_LINE" };
    ImGui::Combo("Polygon Mode", &settings.polygonMode, polygonModes, IM_ARRAYSIZE(polygonModes));

    ImGui::InputFloat("Line Width", &settings.lineWidth);

    const char* cullModes[] = { "VK_CULL_MODE_NONE", "VK_CULL_MODE_BACK_BIT" };
    ImGui::Combo("Cull Mode", &settings.cullMode, cullModes, IM_ARRAYSIZE(cullModes));

    const char* frontFaceOptions[] = { "VK_FRONT_FACE_CLOCKWISE", "VK_FRONT_FACE_COUNTER_CLOCKWISE" };
    ImGui::Combo("Front Face", &settings.frontFace, frontFaceOptions, IM_ARRAYSIZE(frontFaceOptions));

    ImGui::Checkbox("Depth Bias Enabled", &settings.depthBiasEnabled);

    ImGui::InputFloat("Depth Bias Constant Factor", &settings.depthBiasConstantFactor);
    ImGui::InputFloat("Depth Bias Clamp", &settings.depthBiasClamp);
    ImGui::InputFloat("Depth Bias Slope Factor", &settings.depthBiasSlopeFactor);
}

void showDepthStencilSettings(PipelineSettings& settings) {
    ImGui::Separator();
    ImGui::Text("Depth & Stencil");

    ImGui::Checkbox("Depth Test", &settings.depthTest);
    ImGui::Checkbox("Depth Write", &settings.depthWrite);

    const char* depthCompareOptions[] = { "VK_COMPARE_OP_LESS", "VK_COMPARE_OP_GREATER" };
    ImGui::Combo("Depth Compare Operation", &settings.depthCompareOp, depthCompareOptions, IM_ARRAYSIZE(depthCompareOptions));

    ImGui::Checkbox("Depth Bounds Test", &settings.depthBoundsTest);

    ImGui::InputFloat("Depth Bounds Min", &settings.depthBoundsMin);
    ImGui::InputFloat("Depth Bounds Max", &settings.depthBoundsMax);

    ImGui::Checkbox("Stencil Test", &settings.stencilTest);
}

void showMultisamplingSettings(PipelineSettings& settings) {
    ImGui::Separator();
    ImGui::Text("Multisampling");

    ImGui::Checkbox("Sample Shading", &settings.sampleShading);

    const char* sampleCountOptions[] = { "VK_SAMPLE_COUNT_1_BIT", "VK_SAMPLE_COUNT_4_BIT" };
    ImGui::Combo("Rasterization Samples", &settings.rasterizationSamples, sampleCountOptions, IM_ARRAYSIZE(sampleCountOptions));

    ImGui::InputFloat("Min Sample Shading", &settings.minSampleShading);

    ImGui::Checkbox("Alpha to Coverage", &settings.alphaToCoverage);
    ImGui::Checkbox("Alpha to One", &settings.alphaToOne);
}

void showColorBlendingSettings(PipelineSettings& settings) {
    ImGui::Separator();
    ImGui::Text("Color Blending");


    const char* colorWriteMaskOptions[] = { "VK_COLOR_COMPONENT_R_BIT", "VK_COLOR_COMPONENT_G_BIT", "VK_COLOR_COMPONENT_B_BIT", "VK_COLOR_COMPONENT_A_BIT" };
    ImGui::Combo("Color Write Mask", &settings.colorWriteMask, colorWriteMaskOptions, IM_ARRAYSIZE(colorWriteMaskOptions));

    ImGui::Checkbox("Color Blend", &settings.colorBlend);


    const char* blendFactors[] = { "VK_BLEND_FACTOR_ONE", "VK_BLEND_FACTOR_ZERO" };
    const char* blendOps[] = { "VK_BLEND_OP_ADD", "VK_BLEND_OP_SUBTRACT" };

    ImGui::Combo("Source Color Blend Factor", &settings.srcColorBlendFactor, blendFactors, IM_ARRAYSIZE(blendFactors));
    ImGui::Combo("Destination Color Blend Factor", &settings.dstColorBlendFactor, blendFactors, IM_ARRAYSIZE(blendFactors));
    ImGui::Combo("Color Blend Operation", &settings.colorBlendOp, blendOps, IM_ARRAYSIZE(blendOps));

    ImGui::Combo("Source Alpha Blend Factor", &settings.srcAlphaBlendFactor, blendFactors, IM_ARRAYSIZE(blendFactors));
    ImGui::Combo("Destination Alpha Blend Factor", &settings.dstAlphaBlendFactor, blendFactors, IM_ARRAYSIZE(blendFactors));
    ImGui::Combo("Alpha Blend Operation", &settings.alphaBlendOp, blendOps, IM_ARRAYSIZE(blendOps));

    ImGui::Checkbox("Logic Operation Enabled", &settings.logicOpEnable);

    const char* logicOps[] = { "VK_LOGIC_OP_COPY", "VK_LOGIC_OP_XOR" };
    ImGui::Combo("Logic Operation", &settings.logicOp, logicOps, IM_ARRAYSIZE(logicOps));

    ImGui::InputInt("Attachment Count", &settings.attachmentCount);

    ImGui::InputFloat4("Color Blend Constants", settings.blendConstants);
}

void showShaderSettings(PipelineSettings& settings) {
    ImGui::Separator();
    ImGui::Text("Shaders");
    showShaderFileSelector(settings);
}


namespace ed = ax::NodeEditor;




void showPipelineView() {
	if(!editor.context)
		NodeEditorInitialize();

    // ========== Middle: Pipeline Editor ==========

    ed::SetCurrentEditor(editor.context);
    ed::Begin("Pipeline Editor");

    for (const auto& node : editor.nodes)
    	node->render();

    for (const auto& link : editor.links)
    	ed::Link(link.id, link.startPin, link.endPin);

    // Ensure we are inside a valid session
       if (ed::BeginCreate()) {
           Link link;
           if (ed::QueryNewLink(&link.startPin, &link.endPin)) {
               if (link.startPin != link.endPin && ed::AcceptNewItem()) { // Ensure no self-linking
               		link.id = editor.currentId++;
                   editor.links.push_back(link);
                   //ed::CommitNewItem();
               }
           }
       }
       ed::EndCreate(); // End link creation session

       // Handle Link Deletion
       if (ed::BeginDelete()) {
           ax::NodeEditor::LinkId linkId;
           while (ed::QueryDeletedLink(&linkId)) {
               auto it = std::remove_if(editor.links.begin(), editor.links.end(),
                                        [linkId](const Link& link) { return link.id == linkId.Get(); });
               if (it != editor.links.end()) {
                   editor.links.erase(it);
                   //ed::CommitDeletedItem();
               }
           }
       }
       ed::EndDelete(); // End link deletion session

    ed::End();
    ed::SetCurrentEditor(nullptr);
}
