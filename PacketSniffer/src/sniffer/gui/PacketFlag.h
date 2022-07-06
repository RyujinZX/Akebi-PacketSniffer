#pragma once
#include <nlohmann/json.hpp>
namespace sniffer::gui
{
	class PacketFlag
	{
	public:
		PacketFlag();
		PacketFlag(const std::string& name, const std::string& desc, ImColor color, const nlohmann::json& info = {});

		std::string const& name() const;
		std::string const& desc() const;
		ImColor color() const;

		nlohmann::json const& info() const;

		void toJson(nlohmann::json& j) const;
		void fromJson(const nlohmann::json& j);

		bool operator==(const PacketFlag& other) const;
	private:

		std::string m_Name;
		std::string m_Desc;
		ImColor m_Color;
		nlohmann::json m_Info;
	};

	void to_json(nlohmann::json& j, const PacketFlag& pf);
	void from_json(const nlohmann::json& j, PacketFlag& pf);
}