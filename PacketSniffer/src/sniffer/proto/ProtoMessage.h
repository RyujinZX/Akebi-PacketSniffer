#pragma once

namespace sniffer
{
	class ProtoNode;

	class ProtoEnumCache
	{
	public:
		static std::unordered_map<uint32_t, std::string>* GetEnumValues(uint32_t enumID);
		static std::unordered_map<uint32_t, std::string>* GetEnumValues(const std::string& enumName);
		static const std::string* GetEnumValueName(uint32_t enumID, uint32_t value);

		static uint32_t GetID(const std::string& enumName);
		static const std::string* GetName(uint32_t id);

		template<typename TName, typename TValues>
		static uint32_t StoreEnumValues(TName&& name, TValues&& values)
		{
			auto oldID = GetID(name);
			if (oldID != 0)
				return oldID;
			
			auto& newElement = s_EnumValuesPool.emplace_back(std::make_pair(++s_LastIndex, std::forward<TValues>(values)));
			s_IDEnumMap.emplace(s_LastIndex, &newElement);
			s_NameEnumMap.emplace(std::forward<TName>(name), &newElement);

			return s_LastIndex;
		}

	private:
		using EnumValues = std::pair<uint32_t, std::unordered_map<uint32_t, std::string>>;
		
		static inline std::unordered_map<std::string, EnumValues*> s_NameEnumMap;
		static inline std::unordered_map<uint32_t, EnumValues*> s_IDEnumMap;

		static inline std::list<EnumValues> s_EnumValuesPool;

		static inline uint32_t s_LastIndex = 0;

	public:
		ProtoEnumCache() = delete;
		ProtoEnumCache(ProtoEnumCache&) = delete;
		ProtoEnumCache(ProtoEnumCache&&) = delete;
	};

	class ProtoEnum
	{
	public:
		ProtoEnum();

		ProtoEnum(uint32_t enumID, uint32_t value);

		bool operator==(const ProtoEnum& other) const;
		
		uint32_t value() const;
		const std::string& type() const;
		const std::string& repr() const;
		const std::unordered_map<uint32_t, std::string>& values() const;

		void to_json(nlohmann::json& j) const;
		void from_json(const nlohmann::json& j);

	private:

		void update_string_view();

		uint32_t m_enumID;
		uint32_t m_Value;
	};

	class ProtoValue
	{
	public:

		using bseq_type = std::vector<byte>;
		using list_type = std::list<ProtoValue>;
		using map_type = std::unordered_map<ProtoValue, ProtoValue>;

		ProtoValue(const ProtoValue& value);
		ProtoValue(ProtoValue&& value) noexcept;

		ProtoValue& operator=(const ProtoValue& value);
		ProtoValue& operator=(ProtoValue&& value) noexcept;

		void to_json(nlohmann::json& j) const;
		void from_json(const nlohmann::json& j);

		~ProtoValue();

		ProtoValue();
		ProtoValue(bool value);

		ProtoValue(int32_t value);
		ProtoValue(int64_t value);

		ProtoValue(uint32_t value);
		ProtoValue(uint64_t value);

		ProtoValue(float value);
		ProtoValue(double value);

		ProtoValue(const ProtoEnum& value);
		ProtoValue(ProtoEnum&& value);

		ProtoValue(const bseq_type& value);
		ProtoValue(bseq_type&& value);

		ProtoValue(const char* value);
		ProtoValue(const std::string& value);
		ProtoValue(std::string&& value);

		ProtoValue(const list_type& value);
		ProtoValue(list_type&& value);

		ProtoValue(const map_type& value);
		ProtoValue(map_type&& value);

		ProtoValue(const ProtoNode& value);
		ProtoValue(ProtoNode&& value);

		enum class Type
		{
			Unknown, Bool,
			Int32, Int64,
			UInt32, UInt64,
			Float, Double,
			Enum, ByteSequence,
			String, List,
			Map, Node
		};

		Type type() const;

