#pragma once
#include "device.h"
#include <fstream>

class RenderPassNode : public Node {
public:
	RenderPassNode(int id);
	~RenderPassNode() override;

	void render() const override;
	std::ofstream outFile;
	void generateRenderpass();
};
