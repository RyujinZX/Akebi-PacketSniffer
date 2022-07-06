#include "pch.h"
#include "PacketFlag.h"

namespace sniffer::gui
{
	PacketFlag::PacketFlag()
	{

	}

	PacketFlag::PacketFlag(const std::string& name, const std::string& desc, ImColor color, const nlohmann::json& info /*= {}*/)
		: m_Name(name), m_Desc(desc), m_Color(color), m_Info(info)
	{

	}

	std::string const& PacketFlag::name() const
	{
		return m_Name;
	}


	std::string const& PacketFlag::desc() const
	{
		return m_Desc;
	}

	ImColor PacketFlag::color() const
	{
		return m_Color;
	}

	nlohmann::json const& PacketFlag::info() const
	{
		return m_Info;
	}

	void PacketFlag::toJson(nlohmann::json& j) const
	{
		j = nlohmann::json
		{
			{ "name", m_Name },
			{ "desc", m_Desc },
			{ "color", (ImU32)m_Color },
			{ "info", m_Info }
		};
	}

	void PacketFlag::fromJson(const nlohmann::json& j)
	{
		j.at("name").get_to(m_Name);
		j.at("desc").get_to(m_Desc);
		m_Color = ImColor(j.at("color").get<ImU32>());
		j.at("info").get_to(m_Info);
	}

	bool PacketFlag::operator==(const PacketFlag& other) const
	{
		return this == &other;
	}

	void to_json(nlohmann::json& j, const PacketFlag& pf)
	{
		pf.toJson(j);
	}

	void from_json(const nlohmann::json& j, PacketFlag& pf)
	{
		pf.fromJson(j);
	}

}