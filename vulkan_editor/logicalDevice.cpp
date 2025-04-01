#include "logicalDevice.h"
#include <inja/inja.hpp>
using namespace inja;

static json data;

LogicalDeviceNode::LogicalDeviceNode(int id) : Node(id) {}
LogicalDeviceNode::~LogicalDeviceNode() { }

void LogicalDeviceNode::render() const {}

void LogicalDeviceNode::generateLogicalDevice(std::ofstream& outFile, TemplateLoader templateLoader) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << templateLoader.renderTemplateFile("vulkan_templates/logicalDevice.txt", data);

    PhysicalDeviceNode physicalDevice{id};
    physicalDevice.generatePhysicalDevice(outFile, templateLoader);
}
