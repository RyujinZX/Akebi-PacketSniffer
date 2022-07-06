#pragma once
#include <sniffer/packet/NetIODirection.h>
#include <sniffer/pipe/messages/PipeMessage.h>

namespace sniffer::pipe::messages
{
	class PipePacketData : public PipeMessage
	{
	public:
		using PipeMessage::PipeMessage;

		bool manipulationEnabled;
		
		packet::NetIODirection direction;
		int16_t messageID;
		std::vector<byte> head;
		std::vector<byte> content;

		// Inherited via PipeSerializedObject
		void Write(PipeTransfer* transfer) final;
		void Read(PipeTransfer* transfer) final;
	};
}
