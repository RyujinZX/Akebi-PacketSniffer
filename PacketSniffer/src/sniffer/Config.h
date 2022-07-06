#pragma once

namespace sniffer
{
	class Config
	{
	public:
		enum class ProtoIDMode
		{
			SeparateFile,
			InCMDIDField
		};

		static Config& instance();

		config::Field<bool> f_Fullscreen;
		config::Field<std::string> f_ProtoDirPath;
		config::Field<std::string> f_ProtoIDsPath;
		config::Field<config::Enum<ProtoIDMode>> f_ProtoIDMode;
		config::Field<bool> f_ScrollFollowing;
		config::Field<bool> f_ShowUnknownFields;
		config::Field<bool> f_ShowUnknownPackets;
		config::Field<bool> f_ShowUnsettedFields;
		config::Field<bool> f_HighlightRelativities;
		config::Field<bool> f_PacketLevelFilter;

	private:
		Config();

	public:
		Config(Config&) = delete;
		Config(Config&&) = delete;

		void operator=(Config&) = delete;
		void operator=(Config&&) = delete;
	};
}


