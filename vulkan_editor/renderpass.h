#pragma once
#include "swapchain.h"
#include <fstream>

class RenderPassNode : public Node {
public:
	RenderPassNode(int id);
	~RenderPassNode() override;

	void render() const override;
	std::string generateRenderpass(TemplateLoader templateLoader);
};
