#include <pch.h>
#include "ScriptManagerWnd.h"

#include <sniffer/script/ScriptManager.h>
#include <sniffer/script/FileScript.h>
#include <sniffer/script/MemoryScript.h>
#include <sniffer/gui/ScriptEditor.h>

namespace sniffer::gui
{
	ScriptManagerWnd& ScriptManagerWnd::instance()
	{
		static ScriptManagerWnd instance;
		return instance;
	}

	WndInfo& ScriptManagerWnd::GetInfo()
	{
		static WndInfo info = { "Scripts", false };
		return info;
	}

	void ScriptManagerWnd::Draw()
	{
		static std::string tempName;
		static std::string tempFilename;
		static script::ScriptType tempStoreType;

		auto& scripts = script::ScriptManager::GetScripts();
		auto removeIter = scripts.end();

		ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner;
		if (ImGui::BeginTable("ScriptList", 3, flags))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
			ImGui::TableSetupColumn("Storage", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();
	
			auto& style = ImGui::GetStyle();
			float textOffset = (ImGui::GetFrameHeight() - ImGui::GetFontSize()) / 2.0f;
			for (auto it = scripts.begin(); it != scripts.end(); it++)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				auto& script = *it;
				ImGui::PushID(&script);

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffset);
				ImGui::Text("%s", script->GetName().c_str());
				if (script->HasError())
				{
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(0.94f, 0.11f, 0.07f, 1.0f), "Has error");
					if (ImGui::IsItemHovered())
						ShowHelpText(script->GetError().c_str());
				}
				ImGui::TableNextColumn();

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffset);
				ImGui::Text("%s", magic_enum::enum_name(script->GetType()).data());
				ImGui::TableNextColumn();

				bool opened = ScriptEditorManager::IsOpened(script);
				if (opened)
					ImGui::BeginDisabled();

				if (ImGui::Button("Edit"))
					ScriptEditorManager::Open(script);

				if (opened)
					ImGui::EndDisabled();

				ImGui::SameLine();

				if (ImGui::Button("Remove"))
					removeIter = it;

				ImGui::PopID();
			}

			ImGui::EndTable();
		}
		
		if (removeIter != scripts.end())
		{
			script::ScriptManager::RemoveScript(*removeIter);
		}

		if (ImGui::Button("+ Script"))
		{
			tempName = {};
			tempFilename = {};
			tempStoreType = script::ScriptType::MemoryScript;
			ImGui::OpenPopup("AddScriptPopup");
		}

		if (ImGui::BeginPopup("AddScriptPopup"))
		{
			ImGui::InputText("Name", &tempName);
			static auto _wl = std::vector<script::ScriptType>{ script::ScriptType::MemoryScript, script::ScriptType::FileScript };
			ComboEnum("Store type", &tempStoreType, &_wl);
			
			if (tempStoreType == script::ScriptType::FileScript)
				ImGui::InputText("Path to .lua file", &tempFilename);

			if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) && 
				!tempName.empty() && 
				(tempStoreType == script::ScriptType::MemoryScript || std::filesystem::exists(tempFilename)))
			{
				switch (tempStoreType)
				{
				case script::ScriptType::FileScript:
					script::ScriptManager::AddScript(script::FileScript(tempName, tempFilename));
					break;
				case script::ScriptType::MemoryScript:
					script::ScriptManager::AddScript(script::MemoryScript(tempName));
					break;
				default:
					break;
				}

				ImGui::CloseCurrentPopup();
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}


	ScriptManagerWnd::ScriptManagerWnd()
	{
	}

}