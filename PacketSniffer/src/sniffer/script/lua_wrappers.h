#pragma once
#include <lua.h>
#include <sniffer/packet/Packet.h>
#include <sniffer/packet/ModifyType.h>

#include <magic_enum.hpp>
namespace sniffer::script
{
	template<typename T>
	class WrapperBase
	{
	public:
		explicit WrapperBase(const T* container, bool modifiable = true, bool temporary = false)
			: m_Container(const_cast<T*>(container)), m_Modifiable(modifiable), m_Temporary(temporary)
		{ }

		~WrapperBase()
		{
			if (m_Temporary)
				delete m_Container;
		}

		bool IsModifiable() const
		{
			return m_Modifiable;
		}

		virtual void EnableModifying()
		{
			m_Modifiable = true;
		}

		bool IsTemporary() const
		{
			return m_Temporary;
		}
		
		T* GetContainer() const
		{
			return m_Container;
		}

	protected:
		bool m_Modifiable;
		bool m_Temporary;

		T* m_Container;

		WrapperBase(WrapperBase&) = delete;
		WrapperBase& operator=(WrapperBase&) = delete;
		WrapperBase& operator=(WrapperBase&&) = delete;
	};

	template<typename T>
	class Luna
	{
	private:
																				
#define DECL_HAS_FUNCTION(NAME)													\
		template <typename T>													\
		class Has##NAME															\
		{																		\
		private:																\
			typedef char YesType[1];											\
			typedef char NoType[2];												\
																				\
			template <typename C> static YesType& test(decltype(&C::##NAME));	\
			template <typename C> static NoType& test(...);						\
																				\
																				\
		public:																	\
			enum { value = sizeof(test<T>(0)) == sizeof(YesType) };				\
		};																		

		DECL_HAS_FUNCTION(Register);

		struct _userdataType 
		{
			T* pT;
			bool needRelease;
		};

#undef DECL_HAS_FUNCTION
	public:

		using luaFunc = int (T::*)(lua_State* L);
		struct FunctionInfo
		{
			const char* name;
			luaFunc func;
		};

		static void Register(lua_State* L)
		{
			lua_newtable(L);
			int methods = lua_gettop(L);

			luaL_newmetatable(L, T::_classname);
			int metatable = lua_gettop(L);

			lua_pushvalue(L, methods);
			lua_setglobal(L, T::_classname);

			lua_pushliteral(L, "__metatable");
			lua_pushvalue(L, methods);
			lua_settable(L, metatable);

			lua_pushliteral(L, "__index");
			lua_pushvalue(L, methods);
			lua_settable(L, metatable);

			lua_pushliteral(L, "__gc");
			lua_pushcfunction(L, gc_T);
			lua_settable(L, metatable);

			lua_pushliteral(L, "__newindex");
			lua_pushcfunction(L, newindex);
			lua_settable(L, metatable);

			for (auto& funcInfo : T::_functions)
			{
				lua_pushstring(L, funcInfo.name);
				lua_pushlightuserdata(L, (void*)&funcInfo);
				lua_pushcclosure(L, thunk, 1);
				lua_settable(L, methods);
			}

			lua_pop(L, 2);

			if constexpr (HasRegister<T>::value)
				T::Register(L);
		}

		static void Push(lua_State* L, T* wrapper, bool shouldBeReleased = true)
		{
			_userdataType* ud = static_cast<_userdataType*>(lua_newuserdata(L, sizeof(_userdataType)));
			ud->pT = wrapper;
			ud->needRelease = shouldBeReleased;
			luaL_getmetatable(L, T::_classname);
			lua_setmetatable(L, -2);
		}

		static T* Check(lua_State* L, int narg) {
			_userdataType* ud =
				static_cast<_userdataType*>(luaL_checkudata(L, narg, T::_classname));
			if (!ud) luaL_typeerror(L, narg, T::_classname);
			return ud->pT;  // pointer to T object
		}

	private:

		static int luaL_typeerror(lua_State* L, int arg, const char* tname) {
			const char* msg;
			const char* typearg;  /* name for the type of the actual argument */
			if (luaL_getmetafield(L, arg, "__name") == LUA_TSTRING)
				typearg = lua_tostring(L, -1);  /* use the given type name */
			else if (lua_type(L, arg) == LUA_TLIGHTUSERDATA)
				typearg = "light userdata";  /* special name for messages */
			else
				typearg = luaL_typename(L, arg);  /* standard name */
			msg = lua_pushfstring(L, "%s expected, got %s", tname, typearg);
			return luaL_argerror(L, arg, msg);
		}

		static int gc_T(lua_State* L)
		{
			_userdataType* ud = static_cast<_userdataType*>(lua_touserdata(L, 1));
			T* obj = ud->pT;
			if (ud->needRelease)
				delete obj;
			return 0;
		}

		static int thunk(lua_State* L)
		{
			auto pT = Check(L, 1);
			lua_remove(L, 1);

			FunctionInfo *l = static_cast<FunctionInfo*>(lua_touserdata(L, lua_upvalueindex(1)));
			return (pT->*(l->func))(L);
		}

		static int tostring(lua_State* L)
		{
			char buff[64];
			_userdataType* ud = static_cast<_userdataType*>(lua_touserdata(L, 1));
			T* obj = ud->pT;
			sprintf(buff, "%p", obj);
			lua_pushfstring(L, "%s (%s)", T::_classname, buff);
			return 1;
		}

		static int newindex(lua_State* L)
		{
			return luaL_error(L, "Adding fields to API tables is restricted.");
		}
	};

#define LUNA_FUNCTION(NAME) { #NAME , &NAME }

