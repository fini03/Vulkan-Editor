#include "instance.h"
#include "application.h"
#include <string>
#include <inja/inja.hpp>
using namespace inja;

static json data;

InstanceNode::InstanceNode(int id) : Node(id) {}
InstanceNode::~InstanceNode() { }

void InstanceNode::render() const {}

void InstanceNode::generateInstance(std::ofstream& outFile, TemplateLoader templateLoader) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << templateLoader.renderTemplateFile("vulkan_templates/instance.txt", data);

    generateApplication(outFile, templateLoader);

    std::cout << "Code was sucessfully generated in renderer.cpp\n";
}
