#include "physicalDevice.h"
#include "vulkan_editor/instance.h"
#include <inja/inja.hpp>
using namespace inja;

static json data;
static Environment env;
static Template temp = env.parse_template("vulkan_templates/physicalDevice.txt");
static std::string result = env.render(temp, data);

PhysicalDeviceNode::PhysicalDeviceNode(int id) : Node(id) {}
PhysicalDeviceNode::~PhysicalDeviceNode() { }

void PhysicalDeviceNode::render() const {}

void PhysicalDeviceNode::generatePhysicalDevice() {
    outFile.open("Vertex.h", std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << result;

    outFile.close();
    InstanceNode instance{id};
    instance.generateInstance();
}
