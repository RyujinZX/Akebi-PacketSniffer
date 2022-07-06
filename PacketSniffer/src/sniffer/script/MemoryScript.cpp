#include "pch.h"
#include "MemoryScript.h"

namespace sniffer::script
{

	MemoryScript::MemoryScript(const std::string& name) : Script(ScriptType::MemoryScript, name)
	{ }

	MemoryScript::MemoryScript(const std::string& name, const std::string& content) : Script(ScriptType::MemoryScript, name, content)
	{ }

	MemoryScript::MemoryScript(const std::string& name, std::string&& content) : Script(ScriptType::MemoryScript, name, std::move(content))
	{ }

	MemoryScript::MemoryScript() : Script(ScriptType::MemoryScript)
	{ }

	void MemoryScript::to_json(nlohmann::json& j) const
	{
		Script::to_json(j);
		j["content"] = GetContent();
	}

	void MemoryScript::from_json(const nlohmann::json& j)
	{
		Script::from_json(j);
		SetContent(j.at("content").get<std::string>());
	}
}