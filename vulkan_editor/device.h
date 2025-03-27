#pragma once
#include "application.h"
#include <fstream>

class DeviceNode : public Node {
public:
	DeviceNode(int id);
	~DeviceNode() override;

	void render() const override;
	std::ofstream outFile;
	void generateDevice();
};
