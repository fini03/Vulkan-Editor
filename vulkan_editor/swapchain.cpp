#include "swapchain.h"
#include "logicalDevice.h"
#include <inja/inja.hpp>
using namespace inja;

static json data;

SwapchainNode::SwapchainNode(int id) : Node(id) {}
SwapchainNode::~SwapchainNode() { }

void SwapchainNode::render() const {}

std::string SwapchainNode::generateSwapchain(TemplateLoader templateLoader) {

	LogicalDeviceNode logicalDevice{id};
	data["logicalDevice"] = logicalDevice.generateLogicalDevice(templateLoader);

	return templateLoader.renderTemplateFile("vulkan_templates/swapchain.txt", data);
}
