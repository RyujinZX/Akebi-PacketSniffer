#pragma once

#include <sniffer/filter/IFilterComparer.h>
namespace sniffer::filter::comparer
{
	class BaseComparer : public IFilterComparer
	{
	public:
		explicit BaseComparer();
		BaseComparer& operator=(const BaseComparer& other);

		BaseComparer(IFilterComparer::CompareType compareType);

		IFilterComparer::CompareType GetCompareType() override;
		void SetCompareType(IFilterComparer::CompareType compareType) override;

		void FireChanged();

		void to_json(nlohmann::json& j) const override;
		void from_json(const nlohmann::json& j) override;

	protected:
		IFilterComparer::CompareType m_CompareType;

		static bool CompareProtoValue(const ProtoValue& value, const std::string& pattern, IFilterComparer::CompareType compareType);

		static std::vector<const ProtoValue*> FindKeys(const ProtoNode& node, const std::string& key, IFilterComparer::CompareType compareType, bool onlyFirst = false, bool recursive = true);
		static std::vector<const ProtoValue*> FindKeysByPattern(const ProtoMessage& message, const std::string& pattern, IFilterComparer::CompareType compareType, bool onlyFirst = false);

		static bool HasValue(const ProtoValue& object, const std::string& pattern, IFilterComparer::CompareType compare);

		template <typename T>
		static bool CompareObject(const T& firstValue, const T& secondValue, IFilterComparer::CompareType compareType)
		{
			switch (compareType)
			{
			case IFilterComparer::CompareType::Equal:
				return firstValue == secondValue;
			case IFilterComparer::CompareType::Less:
				return firstValue < secondValue;
			case IFilterComparer::CompareType::LessEqual:
				return firstValue <= secondValue;
			case IFilterComparer::CompareType::More:
				return firstValue > secondValue;
			case IFilterComparer::CompareType::MoreEqual:
				return firstValue >= secondValue;
			default:
				return false;
			}
		}

