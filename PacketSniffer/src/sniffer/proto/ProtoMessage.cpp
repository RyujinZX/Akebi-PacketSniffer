#include "pch.h"
#include "ProtoMessage.h"

namespace sniffer
{

#pragma region ProtoEnumCache

	std::unordered_map<uint32_t, std::string>* ProtoEnumCache::GetEnumValues(uint32_t enumID)
	{
		auto it = s_IDEnumMap.find(enumID);
		if (it == s_IDEnumMap.end())
			return nullptr;
		
		return &it->second->second;
	}

	std::unordered_map<uint32_t, std::string>* ProtoEnumCache::GetEnumValues(const std::string& enumName)
	{
		auto it = s_NameEnumMap.find(enumName);
		if (it == s_NameEnumMap.end())
			return nullptr;

		return &it->second->second;
	}

	const std::string* ProtoEnumCache::GetEnumValueName(uint32_t enumID, uint32_t value)
	{
		auto values = GetEnumValues(enumID);
		if (values == nullptr)
			return nullptr;

		auto it = values->find(value);
		if (it == values->end())
			return nullptr;

		return &it->second;
	}

	uint32_t ProtoEnumCache::GetID(const std::string& enumName)
	{
		auto oldValues = s_NameEnumMap.find(enumName);
		if (oldValues != s_NameEnumMap.end())
			return oldValues->second->first;
		return 0;
	}

	const std::string* ProtoEnumCache::GetName(uint32_t id)
	{
		auto it = std::find_if(s_NameEnumMap.begin(), s_NameEnumMap.end(), [&id](const auto& entry) { return entry.second->first == id; });
		if (it == s_NameEnumMap.end())
			return nullptr;

		return &it->first;
	}

#pragma endregion ProtoEnumCache

#pragma region ProtoEnum

	ProtoEnum::ProtoEnum() : m_Value(0), m_enumID(0) { }

	ProtoEnum::ProtoEnum(uint32_t enumID, uint32_t value) : m_Value(value), m_enumID(enumID)
	{ }

	uint32_t ProtoEnum::value() const
	{
		return m_Value;
	}

	const std::string& ProtoEnum::type() const
	{
		static std::string _empty{};
		auto pName = ProtoEnumCache::GetName(m_enumID);
		if (pName == nullptr)
			return _empty;

		return *pName;
	}

	const std::string& ProtoEnum::repr() const
	{
		static std::string _unknown = "UnknownEnumValue";

		auto pRepr = ProtoEnumCache::GetEnumValueName(m_enumID, m_Value);
		if (pRepr == nullptr)
			return _unknown;

		return *pRepr;
	}

	const std::unordered_map<uint32_t, std::string>& ProtoEnum::values() const
	{
		static std::unordered_map<uint32_t, std::string> _empty = {};

		auto pValues = ProtoEnumCache::GetEnumValues(m_enumID);
		if (pValues == nullptr)
			return _empty;

		return *pValues;
	}

	void ProtoEnum::to_json(nlohmann::json& j) const
	{
		j = {
			{ "type", type() },
			{ "value", m_Value },
			{ "values", values() }
		};
	}

	void ProtoEnum::from_json(const nlohmann::json& j)
	{
		j.at("value").get_to(m_Value);
		auto type = j.at("type").get<std::string>();
		if (type.empty())
			return;

		auto values = j.at("values").get<std::unordered_map<uint32_t, std::string>>();
		ProtoEnumCache::StoreEnumValues(std::move(type), std::move(values));

	}

	bool ProtoEnum::operator==(const ProtoEnum& other) const
	{
		return m_enumID == other.m_enumID && m_Value == other.m_Value;
	}

#pragma endregion ProtoEnum

#pragma region ProtoValue

	ProtoValue::ProtoValue(const ProtoValue& value)
	{
		CopyValue(value);
	}

	ProtoValue::ProtoValue(ProtoValue&& value) noexcept : m_ValueType(value.m_ValueType)
	{
		m_ValueContainer = value.m_ValueContainer;

		value.m_ValueType = Type::Unknown;
		value.m_ValueContainer = nullptr;
	}

	sniffer::ProtoValue& ProtoValue::operator=(const ProtoValue& value)
	{
		if (this == &value)
			return *this;

		Release();

		CopyValue(value);
		return *this;
	}

