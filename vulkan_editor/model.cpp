#include "model.h"
#include "utils.h"

namespace ed = ax::NodeEditor;

ModelNode::ModelNode(int id) : Node(id) {
 	outputPins.push_back({ ed::PinId(id * 10 + 1), PinType::VertexOutput });
  	outputPins.push_back({ ed::PinId(id * 10 + 2), PinType::ColorOutput });
    outputPins.push_back({ ed::PinId(id * 10 + 3), PinType::TextureOutput });
    attributesCount = outputPins.size();
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

    std::cout << "Vertex struct successfully written to Vertex.h\n";
}

void ModelNode::generateModel(std::ofstream& outFile, TemplateLoader templateLoader) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << "const std::string m_MODEL_PATH = \"" << modelPath << "\";\n";
    outFile << "const std::string m_TEXTURE_PATH = \"" << texturePath << "\";\n";

    outFile << R"(
class VulkanTutorial {
	public:
    VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void loadModel( Geometry& geometry, const std::string& MODEL_PATH) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(geometry.m_vertices.size());
                    geometry.m_vertices.push_back(vertex);
                }

                geometry.m_indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void createObject(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VmaAllocator vmaAllocator,
        VkQueue graphicsQueue,
        VkCommandPool commandPool,
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout descriptorSetLayout,
        glm::mat4&& model,
        std::string modelPath,
        std::string texturePath,
        std::vector<Object>& objects
    ) {
        Object object{{model}};
        createTextureImage(physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, texturePath, object.m_texture);
        createTextureImageView(device, object.m_texture);
        createTextureSampler(physicalDevice, device, object.m_texture);
        loadModel(object.m_geometry, modelPath);
        createVertexBuffer(physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, object.m_geometry);
        createIndexBuffer(physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, object.m_geometry);
        createUniformBuffers(physicalDevice, device, vmaAllocator, object.m_uniformBuffers);
        createDescriptorSets(device, object.m_texture, descriptorSetLayout, object.m_uniformBuffers, descriptorPool, object.m_descriptorSets);
        objects.push_back(object);
    }
    )";

    generateUtilsManagementCode(outFile, templateLoader);

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
