#pragma once
#include "IWindow.h"

namespace sniffer::gui
{
	class SettingsWnd : public IWindow
	{
	public:
		static SettingsWnd& instance();
		void Draw() override;

		WndInfo& GetInfo() override;

	private:
		SettingsWnd();

	public:
		SettingsWnd(SettingsWnd const&) = delete;
		void operator=(SettingsWnd const&) = delete;
	};
}


