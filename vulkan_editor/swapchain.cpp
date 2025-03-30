#include "swapchain.h"
#include "logicalDevice.h"
#include <inja/inja.hpp>
using namespace inja;

static json data;
static Environment env;
static Template temp = env.parse_template("vulkan_templates/swapchain.txt");
static std::string result = env.render(temp, data);

SwapchainNode::SwapchainNode(int id) : Node(id) {}
SwapchainNode::~SwapchainNode() { }

void SwapchainNode::render() const {}

void SwapchainNode::generateSwapchain() {
    outFile.open("Vertex.h", std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << result;

    outFile.close();

    LogicalDeviceNode logicalDevice{id};
	logicalDevice.generateLogicalDevice();
}
