#include <pch.h>
#include "ScriptEditor.h"

#include <sniffer/script/ScriptManager.h>

namespace sniffer::gui
{

#pragma region ScriptEditor

	ScriptEditor::ScriptEditor(script::Script* script) :
		m_EditorOpened(false), m_AskChangesApply(false), m_AskForUpdateFile(false), m_TextChanged(false),
		m_FileHasUpdate(false), m_Editor(), m_Script(script)
	{
		m_Editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
		SetupEvents();
	}

	ScriptEditor::ScriptEditor(ScriptEditor& other) : 
		m_Script(other.m_Script), m_TextChanged(other.m_TextChanged), m_AskChangesApply(other.m_AskChangesApply),
		m_AskForUpdateFile(other.m_AskForUpdateFile), m_EditorOpened(other.m_EditorOpened), m_FileHasUpdate(other.m_FileHasUpdate),
		m_Editor(other.m_Editor)
	{
		SetupEvents();
	}

	ScriptEditor::ScriptEditor(ScriptEditor&& other) :
		m_Script(other.m_Script), m_TextChanged(other.m_TextChanged), m_AskChangesApply(other.m_AskChangesApply),
		m_AskForUpdateFile(other.m_AskForUpdateFile), m_EditorOpened(other.m_EditorOpened), m_FileHasUpdate(other.m_FileHasUpdate),
		m_Editor(std::move(other.m_Editor))
	{
		SetupEvents();
	}

	ScriptEditor::~ScriptEditor()
	{
		ClearEvents();
	}

	void ScriptEditor::SetupEvents()
	{
		script::ScriptManager::PreScriptRemoveEvent += MY_METHOD_HANDLER(ScriptEditor::OnScriptPreRemove);

		if (m_Script->GetType() == script::ScriptType::FileScript)
		{
			auto filescript = reinterpret_cast<script::FileScript*>(m_Script);
			filescript->ExternalChangedEvent += MY_METHOD_HANDLER(ScriptEditor::OnExternalChanged);
		}
	}

	void ScriptEditor::ClearEvents()
	{
		script::ScriptManager::PreScriptRemoveEvent -= MY_METHOD_HANDLER(ScriptEditor::OnScriptPreRemove);

		if (m_Script != nullptr && m_Script->GetType() == script::ScriptType::FileScript)
		{
			auto filescript = reinterpret_cast<script::FileScript*>(m_Script);
			filescript->ExternalChangedEvent -= MY_METHOD_HANDLER(ScriptEditor::OnExternalChanged);
		}
	}

	void ScriptEditor::OpenEditor()
	{
		m_EditorOpened = true;
		m_Editor.SetText(m_Script->GetContent());
	}

	void ScriptEditor::CloseEditor(bool skipChanged /*= false*/)
	{
		if (!skipChanged && m_TextChanged)
		{
			m_AskChangesApply = true;
			return;
		}

		m_EditorOpened = false;
		m_TextChanged = false;
	}

	bool ScriptEditor::IsEditorOpened() const
	{
		return m_EditorOpened;
	}

	void ScriptEditor::Draw()
	{
		if (m_Script == nullptr)
			return;

		if (m_Script->GetType() == script::ScriptType::FileScript)
		{
			auto filescript = reinterpret_cast<script::FileScript*>(m_Script);
			if (m_AskChangesApply && !m_EditorOpened)
			{
				m_AskForUpdateFile = false;
				m_FileHasUpdate = false;
				filescript->Load();
			}
		}

		if (m_EditorOpened)
			DrawEditor();

		if (m_AskForUpdateFile)
			DrawUpdateFilePopup();

		if (m_AskChangesApply)
			DrawApplyChangesPopup();
	}

	void ScriptEditor::DrawEditor()
	{
		if (!ImGui::Begin(fmt::format("Script: {} {} ### {}", m_Script->GetName(), m_TextChanged ? "*" : "", m_Script->GetName()).c_str(), &m_EditorOpened, ImGuiWindowFlags_MenuBar))
		{
			ImGui::End();
			return;
		}

		ImGui::SetWindowSize(ImVec2(300.0f * 1.6f, 300.0f), ImGuiCond_FirstUseEver);

		auto cpos = m_Editor.GetCursorPosition();
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save && Compile", "Ctrl-S", nullptr, m_TextChanged))
				{
					ApplyEditorChanges();
				}

				if (m_Script->GetType() == script::ScriptType::FileScript && ImGui::MenuItem("Update file", nullptr, nullptr, m_FileHasUpdate))
				{
					auto filescript = reinterpret_cast<script::FileScript*>(m_Script);
					m_AskForUpdateFile = false;
					m_FileHasUpdate = false;
					filescript->Load();		
				}

				if (ImGui::MenuItem("Quit", "Esc"))
				{
					CloseEditor();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				bool ro = m_Editor.IsReadOnly();
				if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
					m_Editor.SetReadOnly(ro);

				ImGui::Separator();

				if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && m_Editor.CanUndo()))
					m_Editor.Undo();
				if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && m_Editor.CanRedo()))
					m_Editor.Redo();

				ImGui::Separator();

				if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, m_Editor.HasSelection()))
					m_Editor.Copy();
				if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && m_Editor.HasSelection()))
					m_Editor.Cut();
				if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && m_Editor.HasSelection()))
					m_Editor.Delete();
				if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
					m_Editor.Paste();

				ImGui::Separator();

				if (ImGui::MenuItem("Select all", nullptr, nullptr))
					m_Editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(m_Editor.GetTotalLines(), 0));

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::MenuItem("Dark palette"))
					m_Editor.SetPalette(TextEditor::GetDarkPalette());
				if (ImGui::MenuItem("Light palette"))
					m_Editor.SetPalette(TextEditor::GetLightPalette());
				if (ImGui::MenuItem("Retro blue palette"))
					m_Editor.SetPalette(TextEditor::GetRetroBluePalette());
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

