#include "header.h"

std::string generateHeaders(TemplateLoader templateLoader) {
	return templateLoader.renderTemplateFile("vulkan_templates/header.txt", data);
}

std::string generateGlobalVariables(TemplateLoader templateLoader) {
	return templateLoader.renderTemplateFile("vulkan_templates/globalVariables.txt", data);
}
