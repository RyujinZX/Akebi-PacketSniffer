#include "pch.h"
#include "PipeHandler.h"

namespace sniffer::pipe
{

	sniffer::packet::RawPacketData PipeHandler::Receive()
	{
		while (true)
		{
			if (!m_Server.IsOpened())
			{
				m_Server.Open(m_PipeName);
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				continue;
			}

			if (!m_Server.WaitForConnection())
				continue;

			auto packetData = m_Server.WaitFor<messages::PipePacketData>();
			if (!packetData)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				continue;
			}

			sniffer::packet::RawPacketData rawData = {};
			rawData.direction = packetData->direction;
			rawData.messageID = packetData->messageID;
			rawData.timestamp = packetData->timestamp();
			rawData.content = packetData->content;
			rawData.head = packetData->head;

			m_ModifyingEnabled = packetData->manipulationEnabled;
			return rawData;
		}
	}

	void PipeHandler::Response(packet::RawPacketData data, packet::ModifyType type)
	{
		if (!m_ModifyingEnabled)
			return;

		auto message = m_Server.CreateMessage<messages::PipeModifyData>();
		message.modifyType = type;

		if (message.modifyType == packet::ModifyType::Modified)
		{
			message.head = std::move(data.head);
			message.content = std::move(data.content);
			message.messageID = data.messageID;
		}
		m_Server.Send(message);
	}

	bool PipeHandler::IsModifyingEnabled() const
	{
		return m_ModifyingEnabled;
	}

	void PipeHandler::SetPipeName(const std::string& name)
	{
		m_PipeName = name;
		m_Server.Close();
		m_Server.Open(m_PipeName);
	}

	const std::string& PipeHandler::GetPipeName() const
	{
		return m_PipeName;
	}

	bool PipeHandler::IsConnected()
	{
		return m_Server.IsConnected();
	}

}