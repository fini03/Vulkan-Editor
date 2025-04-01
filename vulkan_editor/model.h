#pragma once
#include "renderpass.h"

class VertexDataNode {
public:
    virtual void generateVertexBindings(std::ofstream& outFile) = 0;
};

class ColorDataNode {
public:
    virtual void generateColorBindings(std::ofstream& outFile) = 0;
};

class TextureDataNode {
public:
    virtual void generateTextureBindings(std::ofstream& outFile) = 0;
};

class ModelNode : public Node, public VertexDataNode, public ColorDataNode, public TextureDataNode {
public:
	size_t attributesCount = 0;
	char modelPath[256] = "data/models/viking_room.obj";
	char texturePath[256] = "data/images/viking_room.png";

    ModelNode(int id);

    ~ModelNode() override;

    void generateVertexBindings(std::ofstream& outFile) override;

    void generateColorBindings(std::ofstream& outFile) override;

    void generateTextureBindings(std::ofstream& outFile) override;

    void generateVertexStructFilePart1(std::ofstream& outFile);

    void generateVertexStructFilePart2(std::ofstream& outFile);
    void generateModel(std::ofstream& outFile, TemplateLoader templateLoader);

    void render() const override;
};
