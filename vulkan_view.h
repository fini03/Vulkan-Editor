#include "imgui/imgui.h"
#include "imgui-node-editor/imgui_node_editor.h"
#include "model.h"
#include "pipeline.h"
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
    std::cerr << "No PipelineNode found!\n";
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
	// ======== Top: Toolbar with Generate Button ========
    ImGui::Begin("Pipeline Controls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    if (ImGui::Button("Generate Pipeline")) {
        for (const auto& node : editor.nodes) {
            if (auto pipelineNode = dynamic_cast<PipelineNode*>(node.get())) {
                pipelineNode->generate(); // Assuming generate() returns a string
                //std::cout << "Generated Pipeline:\n" << pipelineData << std::endl;
                break; // Assuming only one PipelineNode exists
            }
        }
    }

    ImGui::End(); // End of Pipeline Controls panel
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

                   // Find the nodes being linked
                    Node* startNode = nullptr;
                    Node* endNode = nullptr;

                                   for (const auto& node : editor.nodes) {
                                       if (node->id * 10 + 1 == link.startPin.Get() || node->id * 10 + 2 == link.startPin.Get())
                                           startNode = node.get();
                                       if (node->id * 10 + 1 == link.endPin.Get() || node->id * 10 + 2 == link.endPin.Get())
                                           endNode = node.get();
                                   }

                                   // If the link is from ModelNode to PipelineNode, set the inputs
                                   if (auto modelNode = dynamic_cast<ModelNode*>(startNode)) {
                                       if (auto pipelineNode = dynamic_cast<PipelineNode*>(endNode)) {
                                           pipelineNode->setVertexDataInput(modelNode);
                                           pipelineNode->setTextureDataInput(modelNode);
                                       }
                                   } else if (auto modelNode = dynamic_cast<ModelNode*>(endNode)) {
                                       if (auto pipelineNode = dynamic_cast<PipelineNode*>(startNode)) {
                                           pipelineNode->setVertexDataInput(modelNode);
                                           pipelineNode->setTextureDataInput(modelNode);
                                       }
                                   }
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
