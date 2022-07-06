#include <pch.h>
#include "ScriptManager.h"

#include <unordered_map>

#include <sniffer/script/FileScript.h>
#include <sniffer/script/MemoryScript.h>

namespace sniffer::script
{

	void ScriptManager::Run()
	{
		f_LastIndex = config::CreateField<uint64_t>("Last index", "last_index", "script::manager", false, 0);
		f_ScriptsJson = config::CreateField<nlohmann::json>("Scripts", "scripts", "script::manager",
			false, nlohmann::json());

		Load();
	}

	std::list<Script*>& ScriptManager::GetScripts()
	{
		return m_Scripts;
	}

	Script* ScriptManager::GetScript(uint64_t id)
	{
		auto it = m_IDScriptMap.find(id);
		if (it == m_IDScriptMap.end())
			return nullptr;

		return it->second;
	}

	uint64_t ScriptManager::GetScriptID(Script* script)
	{
		auto it = m_ScriptIDMap.find(script);
		if (it == m_ScriptIDMap.end())
			return 0;

		return it->second;
	}

	void ScriptManager::AddScriptInternal(Script* script, uint64_t id)
	{
		auto& scriptRef = m_Scripts.emplace_back(script);
		m_IDScriptMap[id] = script;
		m_ScriptIDMap[script] = id;
		m_AddedScriptEvent(id, script);
		script->ChangedEvent += FUNCTION_HANDLER(ScriptManager::OnScriptChanged);

		Save();
	}

	void ScriptManager::OnScriptChanged(Script* script)
	{
		Save();
	}

	void ScriptManager::RemoveScript(Script* script)
	{
		auto scriptID = GetScriptID(script);
		if (scriptID == 0)
			return;

		m_Scripts.remove(script);
		m_ScriptIDMap.erase(script);
		m_IDScriptMap.erase(scriptID);

		delete script;

		Save();
	}

	void ScriptManager::RemoveScript(int64_t id)
	{
		auto script = GetScript(id);
		if (script == nullptr)
			return;

		m_Scripts.remove(script);
		m_ScriptIDMap.erase(script);
		m_IDScriptMap.erase(id);

		delete script;

		Save();
	}

	void ScriptManager::Update()
	{
		for (auto& script : m_Scripts)
		{
			if (script->GetType() == ScriptType::FileScript)
			{
				auto fileScript = reinterpret_cast<FileScript*>(script);
				fileScript->Update();
			}
		}
	}

	static bool _isLoading = false; 
	void ScriptManager::Load()
	{
		_isLoading = true;
		for (auto& jsonObject : f_ScriptsJson.value())
		{
			auto id = jsonObject.at("id").get<uint64_t>();
			auto type = jsonObject.at("type").get<config::Enum<ScriptType>>().value();
			auto& scriptContainer = jsonObject.at("script");

			switch (type)
			{
			case sniffer::script::ScriptType::FileScript:
			{
				auto script = new FileScript();
				script->from_json(scriptContainer);
				AddScriptInternal(script, id);
				break;
			}
			case sniffer::script::ScriptType::MemoryScript:
			{
				auto script = new MemoryScript();
				script->from_json(scriptContainer);
				AddScriptInternal(script, id);
				break;
			}

			case sniffer::script::ScriptType::Unknown:
			default:
				break;
			}
		}
		_isLoading = false;
	}

	void ScriptManager::Save()
	{
		if (_isLoading)
			return;

		auto newScriptsJson = nlohmann::json::array();

		for (auto& [id, script] : m_IDScriptMap)
		{
			auto& newElement = newScriptsJson.emplace_back();
			newElement["id"] = id;
			newElement["type"] = config::Enum<ScriptType>(script->GetType());
			auto& scriptContainer = newElement["script"];

			switch (script->GetType())
			{
			case sniffer::script::ScriptType::FileScript:
			{
				auto fileScript = reinterpret_cast<FileScript*>(script);
				fileScript->to_json(scriptContainer);
				break;
			}
			case sniffer::script::ScriptType::MemoryScript:
			{
				auto memoryScript = reinterpret_cast<MemoryScript*>(script);
				memoryScript->to_json(scriptContainer);
				break;
			}

			case sniffer::script::ScriptType::Unknown:
			default:
				break;
			}
		}

		f_ScriptsJson = std::move(newScriptsJson);
	}

	uint64_t ScriptManager::GenerateID()
	{
		f_LastIndex = f_LastIndex + 1;
		return f_LastIndex;
	}

}