#define TEXT_WITH_DESC(desc, text, ...) { ImGui::Text(text, __VA_ARGS__); if (ImGui::IsItemHovered()) ShowHelpText(desc); }
#define SEPARATOR() ImGui::SameLine(0.0f, 0.0f); ImGui::Text(" | "); ImGui::SameLine(0.0f, 0.0f);

		TEXT_WITH_DESC("Line count", "%6d/%-6d %6d lines", cpos.mLine + 1, cpos.mColumn + 1, m_Editor.GetTotalLines());
		SEPARATOR();
		TEXT_WITH_DESC("Is overWrite", "%s", m_Editor.IsOverwrite() ? "Ovr" : "Ins");
		SEPARATOR();
		TEXT_WITH_DESC("Can undo", "%s", m_Editor.CanUndo() ? "*" : " ");
		if (m_Script->GetType() == script::ScriptType::FileScript)
		{
			auto filescript = reinterpret_cast<script::FileScript*>(m_Script);

			SEPARATOR();
			TEXT_WITH_DESC("Filename", "%s", filescript->GetFilePath().c_str());
		}

#undef  TEXT_WITH_DESC
#undef  SEPARATOR

		if (m_Script->HasError())
			ImGui::TextColored(ImVec4(0.94f, 0.11f, 0.07f, 1.0f), "Error: %s.\nFix the problem and recompile file to hide this message.", m_Script->GetError().c_str());

		m_Editor.Render("TextEditor");

		m_TextChanged |= m_Editor.IsTextChanged();

		ImGui::End();

		if (!m_EditorOpened)
		{
			m_EditorOpened = true;
			CloseEditor();
		}

		static Hotkey saveHotkey = Hotkey({ VK_LCONTROL, 'S' });
		if (saveHotkey.IsReleased())
			ApplyEditorChanges();

	}

	void ScriptEditor::DrawApplyChangesPopup()
	{
		if (!ImGui::IsPopupOpen("ApplyChangesPopup"))
			ImGui::OpenPopup("ApplyChangesPopup");

		if (ImGui::BeginPopup("ApplyChangesPopup"))
		{
			ImGui::Text("Content was changed.\nDo you want save & compile before close?");

			bool closed = false;
			if (ImGui::Button("Save & Close"))
			{
				ApplyEditorChanges();
				CloseEditor();
				closed = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Just close"))
			{
				CloseEditor(true);
				closed = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				closed = true;
			}

			if (closed)
			{
				m_AskChangesApply = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ScriptEditor::DrawUpdateFilePopup()
	{
		if (m_Script->GetType() != script::ScriptType::FileScript)
			return;
		
		auto filescript = reinterpret_cast<script::FileScript*>(m_Script);
		if (!ImGui::IsPopupOpen("UpdateFilePopup"))
			ImGui::OpenPopup("UpdateFilePopup");

		if (ImGui::BeginPopup("UpdateFilePopup"))
		{
			ImGui::Text("File was updated.\nDo you want apply changes?");

			bool closed = false;
			if (ImGui::Button("Apply changes"))
			{
				filescript->Load();
				m_Editor.SetText(filescript->GetContent());
				m_FileHasUpdate = false;
				closed = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Replace file"))
			{
				filescript->SetContent(m_Editor.GetText());
				m_TextChanged = false;
				m_FileHasUpdate = false;
				closed = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Skip changes"))
			{
				m_TextChanged = true;
				closed = true;
			}

			if (closed)
			{
				m_AskForUpdateFile = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ScriptEditor::ApplyEditorChanges()
	{
		auto textToSave = m_Editor.GetText();
		m_Script->SetContent(textToSave);
		m_TextChanged = false;
	}

	void ScriptEditor::OnExternalChanged(script::FileScript* sender, bool& canceled)
	{
		canceled = true;
		m_AskForUpdateFile = true;
		m_FileHasUpdate = true;
	}

	void ScriptEditor::OnScriptPreRemove(size_t id, script::Script* script)
	{
		if (script == m_Script)
			m_Script = nullptr;
	}

	const sniffer::script::Script* ScriptEditor::GetScript() const
	{
		return m_Script;
	}


#pragma endregion ScriptEditor

	void ScriptEditorManager::Open(script::Script* script)
	{
		auto it = s_EditorMap.find(script);
		if (it != s_EditorMap.end())
		{
			if (!it->second.IsEditorOpened())
				it->second.OpenEditor();

			return;
		}
		
		s_EditorMap.emplace(script, ScriptEditor(script));
		s_EditorMap.at(script).OpenEditor();
	}

	bool ScriptEditorManager::IsOpened(script::Script* script)
	{
		auto it = s_EditorMap.find(script);
		if (it == s_EditorMap.end())
			return false;

		return it->second.IsEditorOpened();
	}

	void ScriptEditorManager::Draw()
	{
		std::vector<script::Script*> toRemove;
		for (auto& [script, editor] : s_EditorMap)
		{
			if (editor.GetScript() == nullptr || !editor.IsEditorOpened())
			{
				toRemove.push_back(script);
				continue;
			}

			editor.Draw();
		}
	}
}