#include "model.h"
#include "utils.h"

namespace ed = ax::NodeEditor;
static inja::json data;

ModelNode::ModelNode(int id) : Node(id) {
 	outputPins.push_back({ ed::PinId(id * 10 + 1), PinType::VertexOutput });
  	outputPins.push_back({ ed::PinId(id * 10 + 2), PinType::ColorOutput });
    outputPins.push_back({ ed::PinId(id * 10 + 3), PinType::TextureOutput });
    attributesCount = outputPins.size();

    data["modelPath"] = modelPath;
    data["texturePath"] = texturePath;
}

ModelNode::~ModelNode() { }

void ModelNode::generateVertexBindings(std::ofstream& outFile) {
	if (outFile.is_open()) {
        outFile << "        attributeDescriptions[0].binding = 0;\n"
    	        << "        attributeDescriptions[0].location = 0;\n"
                << "        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;\n"
                << "        attributeDescriptions[0].offset = offsetof(Vertex, pos);\n\n";
    } else {
       std::cerr << "Error opening file for writing.\n";
    }
}

void ModelNode::generateColorBindings(std::ofstream& outFile) {
    if (outFile.is_open()) {
        outFile << "        attributeDescriptions[1].binding = 0;\n"
                << "        attributeDescriptions[1].location = 1;\n"
                << "        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;\n"
                << "        attributeDescriptions[1].offset = offsetof(Vertex, color);\n\n";
    } else {
    	std::cerr << "Error opening file for writing.\n";
    }
}

void ModelNode::generateTextureBindings(std::ofstream& outFile) {
    if (outFile.is_open()) {
        outFile << "        attributeDescriptions[2].binding = 0;\n"
                << "        attributeDescriptions[2].location = 2;\n"
                << "        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;\n"
                << "        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);\n\n";
    } else {
        std::cerr << "Error opening file for writing.\n";
    }
}

void ModelNode::generateVertexStructFilePart1(std::ofstream& outFile) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << "struct Vertex {\n"
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
}

void ModelNode::generateVertexStructFilePart2(std::ofstream& outFile) {
   	if (!outFile.is_open()) {
    	std::cerr << "Error opening file for writing.\n";
	    return;
	}

    outFile << "        return attributeDescriptions;\n"
            << "    }\n\n"

            << "    bool operator==(const Vertex& other) const {\n"
            << "        return pos == other.pos && color == other.color && texCoord == other.texCoord;\n"
            << "    }\n"
            << "};\n\n";

    outFile << R"(
namespace std {
    template<> struct hash<glm::vec2> {
        size_t operator()(glm::vec2 const& vec) const {
            return (hash<float>()(vec.x) ^ (hash<float>()(vec.y) << 1)) >> 1;
        }
    };

    template<> struct hash<glm::vec3> {
        size_t operator()(glm::vec3 const& vec) const {
            return ((hash<float>()(vec.x) ^ (hash<float>()(vec.y) << 1)) >> 1) ^ (hash<float>()(vec.z) << 1);
        }
    };

    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1)
                ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}
)";
}

void ModelNode::generateModel(std::ofstream& outFile, TemplateLoader templateLoader) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << templateLoader.renderTemplateFile("vulkan_templates/model.txt", data);
    generateBufferManagementCode(outFile, templateLoader);
    generateImageManagementCode(outFile, templateLoader);

    RenderPassNode renderpass{id};
    renderpass.generateRenderpass(outFile, templateLoader);
}

void ModelNode::render() const {
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
