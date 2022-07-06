#include "pch.h"
#include "SettingsWnd.h"

#include <sniffer/Config.h>

namespace sniffer::gui
{
	SettingsWnd::SettingsWnd()
	{ }

	SettingsWnd& SettingsWnd::instance()
	{
		static SettingsWnd instance;
		return instance;
	}

	void SettingsWnd::Draw()
	{
		auto& config = Config::instance();
		if (ImGui::BeginGroupPanel("Proto"))
		{
			ConfigWidget(config.f_ProtoIDMode, "The mode searching the id's for .proto");

			static bool isChanging = false;
			static std::string protoDirPathTemp = config.f_ProtoDirPath;
			static std::string protoIDsPathTemp = config.f_ProtoIDsPath;

			if (!isChanging)
				ImGui::BeginDisabled();

			ImGui::InputText("Proto dir", &protoDirPathTemp);

			if (config.f_ProtoIDMode.value() == Config::ProtoIDMode::SeparateFile)
				ImGui::InputText("Proto IDs", &protoIDsPathTemp);

			if (!isChanging)
				ImGui::EndDisabled();

			if (isChanging)
			{
				if (ImGui::Button("Save"))
				{
					if (protoDirPathTemp != config.f_ProtoDirPath.value() || protoIDsPathTemp != config.f_ProtoIDsPath.value())
					{
						config.f_ProtoDirPath = protoDirPathTemp;
						config.f_ProtoIDsPath = protoIDsPathTemp;
					}
					isChanging = false;
				}

				if (ImGui::Button("Cancel"))
				{
					protoDirPathTemp = config.f_ProtoDirPath;
					protoIDsPathTemp = config.f_ProtoIDsPath;
					isChanging = false;
				}
			}
			else
			{
				if (ImGui::Button("Change"))
				{
					isChanging = true;
				}
			}
		}
		ImGui::EndGroupPanel();

		ConfigWidget(config.f_ShowUnknownPackets, "Show unknown packets in capture list.");
		ConfigWidget(config.f_ShowUnknownFields, "Show unknown fields in capture list.");
		ConfigWidget(config.f_ShowUnsettedFields, "Show fields even the their data wasn't passed.");
		ConfigWidget(config.f_ScrollFollowing, "Following for items when new data appear above the scroll region.");

		ConfigWidget(config.f_HighlightRelativities, "Highlight the packet relativities in packet capture window.");
		ConfigWidget(config.f_PacketLevelFilter, "Filtering will be execute on packet level,\n so packet will not save if it don't accept filter conditions."
			"\nFiltered packet will not passed to modify scripts.\nIt helps to reduce memory consumption.");
	}

	WndInfo& SettingsWnd::GetInfo()
	{
		static WndInfo info = { "Settings", true };
		return info;
	}
}