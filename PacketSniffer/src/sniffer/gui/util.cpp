#include <pch.h>
#include "util.h"

#include <sniffer/script/ScriptManager.h>
#include <sniffer/script/FileScript.h>
namespace sniffer::gui
{

	void DrawScriptSelector(script::ScriptSelector* selector)
	{
		ImGui::PushID(selector);
		constexpr ImGuiTableFlags c_TableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner;

		if (ImGui::BeginTable("Select scripts", 4, c_TableFlags))
		{
			ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);

			ImGui::TableHeadersRow();

			for (auto& script : script::ScriptManager::GetScripts())
			{
				ImGui::PushID(&script);

				float rowStartY = ImGui::GetCursorScreenPos().y;
				float rowEndY = rowStartY + ImGui::GetFrameHeight();

				std::string errorMessage;
				bool valid = selector == nullptr || selector->IsScriptValid(script, &errorMessage);
				bool selected = selector->IsSelected(script);

				if (!valid && selected)
					selector->SetSelectScript(script, false);

				if (!valid)
					ImGui::BeginDisabled();

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				bool toggle = ImGui::Selectable("## Selectable", &selected, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, ImGui::GetFrameHeight()));

				ImGui::SameLine(0.0f, 0.0f);

				if (ImGui::Checkbox("## Script selected", &selected) || toggle)
					selector->SetSelectScript(script, selected);

				ImGui::TableNextColumn();

				ImGui::Text("%s", script->GetName().c_str());

				if (script->HasError())
				{
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(0.94f, 0.11f, 0.07f, 1.0f), "Error");
					if (ImGui::IsItemHovered())
						ShowHelpText(script->GetError().c_str());
				}

				ImGui::TableNextColumn();

				ImGui::Text("%s", magic_enum::enum_name(script->GetType()).data());

				if (ImGui::IsItemHovered())
				{
					switch (script->GetType())
					{
					case script::ScriptType::MemoryScript:
						ShowHelpText("This script stored in memory, and saved in cfg file.");
						break;
					case script::ScriptType::FileScript:
						ShowHelpText(fmt::format("This script stored in file: {}", reinterpret_cast<script::FileScript*>(&script)->GetFilePath()).c_str());
						break;
					default:
						break;
					}
				}

				ImGui::TableNextColumn();

				ImGui::Text("%llu", script->GetLua().GetContent().size());

				if (!valid)
				{
					ImGui::EndDisabled();

					bool isHovered = false;
					float mousePosY = ImGui::GetMousePos().y;
					if (mousePosY > rowStartY && mousePosY < rowEndY)
					{
						for (int i = 0; i < 4; i++)
						{
							if (ImGui::TableGetColumnFlags(i) & ImGuiTableColumnFlags_IsHovered)
							{
								isHovered = true;
								break;
							}
						}
					}

					if (isHovered)
						ShowHelpText(fmt::format("This script doesn't meet to conditions.\n{}", errorMessage).c_str());
				}

				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		ImGui::PopID();
	}

}