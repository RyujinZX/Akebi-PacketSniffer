#pragma once
#include <imgui.h>
#include <string>

namespace sniffer::gui
{
	struct WndInfo
	{
		std::string name;
		bool defaultOpened;
		std::string title;
		ImVec2 defaultSize;
		ImGuiWindowFlags flags;
	};

	class IWindow
	{
	public:
		virtual WndInfo& GetInfo() = 0;
		virtual void Draw() = 0;
	};
}