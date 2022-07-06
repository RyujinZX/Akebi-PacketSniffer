#include "pch.h"
#include "Config.h"

namespace sniffer
{
	Config::Config() :
		NF(f_ProtoIDMode, "Proto ID mode", "Sniffer::Settings", ProtoIDMode::SeparateFile),
		NF(f_ProtoDirPath, "Proto dir", "Sniffer::Settings", ""),
		NF(f_ProtoIDsPath, "Proto IDs", "Sniffer::Settings", ""),

		NF(f_ScrollFollowing, "Scroll following", "Sniffer::Settings", true),
		NF(f_Fullscreen, "Fullscreen", "Sniffer::Settings", true),
		NF(f_ShowUnknownFields, "Show unknown fields", "Sniffer::Settings", true),
		NF(f_ShowUnknownPackets, "Show unknown packets", "Sniffer::Settings", true),
		NF(f_ShowUnsettedFields, "Show unsetted fields", "Sniffer::Settings", true),
		NF(f_HighlightRelativities, "Highlight relativities", "Sniffer::Settings", true),
		NF(f_PacketLevelFilter, "Packet level filter", "Sniffer::Settings", false)

	{ }

	Config& Config::instance()
	{
		static Config instance;
		return instance;
	}
}