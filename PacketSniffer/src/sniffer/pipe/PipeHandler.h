#pragma once
#include <sniffer/packet/IPacketHandler.h>
#include <sniffer/pipe/PipeServer.h>

namespace sniffer::pipe
{
	class PipeHandler : public packet::IPacketHandler
	{
		
	public:
		PipeHandler() : m_PipeName("genshin_packet_pipe"), m_Server(), m_ModifyingEnabled(false) 
		{
			m_Server.Open(m_PipeName);
		}

		// Blocking function
		packet::RawPacketData Receive() final;
		void Response(packet::RawPacketData data, packet::ModifyType type) final;
		bool IsModifyingEnabled() const final;
		bool IsConnected() final;

		const std::string& GetPipeName() const;
		void SetPipeName(const std::string& name);

	private:

		PipeServer m_Server;
		std::string m_PipeName;
		bool m_ModifyingEnabled;
	};
}


