#pragma once
#include <sniffer/gui/IWindow.h>
namespace sniffer::gui
{
	class ScriptManagerWnd :
		public IWindow
	{
	public:
		static ScriptManagerWnd& instance();

		WndInfo& GetInfo() override;
		void Draw() override;

	private:
		ScriptManagerWnd();

	public:
		ScriptManagerWnd(ScriptManagerWnd const&) = delete;
		void operator=(ScriptManagerWnd const&) = delete;
	};

}

