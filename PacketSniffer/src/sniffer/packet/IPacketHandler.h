#pragma once
#include <sniffer/packet/ModifyType.h>
#include <sniffer/packet/RawPacketData.h>

namespace sniffer::packet 
{
	class IPacketHandler
	{
	public:
		virtual RawPacketData Receive() = 0;
		virtual void Response(RawPacketData data, ModifyType type) = 0;
		virtual bool IsModifyingEnabled() const = 0;
		virtual bool IsConnected() = 0;
	};
}