#include "logicalDevice.h"
#include <inja/inja.hpp>
using namespace inja;

static json data;

LogicalDeviceNode::LogicalDeviceNode(int id) : Node(id) {}
LogicalDeviceNode::~LogicalDeviceNode() { }

void LogicalDeviceNode::render() const {}

std::string LogicalDeviceNode::generateLogicalDevice(TemplateLoader templateLoader) {
	PhysicalDeviceNode physicalDevice{id};
    data["physicalDevice"] = physicalDevice.generatePhysicalDevice(templateLoader);

    return templateLoader.renderTemplateFile("vulkan_templates/logicalDevice.txt", data);
}
