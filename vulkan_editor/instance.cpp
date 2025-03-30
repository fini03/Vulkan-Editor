#include "instance.h"
#include <string>
#include <inja/inja.hpp>
using namespace inja;

static json data;
static Environment env;
static Template temp = env.parse_template("vulkan_templates/instance.txt");
static std::string result = env.render(temp, data);

InstanceNode::InstanceNode(int id) : Node(id) {}
InstanceNode::~InstanceNode() { }

void InstanceNode::render() const {}

void InstanceNode::generateInstance() {
    outFile.open("Vertex.h", std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << result;

    outFile.close();
    ApplicationNode application{id};
    application.generateApplication();
}
