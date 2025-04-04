#include "physicalDevice.h"
#include "vulkan_editor/instance.h"

PhysicalDeviceNode::PhysicalDeviceNode(int id) : Node(id) {}
PhysicalDeviceNode::~PhysicalDeviceNode() { }

void PhysicalDeviceNode::render() const {}

std::string PhysicalDeviceNode::generatePhysicalDevice(TemplateLoader templateLoader) {
	InstanceNode instance{id};
    data["instance"] = instance.generateInstance(templateLoader);

    return templateLoader.renderTemplateFile("vulkan_templates/physicalDevice.txt", data);
}
