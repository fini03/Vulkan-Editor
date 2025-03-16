#include "imgui/imgui.h"
#include "imgui-node-editor/imgui_node_editor.h"
#include "tinyfiledialogs.h"
#include "vulkan_editor/vulkan_base.h"
#include <SDL3/SDL_video.h>
#include <map>
#include "vulkan_generator.h"
#include "vulkan_config.h"
#include "libs/tinyxml2.h"
#include <fstream>
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

struct Pin {
    int id;
    std::string name;
};

struct Node {
    int id;
    std::string name;
    ImVec2 position;
    std::vector<Pin> inputPins;
    std::vector<Pin> outputPins;
    std::optional<PipelineSettings> settings;
};


struct Link {
    int id;
    int startPin;
    int endPin;
};

static std::vector<Node> nodes;
static std::vector<Link> links;
static int nextId = 1;
static int nextLinkId = 100; // Link ID counter

void saveFile() {

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

void drawNodes() {
    if (nodes.empty()) return;

    for (auto& node : nodes) {
        ed::BeginNode(node.id);
        ImGui::Text(node.name.c_str());

        // Draw Input Pins
        for (auto& pin : node.inputPins) {
            ed::BeginPin(pin.id, ed::PinKind::Input);
            ImGui::Text(pin.name.c_str());
            ed::EndPin();
        }

        // Draw Output Pins
        for (auto& pin : node.outputPins) {
            ed::BeginPin(pin.id, ed::PinKind::Output);
            ImGui::Text(pin.name.c_str());
            ed::EndPin();
        }

        ed::EndNode();
    }

    // Draw Links
    for (auto& link : links) {
        ed::Link(link.id, link.startPin, link.endPin);
    }
}


namespace ed = ax::NodeEditor;
static int selectedNode = -1; // Default to -1 (no selection)

void createColorNode() {
    Node node;
    node.id = nextId++;
    node.name = "Color";
    node.position = {200, 200};
    node.inputPins.push_back({nextLinkId++, "Input 1"});
    nodes.push_back(node);
}

void createDepthNode() {
	Node node;
    node.id = nextId++;
    node.name = "Depth";
    node.position = {200, 200};
    node.inputPins.push_back({nextLinkId++, "Input 1"});
    nodes.push_back(node);
}

void createTextureNode() {
	Node node;
    node.id = nextId++;
    node.name = "Texture";
    node.position = {200, 200};
    node.inputPins.push_back({nextLinkId++, "Input 1"});
    nodes.push_back(node);
}

void createPipelineNode() {
    Node node;
    node.id = nextId++;
    node.name = "Pipeline";
    node.position = {200, 200};
    node.settings = PipelineSettings{};
    node.inputPins.push_back({nextLinkId++, "Input 1"});
    node.outputPins.push_back({nextLinkId++, "Output 1"});

    nodes.push_back(node);
}

void createModelNode() {
	Node node;
    node.id = nextId++;
    node.name = "Model";
    node.position = {200, 200};
    node.inputPins.push_back({nextLinkId++, "Input 1"});
    nodes.push_back(node);
}


void showPipelineView() {
    ImGui::Text("Pipelines");

    static const char* predefinedNodes[] = { "Color", "Depth", "Texture", "Pipeline", "Model" };
    static int selectedNodeIndex = -1;

    ImGui::Columns(3, NULL, true); // Three columns: Left = Node List, Middle = Editor, Right = Edit Panel

    // ========== Left: Node Selection Panel ==========
    ImGui::BeginChild("NodeSelection", ImVec2(0, 0), true);
    ImGui::Text("Nodes");
    ImGui::Separator();

    for (int i = 0; i < IM_ARRAYSIZE(predefinedNodes); i++) {
        if (ImGui::Selectable(predefinedNodes[i], selectedNodeIndex == i)) {
            selectedNodeIndex = i;
        }
    }

    if (ImGui::Button("Add Selected Node") && selectedNodeIndex >= 0) {
            // Call the appropriate node creation function
            switch (selectedNodeIndex) {
                case 0: createColorNode(); break;
                case 1: createDepthNode(); break;
                case 2: createTextureNode(); break;
                case 3: createPipelineNode(); break;
                case 4: createModelNode(); break;
            }
            selectedNodeIndex = -1; // Reset selection
        }
    ImGui::EndChild();

    ImGui::NextColumn(); // Move to Middle Column

    // ========== Middle: Pipeline Editor ==========
    ImGui::BeginChild("PipelineEditor", ImVec2(0, 0), true);

    if (!g_Context)
        g_Context = ed::CreateEditor();

    ed::SetCurrentEditor(g_Context);
    ed::Begin("Pipeline Editor");

    drawNodes();

    // Handle Double Click on Node
    ed::NodeId clickedNode = ed::GetDoubleClickedNode();
    if (clickedNode) {
    	selectedNode = clickedNode.Get(); // Store the selected node ID
        printf("Double-click detected on node: %d\n", selectedNode);
    }

    // Ensure we are inside a valid session
       if (ed::BeginCreate()) {
           ax::NodeEditor::PinId startPin, endPin;
           if (ed::QueryNewLink(&startPin, &endPin)) {
               if (startPin != endPin && ed::AcceptNewItem()) { // Ensure no self-linking
                   links.push_back({ nextLinkId++, (int)startPin.Get(), (int)endPin.Get() });
                   //ed::CommitNewItem();
               }
           }
       }
       ed::EndCreate(); // End link creation session

       // Handle Link Deletion
       if (ed::BeginDelete()) {
           ax::NodeEditor::LinkId linkId;
           while (ed::QueryDeletedLink(&linkId)) {
               auto it = std::remove_if(links.begin(), links.end(),
                                        [linkId](const Link& link) { return link.id == linkId.Get(); });
               if (it != links.end()) {
                   links.erase(it);
                   //ed::CommitDeletedItem();
               }
           }
       }
       ed::EndDelete(); // End link deletion session

    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGui::EndChild();

    ImGui::NextColumn(); // Move to Right Column

    // ========== Right: Edit Panel ==========
    ImGui::BeginChild("EditPanel", ImVec2(0, 0), true);

    auto it = std::find_if(nodes.begin(), nodes.end(), [](const Node& n) {
        return n.id == selectedNode;
    });

    if (it != nodes.end()) {
        Node& node = *it;
        ImGui::Text("Editing Pipeline Node: %d", selectedNode);

        // ==== Manage Inputs ====
        ImGui::Text("Inputs:");
        for (size_t i = 0; i < node.inputPins.size(); ++i) {
            ImGui::PushID(i); // Unique ID for each pin

            ImGui::InputText("##InputName", node.inputPins[i].name.data(), node.inputPins[i].name.capacity() + 1);
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                node.inputPins.erase(node.inputPins.begin() + i); // Remove pin
                ImGui::PopID();
                break; // Avoid invalid access
            }

            ImGui::PopID();
        }

        if (ImGui::Button("Add Input")) {
            node.inputPins.push_back({nextLinkId++, "New Input"});
        }

        ImGui::Separator();

        // ==== Manage Outputs ====
        ImGui::Text("Outputs:");
        for (size_t i = 0; i < node.outputPins.size(); ++i) {
            ImGui::PushID(i + 1000); // Unique ID for each output

            ImGui::InputText("##OutputName", node.outputPins[i].name.data(), node.outputPins[i].name.capacity() + 1);
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                node.outputPins.erase(node.outputPins.begin() + i); // Remove pin
                ImGui::PopID();
                break;
            }

            ImGui::PopID();
        }

        if (ImGui::Button("Add Output")) {
            node.outputPins.push_back({nextLinkId++, "New Output"});
        }

        ImGui::Separator();

        if (node.settings) {
            showInputAssemblySettings(node.settings.value());
            showRasterizerSettings(node.settings.value());
            showDepthStencilSettings(node.settings.value());
            showMultisamplingSettings(node.settings.value());
            showColorBlendingSettings(node.settings.value());
            showShaderSettings(node.settings.value());
        } else {
            ImGui::Text("This node has no configurable pipeline settings.");
        }

        if (ImGui::Button("Cancel")) {
            selectedNode = -1; // Reset selection
        }
    } else {
        ImGui::Text("Double-click a node to edit.");
    }

    ImGui::EndChild();

    ImGui::Columns(1); // Reset columns
}