		template<typename T> T const& get() const;
		template<typename T> T& get();

		std::string convert_to_string() const;
		bool operator==(const ProtoValue& other) const;

		inline bool is_bool() const { return type() == Type::Bool; };
		inline bool is_integer32() const { return type() == Type::Int32; };
		inline bool is_integer64() const { return type() == Type::Int64; };
		inline bool is_unsigned32() const { return type() == Type::UInt32; };
		inline bool is_unsigned64() const { return type() == Type::UInt64; };
		inline bool is_float() const { return type() == Type::Float; };
		inline bool is_double() const { return type() == Type::Double; };
		inline bool is_enum() const { return type() == Type::Enum; };
		inline bool is_bytes() const { return type() == Type::ByteSequence; };
		inline bool is_string() const { return type() == Type::String; };
		inline bool is_node() const { return type() == Type::Node; };
		inline bool is_list() const { return type() == Type::List; };
		inline bool is_map() const { return type() == Type::Map; };

		inline bool& to_bool() { return get<bool>(); };
		inline int32_t& to_integer32() { return get<int32_t>(); };
		inline int64_t& to_integer64() { return get<int64_t>(); };
		inline uint32_t& to_unsigned32() { return get<uint32_t>(); };
		inline uint64_t& to_unsigned64() { return get<uint64_t>(); };
		inline float& to_float() { return get<float>(); };
		inline double& to_double() { return get<double>(); };
		inline ProtoEnum& to_enum() { return get<ProtoEnum>(); };
		inline bseq_type& to_bytes() { return get<bseq_type>(); };
		inline std::string& to_string() { return get<std::string>(); };
		inline list_type& to_list() { return get<list_type>(); };
		inline map_type& to_map() { return get<map_type>(); };
		inline ProtoNode& to_node() { return get<ProtoNode>(); };

		// Yeah, it's just copy paste
		inline const bool& to_bool() const { return get<bool>(); };
		inline const int32_t& to_integer32() const { return get<int32_t>(); };
		inline const int64_t& to_integer64() const { return get<int64_t>(); };
		inline const uint32_t& to_unsigned32() const { return get<uint32_t>(); };
		inline const uint64_t& to_unsigned64() const { return get<uint64_t>(); };
		inline const float& to_float() const { return get<float>(); };
		inline const double& to_double() const { return get<double>(); };
		inline const ProtoEnum& to_enum() const { return get<ProtoEnum>(); };
		inline const bseq_type& to_bytes() const { return get<bseq_type>(); };
		inline const std::string& to_string() const { return get<std::string>(); };
		inline const list_type& to_list() const { return get<list_type>(); };
		inline const map_type& to_map() const { return get<map_type>(); };
		inline const ProtoNode& to_node() const { return get<ProtoNode>(); };

		template<typename T>
		ProtoValue& operator=(const T& value);

		template<typename T>
		ProtoValue& operator=(T&& value);

	private:
		void CopyValue(const ProtoValue& other);
		void Release();
		Type m_ValueType;

		void* m_ValueContainer;
	};

	bool ConvertStringToProtoValue(const std::string& content, ProtoValue::Type type, ProtoValue& value);

