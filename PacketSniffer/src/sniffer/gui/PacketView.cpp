#include "pch.h"
#include "PacketView.h"

#include <sniffer/packet/PacketManager.h>

namespace sniffer::gui
{

	PacketView::PacketView(const packet::Packet* packet) : m_Data(packet), is_header_beauty(false), is_content_beauty(false)
	{
		UpdateTime();
		UpdateTemp();
	}

	std::string const& PacketView::name() const
	{
		return m_Data->name();
	}

	uint32_t PacketView::messageID() const
	{
		return m_Data->mid();
	}

	int64_t PacketView::uid() const
	{
		return m_Data->uid();
	}

	packet::NetIODirection PacketView::direction() const
	{
		return m_Data->direction();
	}

	bool PacketView::unknown() const
	{
		return m_IsUnknown;
	}

	int64_t PacketView::timestamp() const
	{
		return m_Data->timestamp();
	}

	tm PacketView::time() const
	{
		return m_Time;
	}

	std::string const& PacketView::timeStr() const
	{
		return m_TimeStr;
	}

	size_t PacketView::size() const
	{
		return raw_content().size() + raw_head().size();
	}

	ProtoMessage const& PacketView::content() const
	{
		return m_Data->content();
	}

	std::vector<byte> const& PacketView::raw_content() const
	{
		return m_Data->raw().content;
	}

	nlohmann::json const& PacketView::json_content(ShowOptions options /*= {}*/) const
	{
		int cache_index = options.show_unsetted | (options.show_unknown << 1);
		auto& cached_json = m_ContentJsonCache[cache_index];
		if (cached_json.is_null())
			content().to_view_json(cached_json, options.show_unknown, options.show_unsetted);

		return cached_json;
	}

	std::string const& PacketView::str_content(ShowOptions options /*= {}*/) const
	{
		int cache_index = options.show_unsetted | (options.show_unknown << 1) | (is_content_beauty << 2);
		auto& cached_str = m_ContentStringCache[cache_index];
		if (cached_str.empty())
			cached_str = json_content(options).dump(is_content_beauty ? 2 : -1);

		return cached_str;
	}

	ProtoMessage const& PacketView::head() const
	{
		return m_Data->head();
	}

	std::vector<byte> const& PacketView::raw_head() const
	{
		return m_Data->raw().head;
	}

	nlohmann::json& PacketView::json_header(ShowOptions options /*= {}*/) const
	{
		int cache_index = options.show_unsetted | (options.show_unknown << 1);
		auto& cached_json = m_HeaderJsonCache[cache_index];
		if (cached_json.is_null())
			head().to_view_json(cached_json, options.show_unknown, options.show_unsetted);

		return cached_json;
	}

	std::string const& PacketView::str_header(ShowOptions options /*= {}*/) const
	{
		int cache_index = options.show_unsetted | (options.show_unknown << 1) | (is_header_beauty << 2);
		auto& cached_str = m_HeaderStringCache[cache_index];
		if (cached_str.empty())
			cached_str = json_header(options).dump(is_header_beauty ? 2 : -1);

		return cached_str;
	}

	const std::unordered_map<std::string, PacketFlag>& PacketView::flags() const
	{
		return m_Flags;
	}

	std::unordered_map<std::string, PacketFlag>& PacketView::flags()
	{
		return m_Flags;
	}

	const packet::Packet* PacketView::data() const
	{
		return m_Data;
	}

	void PacketView::UpdateTemp()
	{
		m_IsUnknown = name() == "<unknown>";
		UpdateTime();
	}

	void PacketView::UpdateTime()
	{
		time_t rawTime = timestamp() / 1000;
		struct tm gmtm;
		localtime_s(&gmtm, &rawTime);

		m_Time = gmtm;

		std::stringstream buffer{};
		buffer << std::put_time(&gmtm, "%H:%M:%S");
		m_TimeStr = buffer.str();
	}

	bool PacketView::operator==(const PacketView& other) const
	{
		return data() == other.data();
	}



	//void PacketView::SetupFlags()
	//{

	//}


}