#pragma once

#include <sniffer/script/Script.h>
#include <sniffer/script/FileScript.h>
#include <TextEditor.h>

namespace sniffer::gui
{
	class ScriptEditor
	{
	public:
		ScriptEditor(ScriptEditor& other);
		ScriptEditor(ScriptEditor&& other);

		ScriptEditor(script::Script* script);
		~ScriptEditor();

		void OpenEditor();
		void CloseEditor(bool skipChanged = false);
		bool IsEditorOpened() const;

		const script::Script* GetScript() const;
		void Draw();
	private:

		script::Script* m_Script;

		bool m_TextChanged;
		bool m_AskChangesApply;
		bool m_AskForUpdateFile;
		bool m_EditorOpened;
		bool m_FileHasUpdate;
		TextEditor m_Editor;

		void ApplyEditorChanges();

		void DrawApplyChangesPopup();
		void DrawUpdateFilePopup();
		void DrawEditor();

		void SetupEvents();
		void ClearEvents();

		void OnExternalChanged(script::FileScript* sender, bool& canceled);
		void OnScriptPreRemove(size_t id, script::Script* script);
	};

	class ScriptEditorManager
	{
	public:
		static void Open(script::Script* script);
		static bool IsOpened(script::Script* script);

		static void Draw();

	private:
		static inline std::unordered_map<script::Script*, ScriptEditor> s_EditorMap;

	};
}
