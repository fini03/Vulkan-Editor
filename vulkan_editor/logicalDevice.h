#pragma once
#include "physicalDevice.h"
#include <fstream>

class LogicalDeviceNode : public Node {
public:
	LogicalDeviceNode(int id);
	~LogicalDeviceNode() override;

	void render() const override;

	void generateLogicalDevice(std::ofstream& outFile);
};
