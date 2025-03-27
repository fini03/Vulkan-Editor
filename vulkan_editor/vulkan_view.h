#pragma once

#include "../imgui-node-editor/imgui_node_editor.h"
#include "pipeline.h"
#include <SDL3/SDL_video.h>
#include <memory>
#include <inja/inja.hpp>

namespace ed = ax::NodeEditor;

class Editor {
public:
    ed::EditorContext* context = nullptr;
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<Link> links;
    int currentId = 1;

    Editor();

    void nodeEditorInitialize();
    void saveFile();
    void showPipelineView();
    void showModelView();
    void startEditor();
};
