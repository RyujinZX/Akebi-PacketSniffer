#pragma once
#include "PacketWndBase.h"

namespace sniffer::gui
{
    class FavoritePacketWnd :
        public PacketWndBase
    {
    public:
        static FavoritePacketWnd& instance();

        WndInfo& GetInfo() override;

        void RemovePacket(const packet::Packet* packet) final;
        void Clear() final;

    private:
        FavoritePacketWnd();

        void DrawContextMenu(const CMItemInfo& CMInfo) final;
        void OnFavoritePacketAdd(const packet::Packet* packet);
        void OnFavoritePacketPreRemove(const packet::Packet* packet);

	public:
        FavoritePacketWnd(FavoritePacketWnd const&) = delete;
		void operator=(FavoritePacketWnd const&) = delete;

    };
}
