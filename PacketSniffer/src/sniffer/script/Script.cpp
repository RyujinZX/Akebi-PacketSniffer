#include "pch.h"
#include "Script.h"

namespace sniffer::script
{
	bool Script::HasError() const
{
		return !m_LastError.empty();
	}

	std::string Script::GetError() const
{
		return m_LastError;
	}

	bool Script::operator==(const Script& other) const
	{
		return false;
	}

	void Script::to_json(nlohmann::json& j) const
	{
		j = nlohmann::json(
			{
				{ "name", m_Name },
				{ "type", config::Enum<ScriptType>(m_Type) }
			});
	}

	void Script::from_json(const nlohmann::json& j)
	{
		config::Enum<ScriptType> type;
		j.at("name").get_to(m_Name);
		j.at("type").get_to(type);
		m_Type = type;
	}

	void Script::CompileScript()
	{
		bool result = m_LuaScript.Compile();

		if (!result)
			return;
	
		m_CompiledEvent(this);

		if (HasError())
		{
			SetError(ErrorType::None, {});
			m_LuaScript.GetLastError(); // Free error if it has
		}
		
	}

	void Script::SetContent(const std::string& content)
	{
		m_LuaScript.SetContent(content);
		m_ChangedEvent(this);
		CompileScript();
	}

	void Script::SetContent(std::string&& content)
	{
		m_LuaScript.SetContent(std::move(content));
		m_ChangedEvent(this);
		CompileScript();
	}

	void Script::SetError(ErrorType type, std::string&& content)
	{
		m_ErrorType = type;
		m_LastError = std::move(content);

		if (m_ErrorType != ErrorType::None)
			m_ErrorEvent(this, m_ErrorType, m_LastError);
	}

	void Script::SetError(ErrorType type, const std::string& content)
	{
		m_ErrorType = type;
		m_LastError = content;

		if (m_ErrorType != ErrorType::None)
			m_ErrorEvent(this, m_ErrorType, m_LastError);
	}

	Script::Script(const Script& other) :
		m_Type(other.m_Type), m_Name(other.m_Name), m_LastError(other.m_LastError), m_ErrorType(other.m_ErrorType),
		m_CompiledEvent(), m_ErrorEvent(), m_ChangedEvent(), CompiledEvent(m_CompiledEvent), ErrorEvent(m_ErrorEvent), ChangedEvent(m_ChangedEvent),
		m_LuaScript(other.m_LuaScript)
	{
		Initialize();
	}

	Script::Script(Script&& other) :
		m_Type(std::move(other.m_Type)), m_Name(std::move(other.m_Name)), m_LastError(std::move(other.m_LastError)), m_ErrorType(std::move(other.m_ErrorType)),
		m_CompiledEvent(), m_ErrorEvent(), m_ChangedEvent(), CompiledEvent(m_CompiledEvent), ErrorEvent(m_ErrorEvent), ChangedEvent(m_ChangedEvent),
		m_LuaScript(std::move(other.m_LuaScript))
	{ 
		Initialize();
	}

	Script::Script(ScriptType type) : 
		m_Type(type), m_Name(), m_LastError(), m_ErrorType(ErrorType::None),
		m_CompiledEvent(), m_ErrorEvent(), m_ChangedEvent(), CompiledEvent(m_CompiledEvent), ErrorEvent(m_ErrorEvent), ChangedEvent(m_ChangedEvent),
		m_LuaScript({})
	{ 
		Initialize();
	}

	Script::Script(ScriptType type, const std::string& name) : 
		m_Type(type), m_Name(name), m_LastError(), m_ErrorType(ErrorType::None),
		m_CompiledEvent(), m_ErrorEvent(), m_ChangedEvent(), CompiledEvent(m_CompiledEvent), ErrorEvent(m_ErrorEvent), ChangedEvent(m_ChangedEvent),
		m_LuaScript({})
	{ 
		Initialize();
	}

	Script::Script(ScriptType type, const std::string& name, const std::string& content) :
		m_Type(type), m_Name(name), m_LastError(), m_ErrorType(ErrorType::None),
		m_CompiledEvent(), m_ErrorEvent(), m_ChangedEvent(), CompiledEvent(m_CompiledEvent), ErrorEvent(m_ErrorEvent), ChangedEvent(m_ChangedEvent),
		m_LuaScript(content)
	{ 
		Initialize();
		CompileScript();
	}

	Script::Script(ScriptType type, const std::string& name, std::string&& content) :
		m_Type(type), m_Name(name), m_LastError(), m_ErrorType(ErrorType::None),
		m_CompiledEvent(), m_ErrorEvent(), m_ChangedEvent(), CompiledEvent(m_CompiledEvent), ErrorEvent(m_ErrorEvent), ChangedEvent(m_ChangedEvent),
		m_LuaScript(std::move(content))
	{ 
		Initialize();
		CompileScript();
	}

	Script& Script::operator=(const Script& other)
	{
		if (&other == this)
			return *this;

		Clear();

		m_Type = other.m_Type;
		m_Name = other.m_Name;
		m_LuaScript = other.m_LastError;
		m_ErrorType = other.m_ErrorType;

		Initialize();

		return *this;
	}

	Script& Script::operator=(Script&& other)
	{
		if (&other == this)
			return *this;

		Clear();

		m_Type = std::move(other.m_Type);
		m_Name = std::move(other.m_Name);
		m_LuaScript = std::move(other.m_LastError);
		m_ErrorType = std::move(other.m_ErrorType);

		Initialize();

		return *this;
	}

	Script::~Script()
	{
		m_LuaScript.ErrorEvent -= MY_METHOD_HANDLER(Script::OnLuaScriptError);
	}

	const std::string& Script::GetName() const
	{
		return m_Name;
	}

	void Script::SetName(const std::string& name)
	{
		m_Name = name;
	}

	const std::string& Script::GetContent() const
	{
		return m_LuaScript.GetContent();
	}

	sniffer::script::ScriptType Script::GetType() const
	{
		return m_Type;
	}

	const sniffer::script::LuaScript& Script::GetLua() const
	{
		return m_LuaScript;
	}

	sniffer::script::LuaScript& Script::GetLua()
	{
		return m_LuaScript;
	}

	void Script::Initialize()
	{
		m_LuaScript.ErrorEvent += MY_METHOD_HANDLER(Script::OnLuaScriptError);
	}

	void Script::OnLuaScriptError(ErrorType type, const std::string& content)
	{
		SetError(type, content);
	}

	void Script::Clear()
	{
		m_LuaScript.ErrorEvent -= MY_METHOD_HANDLER(Script::OnLuaScriptError);
	}
}