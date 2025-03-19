#pragma once
#include "imgui.h"
#include "vulkan_editor/vulkan_base.h"
#include <fstream>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include "node.h"
#include "imgui-node-editor/imgui_node_editor.h"

namespace ed = ax::NodeEditor;

class VertexDataNode {
public:
    virtual void generateVertexBindings() = 0;
};

class ColorDataNode {
public:
    virtual void generateColorBindings() = 0;
};

class TextureDataNode {
public:
    virtual void generateTextureBindings() = 0;
};

class ModelNode : public Node, public VertexDataNode, public ColorDataNode, public TextureDataNode {
public:
	std::ofstream outFile;
	size_t attributesCount = 0;
    ModelNode(int id) : Node(id) {
    	outputPins.push_back({ ed::PinId(id * 10 + 1), PinType::VertexOutput });
     	outputPins.push_back({ ed::PinId(id * 10 + 2), PinType::ColorOutput });
        outputPins.push_back({ ed::PinId(id * 10 + 3), PinType::TextureOutput });
        attributesCount = outputPins.size();
    }

    ~ModelNode() override { }

    void generateVertexBindings() override {
    	outFile.open("Vertex.h", std::ios::app);
        if (outFile.is_open()) {
            outFile << "        attributeDescriptions[0].binding = 0;\n"
                    << "        attributeDescriptions[0].location = 0;\n"
                    << "        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;\n"
                    << "        attributeDescriptions[0].offset = offsetof(Vertex, pos);\n\n";
            outFile.close();
        } else {
            std::cerr << "Error opening file for writing.\n";
        }
    }

    void generateColorBindings() override {
    	outFile.open("Vertex.h", std::ios::app);
        if (outFile.is_open()) {
            outFile << "        attributeDescriptions[1].binding = 0;\n"
                    << "        attributeDescriptions[1].location = 1;\n"
                    << "        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;\n"
                    << "        attributeDescriptions[1].offset = offsetof(Vertex, color);\n\n";
            outFile.close();
        } else {
            std::cerr << "Error opening file for writing.\n";
        }
    }

    void generateTextureBindings() override {
    	outFile.open("Vertex.h", std::ios::app);
        if (outFile.is_open()) {
            outFile << "        attributeDescriptions[2].binding = 0;\n"
                    << "        attributeDescriptions[2].location = 2;\n"
                    << "        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;\n"
                    << "        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);\n\n";
            outFile.close();
        } else {
            std::cerr << "Error opening file for writing.\n";
        }
    }

    void generateVertexStructFile() {
    	outFile.open("Vertex.h", std::ios::trunc);
        if (!outFile.is_open()) {
            std::cerr << "Error opening file for writing.\n";
            return;
        }

        outFile << "#pragma once\n\n"
                << "#include <vulkan/vulkan.h>\n"
                << "#define GLM_FORCE_RADIANS\n"
                << "#define GLM_FORCE_DEPTH_ZERO_TO_ONE\n"
                << "#define GLM_ENABLE_EXPERIMENTAL\n"
                << "#include <glm/glm.hpp>\n"
                << "#include <glm/gtc/matrix_transform.hpp>\n"
                << "#include <glm/gtx/hash.hpp>\n"
                << "#include <vector>\n"
                << "#include <array>\n\n"

                << "struct Vertex {\n"
                << "    glm::vec3 pos;\n"
                << "    glm::vec3 color;\n"
                << "    glm::vec2 texCoord;\n\n"

                << "    static VkVertexInputBindingDescription getBindingDescription() {\n"
                << "        VkVertexInputBindingDescription bindingDescription{};\n"
                << "        bindingDescription.binding = 0;\n"
                << "        bindingDescription.stride = sizeof(Vertex);\n"
                << "        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;\n\n"
                << "        return bindingDescription;\n"
                << "    }\n\n"

                << "    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {\n"
                << "        std::vector<VkVertexInputAttributeDescription>attributeDescriptions(" << attributesCount << ");\n\n";

                outFile.close();
    }

    void generatePart2() {
    	outFile.open("Vertex.h", std::ios::app);
	    if (!outFile.is_open()) {
	        std::cerr << "Error opening file for writing.\n";
	        return;
	    }

        outFile << "        return attributeDescriptions;\n"
                << "    }\n\n"

                << "    bool operator==(const Vertex& other) const {\n"
                << "        return pos == other.pos && color == other.color && texCoord == other.texCoord;\n"
                << "    }\n"
                << "};\n\n"

                << "namespace std {\n"
                << "    template<> struct hash<Vertex> {\n"
                << "        size_t operator()(Vertex const& vertex) const {\n"
                << "            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);\n"
                << "        }\n"
                << "    };\n"
                << "}\n";

        outFile.close();
        std::cout << "Vertex struct successfully written to Vertex.h\n";
    }

    void render() const override {
	    ed::BeginNode(this->id);
	    ImGui::Text("Model");

	    // Draw Pins
		ed::BeginPin(this->id * 10 + 1, ed::PinKind::Output);
        ImGui::Text("*vertex_data");
        ed::EndPin();

        ed::BeginPin(this->id * 10 + 2, ed::PinKind::Output);
        ImGui::Text("*color_data");
        ed::EndPin();

        ed::BeginPin(this->id * 10 + 3, ed::PinKind::Output);
        ImGui::Text("*texture_data");
        ed::EndPin();

	    ed::EndNode();

		for (auto& link : links) {
        	ed::Link(link.id, link.startPin, link.endPin);
    	}

    }
};
