#pragma once
#include <string>
#include <sniffer/packet/RawPacketData.h>
#include <sniffer/proto/ProtoMessage.h>

#include <cheat-base/ISerializable.h>

namespace sniffer::packet
{
	class Packet : public ISerializable 
	{
	public:
		Packet();

		Packet(const RawPacketData& rawData, const ProtoMessage& content, const ProtoMessage& head, uint64_t uniqueID);
		Packet(RawPacketData&& rawData, ProtoMessage&& content, ProtoMessage&& head, uint64_t uniqueID);

		const std::string& name() const;
		uint64_t uid() const;
		uint32_t mid() const;

		int64_t timestamp() const;
		size_t size() const;
		NetIODirection direction() const;

		const RawPacketData& raw() const;
		RawPacketData& raw();

		const ProtoMessage& content() const;
		ProtoMessage& content();

		const ProtoMessage& head() const;
		ProtoMessage& head();

		bool operator==(const Packet& other) const;

		void to_json(nlohmann::json& j) const override;
		void from_json(const nlohmann::json& j) override;

	private:
		RawPacketData m_RawData;
		ProtoMessage m_Head;
		ProtoMessage m_Content;
		uint64_t m_UniqueID;
	};
}