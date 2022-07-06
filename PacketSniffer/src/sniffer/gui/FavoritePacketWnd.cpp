#include "pch.h"
#include "FavoritePacketWnd.h"

#include <sniffer/packet/PacketManager.h>

namespace sniffer::gui
{

	FavoritePacketWnd& FavoritePacketWnd::instance()
	{
		static FavoritePacketWnd instance;
		return instance;
	}

	WndInfo& FavoritePacketWnd::GetInfo()
	{
		static WndInfo info = { "Favorite packets", true };
		return info;
	}

	FavoritePacketWnd::FavoritePacketWnd() 
	{
	
		for (auto& favoritePacket : packet::PacketManager::GetFavoritePackets())
		{
			AddPacket(&favoritePacket);
		}

		packet::PacketManager::FavoritePacketAdded += MY_METHOD_HANDLER(FavoritePacketWnd::OnFavoritePacketAdd);
		packet::PacketManager::FavoritePacketPreRemove += MY_METHOD_HANDLER(FavoritePacketWnd::OnFavoritePacketPreRemove);
	}

	void FavoritePacketWnd::DrawContextMenu(const CMItemInfo& CMInfo)
	{
		if (ImGui::MenuItem("Remove favorite"))
		{
			RemovePacket(CMInfo.packet->data());
		}
	}

	void FavoritePacketWnd::OnFavoritePacketAdd(const packet::Packet* packet)
	{
		AddPacket(packet);
	}

	void FavoritePacketWnd::OnFavoritePacketPreRemove(const packet::Packet* packet)
	{
		if (GetView(packet) != nullptr)
			RemovePacket(packet);
	}

	void FavoritePacketWnd::RemovePacket(const packet::Packet* packet)
	{
		PacketWndBase::RemovePacket(packet);
		packet::PacketManager::RemoveFavoritePacket(packet);
	}

	void FavoritePacketWnd::Clear()
	{
		PacketWndBase::Clear();
		packet::PacketManager::ClearFavoritePackets();
	}

}