	sniffer::ProtoValue& ProtoValue::operator=(ProtoValue&& value) noexcept
	{
		if (this == &value)
			return *this;

		Release();

		m_ValueType = value.m_ValueType;
		m_ValueContainer = value.m_ValueContainer;

		value.m_ValueType = Type::Unknown;
		value.m_ValueContainer = nullptr;
		return *this;
	}

	void ProtoValue::CopyValue(const ProtoValue& other)
	{
		switch (other.m_ValueType)
		{
#define COPY_POINTER_VALUE(VALUE_TYPE) 																													 \
		case VALUE_TYPE:																																 \
			m_ValueContainer = new proto_to_cpp_type<VALUE_TYPE>::type(*reinterpret_cast<proto_to_cpp_type<VALUE_TYPE>::type*>(other.m_ValueContainer)); \
			break

			COPY_POINTER_VALUE(Type::Enum);
			COPY_POINTER_VALUE(Type::ByteSequence);
			COPY_POINTER_VALUE(Type::String);
			COPY_POINTER_VALUE(Type::List);
			COPY_POINTER_VALUE(Type::Map);
			COPY_POINTER_VALUE(Type::Node);

#undef COPY_POINTER_VALUE
		default:
			m_ValueContainer = other.m_ValueContainer;
			break;
		}

		m_ValueType = other.m_ValueType;
	}

	void ProtoValue::Release()
	{
		switch (m_ValueType)
		{
#define RELEASE_POINTER_VALUE(VALUE_TYPE) 														 \
		case VALUE_TYPE:																	 \
			delete reinterpret_cast<proto_to_cpp_type<VALUE_TYPE>::type*>(m_ValueContainer); \
			break

			RELEASE_POINTER_VALUE(Type::Enum);
			RELEASE_POINTER_VALUE(Type::ByteSequence);
			RELEASE_POINTER_VALUE(Type::String);
			RELEASE_POINTER_VALUE(Type::List);
			RELEASE_POINTER_VALUE(Type::Map);
			RELEASE_POINTER_VALUE(Type::Node);

#undef RELEASE_POINTER_VALUE
		default:
			break;
		}
	}

	ProtoValue::~ProtoValue()
	{
		Release();
	}

	ProtoValue::ProtoValue() : m_ValueType(Type::Unknown), m_ValueContainer(nullptr) {}

	ProtoValue::ProtoValue(const char* value) : m_ValueType(Type::String), m_ValueContainer(new std::string(value)) {}

#define INIT_SIMPLE_VALUE32(TYPE, PROTO_TYPE) \
	ProtoValue::ProtoValue(TYPE value) : m_ValueType(PROTO_TYPE), \
		m_ValueContainer(reinterpret_cast<void*>(static_cast<int64_t>(*reinterpret_cast<int32_t*>(&value)))) {}

#define INIT_SIMPLE_VALUE64(TYPE, PROTO_TYPE) \
	ProtoValue::ProtoValue(TYPE value) : m_ValueType(PROTO_TYPE), \
		m_ValueContainer(*reinterpret_cast<void**>(&value)) {}

	INIT_SIMPLE_VALUE32(bool, Type::Bool);
	
	INIT_SIMPLE_VALUE32(int32_t, Type::Int32);
	INIT_SIMPLE_VALUE64(int64_t, Type::Int64);

	INIT_SIMPLE_VALUE32(uint32_t, Type::UInt32);
	INIT_SIMPLE_VALUE64(uint64_t, Type::UInt64);
	INIT_SIMPLE_VALUE32(float, Type::Float);
	INIT_SIMPLE_VALUE64(double, Type::Double);

#undef INIT_SIMPLE_VALUE

#define INIT_POINTER_VALUE(TYPE, PROTO_TYPE) \
	ProtoValue::ProtoValue(const TYPE& value) : m_ValueType(PROTO_TYPE), m_ValueContainer(new TYPE(value)) {} \
	ProtoValue::ProtoValue(TYPE&& value) : m_ValueType(PROTO_TYPE), m_ValueContainer(new TYPE(std::move(value))) {}

	INIT_POINTER_VALUE(ProtoEnum, Type::Enum);
	INIT_POINTER_VALUE(ProtoValue::bseq_type, Type::ByteSequence);
	INIT_POINTER_VALUE(std::string, Type::String);
	INIT_POINTER_VALUE(ProtoValue::list_type, Type::List);
	INIT_POINTER_VALUE(ProtoValue::map_type, Type::Map);
	INIT_POINTER_VALUE(ProtoNode, Type::Node);

#undef INIT_POINTER_VALUE


