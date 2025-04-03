#pragma once
#include "physicalDevice.h"
#include <fstream>

class LogicalDeviceNode : public Node {
public:
	LogicalDeviceNode(int id);
	~LogicalDeviceNode() override;

	void render() const override;

	std::string generateLogicalDevice(TemplateLoader templateLoader);
private:
	inja::json data;
};
