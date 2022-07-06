#include <pch.h>
#include "SnifferWnd.h"

#include <psapi.h>

#include <imgui_internal.h>

#include <sniffer/packet/PacketManager.h>
#include <sniffer/Config.h>
#include <sniffer/gui/FavoritePacketWnd.h>
#include <sniffer/gui/CapturePacketWnd.h>
#include <sniffer/gui/SettingsWnd.h>
#include <sniffer/gui/FilterWnd.h>
#include <sniffer/gui/PacketModifyWnd.h>
#include <sniffer/gui/ScriptManagerWnd.h>

#include <sniffer/gui/ScriptEditor.h>

namespace sniffer::gui
{
	static size_t _memoryUsage = 0;
	static int _memoryUsageDiff = 0;

	static size_t _packetCount = 0;
	static int _packetCountDiff = 0;

	SnifferWnd::SnifferWnd() 
	{
		auto& filterWnd = FilterWnd::instance();

#define ADD_WINDOW(cls) 																								\
		{																												\
			auto& info = cls::instance().GetInfo();																		\
			m_WindowsToggle.push_back({ &cls::instance(), 																\
				config::CreateField<bool>(info.name, info.name, "sniffer::gui", false, info.defaultOpened) });\
		}

		ADD_WINDOW(ScriptManagerWnd);
		ADD_WINDOW(PacketModifyWnd);
		ADD_WINDOW(SettingsWnd);
		ADD_WINDOW(FilterWnd);
		ADD_WINDOW(FavoritePacketWnd);
		ADD_WINDOW(CapturePacketWnd);

#undef ADD_WINDOW
	}


	SnifferWnd& SnifferWnd::instance()
	{
		static SnifferWnd instance;
		return instance;
	}

#undef min
#undef max

	static void UpdatePacketCount()
	{
		UPDATE_DELAY(500);
		
		size_t newSize = packet::PacketManager::GetPacketCount();
		_packetCountDiff = newSize - _packetCount;
		_packetCount = newSize;
	}

	static void UpdateMemoryUsage()
	{
		UPDATE_DELAY(2000);

		PROCESS_MEMORY_COUNTERS counters {};
		if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
			return;

		size_t newSize = counters.WorkingSetSize / static_cast<SIZE_T>(1024 * 1024);

		_memoryUsageDiff = newSize - _memoryUsage;
		_memoryUsage = newSize;
	}

	void SnifferWnd::Draw()
	{

		Config& config = Config::instance();
		// Create dock-space
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (config.f_Fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Packet sniffer", nullptr, window_flags);
		ImGui::PopStyleVar();

		if (config.f_Fullscreen)
			ImGui::PopStyleVar(2);

		ImGuiIO& io = ImGui::GetIO();
		IM_ASSERT(io.ConfigFlags & ImGuiConfigFlags_DockingEnable);

		ImGuiID dockspace_id = ImGui::GetID("PSDS");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Options"))
			{
				ImGui::MenuItem("Fullscreen", nullptr, config.f_Fullscreen.pointer());
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Windows"))
			{
				for (auto& [window, enabled] : m_WindowsToggle)
				{
					if (ImGui::MenuItem(window->GetInfo().name.c_str(), nullptr, enabled.pointer()))
					{
						enabled.FireChanged();
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Credits"))
			{
				if (ImGui::MenuItem("Callow | Github page"))
				{
					util::OpenURL("https://github.com/Akebi-Group/Akebi-GC");
				}
				ImGui::EndMenu();
			}

			float currX = ImGui::GetContentRegionMax().x;
			const char* connectionStatusText = packet::PacketManager::IsConnected() ? "Connected" : "Disconnected";
			ImColor connectionStatusColor = packet::PacketManager::IsConnected() ? ImColor(0.08f, 0.89f, 0.09f) : ImColor(0.90f, 0.05f, 0.12f);
			currX -= ImGui::CalcTextSize(connectionStatusText).x + ImGui::GetStyle().ItemSpacing.x;
			ImGui::SameLine(currX, 0.0f);
			ImGui::TextColored(connectionStatusColor, connectionStatusText);

			UpdateMemoryUsage();
			auto memoryUsageText = fmt::format("MemUsage: {:d} MB | {:+d}", _memoryUsage, _memoryUsageDiff);
			currX -= ImGui::CalcTextSize(memoryUsageText.c_str()).x + ImGui::GetStyle().ItemSpacing.x;
			ImGui::SameLine(currX, 0.0f);
			ImGui::Text(memoryUsageText.c_str());

			UpdatePacketCount();
			auto packetCountText = fmt::format("Packets: {:d} | {:+d}", _packetCount, _packetCountDiff);
			currX -= ImGui::CalcTextSize(packetCountText.c_str()).x + ImGui::GetStyle().ItemSpacing.x;
			ImGui::SameLine(currX, 0.0f);
			ImGui::Text(packetCountText.c_str());

			ImGui::EndMenuBar();
		}
		ImGui::End();

		for (auto& [window, enabled] : m_WindowsToggle)
		{
			if (!enabled)
				continue;

			auto& info = window->GetInfo();
			if (ImGui::Begin(info.name.c_str(), enabled.pointer(), info.flags))
			{
				window->Draw();
			}
			ImGui::End();

			if (!enabled)
				enabled.FireChanged();
		}

		ScriptEditorManager::Draw();
	}
}