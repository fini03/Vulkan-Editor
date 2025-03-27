#pragma once
#include "renderpass.h"
#include <fstream>

class SwapchainNode : public Node {
public:
    SwapchainNode(int id);
    ~SwapchainNode() override;

    void render() const override;
    void generateSwapchain();

private:
    std::ofstream outFile;
};
