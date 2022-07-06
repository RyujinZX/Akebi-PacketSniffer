#include "pch.h"
#include "PacketModifyWnd.h"

#include <sniffer/gui/util.h>
#include <sniffer/packet/PacketManager.h>

namespace sniffer::gui
{
	PacketModifyWnd::PacketModifyWnd() { }

	PacketModifyWnd& PacketModifyWnd::instance()
	{
		static PacketModifyWnd instance;
		return instance;
	}

	void PacketModifyWnd::Draw()
	{
		auto& profiler = packet::PacketManager::GetModifySelectorProfiler();
		DrawProfiler("Profiles", profiler);
		DrawScriptSelector(&profiler.current());
	}

	WndInfo& PacketModifyWnd::GetInfo()
	{
		static WndInfo info = { "Packet modify", false };
		return info;
	}
}