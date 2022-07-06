#include <pch.h>
#include "Packet.h"

namespace sniffer::packet
{

	Packet::Packet(const RawPacketData& rawData, const ProtoMessage& content, const ProtoMessage& head, uint64_t uniqueID)
		: m_RawData(rawData), m_Content(content), m_Head(head), m_UniqueID(uniqueID)
	{ }

	Packet::Packet(RawPacketData&& rawData, ProtoMessage&& content, ProtoMessage&& head, uint64_t uniqueID)
		: m_RawData(std::move(rawData)), m_Content(std::move(content)), m_Head(std::move(head)), m_UniqueID(uniqueID)
	{

	}

	Packet::Packet() {}

	const std::string& Packet::name() const
	{
		return m_Content.type();
	}

	uint64_t Packet::uid() const
	{
		return m_UniqueID;
	}

	uint32_t Packet::mid() const
	{
		return m_RawData.messageID;
	}

	int64_t Packet::timestamp() const
	{
		return m_RawData.timestamp;
	}

	sniffer::packet::NetIODirection Packet::direction() const
	{
		return m_RawData.direction;
	}

	const sniffer::ProtoMessage& Packet::content() const
	{
		return m_Content;
	}

	sniffer::ProtoMessage& Packet::content()
	{
		return m_Content;
	}

	const sniffer::ProtoMessage& Packet::head() const
	{
		return m_Head;
	}

	sniffer::ProtoMessage& Packet::head()
	{
		return m_Head;
	}

	const sniffer::packet::RawPacketData& Packet::raw() const
	{
		return m_RawData;
	}

	sniffer::packet::RawPacketData& Packet::raw()
	{
		return m_RawData;
	}

	size_t Packet::size() const
	{
		return m_RawData.content.size() + m_RawData.head.size();
	}

	bool Packet::operator==(const Packet& other) const
	{
		return other.m_UniqueID == m_UniqueID && other.m_RawData.timestamp == m_RawData.timestamp;
	}

	void Packet::to_json(nlohmann::json& j) const
	{
		j = {
			{ "head_message", m_Head },
			{ "content_message", m_Content },
			{ "uid", m_UniqueID },
			{ "mid", m_RawData.messageID },
			{ "head_raw", m_RawData.head },
			{ "content_raw", m_RawData.content },
			{ "timestamp", m_RawData.timestamp },
			{ "direction", config::Enum(m_RawData.direction) }
		};
	}

	void Packet::from_json(const nlohmann::json& j)
	{
		j.at("head_message").get_to(m_Head);
		j.at("content_message").get_to(m_Content);
		j.at("uid").get_to(m_UniqueID);
		j.at("mid").get_to(m_RawData.messageID);
		j.at("head_raw").get_to(m_RawData.head);
		j.at("content_raw").get_to(m_RawData.content);
		j.at("timestamp").get_to(m_RawData.timestamp);
		m_RawData.direction = j.at("direction").get<config::Enum<NetIODirection>>();
	}
}