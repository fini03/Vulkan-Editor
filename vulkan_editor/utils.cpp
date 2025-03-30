#include "utils.h"
#include <fstream>
#include <iostream>
#include <inja/inja.hpp>
using namespace inja;

static json data;
static Environment env;
static Template temp = env.parse_template("vulkan_templates/utils.txt");
static std::string result = env.render(temp, data);

std::ofstream outFile;

void generateUtilsManagementCode() {
    outFile.open("Vertex.h", std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    outFile << result;
    outFile.close();
}
