#include <pch.h>
#include "comparers.h"

namespace sniffer::filter::comparer
{

#pragma region BaseComparer

	bool BaseComparer::CompareProtoValue(const ProtoValue& protoValue, const std::string& pattern, IFilterComparer::CompareType compareType)
	{
		if (protoValue.is_string())
			return CompareObject<std::string>(protoValue.to_string(), pattern, compareType);

		if (protoValue.is_bool())
		{
			if (compareType != IFilterComparer::CompareType::Equal)
				return false;

			bool result = pattern == "true" || pattern == "1";
			return protoValue.to_bool() == result;
		}

		if (protoValue.is_float())
		{
			auto value = std::stof(pattern);
			float objVal = protoValue.to_float();
			if (compareType == IFilterComparer::CompareType::Equal)
				return fabs(objVal - value) <= ((fabs(objVal) < fabs(value) ? fabs(value) : fabs(objVal)) * 0.01);
			return CompareObject<float>(objVal, value, compareType);
		}

		if (protoValue.is_integer32())
		{
			auto value = std::stoi(pattern);
			return CompareObject<int>(protoValue.to_integer32(), value, compareType);
		}

		if (protoValue.is_integer64())
		{
			auto value = std::stoll(pattern);
			return CompareObject<unsigned int>(protoValue.to_integer64(), value, compareType);
		}

		if (protoValue.is_unsigned32())
		{
			auto value = std::stoul(pattern);
			return CompareObject<unsigned int>(protoValue.to_unsigned32(), value, compareType);
		}

		if (protoValue.is_unsigned64())
		{
			auto value = std::stoull(pattern);
			return CompareObject<unsigned int>(protoValue.to_unsigned64(), value, compareType);
		}

		return false;
	}

	std::vector<const sniffer::ProtoValue*> BaseComparer::FindKeys(const ProtoNode& node, const std::string& key, IFilterComparer::CompareType compareType, bool onlyFirst /*= false*/, bool recursive /*= true*/)
	{
		std::vector<const sniffer::ProtoValue*> result;
		for (auto& field : node.fields())
		{
			auto& name = field.name();
			auto& value = field.value();

			if (CompareObject(key, name, compareType))
			{
				result.push_back(&value);
				
				if (onlyFirst)
					break;
			}

			if (!recursive)
				continue;

			if (value.is_node())
			{
				auto nestedResult = FindKeys(value.to_node(), key, compareType, onlyFirst, recursive);
				result.insert(result.end(), std::make_move_iterator(nestedResult.begin()), std::make_move_iterator(nestedResult.end()));
			}
			else if (value.is_list())
			{
				for (auto& element : value.to_list())
				{
					if (!element.is_node())
						continue;

					auto nestedResult = FindKeys(element.to_node(), key, compareType, onlyFirst, recursive);
					result.insert(result.end(), std::make_move_iterator(nestedResult.begin()), std::make_move_iterator(nestedResult.end()));
				}
			}
			else if (value.is_map())
			{
				for (auto& [_, element] : value.to_map())
				{
					if (!element.is_node())
						continue;

					auto nestedResult = FindKeys(element.to_node(), key, compareType, onlyFirst, recursive);
					result.insert(result.end(), std::make_move_iterator(nestedResult.begin()), std::make_move_iterator(nestedResult.end()));
				}
			}

			if (onlyFirst && !result.empty())
				break;
		}

		return result;
	}

	std::vector<const ProtoValue*> BaseComparer::FindKeysByPattern(const ProtoMessage& message, const std::string& pattern, IFilterComparer::CompareType compareType, bool onlyFirst /*= false*/)
	{
		auto tokens = util::StringSplit("::", pattern);

		std::string& mainToken = tokens[0];
		if (tokens.size() == 1)
		{
			auto values = FindKeys(message, mainToken, compareType, onlyFirst, true);
			return values;
		}

		auto values = FindKeys(message, mainToken, compareType, false, true);
		for (auto it = tokens.begin() + 1; it != tokens.end(); it++)
		{
			bool last_token = (tokens.end() - 1) == it;
			bool _onlyFirst = onlyFirst && last_token;

			std::vector<const ProtoValue*> newValues = {};
			for (auto& value : values)
			{
				if (!value->is_node())
					continue;

				auto foundValues = FindKeys(value->to_node(), *it, compareType, _onlyFirst, false);
				if (!foundValues.empty())
				{
					newValues.insert(newValues.end(),std::make_move_iterator(foundValues.begin()), std::make_move_iterator(foundValues.end()));
					if (_onlyFirst)
						return newValues;
				}
			}

			values = newValues;
			if (values.empty())
				return {};
		}

		return values;
	}

