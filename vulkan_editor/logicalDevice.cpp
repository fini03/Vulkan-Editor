#include "logicalDevice.h"
#include <inja/inja.hpp>
using namespace inja;

static json data;
static Environment env;
static Template temp = env.parse_template("vulkan_templates/logicalDevice.txt");
static std::string result = env.render(temp, data);

LogicalDeviceNode::LogicalDeviceNode(int id) : Node(id) {}
LogicalDeviceNode::~LogicalDeviceNode() { }

void LogicalDeviceNode::render() const {}

void LogicalDeviceNode::generateLogicalDevice() {
    outFile.open("Vertex.h", std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << result;

    outFile.close();
    PhysicalDeviceNode physicalDevice{id};
    physicalDevice.generatePhysicalDevice();
}
