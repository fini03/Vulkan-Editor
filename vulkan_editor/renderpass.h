#pragma once
#include "swapchain.h"
#include <fstream>

class RenderPassNode : public Node {
public:
	RenderPassNode(int id);
	~RenderPassNode() override;

	void render() const override;
	void generateRenderpass(std::ofstream& outFile, TemplateLoader templateLoader);
};
