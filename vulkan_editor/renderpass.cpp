#include "renderpass.h"
#include "swapchain.h"
#include <inja/inja.hpp>
using namespace inja;

static json data;
static Environment env;
static Template temp = env.parse_template("vulkan_templates/renderpass.txt");
static std::string result = env.render(temp, data);

RenderPassNode::RenderPassNode(int id) : Node(id) {}
RenderPassNode::~RenderPassNode() { }

void RenderPassNode::render() const {}

void RenderPassNode::generateRenderpass(std::ofstream& outFile) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << result;

    SwapchainNode swapchain{id};
	swapchain.generateSwapchain(outFile);
}
