#include "Editor.h"

#include "imgui.h"
#include "imnodes.h"

const int hardcoded_node_id = 1;

namespace Natrolite {
	void Editor() {
		ImNodes::BeginNodeEditor();

        ImNodes::BeginNode(1);

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("simple node :)");
        ImNodes::EndNodeTitleBar();

        ImNodes::BeginInputAttribute(2);
        ImGui::Text("input");
        ImNodes::EndInputAttribute();

        ImNodes::BeginOutputAttribute(3);
        ImGui::Indent(40);
        ImGui::Text("output");
        ImNodes::EndOutputAttribute();

        ImNodes::EndNode();

		ImNodes::EndNodeEditor();
	}
}