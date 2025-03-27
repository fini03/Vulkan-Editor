#include "model.h"
#include <optional>

struct PipelineSettings {
    // Define settings for each category
    int inputAssembly = 0;
    bool primitiveRestart = false;

    bool depthClamp = false, rasterizerDiscard = false;
    int polygonMode = 0;
    float lineWidth = 0;
    int cullMode = 0;
    int frontFace = 0;
    bool depthBiasEnabled = false;

    bool depthTest = true, depthWrite = true;
    int depthCompareOp = 0;
    bool depthBoundsTest = false;
    bool stencilTest = false;

    bool sampleShading = false;
    int rasterizationSamples = 0;

    int colorWriteMask = 0xF;
    bool colorBlend = false;
    bool logicOpEnable = false;
    int logicOp = 0;
    int attachmentCount = 1;
    float blendConstants[4] = { 0.0f, 1.0f, 2.0f, 3.0f };

    char vertexShaderPath[256] = "shaders/vert.spv";
    char vertexEntryName[64] = "main";
    char fragmentShaderPath[256] = "shaders/frag.spv";
    char fragmentEntryName[64] = "main";
};

class PipelineNode : public Node {
public:
	std::optional<PipelineSettings> settings = PipelineSettings{};
	std::ofstream outFile;
    PipelineNode(int id);

    ~PipelineNode() override;

    void render() const override;

    void generate(const PipelineSettings& settings);

    void setModel(ModelNode *model) {
        this->model = model;
    }

    void setVertexDataInput(VertexDataNode *vertexData) {
        this->vertexData = vertexData;
    }

    void setColorDataInput(ColorDataNode *colorData) {
        this->colorData = colorData;
    }

    void setTextureDataInput(TextureDataNode *textureData) {
        this->textureData = textureData;
    }

private:
	ModelNode *model = nullptr;
    VertexDataNode *vertexData = nullptr;
    ColorDataNode *colorData = nullptr;
    TextureDataNode *textureData = nullptr;
};
