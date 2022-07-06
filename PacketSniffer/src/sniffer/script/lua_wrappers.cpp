#include "pch.h"
#include "lua_wrappers.h"

namespace sniffer::script
{
	template<typename T>
	static void RegisterEnum(lua_State* L, const char* name)
	{
		constexpr auto entries = magic_enum::enum_entries<T>();

		lua_newtable(L);

		for (auto& [value, name] : entries)
		{
			lua_pushinteger(L, static_cast<uint32_t>(value));
			lua_setfield(L, -2, name.data());
		}

		lua_setglobal(L, name);
	}

	bool CheckParameters(lua_State* L, const std::vector<std::vector<int>>& expected)
	{
		int parameters_count = lua_gettop(L);
		if (parameters_count != expected.size())
		{
			luaL_error(L, "Incorrect size of arguments. Expected %llu but got %d.", expected.size(), parameters_count);
			return false;
		}

		int current_param = 1;
		for (auto& expected_param : expected)
		{
			int actual_type = lua_type(L, current_param);
			bool correct_param = std::any_of(expected_param.begin(), expected_param.end(),
				[actual_type](int expected_type) { return expected_type == actual_type; });

			if (!correct_param)
			{
				std::string concated_types;
				concated_types.reserve(32);

				for (auto& expected_type : expected_param)
				{
					if (!concated_types.empty())
						concated_types.append(" or ");
					concated_types.append("'").append(lua_typename(L, expected_type)).append("'");
				}

				luaL_error(L, "The parameter %d must has type %s, but got %s.",
					current_param, concated_types.c_str(), lua_typename(L, lua_type(L, current_param)));
				return false;
			}
			current_param++;
		}
		return true;
	}

	inline void RawGetField(lua_State* L, int table_idx, const char* name)
	{
		if (table_idx < 0)
			table_idx--;

		lua_pushstring(L, name);
		lua_rawget(L, table_idx);
	}

	bool CheckAndGetField(lua_State* L, int table_idx, const char* name, int expected_type)
	{
		RawGetField(L, table_idx, name);
		auto field_type = lua_type(L, -1);
		if (field_type != expected_type)
		{
			lua_settop(L, 0);
			luaL_error(L, "The table must contain the `%s` field with '%s' type, but got type '%s'.",
				name, lua_typename(L, expected_type), lua_typename(L, field_type));
			return false;
		}
		return true;
	}

#define GET_TYPENAME(IDX) lua_typename(L, lua_type(L, IDX))
#define CHECK_PARAMS(...) if (!CheckParameters(L, std::vector<std::vector<int>>( { __VA_ARGS__ } ))) return 0
#define GET_FIELD(TABLE_IDX, NAME, EXPECTED) if (!CheckAndGetField(L, TABLE_IDX, NAME, EXPECTED)) return 0

#define ONLY_MODIFIABLE() if (!m_Modifiable) { lua_settop(L, 0); luaL_error(L, "Struct must be modifiable to use this functionality."); return 0; }

#pragma region PacketWrapper

	int PacketWrapper::name(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushstring(L, m_Container->name().c_str());
		return 1;
	}

	int PacketWrapper::uid(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushinteger(L, m_Container->uid());
		return 1;
	}

	int PacketWrapper::mid(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushinteger(L, m_Container->mid());
		return 1;
	}

	int PacketWrapper::set_mid(lua_State* L)
	{
		ONLY_MODIFIABLE();
		CHECK_PARAMS({ LUA_TNUMBER });

		auto value = lua_tointeger(L, -1);
		m_Container->raw().messageID = value;
		lua_pop(L, 1);

		return 0;
	}

	int PacketWrapper::timestamp(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushinteger(L, m_Container->timestamp());
		return 1;
	}

	int PacketWrapper::direction(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushinteger(L, static_cast<uint32_t>(m_Container->direction()));
		return 1;
	}

	int PacketWrapper::is_packed(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushboolean(L, m_Container->content().has_flag(ProtoMessage::Flag::IsUnpacked));
		return 1;
	}

	int PacketWrapper::content(lua_State* L)
	{
		lua_settop(L, 0);

		auto content = new MessageWrapper(&m_Container->content(), m_Modifiable, false);
		Luna<MessageWrapper>::Push(L, content);
		return 1;
	}

	int PacketWrapper::head(lua_State* L)
	{
		lua_settop(L, 0);

		auto head = new MessageWrapper(&m_Container->head(), m_Modifiable, false);
		Luna<MessageWrapper>::Push(L, head);
		return 1;
	}

	int PacketWrapper::copy(lua_State* L)
	{
		lua_settop(L, 0);

		auto copiedValue = new PacketWrapper(new packet::Packet(*m_Container), true, true);
		Luna<PacketWrapper>::Push(L, copiedValue);
		return 1;
	}

