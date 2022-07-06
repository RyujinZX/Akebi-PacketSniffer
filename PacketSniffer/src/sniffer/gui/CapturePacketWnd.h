#pragma once
#include "PacketWndBase.h"

namespace sniffer::gui
{
	class CapturePacketWnd :
		public PacketWndBase
	{
	public:
		static CapturePacketWnd& instance();

		WndInfo& GetInfo() final;
		void Clear() final;

	private:
		void DrawContextMenu(const CMItemInfo& CMInfo) final;
		CapturePacketWnd();

		bool IsPacketFiltered(const PacketView& info) final;

		void OnPacketModified(const packet::Packet* packet, packet::ModifyType modifyType, packet::RawPacketData _);

		void FillViewFlags(PacketView* view);
		void FillViewModifyFlags(PacketView* view, packet::ModifyType modifyType);

		void OnUnknownFieldViewChanged(bool& newValue);
		void OnPacketReceived(const packet::Packet* packet, uint64_t number);
		void OnFilterChanged();
	public:
		CapturePacketWnd(CapturePacketWnd const&) = delete;
		void operator=(CapturePacketWnd const&) = delete;
	};
}


