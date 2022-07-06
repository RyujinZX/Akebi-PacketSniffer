#pragma once
#include <cstdint>
#include <vector>

#include <sniffer/packet/NetIODirection.h>

namespace sniffer::packet
{
	struct RawPacketData
	{
	public:
		NetIODirection direction;
		uint32_t messageID;
		uint64_t timestamp;
		std::vector<uint8_t> content;
		std::vector<uint8_t> head;
	};
}