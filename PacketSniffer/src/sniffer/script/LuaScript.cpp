#include "pch.h"

#include "LuaScript.h"
#include "LuaScript_tplt.hpp"

#include <sniffer/script/lua_wrappers.h>

#define LUA_ASSERT(expression, prefix, errorType, failExpr) if (!(expression)) { m_ErrorType = errorType; m_LastError = std::string(prefix) + lua_tostring(m_LuaState, -1); m_ErrorEvent(m_ErrorType, m_LastError); lua_pop(m_LuaState, 1); failExpr }

namespace sniffer::script
{

	LuaScript::LuaScript(const std::string& content) : m_Content(content), m_LuaState(nullptr), m_LastError({}), 
		m_ErrorEvent(), ErrorEvent(m_ErrorEvent), m_ErrorType(ErrorType::None)
	{ }

	LuaScript::LuaScript(std::string&& content) : m_Content(std::move(content)), m_LuaState(nullptr), m_LastError({}),
		m_ErrorEvent(), ErrorEvent(m_ErrorEvent), m_ErrorType(ErrorType::None)
	{ }

	LuaScript::LuaScript(const LuaScript& other)
		: m_Content(other.m_Content), m_LastError(other.m_LastError), m_PackagePath(other.m_PackagePath), m_LuaState(nullptr), 
		m_ErrorEvent(), ErrorEvent(m_ErrorEvent), m_ErrorType(other.m_ErrorType)
	{
		if (other.m_LuaState != nullptr)
			Compile();
	}

	LuaScript::LuaScript(LuaScript&& other) noexcept
		: m_Content(std::move(other.m_Content)), m_LastError(std::move(other.m_LastError)),
		m_LuaState(std::move(other.m_LuaState)), m_PackagePath(std::move(other.m_PackagePath)),
		m_ErrorEvent(), ErrorEvent(m_ErrorEvent), m_ErrorType(std::move(other.m_ErrorType))
	{
		other.m_LuaState = nullptr;
	}

	LuaScript::~LuaScript()
	{
		if (m_LuaState != nullptr)
			lua_close(m_LuaState);
	}

	LuaScript& LuaScript::operator=(const LuaScript& other)
	{
		if (m_LuaState != nullptr)
			lua_close(m_LuaState);

		m_Content = other.m_Content;
		m_LastError = other.m_LastError;
		m_ErrorType = other.m_ErrorType;

		if (other.m_LuaState != nullptr)
			Compile();

		return *this;
	}

	LuaScript& LuaScript::operator=(LuaScript&& other) noexcept
	{
		if (this == &other)
			return *this;

		if (m_LuaState != nullptr)
			lua_close(m_LuaState);

		m_Content = std::move(other.m_Content);
		m_LastError = std::move(other.m_LastError);
		m_ErrorType = std::move(other.m_ErrorType);
		m_LuaState = std::move(other.m_LuaState);

		other.m_LuaState = nullptr;

		return *this;
	}

