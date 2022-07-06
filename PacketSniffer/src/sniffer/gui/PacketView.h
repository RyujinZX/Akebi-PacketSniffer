#pragma once

#include <sniffer/packet/Packet.h>
#include <sniffer/packet/ModifyType.h>
#include <sniffer/gui/PacketFlag.h>

namespace sniffer::gui
{
	class PacketView
	{
	public:
		struct ShowOptions
		{
			bool show_unknown = true;
			bool show_unsetted = true;
		};

		PacketView(const packet::Packet* packet);

		std::string const& name() const;
		uint32_t messageID() const;
		int64_t uid() const;
		packet::NetIODirection direction() const;

		bool unknown() const;

		int64_t timestamp() const;
		tm time() const;
		std::string const& timeStr() const;

		size_t size() const;

		ProtoMessage const& content() const;
		std::vector<byte> const& raw_content() const;
		nlohmann::json const& json_content(ShowOptions options = {}) const;
		std::string const& str_content(ShowOptions options = {}) const;

		ProtoMessage const& head() const;
		std::vector<byte> const& raw_head() const;
		nlohmann::json& json_header(ShowOptions options = {}) const;
		std::string const& str_header(ShowOptions options = {}) const;

		std::unordered_map<std::string, PacketFlag>& flags();
		const std::unordered_map<std::string, PacketFlag>& flags() const;

		const packet::Packet* data() const;

		bool is_content_beauty;
		bool is_header_beauty;

		bool operator==(const PacketView& other) const;

	private:

		tm m_Time;
		std::string m_TimeStr;

		mutable std::array<nlohmann::json, 4> m_ContentJsonCache;
		mutable std::array<std::string, 8> m_ContentStringCache;

		mutable std::array<nlohmann::json, 4> m_HeaderJsonCache;
		mutable std::array<std::string, 8> m_HeaderStringCache;

		std::unordered_map<std::string, PacketFlag> m_Flags;

		const packet::Packet* m_Data;

		bool m_IsUnknown;

		void UpdateTemp();
		void UpdateTime();
	};
}