	int PacketWrapper::enable_modifying(lua_State* L)
	{
		EnableModifying();
		return 0;
	}

	void PacketWrapper::Register(lua_State* L)
	{
		RegisterEnum<packet::ModifyType>(L, "ModifyType");
		RegisterEnum<packet::NetIODirection>(L, "NetIODirection");
	}

	void PacketWrapper::EnableModifying()
	{
		if (m_Modifiable)
			return;

		m_Container = new packet::Packet(*m_Container);
		m_Modifiable = true;
	}

#pragma endregion PacketWrapper

#pragma region MessageWrapper

	int MessageWrapper::node(lua_State* L)
	{
		lua_settop(L, 0);

		auto node = new NodeWrapper(static_cast<ProtoNode*>(m_Container), m_Modifiable, false);
		Luna<NodeWrapper>::Push(L, node);
		return 1;
	}

	int MessageWrapper::copy(lua_State* L)
	{
		lua_settop(L, 0);

		auto copiedValue = new MessageWrapper(new ProtoMessage(*m_Container), true, true);
		Luna<MessageWrapper>::Push(L, copiedValue);
		return 1;
	}

	int MessageWrapper::has_unknown(lua_State* L)
	{
		lua_settop(L, 0);

		lua_pushboolean(L, m_Container->has_flag(ProtoMessage::Flag::HasUnknown));
		return 1;
	}

	int MessageWrapper::has_unsetted(lua_State* L)
	{
		lua_settop(L, 0);

		lua_pushboolean(L, m_Container->has_flag(ProtoMessage::Flag::HasUnsetted));
		return 1;
	}

	int MessageWrapper::is_packed(lua_State* L)
	{
		lua_settop(L, 0);

		lua_pushboolean(L, m_Container->has_flag(ProtoMessage::Flag::IsUnpacked));
		return 1;
	}

#pragma endregion MessageWrapper

#pragma region NodeWrapper

	int NodeWrapper::fields(lua_State* L)
	{
		lua_settop(L, 0);

		lua_newtable(L);
		int i = 1;
		for (auto& field: m_Container->fields())
		{
			lua_pushinteger(L, i);
			auto fieldWrapper = new FieldWrapper(&field, m_Modifiable, false);
			Luna<FieldWrapper>::Push(L, fieldWrapper);
			lua_settable(L, 1);

			i++;
		}
		return 1;
	}

	int NodeWrapper::field(lua_State* L)
	{
		CHECK_PARAMS({ LUA_TNUMBER , LUA_TSTRING });
		
		ProtoField* field = nullptr;
		if (lua_isstring(L, 1))
		{
			auto name = lua_tostring(L, 1);
			if (m_Container->has(name))
				field = &m_Container->field_at(name);
		} 
		else
		{
			auto index = lua_tointeger(L, 1);
			if (m_Container->has(index))
				field = &m_Container->field_at(index);
		}

		if (field == nullptr)
			return 0;

		lua_settop(L, 0);
		auto fieldWrapper = new FieldWrapper(field, m_Modifiable, false);
		Luna<FieldWrapper>::Push(L, fieldWrapper);

		return 1;
	}

#define VALUE_OPERATION(OPERATION, ARG_NUMBER)\
	switch (lua_type(L, ARG_NUMBER))                        	   \
	{													   		   \
	case LUA_TUSERDATA:									   		   \
	{													   		   \
		auto valueWrap = Luna<ValueWrapper>::Check(L, ARG_NUMBER); \
		OPERATION(*valueWrap->GetContainer());			   		   \
		break;											   		   \
	}													   		   \
	case LUA_TNUMBER:									   		   \
	{													   		   \
		if (lua_isinteger(L, ARG_NUMBER))						   \
		{												   		   \
			OPERATION(lua_tointeger(L, ARG_NUMBER));			   \
		}												   		   \
		else											   		   \
		{												   		   \
			OPERATION(lua_tonumber(L, ARG_NUMBER));				   \
		}												   		   \
		break;											   		   \
	}													   		   \
	case LUA_TSTRING:									   		   \
		OPERATION(lua_tostring(L, ARG_NUMBER));					   \
		break;											   		   \
	case LUA_TBOOLEAN:									   		   \
		OPERATION(lua_toboolean(L, ARG_NUMBER));				   \
		break;											   		   \
	case LUA_TTABLE:									   		   \
		/* NOT IMPLEMENTED! */							   		   \
	default:											   		   \
		break;											   		   \
	}

