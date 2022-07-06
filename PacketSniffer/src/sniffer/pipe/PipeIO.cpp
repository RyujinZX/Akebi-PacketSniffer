#include <pch.h>
#include "PipeIO.h"

namespace sniffer::pipe
{
	void PipeIO::Send(messages::PipeMessage& data)
	{
		if (!IsConnected())
			return;

		m_Pipe->WriteObject(data);
	}

	messages::PipeMessage* PipeIO::ReceiveMessage()
	{
		if (!IsConnected())
			return nullptr;

		messages::PipeMessage header;
		m_Pipe->ReadObject(header);

		MessageIDs messageID = static_cast<MessageIDs>(header.packetID());

#define MESSAGE_CASE(mid, type)\
					case mid:                                       		  \
					{										        		  \
						auto data = new type();	            		          \
						m_Pipe->ReadObject(*data);			        		  \
						data->SetMessage(header);                             \
						CallHandlers(*data);					    		  \
						return reinterpret_cast<messages::PipeMessage*>(data);\
					}										  

		switch (messageID)
		{
			MESSAGE_CASE(MessageIDs::PACKET_DATA, messages::PipePacketData);
			MESSAGE_CASE(MessageIDs::MODIFY_DATA, messages::PipeModifyData);
		default:
			break;
		}
		return nullptr;
	}

	void PipeIO::ProcessMessage()
	{
		auto message = ReceiveMessage();
		delete message;
	}
}