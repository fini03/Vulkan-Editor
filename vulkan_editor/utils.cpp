#include "utils.h"
#include <fstream>
#include <iostream>
#include <inja/inja.hpp>
using namespace inja;

static json data;

void generateUtilsManagementCode(std::ofstream& outFile, TemplateLoader templateLoader) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << templateLoader.renderTemplateFile("vulkan_templates/utils.txt", data);
}
