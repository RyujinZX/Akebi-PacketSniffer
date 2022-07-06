#pragma once
#include <string>
#include <cheat-base/config/config.h>
#include <cheat-base/events/event.hpp>
#include <sniffer/gui/IWindow.h>

namespace sniffer::gui
{
	class SnifferWnd
	{
	public:

		SnifferWnd(SnifferWnd const&) = delete;
		void operator=(SnifferWnd const&) = delete;

		static SnifferWnd& instance();
		void Draw();

	private:
		SnifferWnd();
		std::vector<std::pair<IWindow*, config::Field<bool>>> m_WindowsToggle;
	};
}

