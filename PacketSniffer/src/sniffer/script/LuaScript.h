#pragma once
#include <lua.h>
#include <sniffer/packet/ModifyType.h>
#include <sniffer/packet/Packet.h>

namespace sniffer::script
{
	enum class ErrorType
	{
		None, Compile, Environment, Runtime
	};

	class LuaScript
	{
	public:
		LuaScript(const std::string& content);
		LuaScript(std::string&& content);

		LuaScript(const LuaScript& other);
		LuaScript(LuaScript&& other) noexcept;
		~LuaScript();

		LuaScript& operator=(const LuaScript& other);
		LuaScript& operator=(LuaScript&& other) noexcept;

		bool Compile();
		bool IsCompiled() const;

		bool HasFunction(const std::string& funcName) const;

		// This is universal CallFunction, it's commented to decrease compilation time.
		// And instead this method will be used directly methods for each functions
		// template<typename ...Params, typename TReturn>
		// bool CallFunction(const std::string& funcName, TReturn& retValue, Params&... args) const;
		
		bool CallFilterFunction(const std::string& functionName, const packet::Packet* wrapper, bool& filtered);
		bool CallModifyFunction(const std::string& functionName, const packet::Packet* wrapper, packet::Packet*& outPacket, packet::ModifyType& modifyType);

		const std::string& GetContent() const;
		void SetContent(const std::string& content);
		void SetContent(std::string&& content);

		void SetPackagePath(const std::string& path);
		void SetPackagePath(std::string&& path);
		
		bool HasError() const;
		std::pair<ErrorType, std::string> GetLastError() const;

		IEvent<ErrorType, const std::string&>& ErrorEvent;
	
	private:

		TEvent<ErrorType, const std::string&> m_ErrorEvent;
		mutable ErrorType m_ErrorType;
		mutable std::string m_LastError;

		mutable lua_State* m_LuaState;
		std::string m_PackagePath;
		std::string m_Content;

		void SetTableReadOnly(int idx = -1) const;

		template<typename TValue>
		void PushTableValue(const std::string& key, const TValue& value) const;

		template<typename TValue>
		void PopTableValue(const std::string& key, TValue& value) const;

		void PushValue(uint8_t value) const;
		void PushValue(uint16_t value) const;
		void PushValue(uint32_t value) const;
		void PushValue(uint64_t value) const;

		void PushValue(int8_t value) const;
		void PushValue(int16_t value) const;
		void PushValue(int32_t value) const;
		void PushValue(int64_t value) const;

		void PushValue(const std::string& value) const;
		void PushValue(const char* value) const;
		void PushValue(bool value) const;
		void PushValue(float value) const;
		void PushValue(double value) const;
		void PushValue(const nlohmann::json& value) const;
		
		template<typename TFirst, typename TSecond>
		void PushValue(const std::pair<TFirst, TSecond>& value) const;

		template<typename... TArgs>
		void PushValue(const std::tuple<TArgs...>& value) const;

		template<typename TValue>
		void PushValue(const std::vector<TValue>& items) const;

		template<typename TValue>
		void PushValue(const std::list<TValue>& items) const;

		template<typename... TParams>
		void PushValues(const TParams&... args) const;

		void PopValue(uint8_t& value) const;
		void PopValue(uint16_t& value) const;
		void PopValue(uint32_t& value) const;
		void PopValue(uint64_t& value) const;

		void PopValue(int8_t& value) const;
		void PopValue(int16_t& value) const;
		void PopValue(int32_t& value) const;
		void PopValue(int64_t& value) const;

		void PopValue(std::string& value) const;
		void PopValue(char* value) const;
		void PopValue(bool& value) const;
		void PopValue(float& value) const;
		void PopValue(double& value) const;

		template<typename TFirst, typename TSecond>
		void PopValue(std::pair<TFirst, TSecond>& value) const;

		template<typename TValue>
		void PopValue(std::vector<TValue>& items) const;

		template<typename TValue>
		void PopValue(std::list<TValue>& items) const;

		template<typename... TParams>
		void PopValues(TParams&... args) const;

	};

	void dumpstack(lua_State* L);

	/* 
	// Universal implementation of CallFunction
	
	template <typename> struct is_pair : std::false_type { };
	template <typename T, typename U> struct is_pair<std::pair<T, U>> : std::true_type { };
	template <typename T, typename U> struct is_pair<const std::pair<T, U>> : std::true_type { };
	template <typename T, typename U> struct is_pair<volatile std::pair<T, U>> : std::true_type { };
	template <typename T, typename U> struct is_pair<const volatile std::pair<T, U>> : std::true_type { };

	template<typename> struct is_tuple : std::false_type {};
	template<typename... Ts> struct is_tuple<std::tuple<Ts...>> : std::true_type {};
	template<typename... Ts> struct is_tuple<const std::tuple<Ts...>> : std::true_type {};
	template<typename... Ts> struct is_tuple<volatile std::tuple<Ts...>> : std::true_type {};
	template<typename... Ts> struct is_tuple<const volatile std::tuple<Ts...>> : std::true_type {};

	template<typename, class X = bool> struct pack_size : std::integral_constant<size_t, 1> { };
	template<typename T> struct pack_size<T, typename std::enable_if_t<is_tuple<T>::value, bool>> : std::tuple_size<T> { };
	template<typename T> struct pack_size<T, typename std::enable_if_t<is_pair<T>::value, bool>> : std::tuple_size<T> { };

	template<typename... Params, typename TReturn>
	bool sniffer::LuaScript::CallFunction(const std::string& funcName, TReturn& retValue, Params&... args) const
	{
		if (m_LuaState == nullptr)
		{
			m_LastError = "Lua state doesn't initialized.";
			return false;
		}

		if (!HasFunction(funcName))
		{
			m_LastError = fmt::format("Function with name '{}' doesn't exist.", funcName);
			return false;
		}

		lua_getglobal(m_LuaState, funcName.c_str());

		constexpr size_t size = sizeof...(args);
		if constexpr (size > 0)
		{
			PushValues(args...);
		}

		constexpr size_t retSize = pack_size<TReturn>::value;
		LUA_ASSERT(lua_pcall(m_LuaState, size, retSize, 0) == 0, "", { return false; });

		lua_pop(m_LuaState, 2);
		return true;
	}*/
}


