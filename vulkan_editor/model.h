#pragma once
#include <fstream>
#include "renderpass.h"

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
