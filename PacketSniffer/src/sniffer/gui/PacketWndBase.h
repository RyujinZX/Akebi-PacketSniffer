#pragma once

#include <imgui.h>

#include <sniffer/gui/PacketView.h>
#include <sniffer/gui/IWindow.h>

#include <sniffer/filter/FilterGroup.h>

namespace sniffer::gui
{
	class PacketWndBase : public IWindow
	{
	public:
		void Draw() override;
		virtual void AddPacket(const packet::Packet* packet);
		virtual void RemovePacket(const packet::Packet* packet);

		virtual void Clear();
	protected:
		PacketWndBase();

		enum HeaderColumn : ImGuiID
		{
			HeaderColumn_Time,
			HeaderColumn_IO,
			HeaderColumn_UID,
			HeaderColumn_MID,
			HeaderColumn_Name,
			HeaderColumn_Size,
			HeaderColumn_Flags,
			HeaderColumn_None
		};

		// [CM] - ContextMenu
		enum class CMItemType
		{
			None,
			Header,
			KeyValue
		};

		struct CMKeyValue
		{
			std::string key;
			std::string value;
		};

		struct CMItemInfo
		{
			CMItemType itemType;
			HeaderColumn column;
			CMKeyValue keyValue;
			const PacketView* packet;
		};

		bool m_CMOpened;
		CMItemInfo m_CurrentCMInfo;
		virtual void DrawContextMenu(const CMItemInfo& cmInfo);

	protected:

		std::list<PacketView> m_Packets;

		std::unordered_map<const packet::Packet*, PacketView*> m_PacketMap;
		std::list<PacketView*> m_FilteredPackets;

		// Sort
		HeaderColumn m_SortColumn;
		ImGuiSortDirection m_SortDirection;
		
		// Scroll following
		bool m_SFNeedUpdate;
		bool m_SFIndexOdd;
		float m_SFLastScrollY;
		size_t m_SFItemIndex;
		int64_t m_SFItemSeqID;

		using ContextMenuDrawFunc = filter::FilterGroup* (*)(const PacketView& info);
		static std::map<PacketWndBase::HeaderColumn, ContextMenuDrawFunc> s_ColumnCMFunc;
		static std::vector<ContextMenuDrawFunc> s_CMFuncs;

		bool SFUpdate();
		bool SFCheckItem(size_t index, int64_t seqID);

		static void InitTableColumns();
		void DrawHeader();
		void DrawContent();

		bool DrawPacketRow(const PacketView& info);
		void DrawPacketContent(PacketView* info);

		void SortColumn(HeaderColumn column, ImGuiSortDirection sortDirection);
		virtual bool IsPacketFiltered(const PacketView& info);
		void ExecuteFilter();

		PacketView* GetView(const packet::Packet* packet);
		PacketView& EmplacePacket(const packet::Packet* packet);

	};
}