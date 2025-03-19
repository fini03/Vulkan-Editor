#include "imgui/imgui.h"
#include "imgui-node-editor/imgui_node_editor.h"
#include "model.h"
#include "pipeline.h"
#include "tinyfiledialogs.h"
#include "vulkan_editor/vulkan_base.h"
#include <SDL3/SDL_video.h>
#include <algorithm>
#include <map>
#include "vulkan_generator.h"
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
            pipelineNode->generate(); // Assuming generate() returns std::string
        }
    }
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

void showPipelineView() {
    ImGui::Begin("Pipeline Controls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    if (ImGui::Button("Generate Pipeline")) {
    	saveFile();
    }

    ImGui::End();

    if (!editor.context) NodeEditorInitialize();

    ed::SetCurrentEditor(editor.context);
    ed::Begin("Pipeline Editor");

    for (const auto& node : editor.nodes) node->render();

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
                            if (startPinType == PinType::VertexOutput && endPinType == PinType::VertexInput) {
                                pipelineNode->setVertexDataInput(modelNode);
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
}
