#include <pch.h>
#include "PipeServer.h"

namespace sniffer::pipe
{	
	bool PipeServer::IsConnected()
	{
		bool pipeOpened = m_Pipe != nullptr && m_Pipe->IsPipeOpened();
		if (!pipeOpened && m_ClientConnected)
			m_ClientConnected = false;

		return pipeOpened && m_ClientConnected;
	}
	
	bool PipeServer::Open(const std::string& pipeName)
	{
		if (m_Pipe != nullptr)
			delete m_Pipe;

		m_Pipe = new PipeTransfer(pipeName);
		return m_Pipe->Create();
	}
	
	bool PipeServer::WaitForConnection()
	{
		if (IsConnected())
			return true;

		if (m_Pipe == nullptr)
			return false;

		if (!m_Pipe->IsPipeOpened() && !m_Pipe->Create())
			return false;

		m_ClientConnected = m_Pipe->WaitForConnection();
		return m_ClientConnected;
	}

	bool PipeServer::IsOpened()
	{
		return m_Pipe != nullptr && m_Pipe->IsPipeOpened();
	}

	void PipeServer::Close()
	{
		m_Pipe->Close();
	}

}