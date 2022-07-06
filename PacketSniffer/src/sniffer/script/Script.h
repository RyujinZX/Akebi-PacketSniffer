#pragma once
#include <cheat-base/ISerializable.h>
#include "LuaScript.h"
namespace sniffer::script
{
	enum class ScriptType
	{
		Unknown,
		FileScript,
		MemoryScript
	};

	class Script : public ISerializable
	{
	public:

		Script(const Script& other);
		Script(Script&& other);

		Script& operator=(const Script& other);
		Script& operator=(Script&& other);

		~Script();

		const std::string& GetName() const;
		void SetName(const std::string& name);

		const std::string& GetContent() const;
		virtual void SetContent(const std::string& content);
		virtual void SetContent(std::string&& content);

		ScriptType GetType() const;

		const LuaScript& GetLua() const;
		LuaScript& GetLua();

		bool HasError() const;
		std::string GetError() const;

		bool operator==(const Script& other) const;

		void to_json(nlohmann::json& j) const override;
		void from_json(const nlohmann::json& j) override;

		IEvent<Script*>& CompiledEvent;
		IEvent<Script*>& ChangedEvent;
		IEvent<Script*, ErrorType, const std::string&>& ErrorEvent;

	protected:
		Script(ScriptType type);
		Script(ScriptType type, const std::string& name);
		Script(ScriptType type, const std::string& name, const std::string& content);
		Script(ScriptType type, const std::string& name, std::string&& content);

		void SetError(ErrorType type, const std::string& content);
		void SetError(ErrorType type, std::string&& content);

		void CompileScript();
	private:
		
		std::string m_Name;
		LuaScript m_LuaScript;
		ScriptType m_Type;

		ErrorType m_ErrorType;
		std::string m_LastError;

		TEvent<Script*> m_CompiledEvent;
		TEvent<Script*> m_ChangedEvent;
		TEvent<Script*, ErrorType, const std::string&> m_ErrorEvent;

		void Initialize();
		void Clear();

		void OnLuaScriptError(ErrorType type, const std::string& content);
	};
}