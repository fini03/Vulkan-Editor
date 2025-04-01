#include "template_loader.h"

void TemplateLoader::loadTemplateFile(const std::string fileName) {
	inja::Template temp = env.parse_template(fileName);
	templates[fileName] =  temp;

}

std::string TemplateLoader::renderTemplateFile(const std::string fileName, inja::json data) {
	inja::Template temp = templates.at(fileName);
	std::string result = env.render(temp, data);

	return result;
}
