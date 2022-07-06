#include "pch.h"
#include "CapturePacketWnd.h"

#include <sniffer/Config.h>
#include <sniffer/packet/PacketManager.h>
#include <sniffer/filter/FilterManager.h>
namespace sniffer::gui
{

	CapturePacketWnd::CapturePacketWnd() 
	{
		auto& settings = Config::instance();
		settings.f_ShowUnknownPackets.FieldChangedEvent += MY_METHOD_HANDLER(CapturePacketWnd::OnUnknownFieldViewChanged);
		filter::FilterManager::MainFilterChangedEvent += MY_METHOD_HANDLER(CapturePacketWnd::OnFilterChanged);
		packet::PacketManager::PacketReceiveEvent += MY_METHOD_HANDLER(CapturePacketWnd::OnPacketReceived);
		packet::PacketManager::PacketModifiedEvent += MY_METHOD_HANDLER(CapturePacketWnd::OnPacketModified);
	}

	bool CapturePacketWnd::IsPacketFiltered(const PacketView& info)
	{
		auto& settings = Config::instance();
		return (settings.f_PacketLevelFilter ? true : filter::FilterManager::Execute(*info.data())) 
			&& (settings.f_ShowUnknownPackets || !info.unknown());
	}

	void CapturePacketWnd::OnUnknownFieldViewChanged(bool& newValue)
	{
		ExecuteFilter();
	}

	CapturePacketWnd& CapturePacketWnd::instance()
	{
		static CapturePacketWnd instance;
		return instance;
	}

	WndInfo& CapturePacketWnd::GetInfo()
	{
		static WndInfo info = { "Capture packets", true };
		return info;
	}

	void CapturePacketWnd::DrawContextMenu(const CMItemInfo& CMInfo)
	{
		auto& fav = packet::PacketManager::GetFavoritePackets();
		auto it = std::find(fav.begin(), fav.end(), *CMInfo.packet->data());
		bool is_favorite = it != fav.end();
		if (!is_favorite)
		{
			if (ImGui::MenuItem("Add to favorites"))
				packet::PacketManager::EmplaceFavoritePacket(*CMInfo.packet->data());
		}
		else
		{
			if (ImGui::MenuItem("Remove from favorites"))
				packet::PacketManager::RemoveFavoritePacket(*CMInfo.packet->data());
		}
	}

	void CapturePacketWnd::OnPacketReceived(const packet::Packet* packet, uint64_t number)
	{
		auto& newView = EmplacePacket(packet);
		FillViewFlags(&newView);
	}

	void CapturePacketWnd::OnFilterChanged()
	{
		ExecuteFilter();
	}

	void CapturePacketWnd::Clear()
	{
		PacketWndBase::Clear();
		packet::PacketManager::Clear();
	}

	static ImColor _colorPallete[] = {
		ImColor(72, 86, 150),
		ImColor(231, 231, 231),
		ImColor(249, 199, 132),
		ImColor(252, 122, 30),
		ImColor(242, 76, 0),
		ImColor(136, 73, 143),
		ImColor(119, 159, 161),
		ImColor(224, 203, 168),
		ImColor(255, 101, 66),
		ImColor(86, 65, 84),
		ImColor(205, 247, 246),
		ImColor(143, 184, 222),
		ImColor(154, 148, 188),
		ImColor(155, 80, 148),
		ImColor(234, 140, 85)
	};

	void CapturePacketWnd::FillViewFlags(PacketView* newView)
	{
		auto& newViewContent = newView->content();
		auto& newViewFlags = newView->flags();
		if (newViewContent.has_flag(ProtoMessage::Flag::HasUnknown))
		{
			auto flag = PacketFlag("Unk", "This packet has unknown fields.", ImColor(114, 131, 158));
			newViewFlags[flag.name()] = flag;
		}

		if (newViewContent.has_flag(ProtoMessage::Flag::IsUnpacked))
		{
			auto parentPacket = packet::PacketManager::GetParentPacket(newView->uid());
			if (parentPacket != nullptr)
			{
				auto flag = PacketFlag("Pkg", fmt::format("This packet was unpacked from packet {}.", parentPacket->uid()), ImColor(185, 133, 192),
					{ { "rel", parentPacket->uid() } });
				newViewFlags[flag.name()] = flag;
			}
		}

		ImColor color = _colorPallete[newView->uid() % (sizeof(_colorPallete) / sizeof(_colorPallete[0]))];

		auto relatedPackets = packet::PacketManager::GetRelatedPackets(newView->data());
		if (relatedPackets.size() < 2)
			return;

		if (relatedPackets.size() == 2)
		{
			auto request = *relatedPackets.begin();
			auto requestView = GetView(request);
			if (requestView == nullptr)
				return;

			auto& requestFlags = requestView->flags();
			auto request_flag = PacketFlag("Req", fmt::format("Have a linked response with sequence id {}", newView->uid()), color,
				{
				{ "rel", newView->uid()}
				});
			requestFlags[request_flag.name()] = request_flag;

			auto responce_flag = PacketFlag("Rsp", fmt::format("Have a linked request with sequence id {}", newView->uid()), color,
				{
				{ "rel", request->uid()}
				});
			newViewFlags[responce_flag.name()] = responce_flag;
			return;
		}

		std::vector<int64_t> relatedPacketsIds;
		for (auto& packetView : relatedPackets)
		{
			relatedPacketsIds.push_back(packetView->uid());
		}
		relatedPacketsIds.push_back(newView->uid());

		auto relatedFlag = PacketFlag("Rel", fmt::format("Have several packet with the same sequence id."), color,
			{
				{ "rel", std::move(relatedPacketsIds) }
			}
		);

		bool needChangeFlag = relatedPackets.size() == 2;
		if (needChangeFlag)
		{
			auto& firstFlags = GetView(*relatedPackets.begin())->flags();
			auto& secondFlags = GetView(relatedPackets.at(1))->flags();

			firstFlags.erase("Req");
			secondFlags.erase("Rsp");
		}

		for (auto& packet : relatedPackets)
		{
			auto& flags = GetView(packet)->flags();
			flags[relatedFlag.name()] = relatedFlag;
		}	
	}

	void CapturePacketWnd::FillViewModifyFlags(PacketView* view, packet::ModifyType modifyType)
	{
		auto& flags = view->flags();
		if (modifyType == packet::ModifyType::Blocked)
		{
			auto flag = PacketFlag("Blk", "This packet was blocked by sniffer.", ImColor(255, 127, 36));
			flags[flag.name()] = flag;
		}
		else if (modifyType == packet::ModifyType::Modified)
		{
			auto flag = PacketFlag("Mod", "This packet was modified.", ImColor(255, 36, 218));
			flags[flag.name()] = flag;
		}
	}

	void CapturePacketWnd::OnPacketModified(const packet::Packet* packet, packet::ModifyType modifyType, packet::RawPacketData _)
	{
		auto view = GetView(packet);
		if (view != nullptr)
			FillViewModifyFlags(view, modifyType);
	}
}