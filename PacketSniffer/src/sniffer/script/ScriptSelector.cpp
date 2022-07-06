#include "pch.h"
#include "ScriptSelector.h"
#include <sniffer/script/ScriptManager.h>

namespace sniffer::script
{

	ScriptSelector::ScriptSelector()
		: m_FilterFunction(nullptr)
	{
		ScriptManager::PreScriptRemoveEvent += MY_METHOD_HANDLER(ScriptSelector::OnPreScriptRemove);
	}

	ScriptSelector::ScriptSelector(const ScriptSelector& other) : 
		m_FilterFunction(other.m_FilterFunction), m_SelectedScripts(other.m_SelectedScripts)
	{
		ScriptManager::PreScriptRemoveEvent += MY_METHOD_HANDLER(ScriptSelector::OnPreScriptRemove);
	}

	ScriptSelector::ScriptSelector(ScriptSelector&& other) : 
		m_FilterFunction(std::move(other.m_FilterFunction)), m_SelectedScripts(std::move(other.m_SelectedScripts))
	{
		ScriptManager::PreScriptRemoveEvent += MY_METHOD_HANDLER(ScriptSelector::OnPreScriptRemove);
	}

	ScriptSelector& ScriptSelector::operator=(const ScriptSelector& other)
	{
		m_FilterFunction = other.m_FilterFunction;
		m_SelectedScripts = other.m_SelectedScripts;

		return *this;
	}

	ScriptSelector& ScriptSelector::operator=(ScriptSelector&& other)
	{
		m_FilterFunction = std::move(other.m_FilterFunction);
		m_SelectedScripts = std::move(other.m_SelectedScripts);

		return *this;
	}

	ScriptSelector::~ScriptSelector()
	{
		ScriptManager::PreScriptRemoveEvent -= MY_METHOD_HANDLER(ScriptSelector::OnPreScriptRemove);
	}

	void ScriptSelector::SetFilter(std::function<FilterScriptFunc> func)
	{
		m_FilterFunction = func;
	}

	bool ScriptSelector::IsSelected(Script* script) const
	{
		return m_SelectedScripts.count(script) > 0;
	}

	std::unordered_set<Script*> const& ScriptSelector::GetSelectedScripts() const
	{
		return m_SelectedScripts;
	}

	void ScriptSelector::to_json(nlohmann::json& j) const
	{
		j["selected"] = nlohmann::json::array();
		auto& selected = j["selected"];

		for (auto& script : m_SelectedScripts)
		{
			selected.push_back(ScriptManager::GetScriptID(script));
		}
	}

	void ScriptSelector::from_json(const nlohmann::json& j)
	{
		for (auto& scriptId : j["selected"])
		{
			auto script = ScriptManager::GetScript(scriptId);
			if (script == nullptr)
				continue;

			m_SelectedScripts.insert(script);
		}
	}

	void ScriptSelector::OnPreScriptRemove(int64_t id, Script* script)
	{
		SetSelectScript(script, false);
	}

	void ScriptSelector::SetSelectScript(Script* script, bool value)
	{
		if (!value)
		{
			if (IsSelected(script))
			{
				m_SelectedScripts.erase(script);
				SelectChangedEvent(script, value);
			}
			return;
		}

		if (!IsSelected(script))
		{
			m_SelectedScripts.insert(script);
			SelectChangedEvent(script, value);
		}
	}

	bool ScriptSelector::IsScriptValid(const Script* script, std::string* message /*= nullptr*/) const
	{
		if (m_FilterFunction == nullptr)
			return true;

		return m_FilterFunction(script, message);
	}
}