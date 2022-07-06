#pragma once
#include <nlohmann/json.hpp>
#include <sniffer/packet/Packet.h>
#include <cheat-base/ISerializable.h>

namespace sniffer::filter
{
	class IFilter : public ISerializable
	{
	public:
		virtual bool Execute(const packet::Packet& info) = 0;
		TEvent<IFilter*> ChangedEvent;
	};
}