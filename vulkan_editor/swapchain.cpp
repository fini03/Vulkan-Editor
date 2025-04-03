#include "swapchain.h"
#include "logicalDevice.h"

SwapchainNode::SwapchainNode(int id) : Node(id) {}
SwapchainNode::~SwapchainNode() { }

void SwapchainNode::render() const {}

std::string SwapchainNode::generateSwapchain(TemplateLoader templateLoader) {

	LogicalDeviceNode logicalDevice{id};
	data["logicalDevice"] = logicalDevice.generateLogicalDevice(templateLoader);

	return templateLoader.renderTemplateFile("vulkan_templates/swapchain.txt", data);
}
