#pragma once
#include <string>

#include <sniffer/Profiler.h>
#include <sniffer/script/ScriptSelector.h>
#include <fmt/format.h>
#include <cheat-base/render/gui-util.h>

#include <imgui.h>

namespace sniffer::gui
{

	void DrawScriptSelector(script::ScriptSelector* selector);
	

	template<class T>
	void DrawProfiler(const std::string& title, Profiler<T>& profiler, float width = 0)
	{
		auto& currentProfile = profiler.current();
		auto& currentProfileName = profiler.current_name();
		if (width != 0)
			ImGui::PushItemWidth(width);

		if (ImGui::BeginCombo(title.c_str(), currentProfileName.c_str())) // The second parameter is the label previewed before opening the combo.
		{
			for (auto& [name, element] : profiler.profiles())
			{
				bool is_selected = (&currentProfile == &element);
				if (ImGui::Selectable(name.c_str(), is_selected))
					profiler.switch_profile(name);
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (width != 0)
			ImGui::PopItemWidth();

		ImGui::SameLine();
		
		ImGui::PushID(title.c_str());

		bool isOpen = ImGui::IsPopupOpen("Profilers");
		if (isOpen)
			ImGui::BeginDisabled();

		if (ImGui::Button("Configure"))
			ImGui::OpenPopup("Profilers");

		if (isOpen)
			ImGui::EndDisabled();

		if (ImGui::BeginPopup("Profilers"))
		{
			static ImGuiTableFlags flags =
				ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
				| ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
				| ImGuiTableFlags_ScrollY;
			if (ImGui::BeginTable("ProfileTable", 2, flags,
				ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 10), 0.0f))
			{
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Actions");
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableHeadersRow();

				static std::string tempName = "";
				static const std::string* renameName = nullptr;
				const std::string* removeName = nullptr;
				for (auto& [name, element] : profiler.profiles())
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::PushID(&element);

					ImGui::Text(name.c_str());

					ImGui::TableNextColumn();

					if (ImGui::Button("Remove"))
						removeName = &name;
					
					ImGui::SameLine();

					if (ImGui::Button("Rename"))
					{
						tempName = name;
						renameName = &name;
						ImGui::OpenRenamePopup(tempName);
					}
					
					ImGui::PopID();
				}

				ImGui::EndTable();

				std::string newName;
				if (ImGui::DrawRenamePopup(newName))
				{
					profiler.rename_profile(*renameName, newName);
					renameName = nullptr;
				}

				if (removeName != nullptr)
				{
					profiler.remove_profile(*removeName);
					removeName = nullptr;
				}
			}

			if (ImGui::Button("Add new profile"))
			{
				size_t index = 0;
				std::string name{};
				do
				{
					index++;
					name = fmt::format("Profile #{}", index);

				} while (profiler.has_profile(name));

				profiler.add_profile(name, T());
			}

			ImGui::EndPopup();
		}

		ImGui::PopID();
	}
}