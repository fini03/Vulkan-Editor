#include "pipeline.h"
#include "model.h"
#include "header.h"
#include <iostream>
#include <vulkan/vulkan.h>
#include <inja/inja.hpp>

namespace ed = ax::NodeEditor;

std::vector<const char*> topologyOptions = { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST", "VK_PRIMITIVE_TOPOLOGY_LINE_LIST" };
std::vector<const char*> polygonModes = { "VK_POLYGON_MODE_FILL", "VK_POLYGON_MODE_LINE" };
std::vector<const char*> cullModes = { "VK_CULL_MODE_NONE", "VK_CULL_MODE_BACK_BIT" };
std::vector<const char*> frontFaceOptions = { "VK_FRONT_FACE_CLOCKWISE", "VK_FRONT_FACE_COUNTER_CLOCKWISE" };
std::vector<const char*> depthCompareOptions = { "VK_COMPARE_OP_LESS", "VK_COMPARE_OP_GREATER" };
std::vector<const char*> sampleCountOptions = { "VK_SAMPLE_COUNT_1_BIT", "VK_SAMPLE_COUNT_4_BIT" };
std::vector<const char*> colorWriteMaskNames = { "Red", "Green", "Blue", "Alpha" };
std::vector<const char*> logicOps = { "VK_LOGIC_OP_COPY", "VK_LOGIC_OP_XOR" };

PipelineNode::PipelineNode(int id) : Node(id) {
    inputPins.push_back({ ed::PinId(id * 10 + 1), PinType::VertexInput });
    inputPins.push_back({ ed::PinId(id * 10 + 2), PinType::ColorInput });
    inputPins.push_back({ ed::PinId(id * 10 + 3), PinType::TextureInput });
    inputPins.push_back({ ed::PinId(id * 10 + 4), PinType::DepthInput });
}

PipelineNode::~PipelineNode() { }

void PipelineNode::render() const {
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

std::string getColorWriteMaskString(uint32_t mask) {
    std::string result;
    bool first = true;

    if (mask & VK_COLOR_COMPONENT_R_BIT) {
        result += "VK_COLOR_COMPONENT_R_BIT";
        first = false;
    }
    if (mask & VK_COLOR_COMPONENT_G_BIT) {
        if (!first) result += " | ";
        result += "VK_COLOR_COMPONENT_G_BIT";
        first = false;
    }
    if (mask & VK_COLOR_COMPONENT_B_BIT) {
        if (!first) result += " | ";
        result += "VK_COLOR_COMPONENT_B_BIT";
        first = false;
    }
    if (mask & VK_COLOR_COMPONENT_A_BIT) {
        if (!first) result += " | ";
        result += "VK_COLOR_COMPONENT_A_BIT";
    }

    // If no bits are set, default to "0"
    if (result.empty()) {
        result = "0";
    }

    return result;
}

void PipelineNode::fillOutputData(const PipelineSettings& settings) {
	outputData["vertexShaderPath"] = settings.vertexShaderPath;
	outputData["fragmentShaderPath"] = settings.fragmentShaderPath;
    outputData["vertexEntryName"] = settings.vertexEntryName;
    outputData["fragmentEntryName"] = settings.fragmentEntryName;

    outputData["topologyOption"] = topologyOptions[settings.inputAssembly];
    outputData["primitiveRestart"] = (settings.primitiveRestart ? "VK_TRUE" : "VK_FALSE");

    outputData["depthClamp"] = (settings.depthClamp ? "VK_TRUE" : "VK_FALSE");
    outputData["rasterizerDiscard"] = (settings.rasterizerDiscard ? "VK_TRUE" : "VK_FALSE");
    outputData["polygonMode"] = polygonModes[settings.polygonMode];
    outputData["lineWidth"] = settings.lineWidth;
    outputData["cullMode"] = cullModes[settings.cullMode];
    outputData["frontFace"] = frontFaceOptions[settings.frontFace];
    outputData["depthBiasEnabled"] = (settings.depthBiasEnabled ? "VK_TRUE" : "VK_FALSE");

    outputData["sampleShading"] = (settings.sampleShading ? "VK_TRUE" : "VK_FALSE");
    outputData["rasterizationSamples"] = sampleCountOptions[settings.rasterizationSamples];

    outputData["depthTest"] = (settings.depthTest ? "VK_TRUE" : "VK_FALSE");
    outputData["depthWrite"] = (settings.depthWrite ? "VK_TRUE" : "VK_FALSE");
    outputData["depthCompareOp"] = depthCompareOptions[settings.depthCompareOp];
    outputData["depthBoundTest"] = (settings.depthBoundsTest ? "VK_TRUE" : "VK_FALSE");
    outputData["stencilTest"] = (settings.stencilTest ? "VK_TRUE" : "VK_FALSE");

    outputData["colorWriteMask"] = getColorWriteMaskString(settings.colorWriteMask);
    outputData["blendEnable"] = (settings.colorBlend ? "VK_TRUE" : "VK_FALSE");

    outputData["logicOpEnable"] = (settings.logicOpEnable ? "VK_TRUE" : "VK_FALSE");
    outputData["logicOp"] = logicOps[settings.logicOp];
    outputData["attachmentCount"] = settings.attachmentCount;
    outputData["blendConstants"] = { settings.blendConstants[0], settings.blendConstants[1], settings.blendConstants[2], settings.blendConstants[3] };
}

void PipelineNode::generate(TemplateLoader templateLoader, const PipelineSettings& settings) {
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

    outFile.open("renderer.cpp", std::ios::trunc);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    std::string result = model->generateVertexStructFilePart1(outFile);
    result += vertexData->generateVertexBindings(outFile);
    result += colorData->generateColorBindings(outFile);
    result += textureData->generateTextureBindings(outFile);
    result += model->generateVertexStructFilePart2(outFile);

    data["header"] = generateHeaders(templateLoader) + result;
    data["globalVariables"] = generateGlobalVariables(templateLoader);

    fillOutputData(settings);
    outputData["model"] = model->generateModel(templateLoader);
    data["pipeline"] = templateLoader.renderTemplateFile("vulkan_templates/pipeline.txt", outputData);

    outFile << templateLoader.renderTemplateFile("vulkan_templates/class.txt", data);
    outFile.close();
}