	sniffer::ProtoValue::Type ProtoValue::type() const
	{
		return m_ValueType;
	}

	void ProtoValue::to_json(nlohmann::json& j) const
	{
		j = {
			{ "type", static_cast<uint32_t>(m_ValueType) }
		};

		auto& container = j["container"];
		switch (m_ValueType)
		{
#define VALUE_TO_JSON(VALUE_TYPE) \
		case VALUE_TYPE: \
			container = get<proto_to_cpp_type<VALUE_TYPE>::type>(); \
			break
		
			VALUE_TO_JSON(Type::Bool);
			VALUE_TO_JSON(Type::Int32);
			VALUE_TO_JSON(Type::Int64);
			VALUE_TO_JSON(Type::UInt32);
			VALUE_TO_JSON(Type::UInt64);
			VALUE_TO_JSON(Type::Float);
			VALUE_TO_JSON(Type::Double);
			VALUE_TO_JSON(Type::String);
			VALUE_TO_JSON(Type::ByteSequence);

#undef VALUE_TO_JSON

		case Type::Enum:
			to_enum().to_json(container);
			break;
		case Type::List:
		{
			auto& list = to_list();
			container = nlohmann::json::array();
			for (auto& element : list)
			{
				auto& element_container = container.emplace_back();
				element.to_json(element_container);
			}
		}
		break;
		case Type::Map:
		{
			auto& map = to_map();
			container = nlohmann::json::object();
			for (auto& [key, value] : map)
			{
				auto& element_container = container[fmt::format("{}||{}", static_cast<uint32_t>(key.type()), key.convert_to_string())];
				value.to_json(element_container);
			}
		}
		break;
		case Type::Node:
			to_node().to_json(container);
			break;
		default:
			break;
		}
	}

	void ProtoValue::from_json(const nlohmann::json& j)
	{
		uint32_t int_type;
		j.at("type").get_to(int_type);
		m_ValueType = static_cast<Type>(int_type);

		auto& container = j["container"];
		switch (m_ValueType)
		{
#define VALUE_FROM_JSON(VALUE_TYPE) \
		case VALUE_TYPE: \
		{ \
			auto cont_value = container.get<proto_to_cpp_type<VALUE_TYPE>::type>(); \
			m_ValueContainer = *reinterpret_cast<void**>(&cont_value); \
		} \
		break

			VALUE_FROM_JSON(Type::Bool);
			VALUE_FROM_JSON(Type::Int32);
			VALUE_FROM_JSON(Type::Int64);
			VALUE_FROM_JSON(Type::UInt32);
			VALUE_FROM_JSON(Type::UInt64);
			VALUE_FROM_JSON(Type::Float);
			VALUE_FROM_JSON(Type::Double);

#undef VALUE_TO_JSON
		case Type::Enum:
		{
			auto enum_value = new ProtoEnum();
			enum_value->from_json(container);
			m_ValueContainer = enum_value;
			break;
		}
		case Type::ByteSequence:
			m_ValueContainer = new bseq_type(container.get<bseq_type>());
			break;
		case Type::String:
			m_ValueContainer = new std::string(container.get<std::string>());
			break;
		case Type::List:
		{
			auto list = new list_type();
			for (auto& element : container)
			{
				auto& proto_value = list->emplace_back();
				proto_value.from_json(element);
			}
			m_ValueContainer = list;
			break;
		}
		case Type::Map:
		{
			auto map = new map_type();
			for (auto& [key, value] : container.items())
			{
				auto key_parts = util::StringSplit("||", key);
				if (key_parts.size() < 2)
					continue;

				auto key_type = static_cast<Type>(std::stoi(key_parts[0]));
				auto& key_content = key_parts[1];

				ProtoValue key_proto;
				ConvertStringToProtoValue(key_content, key_type, key_proto);

				ProtoValue value_proto;
				value_proto.from_json(value);

				map->insert(std::make_pair(std::move(key_proto), std::move(value_proto)));
			}
			m_ValueContainer = map;
			break;
		}
		case Type::Node:
			m_ValueContainer = new ProtoNode();
			to_node().from_json(container);
			break;
		default:
			break;
		}
	}

