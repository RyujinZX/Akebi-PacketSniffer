#pragma once
#include <sniffer/script/Script.h>
namespace sniffer::script
{

	class ScriptSelector : public ISerializable
	{
	public:
		ScriptSelector();
		ScriptSelector(const ScriptSelector& other);
		ScriptSelector(ScriptSelector&& other);

		ScriptSelector& operator=(const ScriptSelector& other);
		ScriptSelector& operator=(ScriptSelector&& other);

		~ScriptSelector();

		using FilterScriptFunc = bool (const Script* script, std::string* message);
		void SetFilter(std::function<FilterScriptFunc> func);

		bool IsScriptValid(const Script* script, std::string* message = nullptr) const;

		bool IsSelected(Script* script) const;
		std::unordered_set<Script*> const& GetSelectedScripts() const;

		void SetSelectScript(Script* script, bool value);

		void to_json(nlohmann::json& j) const final;
		void from_json(const nlohmann::json& j) final;

		TEvent<Script*, bool> SelectChangedEvent;

	private:

		std::unordered_set<Script*> m_SelectedScripts;
		std::function<FilterScriptFunc> m_FilterFunction;

		void OnPreScriptRemove(int64_t id, Script* script);
	};

}


