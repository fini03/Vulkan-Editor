#include "header.h"

inja::json data;

void generateHeaders(std::ofstream& outFile, TemplateLoader templateLoader) {
    if (!outFile.is_open()) {
    	std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << templateLoader.renderTemplateFile("vulkan_templates/header.txt", data);
}