	class PacketWrapper : public WrapperBase<packet::Packet>
	{
	private:
		int name(lua_State* L);
		int uid(lua_State* L);
		
		int mid(lua_State* L);
		int set_mid(lua_State* L);

		int timestamp(lua_State* L);
		int direction(lua_State* L);

		int is_packed(lua_State* L);

		int content(lua_State* L);
		int head(lua_State* L);

		int copy(lua_State* L);

		int enable_modifying(lua_State* L);

	public:
		using WrapperBase::WrapperBase;

		static inline const char* _classname = "Packet";

		static inline const std::array<Luna<PacketWrapper>::FunctionInfo, 11> _functions
		{
			{
				LUNA_FUNCTION(name),
				LUNA_FUNCTION(uid),
				LUNA_FUNCTION(mid),
				LUNA_FUNCTION(set_mid),
				LUNA_FUNCTION(timestamp),
				LUNA_FUNCTION(direction),
				LUNA_FUNCTION(content),
				LUNA_FUNCTION(head),
				LUNA_FUNCTION(copy),
				LUNA_FUNCTION(is_packed),
				LUNA_FUNCTION(enable_modifying)
			}
		};

		static void Register(lua_State* L);
		void EnableModifying() override;
	};

	class MessageWrapper : public WrapperBase<ProtoMessage>
	{
	private:
		int node(lua_State* L);

		int has_unknown(lua_State* L);
		int has_unsetted(lua_State* L);
		int is_packed(lua_State* L);

		int copy(lua_State* L);

	public:
		using WrapperBase::WrapperBase;

		static inline const char* _classname = "Message";

		static inline const std::array<Luna<MessageWrapper>::FunctionInfo, 5> _functions
		{
			{
				LUNA_FUNCTION(node),
				LUNA_FUNCTION(has_unknown),
				LUNA_FUNCTION(has_unsetted),
				LUNA_FUNCTION(is_packed),
				LUNA_FUNCTION(copy)
			}
		};
	};

	class NodeWrapper : public WrapperBase<ProtoNode>
	{
	private:
		int fields(lua_State* L);
		int field(lua_State* L);
		int set_field(lua_State* L);
		int has_field(lua_State* L);
		int create_field(lua_State* L);
		int add_field(lua_State* L);
		int remove_field(lua_State* L);
		int type(lua_State* L);

		int copy(lua_State* L);

	public:
		using WrapperBase::WrapperBase;

		static inline const char* _classname = "Node";

		static inline const std::array<Luna<NodeWrapper>::FunctionInfo, 9> _functions
		{
			{
				LUNA_FUNCTION(fields),
				LUNA_FUNCTION(field),
				LUNA_FUNCTION(set_field),
				LUNA_FUNCTION(has_field),
				LUNA_FUNCTION(create_field),
				LUNA_FUNCTION(add_field),
				LUNA_FUNCTION(remove_field),
				LUNA_FUNCTION(type),
				LUNA_FUNCTION(copy)
			}
		};
	};

	class FieldWrapper : public WrapperBase<ProtoField>
	{
	private:
		int value(lua_State* L);
		int set_value(lua_State* L);

		int name(lua_State* L);
		int set_name(lua_State* L);

		int number(lua_State* L);

		int is_unknown(lua_State* L);
		int is_unsetted(lua_State* L);

		int is_custom(lua_State* L);
		int make_custom(lua_State* L);

		int copy(lua_State* L);

	public:
		using WrapperBase::WrapperBase;

		static inline const char* _classname = "Field";

		static inline const std::array<Luna<FieldWrapper>::FunctionInfo, 10> _functions
		{
			{
				LUNA_FUNCTION(is_unknown),
				LUNA_FUNCTION(is_unsetted),

				LUNA_FUNCTION(is_custom),
				LUNA_FUNCTION(make_custom),

				LUNA_FUNCTION(value),
				LUNA_FUNCTION(set_value),

				LUNA_FUNCTION(name),
				LUNA_FUNCTION(set_name),

				LUNA_FUNCTION(number),

				LUNA_FUNCTION(copy)
			}
		};
	};

	class ValueWrapper : public WrapperBase<ProtoValue>
	{
	private:
		int get(lua_State* L);
		int set(lua_State* L);

		int type(lua_State* L);
		int copy(lua_State* L);
	public:
		using WrapperBase::WrapperBase;

		static inline const char* _classname = "Value";

		static inline const std::array<Luna<ValueWrapper>::FunctionInfo, 4> _functions
		{
			{
				LUNA_FUNCTION(get),
				LUNA_FUNCTION(set),

				LUNA_FUNCTION(type),
				LUNA_FUNCTION(copy)
			}
		};

		static void Register(lua_State* L);
	};

#undef LUNA_FUNCTION
}