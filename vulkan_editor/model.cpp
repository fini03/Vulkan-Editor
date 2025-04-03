#include "model.h"

namespace ed = ax::NodeEditor;

ModelNode::ModelNode(int id) : Node(id) {
 	outputPins.push_back({ ed::PinId(id * 10 + 1), PinType::VertexOutput });
  	outputPins.push_back({ ed::PinId(id * 10 + 2), PinType::ColorOutput });
    outputPins.push_back({ ed::PinId(id * 10 + 3), PinType::TextureOutput });
    attributesCount = outputPins.size();

    data["modelPath"] = modelPath;
    data["texturePath"] = texturePath;
}

ModelNode::~ModelNode() { }

std::string ModelNode::generateVertexBindings(std::ofstream& outFile) {
	std::string result = "";

    result += "        attributeDescriptions[0].binding = 0;\n";
    result += "        attributeDescriptions[0].location = 0;\n";
    result += "        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;\n";
    result += "        attributeDescriptions[0].offset = offsetof(Vertex, pos);\n\n";

    return result;
}

std::string ModelNode::generateColorBindings(std::ofstream& outFile) {
	std::string result = "";
    result += "        attributeDescriptions[1].binding = 0;\n";
    result += "        attributeDescriptions[1].location = 1;\n";
    result += "        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;\n";
    result += "        attributeDescriptions[1].offset = offsetof(Vertex, color);\n\n";

    return result;
}

std::string ModelNode::generateTextureBindings(std::ofstream& outFile) {
	std::string result = "";
	result += "        attributeDescriptions[2].binding = 0;\n";
	result += "        attributeDescriptions[2].location = 2;\n";
	result += "        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;\n";
    result += "        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);\n\n";

    return result;
}

std::string ModelNode::generateVertexStructFilePart1(std::ofstream& outFile) {
	std::string result = "";
    result +=  R"(
struct Vertex {
	glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
    	VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
)";
    result += "        std::vector<VkVertexInputAttributeDescription>attributeDescriptions(" + std::to_string(attributesCount);
   result += ");\n\n";

   return result;
}

std::string ModelNode::generateVertexStructFilePart2(std::ofstream& outFile) {
	std::string result = "";
    result += "        return attributeDescriptions;\n";
    result += "    }\n\n";

    result += "    bool operator==(const Vertex& other) const {\n";
    result += "        return pos == other.pos && color == other.color && texCoord == other.texCoord;\n";
    result += "    }\n";
    result += "};\n\n";

    result += R"(
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
    return result;
}

std::string ModelNode::generateModel(TemplateLoader templateLoader) {
    RenderPassNode renderpass{id};

    data["buffer"] = templateLoader.renderTemplateFile("vulkan_templates/buffer.txt", data);
    data["image"] = templateLoader.renderTemplateFile("vulkan_templates/image.txt", data);

    data["renderpass"] = renderpass.generateRenderpass(templateLoader);

    return templateLoader.renderTemplateFile("vulkan_templates/model.txt", data);
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