	std::string ProtoValue::convert_to_string() const
	{
		std::string key_str;
		switch (type())
		{
#define TYPE_TO_STRING(PROTO_TYPE) \
		case PROTO_TYPE: \
				key_str = std::to_string(get<proto_to_cpp_type<PROTO_TYPE>::type>());\
				break

			TYPE_TO_STRING(ProtoValue::Type::Int32);
			TYPE_TO_STRING(ProtoValue::Type::Int64);
			TYPE_TO_STRING(ProtoValue::Type::UInt32);
			TYPE_TO_STRING(ProtoValue::Type::UInt64);
			TYPE_TO_STRING(ProtoValue::Type::Float);
			TYPE_TO_STRING(ProtoValue::Type::Double);

#undef TYPE_TO_STRING

		case ProtoValue::Type::String:
			key_str = to_string();
			break;

		default:
			key_str = "<unsupported_type>";
		}
		return key_str;
	}

	bool ProtoValue::operator==(const ProtoValue& other) const
	{
		if (type() != other.type())
			return false;

		switch (type())
		{
		case sniffer::ProtoValue::Type::Bool:
			return get<bool>() == other.get<bool>();
		case sniffer::ProtoValue::Type::Int32:
			return get<int32_t>() == other.get<int32_t>();
		case sniffer::ProtoValue::Type::Int64:
			return get<int64_t>() == other.get<int64_t>();
		case sniffer::ProtoValue::Type::UInt32:
			return get<uint32_t>() == other.get<uint32_t>();
		case sniffer::ProtoValue::Type::UInt64:
			return get<uint64_t>() == other.get<uint64_t>();
		case sniffer::ProtoValue::Type::Float:
			return get<float>() == other.get<float>();
		case sniffer::ProtoValue::Type::Double:
			return get<double>() == other.get<double>();
		case sniffer::ProtoValue::Type::Enum:
			return get<ProtoEnum>() == other.get<ProtoEnum>();
		case sniffer::ProtoValue::Type::ByteSequence:
			return get<bseq_type>() == other.get<bseq_type>();
		case sniffer::ProtoValue::Type::String:
			return get<std::string>() == other.get<std::string>();

		default:
			throw std::invalid_argument(fmt::format("Proto value with type `{}` is not comparable.", magic_enum::enum_name(type())));
			return false;
		}
	}

#pragma endregion ProtoValue

	bool ConvertStringToProtoValue(const std::string& content, ProtoValue::Type type, ProtoValue& value)
	{
		switch (type)
		{
		case sniffer::ProtoValue::Type::Int32:
			value = ProtoValue(static_cast<int32_t>(stoi(content)));
			break;
		case sniffer::ProtoValue::Type::Int64:
			value = ProtoValue(static_cast<int64_t>(stoll(content)));
			break;
		case sniffer::ProtoValue::Type::UInt32:
			value = ProtoValue(static_cast<uint32_t>(stoul(content)));
			break;
		case sniffer::ProtoValue::Type::UInt64:
			value = ProtoValue(static_cast<uint64_t>(stoull(content)));
			break;
		case sniffer::ProtoValue::Type::Float:
			value = ProtoValue(static_cast<uint64_t>(stof(content)));
			break;
		case sniffer::ProtoValue::Type::Double:
			value = ProtoValue(static_cast<uint64_t>(stod(content)));
			break;
		case sniffer::ProtoValue::Type::String:
			value = ProtoValue(content);
			break;
		default:
			return false;
			break;
		}
		return true;
	}

#pragma region ProtoField

	ProtoField::ProtoField(int index, const std::string& name) : m_Number(index), m_Name(name), m_Flags(0) { }
	ProtoField::ProtoField(int index, const std::string& name, ProtoValue&& value) : m_Number(index), m_Name(name), m_Value(std::move(value)), m_Flags(0) { }
	ProtoField::ProtoField() : ProtoField(-1, {}) { }

	void ProtoField::set_flag(Flag flag, bool enabled)
	{
		if (enabled)
			m_Flags |= static_cast<uint32_t>(flag);
		else
			m_Flags &= ~static_cast<uint32_t>(flag);
	}

	bool ProtoField::has_flag(Flag flag) const
	{
		return m_Flags & static_cast<uint32_t>(flag);
	}

	uint32_t ProtoField::flags() const
	{
		return m_Flags;
	}

	void ProtoField::set_flags(uint32_t flags)
	{
		m_Flags = flags;
	}

	sniffer::ProtoValue const& ProtoField::value() const
	{
		return m_Value;
	}

	sniffer::ProtoValue& ProtoField::value()
	{
		return m_Value;
	}

