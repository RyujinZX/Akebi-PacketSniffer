#include <pch.h>
#include "sniffer.h"

#include <cheat-base/globals.h>
#include <sniffer/packet/PacketManager.h>
#include <sniffer/pipe/PipeHandler.h>

#include <sniffer/script/ScriptManager.h>
#include <sniffer/filter/FilterManager.h>

#include <sniffer/gui/SnifferWnd.h>

namespace sniffer
{
	static void OnRender();

	void Run()
	{
		Logger::SetLevel(Logger::Level::Debug);

		config::Initialize("cfg.json");
		config::SetupUpdate(&events::RenderEvent);

		events::RenderEvent += FUNCTION_HANDLER(OnRender);
		
		script::ScriptManager::Run();
		filter::FilterManager::Run();

		packet::PacketManager::SetHandler(new pipe::PipeHandler());
		packet::PacketManager::Run();
	}

	static void OnRender()
	{
		packet::PacketManager::Update();
		script::ScriptManager::Update();
		
		auto& wnd = gui::SnifferWnd::instance();
		wnd.Draw();
	}
}