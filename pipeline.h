#include "model.h"
#include <iostream>

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
    float depthBiasConstantFactor = 0.0f, depthBiasClamp = 0.0f, depthBiasSlopeFactor = 0.0f;

    bool depthTest = true, depthWrite = true;
    int depthCompareOp = 0;
    bool depthBoundsTest = false;
    float depthBoundsMin = 0.0f, depthBoundsMax = 0.0f;
    bool stencilTest = false;

    bool sampleShading = false;
    int rasterizationSamples = 0;
    float minSampleShading = 0.0f;
    bool alphaToCoverage = false, alphaToOne = false;

    int colorWriteMask = 0xF;
    bool colorBlend = false;
    int srcColorBlendFactor = 0, dstColorBlendFactor = 0, colorBlendOp = 0;
    int srcAlphaBlendFactor = 0, dstAlphaBlendFactor = 0, alphaBlendOp = 0;
    bool logicOpEnable = false;
    int logicOp = 0;
    int attachmentCount = 1;
    float blendConstants[4] = { 0.0f, 1.0f, 2.0f, 3.0f };

    char vertexShaderPath[256] = "path/to/shader.vert";
    char vertexEntryName[64] = "main";
    char fragmentShaderPath[256] = "path/to/shader.frag";
    char fragmentEntryName[64] = "main";
};

class PipelineNode : public Node {
public:
    PipelineNode(int id) : Node(id) {
    	inputPins.push_back({ ed::PinId(id * 10 + 1), PinType::VertexInput });
        inputPins.push_back({ ed::PinId(id * 10 + 2), PinType::ColorInput });
        inputPins.push_back({ ed::PinId(id * 10 + 3), PinType::TextureInput });
        inputPins.push_back({ ed::PinId(id * 10 + 4), PinType::DepthInput });
    }

    ~PipelineNode() override { }

    void render() const override {
    	ed::BeginNode(this->id);
	    ImGui::Text("Pipeline");

	    // Draw Pins
       ed::BeginPin(this->id * 10 + 1, ed::PinKind::Input);
       ImGui::Text("*vertex_data");
       ed::EndPin();

       ed::BeginPin(this->id * 10 + 2, ed::PinKind::Input);
       ImGui::Text("*color_data");
       ed::EndPin();

       ed::BeginPin(this->id * 10 + 3, ed::PinKind::Input);
       ImGui::Text("*texture_data");
       ed::EndPin();

	    ed::EndNode();

		for (auto& link : links) {
       		ed::Link(link.id, link.startPin, link.endPin);
   		}
    }

    void generate() {
        if (!vertexData) {
            std::cerr << "No vertex data input set" << std::endl;
            return;
        }

        if (!colorData) {
	        std::cerr << "No color data input set" << std::endl;
	        return;
        }

        if (!textureData) {
            std::cerr << "No texture data input set" << std::endl;
            return;
        }

        model->generateVertexStructFilePart1();
        vertexData->generateVertexBindings();
        colorData->generateColorBindings();
        textureData->generateTextureBindings();
        model->generateVertexStructFilePart2();
        model->generateModel();
        std::cout << "VkCreatePipeline stuff quack quack\n\n";
    }

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