	int NodeWrapper::set_field(lua_State* L)
	{
		ONLY_MODIFIABLE();
		CHECK_PARAMS({ LUA_TNUMBER , LUA_TSTRING }, { LUA_TUSERDATA, LUA_TNUMBER, LUA_TSTRING/*, LUA_TTABLE */, LUA_TBOOLEAN });

		if ((lua_isstring(L, 1) && !m_Container->has(lua_tostring(L, 1))) ||
			(lua_isinteger(L, 1) && !m_Container->has(lua_tointeger(L, 1))))
		{
			lua_settop(L, 0);
			lua_pushboolean(L, false);
			return 1;
		}

#define SET_FIELD(VALUE) if (lua_isstring(L, 1)) { m_Container->field_at(lua_tostring(L, 1)).set_value(VALUE); } else \
		{ m_Container->field_at(lua_tointeger(L, 1)).set_value(VALUE); }

		VALUE_OPERATION(SET_FIELD, 2)

#undef SET_FIELD

		lua_settop(L, 0);
		lua_pushboolean(L, true);
		return 1;
	}

	int NodeWrapper::has_field(lua_State* L)
	{
		CHECK_PARAMS({ LUA_TNUMBER , LUA_TSTRING });
		bool exists = false;
		if (lua_isstring(L, 1))
		{
			auto name = lua_tostring(L, 1);
			exists = m_Container->has(name);
		}
		else
		{
			auto index = lua_tointeger(L, 1);
			exists = m_Container->has(index);
		}

		lua_settop(L, 0);
		lua_pushboolean(L, exists);
		return 1;
	}

	int NodeWrapper::create_field(lua_State* L)
	{
		ONLY_MODIFIABLE();
		CHECK_PARAMS({ LUA_TNUMBER }, { LUA_TSTRING }, { LUA_TUSERDATA, LUA_TNUMBER, LUA_TSTRING /*, LUA_TTABLE */, LUA_TBOOLEAN });

		auto index = lua_tointeger(L, 1);
		auto name = lua_tostring(L, 2);

#define CREATE_FIELD(VALUE) m_Container->emplace_field(index, name, VALUE)

		VALUE_OPERATION(CREATE_FIELD, 3);

#undef CREATE_FIELD

		return 0;
	}

	int NodeWrapper::add_field(lua_State* L)
	{
		ONLY_MODIFIABLE();
		CHECK_PARAMS({ LUA_TUSERDATA });

		auto fieldWrap = Luna<FieldWrapper>::Check(L, 1);
		m_Container->emplace_field(*fieldWrap->GetContainer());

		lua_settop(L, 0);
		return 0;
	}

	int NodeWrapper::remove_field(lua_State* L)
	{
		ONLY_MODIFIABLE();
		CHECK_PARAMS({ LUA_TNUMBER, LUA_TSTRING });
		bool success = false;
		if (lua_isstring(L, 1) && m_Container->has(lua_tostring(L, 1)))
		{
			m_Container->remove_field(lua_tostring(L, 1));
			success = true;
		}
		else if (lua_isstring(L, 1) && m_Container->has(lua_tonumber(L, 1)))
		{
			m_Container->remove_field(lua_tointeger(L, 1));
			success = true;
		}

		lua_settop(L, 0);
		lua_pushboolean(L, success);
		return 1;
	}

	int NodeWrapper::type(lua_State* L)
	{
		lua_settop(L, 0);

		lua_pushstring(L, m_Container->type().c_str());
		return 1;
	}

	int NodeWrapper::copy(lua_State* L)
	{
		lua_settop(L, 0);

		auto copiedValue = new NodeWrapper(new ProtoNode(*m_Container), true, true);
		Luna<NodeWrapper>::Push(L, copiedValue);
		return 1;
	}

#pragma endregion NodeWrapper

#pragma region FieldWrapper

	int FieldWrapper::value(lua_State* L)
	{
		lua_settop(L, 0);

		auto valueWrap = new ValueWrapper(&m_Container->value(), m_Modifiable, false);
		Luna<ValueWrapper>::Push(L, valueWrap);
		return 1;
	}

	int FieldWrapper::set_value(lua_State* L)
	{
		ONLY_MODIFIABLE();
		CHECK_PARAMS({ LUA_TUSERDATA, LUA_TNUMBER, LUA_TSTRING /*, LUA_TTABLE */, LUA_TBOOLEAN });

#define SET_VALUE(VALUE) m_Container->set_value(VALUE)

		VALUE_OPERATION(SET_VALUE, 1);

#undef SET_VALUE

		lua_settop(L, 0);
		return 0;
	}

	int FieldWrapper::name(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushstring(L, m_Container->name().c_str());
		return 1;
	}

	int FieldWrapper::set_name(lua_State* L)
	{
		ONLY_MODIFIABLE();
		CHECK_PARAMS({ LUA_TSTRING });

		m_Container->set_name(lua_tostring(L, 1));
		lua_settop(L, 0);

		return 0;
	}

