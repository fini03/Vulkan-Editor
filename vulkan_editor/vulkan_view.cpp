#include "vulkan_view.h"
#include "model.h"
#include "template_loader.h"
#include <vulkan/vulkan.h>

TemplateLoader templateLoader;

PipelineNode* selectedPipelineNode = nullptr;
ModelNode* selectedModelNode = nullptr;
std::vector<uint32_t> colorWriteMaskOptions = { VK_COLOR_COMPONENT_R_BIT, VK_COLOR_COMPONENT_G_BIT, VK_COLOR_COMPONENT_B_BIT, VK_COLOR_COMPONENT_A_BIT };

Editor::Editor(const std::vector<std::string> templateFileNames) {
    if (!context) nodeEditorInitialize();

    for (const std::string& fileName : templateFileNames) {
    	templateLoader.loadTemplateFile(fileName);
    }
}

void Editor::nodeEditorInitialize() {
    context = ed::CreateEditor();
    nodes.emplace_back(std::make_unique<PipelineNode>(currentId++));
    nodes.emplace_back(std::make_unique<ModelNode>(currentId++));
}

void Editor::saveFile() {
    for (const auto& node : nodes) {
        if (auto pipelineNode = dynamic_cast<PipelineNode*>(node.get())) {
            pipelineNode->generate(templateLoader, pipelineNode->settings.value());
        }
    }
}

void Editor::showInputAssemblySettings(PipelineSettings& settings) {
    ImGui::Text("Input Assembly");

    ImGui::Combo("Vertex Topology", &settings.inputAssembly, topologyOptions.data(), topologyOptions.size());

    ImGui::Checkbox("Primitive Restart", &settings.primitiveRestart);
}

void Editor::showRasterizerSettings(PipelineSettings& settings) {
    ImGui::Separator();
    ImGui::Text("Rasterizer");

    ImGui::Checkbox("Depth Clamp", &settings.depthClamp);
    ImGui::Checkbox("Rasterizer Discard", &settings.rasterizerDiscard);

    ImGui::Combo("Polygon Mode", &settings.polygonMode, polygonModes.data(), polygonModes.size());

    ImGui::InputFloat("Line Width", &settings.lineWidth);

    ImGui::Combo("Cull Mode", &settings.cullMode, cullModes.data(), cullModes.size());

    ImGui::Combo("Front Face", &settings.frontFace, frontFaceOptions.data(), frontFaceOptions.size());

    ImGui::Checkbox("Depth Bias Enabled", &settings.depthBiasEnabled);
}

void Editor::showDepthStencilSettings(PipelineSettings& settings) {
    ImGui::Separator();
    ImGui::Text("Depth & Stencil");

    ImGui::Checkbox("Depth Test", &settings.depthTest);
    ImGui::Checkbox("Depth Write", &settings.depthWrite);

    ImGui::Combo("Depth Compare Operation", &settings.depthCompareOp, depthCompareOptions.data(), depthCompareOptions.size());

    ImGui::Checkbox("Depth Bounds Test", &settings.depthBoundsTest);

    ImGui::Checkbox("Stencil Test", &settings.stencilTest);
}

void Editor::showMultisamplingSettings(PipelineSettings& settings) {
    ImGui::Separator();
    ImGui::Text("Multisampling");

    ImGui::Checkbox("Sample Shading", &settings.sampleShading);

    ImGui::Combo("Rasterization Samples", &settings.rasterizationSamples, sampleCountOptions.data(), sampleCountOptions.size());
}

