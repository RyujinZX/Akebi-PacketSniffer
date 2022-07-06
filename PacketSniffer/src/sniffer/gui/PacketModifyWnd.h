#pragma once

#include "IWindow.h"
#include "PacketView.h"

namespace sniffer::gui
{
	class PacketModifyWnd : public IWindow
	{
	public:
		static PacketModifyWnd& instance();
		void Draw() override;

		WndInfo& GetInfo() override;

	private:
		PacketModifyWnd();
	public:
		PacketModifyWnd(PacketModifyWnd const&) = delete;
		void operator=(PacketModifyWnd const&) = delete;
	};
}


