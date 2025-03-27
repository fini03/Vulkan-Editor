#include "../imgui-node-editor/imgui_node_editor.h"
#include <iostream>
#include <algorithm>
#include <vector>

struct Link {
    int id;
    ax::NodeEditor::PinId startPin;
    ax::NodeEditor::PinId endPin;
};

enum class PinType {
    VertexOutput,
    TextureOutput,
    VertexInput,
    TextureInput,
    DepthInput,
    DepthOutput,
    ColorInput,
    ColorOutput,
    Unknown
};

struct Pin {
    ax::NodeEditor::PinId id;
    PinType type;
};

class Node {
public:
    virtual void render() const = 0;
    Node(int id) : id(id) {
        //ImNodes::SetNodeScreenSpacePos(id, ImGui::GetMousePos());
        //ImNodes::SnapNodeToGrid(id);
    }
    virtual ~Node() {};

    // Public getters for id and links
    int getId() const { return id; }
    const std::vector<Link>& getLinks() const { return links; }

    // Public method to manage links
    void addLink(const Link& link) { links.push_back(link); }
    void removeLink(int linkId) {
        links.erase(std::remove_if(links.begin(), links.end(),
                                   [linkId](const Link& link) { return link.id == linkId; }),
                    links.end());
    }

public:
	int id;
	std::vector<Pin> inputPins;
    std::vector<Pin> outputPins;
	std::vector<Link> links;
};