void Editor::showColorBlendingSettings(PipelineSettings& settings) {
    ImGui::Separator();
    ImGui::Text("Color Blending");

    ImGui::Text("Color Write Mask (Multiple selection)");

    for (int i = 0; i < 4; i++) {
        bool selected = (settings.colorWriteMask & colorWriteMaskOptions[i]) != 0;
        if (ImGui::Checkbox(colorWriteMaskNames[i], &selected)) {
        	if (selected) {
            	settings.colorWriteMask |= colorWriteMaskOptions[i];  // Enable flag
         	} else {
            	settings.colorWriteMask &= ~colorWriteMaskOptions[i]; // Disable flag
            }
        }
        if (i < 3) ImGui::SameLine();
    }

    ImGui::Checkbox("Color Blend", &settings.colorBlend);

    ImGui::Checkbox("Logic Operation Enabled", &settings.logicOpEnable);

    ImGui::Combo("Logic Operation", &settings.logicOp, logicOps.data(), logicOps.size());

    ImGui::InputInt("Attachment Count", &settings.attachmentCount);

    ImGui::InputFloat4("Color Blend Constants", settings.blendConstants);
}

void Editor::showShaderFileSelector(PipelineSettings& settings) {
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

void Editor::showShaderSettings(PipelineSettings& settings) {
    ImGui::Separator();
    ImGui::Text("Shaders");
    showShaderFileSelector(settings);
}


void Editor::showPipelineView() {
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

    if (!context) nodeEditorInitialize();

    ed::SetCurrentEditor(context);
    ed::Begin("Pipeline Editor");

    for (const auto& node : nodes) {
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
                link.id = currentId++;

                Node* startNode = nullptr;
                Node* endNode = nullptr;
                PinType startPinType = PinType::Unknown, endPinType = PinType::Unknown;

                for (const auto& node : nodes) {
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
                    links.push_back(link);

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
        auto it = std::remove_if(links.begin(), links.end(),
            [linkId](const Link& link) { return link.id == linkId.Get(); });
            if (it != links.end()) {
            	links.erase(it);
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
        showInputAssemblySettings(selectedPipelineNode->settings.value());
        showRasterizerSettings(selectedPipelineNode->settings.value());
        showDepthStencilSettings(selectedPipelineNode->settings.value());
        showMultisamplingSettings(selectedPipelineNode->settings.value());
        showColorBlendingSettings(selectedPipelineNode->settings.value());
        showShaderSettings(selectedPipelineNode->settings.value());
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

void Editor::showModelView() {
 	for (const auto& node : nodes) {
        if (auto modelNode = dynamic_cast<ModelNode*>(node.get())) {
            selectedModelNode = modelNode;
        }
    }

    if(!selectedModelNode) return;

    ImGui::Text("Model File:");
    ImGui::InputText("##modelFile", selectedModelNode->modelPath, IM_ARRAYSIZE(selectedModelNode->modelPath));
    ImGui::SameLine();
    if (ImGui::Button("...##Model")) {
        const char* filter[] = { "*.obj", "*.fbx", "*.gltf", "*.glb", "*.dae", "*.*" }; // Model file filters
        const char* selectedPath = tinyfd_openFileDialog("Select Model File", "", 6, filter, "Model Files", 0);
        if (selectedPath) {
            strncpy(selectedModelNode->modelPath, selectedPath, IM_ARRAYSIZE(selectedModelNode->modelPath));
        }
    }

    ImGui::Text("Texture File:");
    ImGui::InputText("##textureFile", selectedModelNode->texturePath, IM_ARRAYSIZE(selectedModelNode->texturePath));
    ImGui::SameLine();
    if (ImGui::Button("...##Texture")) {
        const char* filter[] = { "*.png", "*.jpg", "*.jpeg", "*.tga", "*.bmp", "*.dds", "*.*" }; // Texture file filters
        const char* selectedPath = tinyfd_openFileDialog("Select Texture File", "", 7, filter, "Texture Files", 0);
        if (selectedPath) {
            strncpy(selectedModelNode->texturePath, selectedPath, IM_ARRAYSIZE(selectedModelNode->texturePath));
        }
    }
}

void Editor::startEditor() {
    if (ImGui::BeginTabBar("MainTabBar")) {
        if (ImGui::BeginTabItem("Model")) {
            showModelView();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Graphics Pipeline")) {
            showPipelineView();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}
