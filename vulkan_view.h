#include "imgui/imgui.h"
#include "imgui-node-editor/imgui_node_editor.h"
#include "model.h"
#include "pipeline.h"
#include "tinyfiledialogs.h"
#include "vulkan_editor/vulkan_base.h"
#include <SDL3/SDL_video.h>
#include <algorithm>
#include <map>
#include "vulkan_config.h"
#include "libs/tinyxml2.h"
#include <fstream>
#include <memory>
#include <optional>

namespace ed = ax::NodeEditor;

VulkanConfig config = {};
namespace ed = ax::NodeEditor;


struct Editor {
    ed::EditorContext* context = nullptr;
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<Link> links;
    int currentId = 1;
};

Editor editor = {};

void NodeEditorInitialize() {
    editor.context = ed::CreateEditor();
    editor.nodes.emplace_back(std::make_unique<PipelineNode>(editor.currentId++));
    editor.nodes.emplace_back(std::make_unique<ModelNode>(editor.currentId++));
}

void saveFile() {
    for (const auto& node : editor.nodes) {
        if (auto pipelineNode = dynamic_cast<PipelineNode*>(node.get())) {
            pipelineNode->generate();
        }
    }
}

void showModelView(VulkanContext* context) {
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
    ImGui::EndTabItem();
}
static PipelineNode* selectedPipelineNode = nullptr;
void showPipelineView() {
	// Place the Generate button on the same row, aligned to the right.
    // First, get the available width of the window's content region.
    float windowWidth = ImGui::GetWindowWidth();

    float buttonWidth = 200.0f;
    float padding = 12.0f;

    ImGui::SameLine();
    // Set the cursor position X to (window width - button width)
    ImGui::SetCursorPosX(windowWidth - buttonWidth - padding);
    if (ImGui::Button("Generate GVE Project Header", ImVec2(buttonWidth, 0))) {
        saveFile();
    }
    // Set up window layout: Left = Node Editor, Right = Edit Panel
    ImGui::Columns(2, NULL, true);

    // ========== Left: Pipeline Editor ==========
    ImGui::BeginChild("PipelineEditor", ImVec2(0, 0), true);

    if (!editor.context) NodeEditorInitialize();

    ed::SetCurrentEditor(editor.context);
    ed::Begin("Pipeline Editor");

    for (const auto& node : editor.nodes) {
    	node->render();
	    // Detect double-click on a PipelineNode
		ed::NodeId clickedNode = ed::GetDoubleClickedNode();
	    if (auto pipelineNode = dynamic_cast<PipelineNode*>(node.get())) {
	        if (clickedNode.Get() == pipelineNode->id) {
	            selectedPipelineNode = pipelineNode; // Store selected node
	        }
	    }
    }

    if (ed::BeginCreate()) {
        Link link;
        if (ed::QueryNewLink(&link.startPin, &link.endPin)) {
            if (link.startPin != link.endPin && ed::AcceptNewItem()) {
                link.id = editor.currentId++;

                Node* startNode = nullptr;
                Node* endNode = nullptr;
                PinType startPinType = PinType::Unknown, endPinType = PinType::Unknown;

                for (const auto& node : editor.nodes) {
                    for (const auto& pin : node->outputPins) {
                        if (pin.id == link.startPin) {
                            startNode = node.get();
                            startPinType = pin.type;
                        }
                    }
                    for (const auto& pin : node->inputPins) {
                        if (pin.id == link.endPin) {
                            endNode = node.get();
                            endPinType = pin.type;
                        }
                    }
                }

                if (startNode && endNode) {
                    startNode->addLink(link);
                    endNode->addLink(link);
                    editor.links.push_back(link);

                    if (auto modelNode = dynamic_cast<ModelNode*>(startNode)) {
                        if (auto pipelineNode = dynamic_cast<PipelineNode*>(endNode)) {
                        	pipelineNode->setModel(modelNode);
                            if (startPinType == PinType::VertexOutput && endPinType == PinType::VertexInput) {
                                pipelineNode->setVertexDataInput(modelNode);
                            }
                            if (startPinType == PinType::ColorOutput && endPinType == PinType::ColorInput) {
                                pipelineNode->setColorDataInput(modelNode);
                            }
                            if (startPinType == PinType::TextureOutput && endPinType == PinType::TextureInput) {
                                pipelineNode->setTextureDataInput(modelNode);
                            }
                        }
                    }
                }
            }
        }
    }
    ed::EndCreate();

    if (ed::BeginDelete()) {
        ed::LinkId linkId;
        while (ed::QueryDeletedLink(&linkId)) {
        auto it = std::remove_if(editor.links.begin(), editor.links.end(),
            [linkId](const Link& link) { return link.id == linkId.Get(); });
            if (it != editor.links.end()) {
            	editor.links.erase(it);
            }
        }
    }
    ed::EndDelete();

    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGui::EndChild();

    // Move to right panel
    ImGui::NextColumn();

    // ========== Right: Edit Panel ==========
    ImGui::BeginChild("PipelineSettings", ImVec2(0, 0), true);

    ImGui::Text("Pipeline Settings");
    ImGui::Separator();

    if (selectedPipelineNode&& selectedPipelineNode->settings) {
        selectedPipelineNode->showInputAssemblySettings(selectedPipelineNode->settings.value());
        selectedPipelineNode->showRasterizerSettings(selectedPipelineNode->settings.value());
        selectedPipelineNode->showDepthStencilSettings(selectedPipelineNode->settings.value());
        selectedPipelineNode->showMultisamplingSettings(selectedPipelineNode->settings.value());
        selectedPipelineNode->showColorBlendingSettings(selectedPipelineNode->settings.value());
        selectedPipelineNode->showShaderSettings(selectedPipelineNode->settings.value());
    } else {
        ImGui::Text("This node has no configurable pipeline settings.");
    }

    if (ImGui::Button("Cancel")) {
        selectedPipelineNode = nullptr; // Reset selection
    }


    ImGui::EndChild();

    // End column layout
    ImGui::Columns(1);
}
