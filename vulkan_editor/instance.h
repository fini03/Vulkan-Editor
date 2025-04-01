#pragma once
#include "application.h"
#include <fstream>

class InstanceNode : public Node {
public:
	InstanceNode(int id);
	~InstanceNode() override;

	void render() const override;

	void generateInstance(std::ofstream& outFile, TemplateLoader templateLoader);
};
