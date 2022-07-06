#pragma once
#include <vector>

#include <sniffer/pipe/PipeIO.h>

namespace sniffer::pipe
{
	class PipeServer : public PipeIO
	{
	public:
		PipeServer() : PipeIO(), m_ClientConnected(false) {}

		bool IsConnected() final;

		bool Open(const std::string& pipeName);
		bool WaitForConnection();

		bool IsOpened();
		void Close();
	private:

		mutable bool m_ClientConnected;
	};
}