	bool BaseComparer::HasValue(const ProtoValue& object, const std::string& pattern, IFilterComparer::CompareType compareType)
	{
		if (object.is_node())
		{
			for (auto& nested : object.to_node().fields())
			{
				if (HasValue(nested.value(), pattern, compareType))
					return true;
			}
			return false;
		}

		if (object.is_list())
		{
			for (auto& item : object.to_list())
			{
				if (HasValue(item, pattern, compareType))
					return true;
			}
			return false;
		}

		if (object.is_map())
		{
			for (auto& [_, item] : object.to_map())
			{
				if (HasValue(item, pattern, compareType))
					return true;
			}
			return false;
		}

		return CompareProtoValue(object, pattern, compareType);
	}

	BaseComparer::BaseComparer() : m_CompareType(IFilterComparer::CompareType::Equal)
	{
	}

	BaseComparer::BaseComparer(IFilterComparer::CompareType compareType) : m_CompareType(compareType)
	{

	}

	IFilterComparer::CompareType BaseComparer::GetCompareType()
	{
		return m_CompareType;
	}

	void BaseComparer::SetCompareType(IFilterComparer::CompareType compareType)
	{
		m_CompareType = compareType;
	}

	void BaseComparer::to_json(nlohmann::json& j) const
	{
		config::Enum<IFilterComparer::CompareType> compareType = m_CompareType;
		j["compareType"] = compareType;
	}

	void BaseComparer::from_json(const nlohmann::json& j)
	{
		config::Enum<IFilterComparer::CompareType> compareType;
		j.at("compareType").get_to(compareType);
		m_CompareType = compareType;
	}

	BaseComparer& BaseComparer::operator=(const BaseComparer& other)
	{
		ChangedEvent(this);
		return *this;
	}

	void BaseComparer::FireChanged()
	{
		ChangedEvent(this);
	}

#pragma endregion BaseComparer

#pragma region AnyKey
	bool AnyKey::Execute(const packet::Packet& packet)
	{
		if (key.empty())
			return true;

		return !FindKeysByPattern(packet.content(), key, m_CompareType, true).empty();
	}

	void AnyKey::to_json(nlohmann::json& j) const
	{
		BaseComparer::to_json(j);
		j["key"] = key;
	}

	void AnyKey::from_json(const nlohmann::json& j)
	{
		BaseComparer::from_json(j);
		j.at("key").get_to(key);
	}

	std::vector<IFilterComparer::CompareType>& AnyKey::GetCompareTypes()
	{
		return s_CompareTypes;
	}
#pragma endregion AnyKey

#pragma region AnyValue
	bool AnyValue::Execute(const packet::Packet& packet)
	{
		if (value.empty())
			return true;

		return HasValue(packet.content(), value, m_CompareType);
	}

	void AnyValue::to_json(nlohmann::json& j) const
	{
		BaseComparer::to_json(j);
		j["value"] = value;
	}

	void AnyValue::from_json(const nlohmann::json& j)
	{
		BaseComparer::from_json(j);
		j.at("value").get_to(value);
	}

	std::vector<IFilterComparer::CompareType>& AnyValue::GetCompareTypes()
	{
		return s_CompareTypes;
	}
#pragma endregion AnyValue

#pragma region KeyValue

	bool KeyValue::Execute(const packet::Packet& packet)
	{
		if (key.empty())
			return true;

		auto values = FindKeysByPattern(packet.content(), key, keyCT);
		return !values.empty() && std::any_of(values.begin(), values.end(), [this](const ProtoValue* protoValue) { return CompareProtoValue(*protoValue, value, valueCT); });
	}

	void KeyValue::to_json(nlohmann::json& j) const
	{
		j["keyCT"] = config::Enum<IFilterComparer::CompareType>(keyCT);
		j["key"] = key;

		j["valueCT"] = config::Enum<IFilterComparer::CompareType>(valueCT);
		j["value"] = value;
	}

	void KeyValue::from_json(const nlohmann::json& j)
	{
		keyCT = j.at("keyCT").get<config::Enum<IFilterComparer::CompareType>>().value();
		j.at("key").get_to(key);

		valueCT = j.at("valueCT").get<config::Enum<IFilterComparer::CompareType>>().value();
		j.at("value").get_to(value);
	}

	std::vector<IFilterComparer::CompareType>& KeyValue::GetCompareTypes()
	{
		return s_CompareTypes;
	}
#pragma endregion KeyValue

#pragma region PacketID
	bool PacketID::Execute(const packet::Packet& packet)
	{
		return packet.uid() == id;
	}

	void PacketID::to_json(nlohmann::json& j) const
	{
		BaseComparer::to_json(j);
		j["id"] = id;
	}

	void PacketID::from_json(const nlohmann::json& j)
	{
		BaseComparer::from_json(j);
		j.at("id").get_to(id);
	}

	std::vector<IFilterComparer::CompareType>& PacketID::GetCompareTypes()
	{
		return s_CompareTypes;
	}
#pragma endregion PacketID

#pragma region PacketName

	bool PacketName::Execute(const packet::Packet& packet)
	{
		if (name.empty())
			return true;

		return CompareObject(packet.name(), name, m_CompareType);
	}

	void PacketName::to_json(nlohmann::json& j) const
	{
		BaseComparer::to_json(j);
		j["name"] = name;
	}

