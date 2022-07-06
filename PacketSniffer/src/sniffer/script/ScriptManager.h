#pragma once

#include <sniffer/script/Script.h>
#include <sniffer/script/FileScript.h>
#include <sniffer/script/MemoryScript.h>

#include <sniffer/gui/IWindow.h>
namespace sniffer::script
{
	class ScriptManager
	{
	public:
		static void Run();

		static std::list<Script*>& GetScripts();
		static Script* GetScript(uint64_t id);
		static uint64_t GetScriptID(Script* script);
		
		static void RemoveScript(Script* script);
		static void RemoveScript(int64_t id);

		static void Update();

		template<typename T>
		static sniffer::script::Script* AddScript(T &&script)
		{
			using TClear = typename std::remove_reference<T>::type;
			auto allocated = new TClear(std::forward<T>(script));
			auto id = GenerateID();
			AddScriptInternal(allocated, id);
			return allocated;
		}

	private:
		static inline std::list<Script*> m_Scripts = {};
		static inline std::unordered_map<Script*, uint64_t> m_ScriptIDMap = {};
		static inline std::unordered_map<uint64_t, Script*> m_IDScriptMap = {};

		static inline TEvent<size_t, Script*> m_AddedScriptEvent = {};
		static inline TEvent<size_t, Script*> m_PreScriptRemoveEvent = {};

		static inline config::Field<uint64_t> f_LastIndex = {};
		static inline config::Field<nlohmann::json> f_ScriptsJson = {};
		
		static void Load();
		static void Save();

		static void AddScriptInternal(Script* script, uint64_t id);

		static void OnScriptChanged(Script* script);

		static uint64_t GenerateID();

	public:
		static inline IEvent<uint64_t, Script*>& AddedScriptEvent = m_AddedScriptEvent;
		static inline IEvent<uint64_t, Script*>& PreScriptRemoveEvent = m_PreScriptRemoveEvent;
	};

}

