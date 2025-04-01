#include "physicalDevice.h"
#include "vulkan_editor/instance.h"
#include <inja/inja.hpp>
using namespace inja;

static json data;

PhysicalDeviceNode::PhysicalDeviceNode(int id) : Node(id) {}
PhysicalDeviceNode::~PhysicalDeviceNode() { }

void PhysicalDeviceNode::render() const {}

void PhysicalDeviceNode::generatePhysicalDevice(std::ofstream& outFile, TemplateLoader templateLoader) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << templateLoader.renderTemplateFile("vulkan_templates/physicalDevice.txt", data);

    InstanceNode instance{id};
    instance.generateInstance(outFile, templateLoader);
}