	void PacketName::from_json(const nlohmann::json& j)
	{
		BaseComparer::from_json(j);
		j.at("name").get_to(name);
	}

	std::vector<IFilterComparer::CompareType>& PacketName::GetCompareTypes()
	{
		return s_CompareTypes;
	}
#pragma endregion PacketName

#pragma region PacketSize
	bool PacketSize::Execute(const packet::Packet& packet)
	{
		return CompareObject<size_t>(packet.size(), size, m_CompareType);
	}

	void PacketSize::to_json(nlohmann::json& j) const
	{
		BaseComparer::to_json(j);
		j["size"] = size;
	}

	void PacketSize::from_json(const nlohmann::json& j)
	{
		BaseComparer::from_json(j);
		j.at("size").get_to(size);
	}

	std::vector<IFilterComparer::CompareType>& PacketSize::GetCompareTypes()
	{
		return s_CompareTypes;
	}
#pragma endregion PacketSize

#pragma region PacketTime
	bool PacketTime::ParseTime()
	{
		tm newTm = {};
		auto regexData = std::regex("([0-9]{1,2})/([0-9]{1,2})/([0-9]{1,4})", std::regex_constants::ECMAScript);
		auto regexTime = std::regex("([0-9]{1,2})\\:([0-9]{1,2})\\:([0-9]{1,2})", std::regex_constants::ECMAScript);

		m_Invalid = true;

		std::smatch dataMatch;
		if (std::regex_search(time, dataMatch, regexData))
		{
			auto day = std::stoi(dataMatch[1].str());
			if (day == 0 || day > 31)
				return false;

			auto month = std::stoi(dataMatch[2].str());
			if (month == 0 || month > 12)
				return false;

			auto year = std::stoi(dataMatch[3].str());
			if (year < 1900)
				return false;

			newTm.tm_mday = day;
			newTm.tm_mon = month - 1;
			newTm.tm_year = year - 1900;
		}

		std::smatch timeMatch;
		if (std::regex_search(time, timeMatch, regexTime))
		{
			auto hour = std::stoi(timeMatch[1].str());
			if (hour > 23)
				return false;

			auto minutes = std::stoi(timeMatch[2].str());
			if (minutes > 59)
				return false;

			auto seconds = std::stoi(timeMatch[3].str());
			if (seconds > 59)
				return false;

			newTm.tm_hour = hour;
			newTm.tm_min = minutes;
			newTm.tm_sec = seconds;
		}

		m_Time = newTm;
		m_Invalid = dataMatch.empty() && timeMatch.empty();
		return !m_Invalid;
	}

	int64_t PacketTime::FixTime(const tm& infoTime)
	{
		tm time = m_Time;

		if (time.tm_year == 0)
		{
			time.tm_year = infoTime.tm_year;
			time.tm_mon = infoTime.tm_mon;
			time.tm_mday = infoTime.tm_mday;
		}
		return mktime(&time);
	}

	bool PacketTime::Execute(const packet::Packet& packet)
	{
		if (m_Invalid)
			return true;

		int64_t timestamp = FixTime(TimestampToTm(packet.timestamp()));
		return CompareObject(packet.timestamp() / 1000, timestamp, m_CompareType);
	}

	void PacketTime::to_json(nlohmann::json& j) const
	{
		BaseComparer::to_json(j);
		j["time"] = time;
	}

	void PacketTime::from_json(const nlohmann::json& j)
	{
		BaseComparer::from_json(j);
		j.at("time").get_to(time);
		ParseTime();
	}

	std::vector<IFilterComparer::CompareType>& PacketTime::GetCompareTypes()
	{
		return s_CompareTypes;
	}

	static std::string GetTimeStr(const tm& tm)
	{
		std::stringstream buffer{};
		buffer << std::put_time(&tm, "%d/%m/%Y %H:%M:%S");
		return buffer.str();
	}

	PacketTime::PacketTime(IFilterComparer::CompareType compareType, int64_t timestamp) : BaseComparer(compareType),
		time({}),
		m_Invalid(false),
		m_Time({})
	{
		auto fixed_timestamp = timestamp / 1000;
		localtime_s(&m_Time, &fixed_timestamp);
		time = GetTimeStr(m_Time);
	}

	PacketTime::PacketTime(IFilterComparer::CompareType compareType, const std::string& timeStr) : BaseComparer(compareType),
		time(timeStr),
		m_Invalid(true),
		m_Time({})
	{
		ParseTime();
	}

	PacketTime::PacketTime(IFilterComparer::CompareType compareType, const tm& tm) : BaseComparer(compareType),
		time({}),
		m_Invalid(false),
		m_Time(tm)
	{
		time = GetTimeStr(m_Time);
	}

	tm PacketTime::TimestampToTm(int64_t timestamp)
	{
		time_t rawTime = timestamp / 1000;
		struct tm gmtm;
		localtime_s(&gmtm, &rawTime);

		return gmtm;
	}

#pragma endregion PacketTime
}