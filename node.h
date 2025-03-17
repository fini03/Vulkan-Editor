#include "imgui-node-editor/imgui_node_editor.h"
#include <iostream>
#include <vector>

struct Link {
    int id;
    ax::NodeEditor::PinId startPin;
    ax::NodeEditor::PinId endPin;
};

class Node {
public:
    virtual void render() const = 0;
    Node(int id) : id(id) {
        //ImNodes::SetNodeScreenSpacePos(id, ImGui::GetMousePos());
        //ImNodes::SnapNodeToGrid(id);
    }
    virtual ~Node() {};

protected:
	int id;
	std::vector<Link> links;
};
