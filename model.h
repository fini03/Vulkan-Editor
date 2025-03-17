#pragma once
#include "imgui.h"
#include "vulkan_editor/vulkan_base.h"
#include "vulkan_utils.h"
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
    virtual void generateVertexBindings() const = 0;
};

class TextureDataNode {
public:
    virtual void generateTextureBindings() const = 0;
};

class ModelNode : public Node, public VertexDataNode, public TextureDataNode {
public:
    ModelNode(int id) : Node(id) { }

    ~ModelNode() override { }

    void generateVertexBindings() const override {
        std::cout << "VkVertexBindingsquackquackquack\n\n";
    }

    void generateTextureBindings() const override {
        std::cout << "Code for setting up texture bindings\n\n";
    }

    void render() const override {
	    ed::BeginNode(this->id);
	    ImGui::Text("Model");

	    // Draw Pins
        ed::BeginPin(this->id * 10 + 1, ed::PinKind::Output);
        ImGui::Text("*data");
        ed::EndPin();

	    ed::EndNode();

		for (auto& link : links) {
        	ed::Link(link.id, link.startPin, link.endPin);
    	}

    }
};


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

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

class Model {
public:
	std::vector<Vertex> vertices = {};
	std::vector<uint32_t> indices = {};
    VulkanImage textureImage = {};

    VulkanBuffer vertexBuffer = {};
    VulkanBuffer indexBuffer = {};

    VkVertexInputBindingDescription bindingDescription;
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

    // Methods
    void createModel(VulkanContext* context, const char* modelPath, const char* texturePath);
    void destroyModel(VulkanContext* context);
};
