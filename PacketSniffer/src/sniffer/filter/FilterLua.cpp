#include "pch.h"
#include "FilterLua.h"

#include <sniffer/script/ScriptManager.h>
#include <sniffer/script/lua_wrappers.h>
namespace sniffer::filter
{
	bool FilterLua::Execute(const packet::Packet& packet)
	{
		if (m_Script == nullptr || !m_Script->GetLua().HasFunction("on_filter"))
			return true;

		bool result = true;
		auto& lua = m_Script->GetLua();

		if (!lua.CallFilterFunction("on_filter", &packet, result))
			m_ExecuteError = lua.GetLastError().second;
		
		return result;
	}

	void FilterLua::to_json(nlohmann::json& j) const
	{
		int64_t id = 0;
		if (m_Script != nullptr)
		{
			id = script::ScriptManager::GetScriptID(m_Script);
		}
		j = { { "script_id", id } };
	}

	void FilterLua::from_json(const nlohmann::json& j)
	{
		int64_t id = j["script_id"];
		if (id == 0)
			return;

		ChangeScript(script::ScriptManager::GetScript(id));
	}

	void FilterLua::OnScriptChanged(script::Script* script)
	{
		m_ExecuteError = {};
	}

	void FilterLua::OnPreScriptRemove(int64_t id, script::Script* script)
	{
		if (script != m_Script)
			return;
		
		m_Script = nullptr;
		IFilter::ChangedEvent(this);
	}

	FilterLua::FilterLua() : m_Script(nullptr), m_ExecuteError({})
	{
		script::ScriptManager::PreScriptRemoveEvent += MY_METHOD_HANDLER(FilterLua::OnPreScriptRemove);
	}

	void FilterLua::ChangeScript(script::Script* newScript)
	{
		if (m_Script != nullptr)
		{
			m_Script->ChangedEvent -= MY_METHOD_HANDLER(FilterLua::OnScriptChanged);
		}

		m_Script = newScript;
		m_Script->ChangedEvent += MY_METHOD_HANDLER(FilterLua::OnScriptChanged);
	}

	FilterLua::~FilterLua()
	{
		if (m_Script != nullptr)
			m_Script->ChangedEvent -= MY_METHOD_HANDLER(FilterLua::OnScriptChanged);

		script::ScriptManager::PreScriptRemoveEvent -= MY_METHOD_HANDLER(FilterLua::OnPreScriptRemove);
	}

	const sniffer::script::Script* FilterLua::GetScript() const
	{
		return m_Script;
	}

	sniffer::script::Script* FilterLua::GetScript()
	{
		return m_Script;
	}

	void FilterLua::SetScript(script::Script* script)
	{
		m_Script = script;
	}

}