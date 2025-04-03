#pragma once
#include "renderpass.h"

class VertexDataNode {
public:
    virtual std::string generateVertexBindings(std::ofstream& outFile) = 0;
};

class ColorDataNode {
public:
    virtual std::string generateColorBindings(std::ofstream& outFile) = 0;
};

class TextureDataNode {
public:
    virtual std::string generateTextureBindings(std::ofstream& outFile) = 0;
};

class ModelNode : public Node, public VertexDataNode, public ColorDataNode, public TextureDataNode {
public:
	size_t attributesCount = 0;
	char modelPath[256] = "data/models/viking_room.obj";
	char texturePath[256] = "data/images/viking_room.png";

    ModelNode(int id);

    ~ModelNode() override;

    std::string generateVertexBindings(std::ofstream& outFile) override;
    std::string generateColorBindings(std::ofstream& outFile) override;
    std::string generateTextureBindings(std::ofstream& outFile) override;

    std::string generateVertexStructFilePart1(std::ofstream& outFile);
    std::string generateVertexStructFilePart2(std::ofstream& outFile);

    std::string generateModel(TemplateLoader templateLoader);

    void render() const override;
private:
	inja::json data;
};