	void ProtoField::set_value(ProtoValue&& value)
	{
		m_Value = std::move(value);
		set_flag(Flag::Unsetted, false);
	}

	void ProtoField::set_value(const ProtoValue& value)
	{
		m_Value = value;
		set_flag(Flag::Unsetted, false);
	}

	void ProtoField::set_name(const std::string& value)
	{
		m_Name = value;
	}

	std::string const& ProtoField::name() const
	{
		return m_Name;
	}

	int ProtoField::number() const
	{
		return m_Number;
	}

	void ProtoField::to_json(nlohmann::json& j) const
	{
		j = {
			{ "name", m_Name },
			{ "flags", m_Flags },
			{ "index", m_Number }
		};

		auto& value_container = j["value"];
		m_Value.to_json(value_container);
	}

	void ProtoField::from_json(const nlohmann::json& j)
	{
		j.at("name").get_to(m_Name);
		j.at("flags").get_to(m_Flags);
		j.at("index").get_to(m_Number);
		
		auto& value_container = j.at("value");
		m_Value.from_json(value_container);
	}

	bool ProtoField::operator==(const ProtoField& other) const
	{
		return number() == other.number() && name() == other.name() && value() == other.value();
	}

#pragma endregion ProtoField

#pragma region ProtoNode

	ProtoNode::ProtoNode() {}

	ProtoNode::ProtoNode(const ProtoNode& other) : m_Fields(other.m_Fields), m_Typename(other.m_Typename)
	{
		FillMaps();
	}

	ProtoNode::ProtoNode(ProtoNode&& other) noexcept : m_Fields(std::move(other.m_Fields)), m_Typename(std::move(other.m_Typename)),
		m_IndexMap(std::move(other.m_IndexMap)), m_NameMap(std::move(other.m_NameMap)) { }

	sniffer::ProtoNode& ProtoNode::operator=(const ProtoNode& other)
	{
		m_Fields = other.m_Fields;
		m_Typename = other.m_Typename;
		FillMaps();

		return *this;
	}

	sniffer::ProtoNode& ProtoNode::operator=(ProtoNode&& other) noexcept
	{
		m_Fields = std::move(other.m_Fields);
		m_IndexMap = std::move(other.m_IndexMap);
		m_NameMap = std::move(other.m_NameMap);
		m_Typename = std::move(other.m_Typename);
		return *this;
	}

	std::list<sniffer::ProtoField> const& ProtoNode::fields() const
	{
		return m_Fields;
	}

	std::list<sniffer::ProtoField>& ProtoNode::fields()
	{
		return m_Fields;
	}

	bool ProtoNode::has(int fieldIndex) const
	{
		return m_IndexMap.find(fieldIndex) != m_IndexMap.end();
	}

	bool ProtoNode::has(const std::string& name) const
	{
		return m_NameMap.find(name) != m_NameMap.end();
	}

	const sniffer::ProtoField& ProtoNode::field_at(int fieldIndex) const
	{
		return *m_IndexMap.at(fieldIndex);
	}

	const sniffer::ProtoField& ProtoNode::field_at(const std::string& name) const
	{
		return *m_NameMap.at(name);
	}

	sniffer::ProtoField& ProtoNode::field_at(int fieldIndex)
	{
		return *m_IndexMap.at(fieldIndex);
	}

	sniffer::ProtoField& ProtoNode::field_at(const std::string& name)
	{
		return *m_NameMap.at(name);
	}

	sniffer::ProtoField& ProtoNode::push_field(ProtoField&& field)
	{
		auto map_it = m_IndexMap.find(field.number());
		if (map_it != m_IndexMap.end())
		{
			auto it = std::find_if(m_Fields.begin(), m_Fields.end(), [&map_it](const ProtoField& element) { return map_it->second == &element; });
			assert(it != m_Fields.end());

			*it = std::move(field);
			return *it;
		}

		auto& new_item = m_Fields.emplace_back(std::move(field));

		if (!new_item.has_flag(ProtoField::Flag::Custom))
			m_IndexMap[new_item.number()] = &new_item;

		m_NameMap[new_item.name()] = &new_item;
		return new_item;
	}

	void ProtoNode::remove_field(const ProtoField& field)
	{
		if (!field.has_flag(ProtoField::Flag::Custom))
			m_IndexMap.erase(field.number());

		m_NameMap.erase(field.name());
		m_Fields.remove(field);
	}

	void ProtoNode::remove_field(int fieldIndex)
	{
		auto& field = field_at(fieldIndex);
		remove_field(field);
	}