	bool LuaScript::Compile()
	{
		m_LuaState = nullptr;

		m_LuaState = luaL_newstate();
		LUA_ASSERT(m_LuaState != nullptr, "Failed create new state.\n", ErrorType::Compile, { return false; });

		luaL_openlibs(m_LuaState);
		bool failed = luaL_loadstring(m_LuaState, m_Content.c_str());
		LUA_ASSERT(!failed, "Failed load script.\n", ErrorType::Compile, { lua_close(m_LuaState); m_LuaState = nullptr; return false; });

		Luna<PacketWrapper>::Register(m_LuaState);
		Luna<MessageWrapper>::Register(m_LuaState);
		Luna<NodeWrapper>::Register(m_LuaState);
		Luna<FieldWrapper>::Register(m_LuaState);
		Luna<ValueWrapper>::Register(m_LuaState);

		if (!m_PackagePath.empty())
		{

			lua_rawgeti(m_LuaState, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
			PushValue("package");
			lua_rawget(m_LuaState, -2);

			char pathOut[MAX_PATH] = {};
			GetModuleFileNameA(nullptr, pathOut, MAX_PATH);

			std::string currentPath = std::filesystem::path(pathOut).parent_path().string();
			std::array<const char*, 2> pathKeys = { "path", "cpath" };
			for (auto& key : pathKeys)
			{
				PushValue(key);
				lua_rawget(m_LuaState, -2);

				std::string value;
				PopValue(value);
				
				std::string::size_type pos = 0u;
				while ((pos = value.find(currentPath, pos)) != std::string::npos) {
					value.replace(pos, currentPath.length(), m_PackagePath);
					pos += m_PackagePath.length();
				}

				PushValue(key);
				PushValue(value);
				lua_rawset(m_LuaState, -3);
			}

			lua_pop(m_LuaState, 2);
		}

		failed = lua_pcall(m_LuaState, 0, LUA_MULTRET, 0);
		LUA_ASSERT(!failed, "Failed to execute global scope.\n", ErrorType::Environment, { lua_close(m_LuaState); m_LuaState = nullptr; return false; });

		return true;
	}

	bool LuaScript::IsCompiled() const
	{
		return m_LuaState != nullptr;
	}

	bool LuaScript::HasFunction(const std::string& funcName) const
	{
		if (m_LuaState == nullptr)
			return false;

		lua_getglobal(m_LuaState, funcName.c_str());
		bool isFunction = lua_isfunction(m_LuaState, -1);
		lua_pop(m_LuaState, 1);
		return isFunction;
	}

	bool LuaScript::CallFilterFunction(const std::string& functionName, const packet::Packet* packet, bool& filtered)
	{
		if (m_LuaState == nullptr)
		{
			m_LastError = "Lua state doesn't initialized.";
			return false;
		}

		if (!HasFunction(functionName))
		{
			m_LastError = fmt::format("Function with name '{}' doesn't exist.", functionName);
			return false;
		}

		lua_getglobal(m_LuaState, functionName.c_str());

		Luna<PacketWrapper>::Push(m_LuaState, new PacketWrapper(packet, false));
		LUA_ASSERT(lua_pcall(m_LuaState, 1, 1, 0) == 0, "", ErrorType::Runtime, { return false; });

		PopValue(filtered);
		return true;
	}

	bool LuaScript::CallModifyFunction(const std::string& functionName, const packet::Packet* packet, packet::Packet*& outPacket, packet::ModifyType& modifyType)
	{
		if (m_LuaState == nullptr)
		{
			m_LastError = "Lua state doesn't initialized.";
			return false;
		}

		if (!HasFunction(functionName))
		{
			m_LastError = fmt::format("Function with name '{}' doesn't exist.", functionName);
			return false;
		}

		lua_getglobal(m_LuaState, functionName.c_str());

		auto wrapper = new PacketWrapper(packet, false);
		Luna<PacketWrapper>::Push(m_LuaState, wrapper, false);
		LUA_ASSERT(lua_pcall(m_LuaState, 1, 1, 0) == 0, "", ErrorType::Runtime, { return false; });

		uint32_t modifyTypeInt;
		PopValue(modifyTypeInt);
		modifyType = static_cast<packet::ModifyType>(modifyTypeInt);

		outPacket = nullptr;
		if (wrapper->IsModifiable())
			outPacket = wrapper->GetContainer();

		delete wrapper;

		return true;
	}

	const std::string& LuaScript::GetContent() const
	{
		return m_Content;
	}

	void LuaScript::SetContent(const std::string& content)
	{
		m_Content = content;
	}

	void LuaScript::SetContent(std::string&& content)
	{
		m_Content = std::move(content);
	}

	void LuaScript::SetPackagePath(const std::string& path)
	{
		m_PackagePath = path;
	}

	void LuaScript::SetPackagePath(std::string&& path)
	{
		m_PackagePath = std::move(path);
	}

	bool LuaScript::HasError() const
	{
		return !m_LastError.empty();
	}

	std::pair<ErrorType, std::string> LuaScript::GetLastError() const
	{
		std::string lastError = m_LastError; // copy
		ErrorType lastErrorType = m_ErrorType;
		m_LastError = {};
		m_ErrorType = ErrorType::None;
		return { lastErrorType, lastError };
	}

	static int MetatableNewIndexRemoved(lua_State* L)
	{
		return luaL_error(L, "Adding fields is not allowed for read only tables.");
	}

	void LuaScript::SetTableReadOnly(int idx) const
	{	
		if (lua_getmetatable(m_LuaState, idx) == 0)
			lua_newtable(m_LuaState);

		lua_pushcfunction(m_LuaState, MetatableNewIndexRemoved);
		lua_setfield(m_LuaState, -2, "__newindex");
		lua_setmetatable(m_LuaState, -1);
	}

	void LuaScript::PushValue(uint8_t value) const
	{
		lua_pushinteger(m_LuaState, value);
	}

	void LuaScript::PushValue(uint16_t value) const
	{
		lua_pushinteger(m_LuaState, value);
	}

	void LuaScript::PushValue(uint32_t value) const
	{
		lua_pushinteger(m_LuaState, value);
	}

	void LuaScript::PushValue(uint64_t value) const
	{
		lua_pushinteger(m_LuaState, value);
	}

	void LuaScript::PushValue(int8_t value) const
	{
		lua_pushinteger(m_LuaState, value);
	}

	void LuaScript::PushValue(int16_t value) const
	{
		lua_pushinteger(m_LuaState, value);
	}

	void LuaScript::PushValue(int32_t value) const
	{
		lua_pushinteger(m_LuaState, value);
	}

	void LuaScript::PushValue(int64_t value) const
	{
		lua_pushinteger(m_LuaState, value);
	}

	void LuaScript::PushValue(const std::string& value) const
	{
		lua_pushstring(m_LuaState, value.c_str());
	}

	void LuaScript::PushValue(const char* value) const
	{
		lua_pushstring(m_LuaState, value);
	}

	void LuaScript::PushValue(bool value) const
	{
		lua_pushboolean(m_LuaState, value);
	}

	void LuaScript::PushValue(float value) const
	{
		lua_pushnumber(m_LuaState, value);
	}

	void LuaScript::PopValue(uint8_t& value) const
	{
		value = static_cast<uint8_t>(lua_tointeger(m_LuaState, -1));
		lua_pop(m_LuaState, 1);
	}

	void LuaScript::PopValue(uint16_t& value) const
	{
		value = static_cast<uint16_t>(lua_tointeger(m_LuaState, -1));
		lua_pop(m_LuaState, 1);
	}

	void LuaScript::PopValue(uint32_t& value) const
	{
		value = static_cast<uint32_t>(lua_tointeger(m_LuaState, -1));
		lua_pop(m_LuaState, 1);
	}

	void LuaScript::PopValue(uint64_t& value) const
	{
		value = static_cast<uint64_t>(lua_tointeger(m_LuaState, -1));
		lua_pop(m_LuaState, 1);
	}

	void LuaScript::PopValue(int8_t& value) const
	{
		value = static_cast<int8_t>(lua_tointeger(m_LuaState, -1));
		lua_pop(m_LuaState, 1);
	}

	void LuaScript::PopValue(int16_t& value) const
	{
		value = static_cast<int16_t>(lua_tointeger(m_LuaState, -1));
		lua_pop(m_LuaState, 1);
	}

	void LuaScript::PopValue(int32_t& value) const
	{
		value = static_cast<int32_t>(lua_tointeger(m_LuaState, -1));
		lua_pop(m_LuaState, 1);
	}

	void LuaScript::PopValue(int64_t& value) const
	{
		value = lua_tointeger(m_LuaState, -1);
		lua_pop(m_LuaState, 1);
	}

	void LuaScript::PopValue(std::string& value) const
	{
		value = lua_tostring(m_LuaState, -1);
		lua_pop(m_LuaState, 1);
	}

	void LuaScript::PopValue(bool& value) const
	{
		value = lua_toboolean(m_LuaState, -1);
		lua_pop(m_LuaState, 1);
	}

	void LuaScript::PopValue(float& value) const
	{
		value = lua_tonumber(m_LuaState, -1);
		lua_pop(m_LuaState, 1);
	}
	
	void LuaScript::PopValue(char* value) const
	{
		auto str = lua_tostring(m_LuaState, -1);
		
		std::memcpy(value, str, strlen(str));
		lua_pop(m_LuaState, 1);
	}

	void dumpstack(lua_State* L)
	{
		int top = lua_gettop(L);
		for (int i = 1; i <= top; i++) {
			printf("%d\t%s\t", i, luaL_typename(L, i));
			switch (lua_type(L, i)) {
			case LUA_TNUMBER:
				printf("%g\n", lua_tonumber(L, i));
				break;
			case LUA_TSTRING:
				printf("%s\n", lua_tostring(L, i));
				break;
			case LUA_TBOOLEAN:
				printf("%s\n", (lua_toboolean(L, i) ? "true" : "false"));
				break;
			case LUA_TNIL:
				printf("%s\n", "nil");
				break;
			default:
				printf("%p\n", lua_topointer(L, i));
				break;
			}
		}
	}
}