		template<>
		static bool CompareObject(const std::string& value, const std::string& pattern, IFilterComparer::CompareType compare)
		{
			auto predicate = [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); };
			switch (compare)
			{
			case IFilterComparer::CompareType::Regex:
			{
				try
				{
					std::regex _regex(pattern);
					return std::regex_match(value, _regex);
				}
				catch (const std::regex_error& e)
				{
					UNREFERENCED_PARAMETER(e);
					return false;
				}
			}
			case IFilterComparer::CompareType::Equal:
				return std::equal(
					value.begin(), value.end(),
					pattern.begin(), pattern.end(),
					predicate);
			case IFilterComparer::CompareType::Contains:
				return std::search(
					value.begin(), value.end(),
					pattern.begin(), pattern.end(),
					predicate) != value.end();
			default:
				return false;
			}
		}
	};

	class AnyKey :
		public BaseComparer
	{
	public:
		AnyKey() : BaseComparer(), key() {}
		AnyKey(IFilterComparer::CompareType compareType, const std::string& key) : BaseComparer(compareType), key(key) {}

		bool Execute(const packet::Packet& packet) final;

		void to_json(nlohmann::json& j) const final;
		void from_json(const nlohmann::json& j) final;

		std::vector<IFilterComparer::CompareType>& GetCompareTypes() final;

		std::string key;
	private:

		inline static std::vector<IFilterComparer::CompareType> s_CompareTypes =
		{
			IFilterComparer::CompareType::Contains, IFilterComparer::CompareType::Equal, IFilterComparer::CompareType::Regex
		};
	};

	class AnyValue :
		public BaseComparer
	{
	public:
		AnyValue() : BaseComparer(), value() {}
		AnyValue(IFilterComparer::CompareType compareType, const std::string& value) : BaseComparer(compareType), value(value) {}

		bool Execute(const packet::Packet& packet) final;

		void to_json(nlohmann::json& j) const final;
		void from_json(const nlohmann::json& j) final;

		std::vector<IFilterComparer::CompareType>& GetCompareTypes() final;

		std::string value;
	private:

		inline static std::vector<IFilterComparer::CompareType> s_CompareTypes =
		{
			IFilterComparer::CompareType::Contains, IFilterComparer::CompareType::Equal, IFilterComparer::CompareType::Regex,
			IFilterComparer::CompareType::Less, IFilterComparer::CompareType::LessEqual,
			IFilterComparer::CompareType::More, IFilterComparer::CompareType::MoreEqual
		};
	};

	class KeyValue :
		public BaseComparer
	{
	public:

		KeyValue() : BaseComparer(), keyCT(IFilterComparer::CompareType::Equal), valueCT(IFilterComparer::CompareType::Equal), key(), value() {}
		KeyValue(IFilterComparer::CompareType keyCT, const std::string& key, IFilterComparer::CompareType valueCT, const std::string& value) :
			BaseComparer(), keyCT(keyCT), key(key), valueCT(valueCT), value(value) {}

		bool Execute(const packet::Packet& packet) final;

		void to_json(nlohmann::json& j) const final;
		void from_json(const nlohmann::json& j) final;

		std::vector<IFilterComparer::CompareType>& GetCompareTypes() final;

		inline static std::vector<IFilterComparer::CompareType> keyCTs =
		{
			IFilterComparer::CompareType::Contains, IFilterComparer::CompareType::Equal, IFilterComparer::CompareType::Regex
		};
		IFilterComparer::CompareType keyCT;
		std::string key;

		IFilterComparer::CompareType valueCT;
		inline static std::vector<IFilterComparer::CompareType> ValueCTs =
		{
			IFilterComparer::CompareType::Contains, IFilterComparer::CompareType::Equal, IFilterComparer::CompareType::Regex,
			IFilterComparer::CompareType::Less, IFilterComparer::CompareType::LessEqual,
			IFilterComparer::CompareType::More, IFilterComparer::CompareType::MoreEqual
		};

		std::string value;
	private:

		inline static std::vector<IFilterComparer::CompareType> s_CompareTypes = { };
	};

	class PacketID :
		public BaseComparer
	{
	public:
		PacketID() : BaseComparer(), id(0) {}
		PacketID(IFilterComparer::CompareType compareType, int id) : BaseComparer(compareType), id(id) {}

		bool Execute(const packet::Packet& packet) final;

		void to_json(nlohmann::json& j) const final;
		void from_json(const nlohmann::json& j) final;

		std::vector<IFilterComparer::CompareType>& GetCompareTypes() final;

		int id;
	private:

		inline static std::vector<IFilterComparer::CompareType> s_CompareTypes =
		{
			IFilterComparer::CompareType::Less, IFilterComparer::CompareType::LessEqual,
			IFilterComparer::CompareType::More, IFilterComparer::CompareType::MoreEqual,
			IFilterComparer::CompareType::Equal
		};
	};

	class PacketName :
		public BaseComparer
	{
	public:
		PacketName() : BaseComparer(), name() {}
		PacketName(IFilterComparer::CompareType compareType, std::string name) : BaseComparer(compareType), name(name) {}

		bool Execute(const packet::Packet& packet) final;
		void to_json(nlohmann::json& j) const final;
		void from_json(const nlohmann::json& j) final;

		std::vector<IFilterComparer::CompareType>& GetCompareTypes() final;

		std::string name;

	private:

		inline static std::vector<IFilterComparer::CompareType> s_CompareTypes =
		{
			IFilterComparer::CompareType::Contains, IFilterComparer::CompareType::Equal, IFilterComparer::CompareType::Regex
		};
	};

	class PacketSize :
		public BaseComparer
	{
	public:
		PacketSize() : BaseComparer(), size(0) {}
		PacketSize(IFilterComparer::CompareType compareType, int size) : BaseComparer(compareType), size(size) {}

		bool Execute(const packet::Packet& packet) final;

		void to_json(nlohmann::json& j) const final;
		void from_json(const nlohmann::json& j) final;

		std::vector<IFilterComparer::CompareType>& GetCompareTypes() final;

		int size;
	private:

		inline static std::vector<IFilterComparer::CompareType> s_CompareTypes =
		{
			IFilterComparer::CompareType::Less, IFilterComparer::CompareType::LessEqual,
			IFilterComparer::CompareType::More, IFilterComparer::CompareType::MoreEqual,
			IFilterComparer::CompareType::Equal
		};
	};

	class PacketTime :
		public BaseComparer
	{
	public:
		PacketTime() : BaseComparer(), time(), m_Invalid(true), m_Time() {}
		PacketTime(IFilterComparer::CompareType compareType, int64_t timestamp);
		PacketTime(IFilterComparer::CompareType compareType, const std::string& timeStr);
		PacketTime(IFilterComparer::CompareType compareType, const tm& tm);

		bool Execute(const packet::Packet& packet) final;

		void to_json(nlohmann::json& j) const final;
		void from_json(const nlohmann::json& j) final;

		inline bool IsInvalid() const { return m_Invalid; }

		bool ParseTime();

		std::vector<IFilterComparer::CompareType>& GetCompareTypes() final;

		std::string time;
	private:

		inline static std::vector<IFilterComparer::CompareType> s_CompareTypes =
		{
			IFilterComparer::CompareType::Less, IFilterComparer::CompareType::LessEqual,
			IFilterComparer::CompareType::More, IFilterComparer::CompareType::MoreEqual,
			IFilterComparer::CompareType::Equal
		};

		static tm TimestampToTm(int64_t timestamp);
		int64_t FixTime(const tm& infoTime);
		tm m_Time;
		bool m_Invalid;
	};
}