	void ProtoNode::remove_field(const std::string& name)
	{
		auto& field = field_at(name);
		remove_field(field);
	}

	static void value_to_view_json(nlohmann::json& j, const ProtoValue& value, bool includeUnknown, bool includeUnsetted)
	{
		switch (value.type())
		{
		case ProtoValue::Type::Unknown:
			break;

		case ProtoValue::Type::Bool:
			j = value.get<bool>();
			break;
		case ProtoValue::Type::Int32:
			j = value.get<int32_t>();
			break;
		case ProtoValue::Type::Int64:
			j = value.get<int64_t>();
			break;
		case ProtoValue::Type::UInt32:
			j = value.get<uint32_t>();
			break;
		case ProtoValue::Type::UInt64:
			j = value.get<uint64_t>();
			break;
		case ProtoValue::Type::Float:
			j = value.get<float>();
			break;
		case ProtoValue::Type::Double:
			j = value.get<double>();
			break;
		case ProtoValue::Type::Enum:
			j = value.get<ProtoEnum>().repr();
			break;
		case ProtoValue::Type::ByteSequence:
		{
			auto& byte_str = value.get<std::vector<byte>>();
			j = util::base64_encode(byte_str.data(), byte_str.size());
			break;
		}
		case ProtoValue::Type::String:
			j = value.get<std::string>();
			break;
		case ProtoValue::Type::Node:
			value.get<ProtoNode>().to_view_json(j, includeUnknown, includeUnsetted);
			break;
		case ProtoValue::Type::List:
		{
			auto& list = value.to_list();
			j = nlohmann::json::array();
			for (const auto& element : list)
			{
				auto& json_element = j.emplace_back();
				value_to_view_json(json_element, element, includeUnknown, includeUnsetted);
			}
			break;
		}
		case ProtoValue::Type::Map:
		{
			auto& map = value.to_map();
			j = nlohmann::json::object();
			for (const auto& [key, value]: map)
			{
				nlohmann::json& json_element = j[key.convert_to_string()];
				value_to_view_json(json_element, value, includeUnknown, includeUnsetted);
			}
			break;
		}
		}
	}

	void ProtoNode::to_view_json(nlohmann::json& j, bool includeUnknown, bool includeUnsetted) const
	{
		for (const auto& field : fields())
		{
			if (!includeUnknown && field.has_flag(ProtoField::Flag::Unknown))
				continue;

			if (!includeUnsetted && field.has_flag(ProtoField::Flag::Unsetted))
				continue;

			auto& container = j[field.name()];
			value_to_view_json(j[field.name()], field.value(), includeUnknown, includeUnsetted);
		}
	}

	void ProtoNode::to_json(nlohmann::json& j) const
	{
		j = {
			{ "type", m_Typename }
		};

		auto& fields_container = j["fields"];
		fields_container = nlohmann::json::array();
		for (auto& field : m_Fields)
		{
			auto& field_container = fields_container.emplace_back();
			field.to_json(field_container);
		}
	}

	void ProtoNode::from_json(const nlohmann::json& j)
	{
		j.at("type").get_to(m_Typename);
		for (auto& field_container : j.at("fields"))
		{
			ProtoField field = ProtoField(-1, {});
			field.from_json(field_container);
			push_field(std::move(field));
		}
	}

	const std::string& ProtoNode::type() const
	{
		return m_Typename;
	}

	void ProtoNode::set_type(const std::string& type)
	{
		m_Typename = type;
	}

	void ProtoNode::FillMaps()
	{
		m_IndexMap.clear();
		m_NameMap.clear();
		for (auto& field : m_Fields)
		{
			m_IndexMap[field.number()] = &field;
			m_NameMap[field.name()] = &field;
		}
	}

#pragma endregion ProtoNode

#pragma region ProtoMessage

	void ProtoMessage::set_flag(Flag option, bool enabled)
	{
		if (enabled)
			m_Flags |= static_cast<uint32_t>(option);
		else
			m_Flags &= ~static_cast<uint32_t>(option);
	}

	bool ProtoMessage::has_flag(Flag option) const
	{
		return m_Flags & static_cast<uint32_t>(option);
	}

	void to_json(nlohmann::json& j, const ProtoMessage& message)
	{
		message.to_json(j);
	}

	void from_json(const nlohmann::json& j, ProtoMessage& message)
	{
		message.from_json(j);
	}

#pragma endregion ProtoMessage
}