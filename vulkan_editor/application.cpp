#include "application.h"

#include <inja/inja.hpp>
using namespace inja;

static json data;

void generateApplication(std::ofstream& outFile, TemplateLoader templateLoader) {
    if (!outFile.is_open()) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

}
