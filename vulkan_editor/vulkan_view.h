#pragma once

#include "../imgui-node-editor/imgui_node_editor.h"
#include "../libs/tinyfiledialogs.h"
#include "pipeline.h"
#include <SDL3/SDL_video.h>
#include <memory>
#include <inja/inja.hpp>

namespace ed = ax::NodeEditor;

extern std::vector<const char*>topologyOptions;
extern std::vector<const char*> polygonModes;
extern std::vector<const char*> cullModes;
extern std::vector<const char*>frontFaceOptions;
extern std::vector<const char*> depthCompareOptions;
extern std::vector<const char*> sampleCountOptions;
extern std::vector<const char*> colorWriteMaskNames;
extern std::vector<const char*> logicOps;

class Editor {
public:
    ed::EditorContext* context = nullptr;
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<Link> links;
    int currentId = 1;

    Editor();

    void startEditor();
    void nodeEditorInitialize();

    void saveFile();

    void showModelView();

    void showPipelineView();
    void showInputAssemblySettings(PipelineSettings& settings);
    void showRasterizerSettings(PipelineSettings& settings);
    void showDepthStencilSettings(PipelineSettings& settings);
    void showMultisamplingSettings(PipelineSettings& settings);
    void showColorBlendingSettings(PipelineSettings& settings);
    void showShaderFileSelector(PipelineSettings& settings);
    void showShaderSettings(PipelineSettings& settings);
};