	template<typename>
	struct cpp_to_proto_type {};
	template<> struct cpp_to_proto_type<bool> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::Bool;  static constexpr bool allocated = false; };
	template<> struct cpp_to_proto_type<int32_t> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::Int32; static constexpr bool allocated = false; };
	template<> struct cpp_to_proto_type<int64_t> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::Int64; static constexpr bool allocated = false; };
	template<> struct cpp_to_proto_type<uint32_t> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::UInt32; static constexpr bool allocated = false; };
	template<> struct cpp_to_proto_type<uint64_t> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::UInt64; static constexpr bool allocated = false; };
	template<> struct cpp_to_proto_type<float> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::Float; static constexpr bool allocated = false; };
	template<> struct cpp_to_proto_type<double> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::Double; static constexpr bool allocated = false; };
	template<> struct cpp_to_proto_type<ProtoEnum> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::Enum; static constexpr bool allocated = true; };
	template<> struct cpp_to_proto_type<ProtoValue::bseq_type> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::ByteSequence; static constexpr bool allocated = true; };
	template<> struct cpp_to_proto_type<std::string> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::String; static constexpr bool allocated = true; };
	template<> struct cpp_to_proto_type<ProtoNode> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::Node; static constexpr bool allocated = true; };
	template<> struct cpp_to_proto_type<ProtoValue::list_type> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::List; static constexpr bool allocated = true; };
	template<> struct cpp_to_proto_type<ProtoValue::map_type> { static constexpr ProtoValue::Type protoType = ProtoValue::Type::Map; static constexpr bool allocated = true; };

	template<ProtoValue::Type>
	struct proto_to_cpp_type {};
	template<> struct proto_to_cpp_type<ProtoValue::Type::Bool> { using type = bool; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::Int32> { using type = int32_t; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::Int64> { using type = int64_t; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::UInt32> { using type = uint32_t; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::UInt64> { using type = uint64_t; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::Float> { using type = float; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::Double> { using type = double; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::Enum> { using type = ProtoEnum; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::ByteSequence> { using type = ProtoValue::bseq_type; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::String> { using type = std::string; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::Node> { using type = ProtoNode; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::List> { using type = ProtoValue::list_type; };
	template<> struct proto_to_cpp_type<ProtoValue::Type::Map> { using type = ProtoValue::map_type; };

	template<typename T>
	T const& ProtoValue::get() const
	{
		constexpr Type type = cpp_to_proto_type<T>::protoType;
		constexpr bool allocated = cpp_to_proto_type<T>::allocated;

		if (type != m_ValueType)
			throw std::invalid_argument("Incorrect type.");

		if (allocated)
			return *reinterpret_cast<T*>(m_ValueContainer);

		return *reinterpret_cast<const T*>(&m_ValueContainer);
	}

	template<typename T>
	T& ProtoValue::get()
	{
		return const_cast<T&>(const_cast<const ProtoValue*>(this)->get<T>());
	}

	template<typename T>
	ProtoValue& ProtoValue::operator=(const T& value)
	{
		*this = ProtoValue(value);
		return *this;
	}

	template<typename T>
	ProtoValue& ProtoValue::operator=(T&& value)
	{
		*this = ProtoValue(std::move(value));
		return *this;
	}

	class ProtoField
	{
	public:
		enum class Flag : uint32_t
		{
			Unknown = 1,
			Unsetted = 2,
			Custom = 4
		};

		ProtoField();
		ProtoField(int number, const std::string& name);
		ProtoField(int number, const std::string& name, ProtoValue&& value);

		template<typename TValue>
		ProtoField(int number, const std::string& name, const TValue& value);

		template<typename TValue>
		ProtoField(int number, const std::string& name, TValue&& value);

		void set_flag(Flag flag, bool enabled = true);
		bool has_flag(Flag flag) const;
		uint32_t flags() const;
		void set_flags(uint32_t flags);

		template<typename T> T const& get() const;
		template<typename T> T& get();

		ProtoValue const& value() const;
		ProtoValue& value();

		void set_value(const ProtoValue& value);
		void set_value(ProtoValue&& value);
		void set_name(const std::string& value);

		template<typename T>
		void set_value(const T& value) { set_value(ProtoValue(value)); }

		std::string const& name() const;
		int number() const;

		bool operator==(const ProtoField& other) const;

		void to_json(nlohmann::json& j) const;
		void from_json(const nlohmann::json& j);

	private:
		int m_Number;
		std::string m_Name;
		ProtoValue m_Value;
		uint32_t m_Flags;
	};


	template<typename T>
	T const& sniffer::ProtoField::get() const
	{
		return value().get<T>();
	}

	template<typename T>
	T& sniffer::ProtoField::get()
	{
		return value().get<T>();
	}

	template<typename TValue>
	sniffer::ProtoField::ProtoField(int index, const std::string& name, const TValue& value) : m_Number(index), m_Name(name), m_Value(value), m_Flags(0) {
		if (m_Number == -1)
			set_flag(ProtoField::Flag::Custom);
	}

	template<typename TValue>
	sniffer::ProtoField::ProtoField(int index, const std::string& name, TValue&& value) : m_Number(index), m_Name(name), m_Value(std::move(value)), m_Flags(0) {
		if (m_Number == -1)
			set_flag(ProtoField::Flag::Custom);
	}

	class ProtoNode
	{
	public:
		ProtoNode();
		ProtoNode(const ProtoNode& other);
		ProtoNode(ProtoNode&& other) noexcept;

		ProtoNode& operator=(const ProtoNode& other);
		ProtoNode& operator=(ProtoNode&& other) noexcept;

		std::list<ProtoField>& fields();
		std::list<ProtoField> const& fields() const;
		bool has(int fieldIndex) const;
		bool has(const std::string& name) const;

		const ProtoField& field_at(int fieldIndex) const;
		const ProtoField& field_at(const std::string& name) const;

		ProtoField& field_at(int fieldIndex);
		ProtoField& field_at(const std::string& name);

		ProtoField& push_field(ProtoField&& field);

		template<typename... TParams>
		ProtoField& emplace_field(TParams... args);

		void remove_field(const ProtoField& field);
		void remove_field(int fieldIndex);
		void remove_field(const std::string& name);

		void to_view_json(nlohmann::json& j, bool includeUnknown, bool includeUnsetted) const;

		void to_json(nlohmann::json& j) const;
		void from_json(const nlohmann::json& j);

		const std::string& type() const;
		void set_type(const std::string& type);
	private:

		void FillMaps();
		std::unordered_map<std::string, ProtoField*> m_NameMap;
		std::unordered_map<int, ProtoField*> m_IndexMap;
		std::list<ProtoField> m_Fields;
		std::string m_Typename;
	};

	template<typename... TParams>
	ProtoField& sniffer::ProtoNode::emplace_field(TParams... args)
	{
		return push_field(ProtoField(args...));
	}

	class ProtoMessage : public ProtoNode
	{

	public:
		enum class Flag : uint32_t
		{
			HasUnknown = 1,
			HasUnsetted = 2,
			IsUnpacked = 4
		};

		void set_flag(Flag option, bool enabled = true);
		bool has_flag(Flag option) const;

		inline bool operator==(const ProtoMessage& other) const { return false; }
	private:
		uint32_t m_Flags;
	};

	void to_json(nlohmann::json& j, const ProtoMessage& message);
	void from_json(const nlohmann::json& j, ProtoMessage& message);
}

namespace std {

	template <>
	struct hash<sniffer::ProtoValue>
	{
		std::size_t operator()(const sniffer::ProtoValue& k) const
		{
			switch (k.type())
			{
			case sniffer::ProtoValue::Type::Bool:
				return hash<bool>()(k.get<bool>());
			case sniffer::ProtoValue::Type::Int32:
				return hash<int32_t>()(k.get<int32_t>());
			case sniffer::ProtoValue::Type::Int64:
				return hash<int64_t>()(k.get<int64_t>());
			case sniffer::ProtoValue::Type::UInt32:
				return hash<uint32_t>()(k.get<uint32_t>());
			case sniffer::ProtoValue::Type::UInt64:
				return hash<uint64_t>()(k.get<uint64_t>());
			case sniffer::ProtoValue::Type::Float:
				return hash<float>()(k.get<float>());
			case sniffer::ProtoValue::Type::Double:
				return hash<double>()(k.get<double>());
			case sniffer::ProtoValue::Type::String:
				return hash<std::string>()(k.get<std::string>());

			default:
				throw std::invalid_argument(fmt::format("Proto value with type `{}` hash can't be computed", magic_enum::enum_name(k.type())));
			}

			return 0; // Isn't reachable
		}
	};

}