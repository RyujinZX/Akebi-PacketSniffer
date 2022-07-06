#pragma once

#include <list>

#include "LuaScript.h"

namespace sniffer::script
{
	template<typename TValue>
	void LuaScript::PopValue(std::list<TValue>& items) const
	{
		lua_pushnil(m_LuaState);

		while (lua_next(m_LuaState, -2) != 0)
		{
			lua_pushvalue(m_LuaState, -2);
			assert(lua_isinteger(m_LuaState, -1));

			lua_pop(m_LuaState, 1);

			auto& new_item = items.emplace_back(TValue());
			PopValue(new_item);
		}

		lua_pop(m_LuaState, 1);
	}

	template<typename TValue>
	void LuaScript::PushValue(const std::list<TValue>& items) const
	{
		lua_newtable(m_LuaState);
		size_t i = 0;
		for (auto it = items.begin(); it != items.end(); i++, it++)
		{
			PushValue(*it);
			lua_rawseti(m_LuaState, -2, i + 1);
		}
		SetTableReadOnly();
	}

	template<typename... TParams>
	void LuaScript::PopValues(TParams&... args) const
	{
		(PopValue(args), ...);
	}

	template<typename TValue>
	void LuaScript::PopValue(std::vector<TValue>& items) const
	{
		lua_pushnil(m_LuaState);

		while (lua_next(m_LuaState, -2) != 0)
		{
			lua_pushvalue(m_LuaState, -2);
			assert(lua_isinteger(m_LuaState, -1));

			lua_pop(m_LuaState, 1);

			auto& new_item = items.emplace_back(TValue());
			PopValue(new_item);
		}

		lua_pop(m_LuaState, 1);
	}

	template<typename... TArgs>
	void LuaScript::PushValue(const std::tuple<TArgs...>& value) const
	{
		std::apply(PushValue, value);
	}

	template<typename TValue>
	void LuaScript::PushValue(const std::vector<TValue>& items) const
	{
		lua_newtable(m_LuaState);
		for (size_t i = 0; i < items.size(); i++)
		{
			PushValue(items[i]);
			lua_rawseti(m_LuaState, -2, i + 1);
		}
		SetTableReadOnly();
	}

	template<typename... TParams>
	void LuaScript::PushValues(const TParams&... rest) const
	{
		(PushValue(rest), ...);
	}

	template<typename TFirst, typename TSecond>
	void LuaScript::PopValue(std::pair<TFirst, TSecond>& value) const
	{
		PopValue(value.second);
		PopValue(value.first);
	}

	template<typename TFirst, typename TSecond>
	void LuaScript::PushValue(const std::pair<TFirst, TSecond>& value) const
	{
		PopValue(value.first);
		PopValue(value.second);
	}

	template<typename TValue>
	void LuaScript::PopTableValue(const std::string& key, TValue& value) const
	{
		PushValue(key);
		lua_rawget(m_LuaState, -2);
		PopValue(value);
	}

	template<typename TValue>
	void LuaScript::PushTableValue(const std::string& key, const TValue& value) const
	{
		PushValue(value);
		lua_setfield(m_LuaState, -2, key.c_str());
	}
}