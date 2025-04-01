#pragma once
#include "node.h"
#include "template_loader.h"
#include <fstream>

class ApplicationNode : public Node {
public:
	ApplicationNode(int id);
	~ApplicationNode() override;

	void render() const override;
	void generateApplication(std::ofstream& outFile, TemplateLoader templateLoader);
};
