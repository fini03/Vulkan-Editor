#include "pipeline.h"
#include "model.h"
#include "headers.h"
#include "globalVariables.h"
#include <iostream>
#include <vulkan/vulkan.h>

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

void PipelineNode::generate(const PipelineSettings& settings) {
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

    generateHeaders();
    model->generateVertexStructFilePart1();
    vertexData->generateVertexBindings();
    colorData->generateColorBindings();
    textureData->generateTextureBindings();
    model->generateVertexStructFilePart2();
    generateGlobalVariables();
    model->generateModel();

    outFile.open("Vertex.h", std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << R"(

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    void createGraphicsPipeline(VkDevice device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout, Pipeline& graphicsPipeline) {
)";

    outFile << "        auto vertShaderCode = readFile(\"" << settings.vertexShaderPath << "\");\n";
    outFile << "        auto fragShaderCode = readFile(\"" << settings.fragmentShaderPath << "\");\n";

    outFile << R"(
        VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
    )";

    outFile << "	vertShaderStageInfo.pName = \"" << settings.vertexEntryName << "\";\n";
    outFile << R"(
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
    )";

    outFile << "    fragShaderStageInfo.pName = \"" << settings.fragmentEntryName << "\";\n";

    outFile << R"(
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

    )";

    outFile << "	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};\n";
    outFile << "        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;\n";
    outFile << "        inputAssembly.topology = " << topologyOptions[settings.inputAssembly] << ";\n";
    outFile << "        inputAssembly.primitiveRestartEnable = "<< (settings.primitiveRestart ? "VK_TRUE" : "VK_FALSE") << ";\n\n";

    outFile << "        VkPipelineRasterizationStateCreateInfo rasterizer{};\n";
    outFile << "        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;\n";
    outFile << "        rasterizer.depthClampEnable = " << (settings.depthClamp ? "VK_TRUE" : "VK_FALSE") << ";\n";
    outFile << "        rasterizer.rasterizerDiscardEnable = " << (settings.rasterizerDiscard ? "VK_TRUE" : "VK_FALSE") << ";\n";
    outFile << "        rasterizer.polygonMode = " << polygonModes[settings.polygonMode] << ";\n";
    outFile << "        rasterizer.lineWidth = " << settings.lineWidth << ";\n";
    outFile << "        rasterizer.cullMode = " << cullModes[settings.cullMode] << ";\n";
    outFile << "        rasterizer.frontFace = " << frontFaceOptions[settings.frontFace] << ";\n";
    outFile << "        rasterizer.depthBiasEnable = " << (settings.depthBiasEnabled ? "VK_TRUE" : "VK_FALSE") << ";\n\n";

    outFile << "        VkPipelineMultisampleStateCreateInfo multisampling{};\n";
    outFile << "        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;\n";
    outFile << "        multisampling.sampleShadingEnable = " << (settings.sampleShading ? "VK_TRUE" : "VK_FALSE") << ";\n";
    outFile << "        multisampling.rasterizationSamples = " << sampleCountOptions[settings.rasterizationSamples] << ";\n\n";

    outFile << "        VkPipelineDepthStencilStateCreateInfo depthStencil{};\n";
    outFile << "        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;\n";
    outFile << "        depthStencil.depthTestEnable = " << (settings.depthTest ? "VK_TRUE" : "VK_FALSE") << ";\n";
    outFile << "        depthStencil.depthWriteEnable = " << (settings.depthWrite ? "VK_TRUE" : "VK_FALSE") << ";\n";
    outFile << "        depthStencil.depthCompareOp = " << depthCompareOptions[settings.depthCompareOp] << ";\n";
    outFile << "        depthStencil.depthBoundsTestEnable = " << (settings.depthBoundsTest ? "VK_TRUE" : "VK_FALSE") << ";\n";
    outFile << "        depthStencil.stencilTestEnable = "  << (settings.stencilTest ? "VK_TRUE" : "VK_FALSE") << ";\n\n";

    outFile << "        VkPipelineColorBlendAttachmentState colorBlendAttachment{};\n";
    outFile << "        colorBlendAttachment.colorWriteMask = ";

    // Convert selected colorWriteMask to a string
    bool first = true;
    if (settings.colorWriteMask & VK_COLOR_COMPONENT_R_BIT) {
        outFile << "VK_COLOR_COMPONENT_R_BIT";
        first = false;
    }
    if (settings.colorWriteMask & VK_COLOR_COMPONENT_G_BIT) {
        if (!first) outFile << " | ";
        outFile << "VK_COLOR_COMPONENT_G_BIT";
        first = false;
    }
    if (settings.colorWriteMask & VK_COLOR_COMPONENT_B_BIT) {
        if (!first) outFile << " | ";
        outFile << "VK_COLOR_COMPONENT_B_BIT";
        first = false;
    }
    if (settings.colorWriteMask & VK_COLOR_COMPONENT_A_BIT) {
        if (!first) outFile << " | ";
        outFile << "VK_COLOR_COMPONENT_A_BIT";
    }

    // If no bits are set, default to 0
    if (settings.colorWriteMask == 0) {
        outFile << "0";
    }

    outFile << ";\n";
    outFile << "        colorBlendAttachment.blendEnable = " << (settings.colorBlend ? "VK_TRUE" : "VK_FALSE") << ";\n\n";

    outFile << "        VkPipelineColorBlendStateCreateInfo colorBlending{};\n";
    outFile << "        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;\n";
    outFile << "        colorBlending.logicOpEnable = " << (settings.logicOpEnable ? "VK_TRUE" : "VK_FALSE") << ";\n";
    outFile << "        colorBlending.logicOp = " << logicOps[settings.logicOp] << ";\n";
    outFile << "        colorBlending.attachmentCount = " << settings.attachmentCount << ";\n";
    outFile << "	    colorBlending.pAttachments = &colorBlendAttachment;\n";
    outFile << "        colorBlending.blendConstants[0] = " << settings.blendConstants[0] << ";\n";
    outFile << "        colorBlending.blendConstants[1] = " << settings.blendConstants[1] << ";\n";
    outFile << "        colorBlending.blendConstants[2] = " << settings.blendConstants[2] << ";\n";
    outFile << "        colorBlending.blendConstants[3] = " << settings.blendConstants[3] << ";\n";


    outFile << R"(
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipeline.m_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = graphicsPipeline.m_pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline.m_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }
};)";

    outFile.close();

    generateMain();
}
