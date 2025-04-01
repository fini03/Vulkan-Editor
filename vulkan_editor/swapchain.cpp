#include "swapchain.h"
#include "logicalDevice.h"
#include <inja/inja.hpp>
using namespace inja;

static json data;

SwapchainNode::SwapchainNode(int id) : Node(id) {}
SwapchainNode::~SwapchainNode() { }

void SwapchainNode::render() const {}

void SwapchainNode::generateSwapchain(std::ofstream& outFile, TemplateLoader templateLoader) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << templateLoader.renderTemplateFile("vulkan_templates/swapchain.txt", data);

    LogicalDeviceNode logicalDevice{id};
	logicalDevice.generateLogicalDevice(outFile, templateLoader);
}
