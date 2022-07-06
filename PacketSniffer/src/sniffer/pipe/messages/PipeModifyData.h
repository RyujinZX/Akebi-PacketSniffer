#pragma once
#include <cheat-base/PipeTransfer.h>
#include <sniffer/packet/ModifyType.h>
#include <sniffer/pipe/messages/PipeMessage.h>

namespace sniffer::pipe::messages
{
	class PipeModifyData : public PipeMessage
	{
	public:
		using PipeMessage::PipeMessage;

		packet::ModifyType modifyType;
		uint32_t messageID;
		std::vector<uint8_t> head;
		std::vector<uint8_t> content;

		PipeModifyData();
		~PipeModifyData() {}

		// Inherited via PipeSerializedObject
		virtual void Write(PipeTransfer* transfer) final;
		virtual void Read(PipeTransfer* transfer) final;
	};
}