#include "renderpass.h"
#include "swapchain.h"
#include <inja/inja.hpp>
using namespace inja;

static json data;

RenderPassNode::RenderPassNode(int id) : Node(id) {}
RenderPassNode::~RenderPassNode() { }

void RenderPassNode::render() const {}

void RenderPassNode::generateRenderpass(std::ofstream& outFile, TemplateLoader templateLoader) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << templateLoader.renderTemplateFile("vulkan_templates/renderpass.txt", data);

    SwapchainNode swapchain{id};
	swapchain.generateSwapchain(outFile, templateLoader);
}
