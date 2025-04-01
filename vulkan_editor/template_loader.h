#pragma once

#include <iostream>
#include <map>
#include <string>
#include <inja/inja.hpp>

class TemplateLoader {
	public:
	inja::Environment env;
	std::map<const std::string, inja::Template> templates;

	void loadTemplateFile(const std::string fileName);
	std::string renderTemplateFile(const std::string fileName, inja::json data);
};
