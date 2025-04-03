#include "renderpass.h"
#include "swapchain.h"

RenderPassNode::RenderPassNode(int id) : Node(id) {}
RenderPassNode::~RenderPassNode() { }

void RenderPassNode::render() const {}

std::string RenderPassNode::generateRenderpass(TemplateLoader templateLoader) {
    SwapchainNode swapchain{id};

    data["swapchain"] = swapchain.generateSwapchain(templateLoader);
    return templateLoader.renderTemplateFile("vulkan_templates/renderpass.txt", data);
}
