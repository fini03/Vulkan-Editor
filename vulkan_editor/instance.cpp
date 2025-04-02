#include "instance.h"
#include "application.h"
#include <string>
#include <inja/inja.hpp>
using namespace inja;

static json data;

InstanceNode::InstanceNode(int id) : Node(id) {}
InstanceNode::~InstanceNode() { }

void InstanceNode::render() const {}

std::string InstanceNode::generateInstance(TemplateLoader templateLoader) {
	data["application"] = templateLoader.renderTemplateFile("vulkan_templates/application.txt", data);
	data["utils"] = templateLoader.renderTemplateFile("vulkan_templates/utils.txt", data);

    return templateLoader.renderTemplateFile("vulkan_templates/instance.txt", data);

    std::cout << "Code was sucessfully generated in renderer.cpp\n";
}
