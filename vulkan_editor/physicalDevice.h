#pragma once
#include "instance.h"
#include <fstream>

class PhysicalDeviceNode : public Node {
public:
	PhysicalDeviceNode(int id);
	~PhysicalDeviceNode() override;

	void render() const override;
	std::ofstream outFile;
	void generatePhysicalDevice();
};
