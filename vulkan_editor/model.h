#pragma once
#include "../imgui/imgui.h"
#include <fstream>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
//#include "node.h"
#include "swapchain.h"
#include "../imgui-node-editor/imgui_node_editor.h"
#include "../libs/tinyfiledialogs.h"

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
	char modelPath[256] = "data/models/viking_room.obj";
	char texturePath[256] = "data/images/viking_room.png";

    ModelNode(int id);

    ~ModelNode() override;

    void generateVertexBindings() override;

    void generateColorBindings() override;

    void generateTextureBindings() override;

    void generateVertexStructFilePart1();

    void generateVertexStructFilePart2();
    void generateModel();

    void render() const override;
};
