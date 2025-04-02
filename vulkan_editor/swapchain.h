#pragma once
#include "logicalDevice.h"
#include <fstream>

class SwapchainNode : public Node {
public:
    SwapchainNode(int id);
    ~SwapchainNode() override;

    void render() const override;
    std::string generateSwapchain(TemplateLoader templateLoader);
};
