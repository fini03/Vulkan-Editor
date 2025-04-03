#include <fstream>
#include <iostream>
#include "template_loader.h"

static inja::json data;

std::string generateHeaders(TemplateLoader templateLoader);
std::string generateGlobalVariables(TemplateLoader templateLoader);
