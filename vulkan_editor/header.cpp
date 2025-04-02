#include "header.h"

static inja::json data;

std::string generateHeaders(TemplateLoader templateLoader) {
	return templateLoader.renderTemplateFile("vulkan_templates/header.txt", data);
}

std::string generateGlobalVariables(TemplateLoader templateLoader) {
	return templateLoader.renderTemplateFile("vulkan_templates/globalVariables.txt", data);
}
