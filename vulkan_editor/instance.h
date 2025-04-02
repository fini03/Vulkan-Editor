#pragma once

#include "template_loader.h"
#include "node.h"
#include <fstream>

class InstanceNode : public Node {
public:
	InstanceNode(int id);
	~InstanceNode() override;

	void render() const override;

	std::string generateInstance(TemplateLoader templateLoader);
};