	int FieldWrapper::number(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushinteger(L, m_Container->number());
		return 1;
	}

	int FieldWrapper::is_unknown(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushboolean(L, m_Container->has_flag(ProtoField::Flag::Unknown));
		return 1;
	}

	int FieldWrapper::is_unsetted(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushboolean(L, m_Container->has_flag(ProtoField::Flag::Unsetted));
		return 1;
	}

	int FieldWrapper::is_custom(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushboolean(L, m_Container->has_flag(ProtoField::Flag::Custom));
		return 1;
	}

	int FieldWrapper::make_custom(lua_State* L)
	{
		ONLY_MODIFIABLE();

		lua_settop(L, 0);
		m_Container->set_flag(ProtoField::Flag::Custom);
		return 0;
	}

	int FieldWrapper::copy(lua_State* L)
	{
		lua_settop(L, 0);

		auto copiedValue = new FieldWrapper(new ProtoField(*m_Container), true, true);
		Luna<FieldWrapper>::Push(L, copiedValue);
		return 1;
	}

#pragma endregion FieldWrapper

#pragma region ValueWrapper
	
	static bool PushProtoValue(lua_State* L, const ProtoValue* value, bool modifiable)
	{
		switch (value->type())
		{
		case ProtoValue::Type::Bool:
			lua_pushboolean(L, value->to_bool());
			break;
		case ProtoValue::Type::Int32:
			lua_pushinteger(L, value->to_integer32());
			break;
		case ProtoValue::Type::Int64:
			lua_pushinteger(L, value->to_integer64());
			break;
		case ProtoValue::Type::UInt32:
			lua_pushinteger(L, value->to_unsigned32());
			break;
		case ProtoValue::Type::UInt64:
			lua_pushinteger(L, value->to_unsigned64());
			break;
		case ProtoValue::Type::Float:
			lua_pushnumber(L, value->to_float());
			break;
		case ProtoValue::Type::Double:
			lua_pushnumber(L, value->to_double());
			break;
		case ProtoValue::Type::String:
			lua_pushstring(L, value->to_string().c_str());
			break;
		case ProtoValue::Type::Enum:
			lua_pushnumber(L, value->to_enum().value());
			break;
		case ProtoValue::Type::ByteSequence:
		{
			lua_newtable(L);
			int i = 1;
			for (auto& b : value->to_bytes())
			{
				lua_pushinteger(L, i);
				lua_pushinteger(L, b);
				i++;
			}
			break;
		}
		case ProtoValue::Type::List:
		{
			lua_newtable(L);
			int i = 1;
			for (auto& value : value->to_list())
			{
				lua_pushinteger(L, i);
				Luna<ValueWrapper>::Push(L, new ValueWrapper(&value, modifiable));
				i++;
			}
			break;
		}
		case ProtoValue::Type::Map:
		{
			lua_newtable(L);
			for (auto& [key, value] : value->to_map())
			{
				PushProtoValue(L, &key, modifiable);
				Luna<ValueWrapper>::Push(L, new ValueWrapper(&value, modifiable));
			}
			break;
		}
		case ProtoValue::Type::Node:
		{
			Luna<NodeWrapper>::Push(L, new NodeWrapper(&value->to_node(), modifiable));
			break;
		}
		default:
			return false;
		}
		return true;
	}

	int ValueWrapper::get(lua_State* L)
	{
		lua_settop(L, 0);
		return PushProtoValue(L, m_Container, m_Modifiable) ? 1: 0;
	}

	int ValueWrapper::set(lua_State* L)
	{
		ONLY_MODIFIABLE();
		CHECK_PARAMS({ LUA_TUSERDATA, LUA_TNUMBER, LUA_TSTRING /*, LUA_TTABLE */, LUA_TBOOLEAN });

#define SET_VALUE(VALUE) *m_Container = ProtoValue(VALUE)

		VALUE_OPERATION(SET_VALUE, 1);

#undef SET_VALUE
		return 0;
	}

	int ValueWrapper::type(lua_State* L)
	{
		lua_settop(L, 0);
		lua_pushinteger(L, static_cast<uint32_t>(m_Container->type()));
		return 1;
	}

	int ValueWrapper::copy(lua_State* L)
	{
		lua_settop(L, 0);

		auto copiedValue = new ValueWrapper(new ProtoValue(*m_Container), true, true);
		Luna<ValueWrapper>::Push(L, copiedValue);
		return 1;
	}

	void ValueWrapper::Register(lua_State* L)
	{
		RegisterEnum<ProtoValue::Type>(L, "ValueType");
	}

#pragma endregion ValueWrapper

#undef VALUE_OPERATION

}