#include "pch.h"
#include "PacketWndBase.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <sniffer/Config.h>
#include <sniffer/filter/FilterManager.h>
#include <sniffer/filter/comparers.h>
#include <sniffer/packet/PacketManager.h>

namespace sniffer::gui
{
	static std::unordered_set<int64_t> s_RelatedIDs;
	static int64_t s_SelectedPacketID = 0;

	static filter::FilterGroup* DrawTimeContextMenu(const PacketView& info);
	static filter::FilterGroup* DrawSizeContextMenu(const PacketView& info);
	static filter::FilterGroup* DrawUIDContextMenu(const PacketView& info);
	static filter::FilterGroup* DrawNameContextMenu(const PacketView& info);

	std::map<PacketWndBase::HeaderColumn, PacketWndBase::ContextMenuDrawFunc> PacketWndBase::s_ColumnCMFunc = {
		{ HeaderColumn_Time, DrawTimeContextMenu},
		{ HeaderColumn_UID,  DrawUIDContextMenu },
		{ HeaderColumn_Name, DrawNameContextMenu },
		{ HeaderColumn_Size, DrawSizeContextMenu }
	};

	std::vector<PacketWndBase::ContextMenuDrawFunc> PacketWndBase::s_CMFuncs = {
		DrawTimeContextMenu, DrawSizeContextMenu, DrawUIDContextMenu,
		DrawNameContextMenu
	};

#define CALC_LINE_HEIGHT() (ImGui::GetFontSize() + ImGui::GetStyle().CellPadding.y * 2.0f)
#define BEGIN_PACKET_TABLE() ImGui::BeginTable(c_PacketTableName, c_ColumnCount, c_TableFlags)

	constexpr int c_ColumnCount = 7;
	constexpr const char* c_PacketTableName = "packetTable";
	constexpr ImGuiTableFlags c_TableFlags = ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner;

	// Compare lambdas
	static auto _compareTime = [](const PacketView& a, const PacketView& b) { return a.timestamp() < b.timestamp(); };
	static auto _compareMID = [](const PacketView& a, const PacketView& b) { return a.messageID() < b.messageID(); };
	static auto _compareUID = [](const PacketView& a, const PacketView& b) { return a.uid() < b.uid(); };
	static auto _compareName = [](const PacketView& a, const PacketView& b) { return a.name() < b.name(); };
	static auto _compareSize = [](const PacketView& a, const PacketView& b) { return a.size() < b.size(); };

	static auto _compareTimePtr = [](const PacketView* a, const PacketView* b) { return a->timestamp() < b->timestamp(); };
	static auto _compareMIDPtr = [](const PacketView* a, const PacketView* b) { return a->messageID() < b->messageID(); };
	static auto _compareUIDPtr = [](const PacketView* a, const PacketView* b) { return a->uid() < b->uid(); };
	static auto _compareNamePtr = [](const PacketView* a, const PacketView* b) { return a->name() < b->name(); };
	static auto _compareSizePtr = [](const PacketView* a, const PacketView* b) { return a->size() < b->size(); };


	PacketWndBase::PacketWndBase()
		: m_Packets(), m_FilteredPackets(),
		m_SortColumn(HeaderColumn_Time), m_SortDirection(ImGuiSortDirection_Descending),
		m_SFNeedUpdate(false), m_SFLastScrollY(0.0f), m_SFItemIndex(-1), m_SFItemSeqID(-1)
	{ }

	void PacketWndBase::Draw()
	{
		ImGuiWindow* window = GImGui->CurrentWindow;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		float lineHeight = CALC_LINE_HEIGHT();
		float contentPosY = window->InnerRect.Min.y + ImGui::GetStyle().WindowPadding.y;
		float prevClipRectMinY = window->ClipRect.Min.y;
		window->ClipRect.Min.y = contentPosY + lineHeight;

		ImGui::Dummy({ 0.0f, lineHeight });

		// Drawing table content
		DrawContent();

		window->ClipRect.Min.y = prevClipRectMinY;
		window->DC.CursorPos.y = contentPosY; // Absolute position to header
		DrawHeader();
	}

	sniffer::gui::PacketView* PacketWndBase::GetView(const packet::Packet* packet)
	{
		auto it = m_PacketMap.find(packet);
		if (it != m_PacketMap.end())
			return it->second;
		return nullptr;
	}

	void PacketWndBase::AddPacket(const packet::Packet* packet)
	{
		EmplacePacket(packet);
	}

	sniffer::gui::PacketView& PacketWndBase::EmplacePacket(const packet::Packet* packet)
	{
		auto& newView = m_Packets.emplace_back(packet);
		m_PacketMap.emplace(packet, &newView);

		if (m_SortColumn == HeaderColumn_Time)
		{
			if (IsPacketFiltered(newView))
				m_FilteredPackets.push_back(&newView);
			return newView;
		}

#define INSERT_SORTED(column, compare)																									 \
			case column:																												 \
			{																															 \
				m_Packets.insert(std::lower_bound(m_Packets.begin(), m_Packets.end(), newView, compare), newView);	                         \
				if (IsPacketFiltered(newView))																		 \
					m_FilteredPackets.insert(std::lower_bound(m_FilteredPackets.begin(), m_FilteredPackets.end(), &newView, compare##Ptr), &newView); \
			}																															 \
			break;

		switch (m_SortColumn)
		{
			INSERT_SORTED(HeaderColumn_UID, _compareUID);
			INSERT_SORTED(HeaderColumn_MID, _compareMID);
			INSERT_SORTED(HeaderColumn_Name, _compareName);
			INSERT_SORTED(HeaderColumn_Size, _compareSize);
		default:
			break;
		}

		return newView;
	}

	void PacketWndBase::RemovePacket(const packet::Packet* packet)
	{
		auto view = GetView(packet);
		if (view == nullptr)
			return;

		m_PacketMap.erase(packet);
		m_FilteredPackets.remove(view);
		m_Packets.remove(*view);
	}

	void PacketWndBase::Clear()
	{
		m_PacketMap.clear();
		m_FilteredPackets.clear();
		m_Packets.clear();
	}

	void PacketWndBase::DrawContextMenu(const CMItemInfo& CMInfo)
	{ 
	
	}

	bool PacketWndBase::SFUpdate()
	{
		ImGuiWindow* window = GImGui->CurrentWindow;
		bool itemFound = m_SFLastScrollY == window->Scroll.y && !m_SFNeedUpdate && m_SFItemIndex != -1 && window->Scroll.y != 0.0f;
		m_SFLastScrollY = window->Scroll.y;

		if (!itemFound)
			return false;

		auto begin = m_SortDirection == ImGuiSortDirection_Descending ? --m_FilteredPackets.end() : m_FilteredPackets.begin();
		auto end = m_SortDirection == ImGuiSortDirection_Descending ? m_FilteredPackets.begin() : m_FilteredPackets.end();

		// Scrolling position updating
		// Due to ImGui design we should update scrolling before the drawing it
		// So, we just go through all items, and, find index of "attached" line, if it was changed - updating scroll
		size_t itemIndex = -1;
		size_t rowCounter = 0;

		// Finding item index
		for (auto& it = begin; it != end; m_SortDirection == ImGuiSortDirection_Descending ? --it : it++)
		{
			auto& info = *it;
			auto index = rowCounter++;
			if (info->uid() == m_SFItemSeqID)
			{
				itemIndex = index;
				break;
			}
		}

		if (itemIndex == -1)
			return false;

		if (itemIndex == m_SFItemIndex)
			return true;

		// Updating scroll if index changed
		// We also should update the window rects and cursor position to correct drawing
		float offset = (itemIndex - m_SFItemIndex) * CALC_LINE_HEIGHT();

		window->Scroll.y += offset;

		window->ContentRegionRect.Min.y -= offset;
		window->ContentRegionRect.Max.y -= offset;

		window->WorkRect.Min.y -= offset;
		window->WorkRect.Max.y -= offset;

		window->ParentWorkRect.Min.y -= offset;
		window->ParentWorkRect.Max.y -= offset;

		window->DC.CursorPos.y -= offset;
		window->DC.CursorPosPrevLine.y -= offset;
		window->DC.CursorStartPos.y -= offset;
		window->DC.CursorMaxPos.y -= offset;

		m_SFItemIndex = itemIndex;
		m_SFLastScrollY = window->Scroll.y;
		return true;
	}

	bool PacketWndBase::SFCheckItem(size_t index, int64_t seqID)
	{
		ImGuiWindow* window = GImGui->CurrentWindow;
		if (window->DC.CursorPos.y <= window->ClipRect.Min.y)
			return false;

		m_SFItemSeqID = seqID;
		m_SFItemIndex = index;
		m_SFIndexOdd = index & 1;
		return true;
	}

	void PacketWndBase::InitTableColumns()
	{
		ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed,
			0.0f, HeaderColumn_Time);
		ImGui::TableSetupColumn("IO", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort,
			0.0f, HeaderColumn_IO);
		ImGui::TableSetupColumn("UID", ImGuiTableColumnFlags_WidthFixed,
			0.0f, HeaderColumn_UID);
		ImGui::TableSetupColumn("MID", ImGuiTableColumnFlags_WidthFixed,
			0.0f, HeaderColumn_MID);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None,
			0.0f, HeaderColumn_Name);
		ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed,
			0.0f, HeaderColumn_Size);
		ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort,
			0.0f, HeaderColumn_Flags);
	}

	void PacketWndBase::DrawHeader()
	{
		// Header && Sorting
		// Drawing header should be drawed after content because we need to draw it even the table was scrolled
		// So we need draw it in absolute position, and to remove overdrawing by content we put it after content
		if (BEGIN_PACKET_TABLE())
		{
			InitTableColumns();
			ImGui::TableHeadersRow();

			ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs();
			if (sorts_specs != nullptr && (sorts_specs->SpecsDirty))
			{
				auto direction = sorts_specs->Specs->SortDirection;
				auto column = static_cast<HeaderColumn>(sorts_specs->Specs->ColumnIndex);
				SortColumn(column, direction);
				sorts_specs->SpecsDirty = false;
			}

			ImGui::EndTable();
		}
		ImGui::PopStyleVar();
	}

	static void FillRelatedIDs(const std::unordered_map<std::string, PacketFlag>& flags)
	{
		s_RelatedIDs.clear();

		auto& config = Config::instance();
		if (!config.f_HighlightRelativities)
			return;

		for (auto& [name, flag] : flags)
		{
			auto& info = flag.info();
			auto rel_it = info.find("rel");
			if (rel_it == info.end())
				continue;

			if (rel_it->is_array())
			{
				for (auto& sequenceID : *rel_it)
				{
					if (!sequenceID.is_number_integer())
						continue;

					s_RelatedIDs.insert(static_cast<int64_t>(sequenceID));
				}
				continue;
			}

			if (rel_it->is_number_integer())
			{
				s_RelatedIDs.insert(static_cast<int64_t>(*rel_it));
			}
		}
	}


	void PacketWndBase::DrawContent()
	{
		if (m_FilteredPackets.empty())
			return;
		
		ImGuiWindow* window = GImGui->CurrentWindow;

		const float clipY = window->ClipRect.Min.y;
		auto begin = m_SortDirection == ImGuiSortDirection_Descending ? m_FilteredPackets.end() : m_FilteredPackets.begin();
		auto end = m_SortDirection == ImGuiSortDirection_Descending ? m_FilteredPackets.begin() : m_FilteredPackets.end();

		// TODO: Add processing for events where need to update scroll following (_sfNeedUpdate)
		bool sfItemFound = Config::instance().f_ScrollFollowing && SFUpdate();

		// Logic of drawing:
		//   - If content closed:
		//     - Draw packet header raw
		//	   - Go to next raw
		//   - If content opened:
		//     - Draw packet header raw
		//     - End `packetTable`
		//     - Draw packet content (table with one column)
		//     - Begin `packetTable`
		//   - If item position more than drawable region:
		//     - Skip last items.
		//     Note: 
		//        This optimization only works if you in beginning of the list,
		//        but if you scroll it to end, it still will be lag.
		//        We can't resolve because the opened content can have different heights,
		//        so calculate it without drawing hard.
		//     Improvement proposal: 
		//        We can just save all opened contents ids, and draw it, 
		//        and then just add dummy for the remaining space.
		bool tableOpened = BEGIN_PACKET_TABLE();
		if (tableOpened)
			InitTableColumns();

		size_t rowCounter = 0;
		int64_t prevSelectedPacketId = s_SelectedPacketID;
		PacketView* selectedPacketView = nullptr;
		s_SelectedPacketID = 0;
		for (auto& it = begin; it != end; m_SortDirection == ImGuiSortDirection_Descending ? it-- : it++)
		{
			auto currIt = it; // Copy iterator
			auto& info = *(m_SortDirection == ImGuiSortDirection_Descending ? --currIt : currIt);
			auto index = rowCounter++;

			bool outOfSize = window->DC.CursorPos.y > window->ClipRect.Max.y;
			if (outOfSize)
				break;

			if (!sfItemFound)
				sfItemFound = SFCheckItem(index, info->uid());

			if (!tableOpened)
				continue;

			ImGui::TableNextRow();
			ImGui::PopStyleVar();

			bool packetContentOpened = DrawPacketRow(*info);

			if (s_SelectedPacketID == info->uid())
				selectedPacketView = info;

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

			bool isIndexOdd = index & 1;
			if (sfItemFound && (m_SFItemIndex & 1) != m_SFIndexOdd)
				isIndexOdd = !isIndexOdd;

			auto rowColor = prevSelectedPacketId != 0 && s_RelatedIDs.count(info->uid()) > 0 ?
				static_cast<ImU32>(ImColor(224, 201, 20, 100))
				: ImGui::GetColorU32(isIndexOdd ? ImGuiCol_TableRowBgAlt : ImGuiCol_TableRowBg);

			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowColor);

			if (packetContentOpened)
			{
				ImGui::EndTable();

				window->ClipRect.Min.y = clipY;

				if (ImGui::BeginTable("packetContent", 1, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersOuterV))
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::PopStyleVar();
					ImGui::Spacing();

					DrawPacketContent(info);
					
					ImGui::Spacing();
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
					ImGui::EndTable();
				}

				window->DC.CursorPos.y -= 1.0f;
				window->ClipRect.Min.y = clipY;
				tableOpened = BEGIN_PACKET_TABLE();
				if (tableOpened) InitTableColumns();
			}
			
		}

		if (tableOpened)
			ImGui::EndTable();

		if (rowCounter < m_FilteredPackets.size())
			ImGui::Dummy({ 0.0f, CALC_LINE_HEIGHT() * (m_FilteredPackets.size() - rowCounter)});

		if (m_CMOpened)
		{
			ImGui::OpenPopup("CMRow");
			m_CMOpened = false;
		}

		if (ImGui::BeginPopup("CMRow"))
		{
			if (m_CurrentCMInfo.itemType == PacketWndBase::CMItemType::Header)
			{
				filter::FilterGroup* filterGroup = nullptr;
				if (s_ColumnCMFunc.count(m_CurrentCMInfo.column) > 0)
					filterGroup = s_ColumnCMFunc[m_CurrentCMInfo.column](*m_CurrentCMInfo.packet);

				if (ImGui::BeginMenu("Filters"))
				{
					for (auto func : s_CMFuncs)
					{
						auto fgResult = func(*m_CurrentCMInfo.packet);
						if (fgResult)
							filterGroup = fgResult;
					}
					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Clear"))
				{
					Clear();
					m_CurrentCMInfo.packet = nullptr;
				}

				if (filterGroup != nullptr)
				{
					for (auto& [type, filter] : filterGroup->GetFilters())
						filter::FilterManager::AddIGFilter(type, filter);
					
					filterGroup->Clear(false);

					m_CurrentCMInfo.packet = nullptr;
					ImGui::CloseCurrentPopup();
				}
			}

			if (m_CurrentCMInfo.packet != nullptr)
				DrawContextMenu(m_CurrentCMInfo);
			ImGui::EndPopup();
		}

		if (s_SelectedPacketID != prevSelectedPacketId && selectedPacketView != nullptr)
			FillRelatedIDs(selectedPacketView->flags());
	}

	bool PacketWndBase::DrawPacketRow(const PacketView& info)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		window->IDStack.push_back(0xDEADBEFF);

		auto table = ImGui::GetCurrentTable();

		float rowStartY = ImGui::GetCursorScreenPos().y;
		float rowEndY = rowStartY + CALC_LINE_HEIGHT();

		auto rowID = window->GetID(info.uid());
		ImGui::PushID(rowID);

		auto firstColumn = static_cast<HeaderColumn>(table->Columns[table->DisplayOrderToIndex[0]].UserID);
		bool treeDrawed = false;
		bool isOpen = false;
#define START_COLUMN_DRAW(column)																				    			 \
				ImGui::TableNextColumn();														                    			 \
				if (!treeDrawed && column == firstColumn)                                                                        \
				{																												 \
					bool selected = false;																						 \
					bool toggle = ImGui::Selectable("", &selected, ImGuiSelectableFlags_SpanAllColumns);						 \
																																 \
					ImGui::SameLine(0.0f, 0.0f);																				 \
																																 \
					isOpen = ImGui::TreeNodeBehavior(rowID, ImGuiTreeNodeFlags_NoTreePushOnOpen, "");		        			 \
																																 \
					if (toggle)																									 \
					{																											 \
						isOpen = !isOpen;																						 \
						window->DC.StateStorage->SetInt(rowID, isOpen);										        			 \
					}																											 \
					treeDrawed = true;																							 \
					ImGui::SameLine();                                                                              			 \
				}																												 

		START_COLUMN_DRAW(HeaderColumn_Time);
		ImGui::Text(info.timeStr().c_str());

		START_COLUMN_DRAW(HeaderColumn_IO);
		ImGui::Text(info.direction() == packet::NetIODirection::Send ? "Send" : "Receive");

		START_COLUMN_DRAW(HeaderColumn_UID);
		ImGui::Text("%u", info.uid());

		START_COLUMN_DRAW(HeaderColumn_MID);
		if (info.messageID() == 0)
			ImGui::Text("-");
		else
			ImGui::Text("%u", info.messageID());

		START_COLUMN_DRAW(HeaderColumn_Name);
		ImGui::Text("%s", info.name().c_str());

		START_COLUMN_DRAW(HeaderColumn_Size);
		ImGui::Text("%llu", info.size());

		START_COLUMN_DRAW(HeaderColumn_Flags);

		bool first = true;
		for (auto& [name, flag] : info.flags())
		{
			if (!first)
				ImGui::SameLine();
			else
				first = false;

			ImGui::TextColored(flag.color(), name.c_str());

			auto& desc = flag.desc();
			if (ImGui::IsItemHovered() && !desc.empty())
				ShowHelpText(static_cast<std::string>(desc).c_str());
		}

		float mousePosY = ImGui::GetMousePos().y;
		bool mouseInRawRegion = mousePosY > rowStartY && mousePosY < rowEndY;

		if (mouseInRawRegion)
		{
			bool rowHovered = false;
			bool rowClicked = ImGui::IsMouseReleased(1) && mouseInRawRegion;
			for (int column = 0; column < 7; column++)
			{
				auto userID = table->Columns[column].UserID;
				auto columnType = static_cast<HeaderColumn>(userID);

				auto columnHovered = ImGui::TableGetColumnFlags(column) & ImGuiTableColumnFlags_IsHovered;
				rowHovered |= columnHovered;

				if (columnHovered && rowClicked)
				{
					m_CurrentCMInfo.itemType = CMItemType::Header;
					m_CurrentCMInfo.column = columnType;
					m_CurrentCMInfo.packet = &info;
					m_CMOpened = true;
				}
			}

			if (rowHovered)
				s_SelectedPacketID = info.uid();
		}

		ImGui::PopID();
		ImGui::PopID();

		return isOpen;
	}

	void PacketWndBase::SortColumn(HeaderColumn sortColumn, ImGuiSortDirection sortDirection)
	{
		switch (sortColumn)
		{
#define SORT_COLUMN(column, compare) case column: m_Packets.sort(compare); m_FilteredPackets.sort(compare##Ptr); break;

			SORT_COLUMN(HeaderColumn_Time, _compareTime);
			SORT_COLUMN(HeaderColumn_MID, _compareMID);
			SORT_COLUMN(HeaderColumn_UID, _compareUID);
			SORT_COLUMN(HeaderColumn_Name, _compareName);
			SORT_COLUMN(HeaderColumn_Size, _compareSize);

#undef SORT_COLUMN				
		}

		m_SortColumn = sortColumn;
		m_SortDirection = sortDirection;
	}

	bool PacketWndBase::IsPacketFiltered(const PacketView& info)
	{
		return true;
	}

	void PacketWndBase::ExecuteFilter()
	{
		m_FilteredPackets.clear();
		for (auto& info : m_Packets)
		{
			if (IsPacketFiltered(info))
				m_FilteredPackets.push_back(&info);
		}
	}

	void RenderProtoNode(const ProtoNode& node, PacketView::ShowOptions options, bool defaultOpen);
	void RenderProtoValue(const std::string& name, const ProtoValue& value, PacketView::ShowOptions options, bool defaultOpen);

	void RenderProtoMap(const ProtoValue::map_type& map, PacketView::ShowOptions options, bool defaultOpen)
	{
		for (auto& [key, value] : map)
		{
			RenderProtoValue(key.convert_to_string(), value, options, defaultOpen);
		}
	}

	void RenderProtoList(const ProtoValue::list_type& list, PacketView::ShowOptions options, bool defaultOpen)
	{
		int i = 0;
		for (auto& el : list)
		{
			RenderProtoValue(std::to_string(i), el, options, defaultOpen);
			i++;
		}
	}

	bool RenderPredefinedNode(const std::string& name, const ProtoNode& node, PacketView::ShowOptions options, bool defaultOpen)
	{
		if (node.type() == "Vector")
		{
			float vector[3] = 
			{
				node.field_at(1).value().to_float(),
				node.field_at(2).value().to_float(),
				node.field_at(3).value().to_float(),
			};


			ImGui::InputFloat3(name.c_str(), vector, "%.3f", ImGuiInputTextFlags_ReadOnly);
			return true;
		}

		if (node.type() == "Vector3Int")
		{
			int32_t vector[3] =
			{
				node.field_at(1).value().to_integer32(),
				node.field_at(2).value().to_integer32(),
				node.field_at(3).value().to_integer32(),
			};

			ImGui::InputInt3(name.c_str(), vector, ImGuiInputTextFlags_ReadOnly);
			return true;
		}

		if (node.type() == "VectorPlane")
		{
			float vector[2] =
			{
				node.field_at(1).value().to_float(),
				node.field_at(2).value().to_float(),
			};
			ImGui::InputFloat2(name.c_str(), vector, "%.3f", ImGuiInputTextFlags_ReadOnly);
			return true;
		}
		return false;
	}

	void RenderProtoValue(const std::string& name, const ProtoValue& value, PacketView::ShowOptions options, bool defaultOpen)
	{
		switch (value.type())
		{
		case ProtoValue::Type::Bool:
		{
			bool enabled = value.to_bool();
			ImGui::Checkbox(name.c_str(), &enabled);
			break;
		}
		case ProtoValue::Type::Int32:
			ImGui::InputScalar(name.c_str(), ImGuiDataType_S32, const_cast<int32_t*>(&value.to_integer32()), 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
			break;
		case ProtoValue::Type::Int64:
			ImGui::InputScalar(name.c_str(), ImGuiDataType_S64, const_cast<int64_t*>(&value.to_integer64()), 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
			break;
		case ProtoValue::Type::UInt32:
			ImGui::InputScalar(name.c_str(), ImGuiDataType_U32, const_cast<uint32_t*>(&value.to_unsigned32()), 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
			break;
		case ProtoValue::Type::UInt64:
			ImGui::InputScalar(name.c_str(), ImGuiDataType_U64, const_cast<uint64_t*>(&value.to_unsigned64()), 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
			break;
		case ProtoValue::Type::Float:
			ImGui::InputScalar(name.c_str(), ImGuiDataType_Float, const_cast<float*>(&value.to_float()), 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
			break;
		case ProtoValue::Type::Double:
			ImGui::InputScalar(name.c_str(), ImGuiDataType_Double, const_cast<double*>(&value.to_double()), 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
			break;
		case ProtoValue::Type::String:
			ImGui::InputText(name.c_str(), const_cast<std::string*>(&value.to_string()), ImGuiInputTextFlags_ReadOnly);
			break;
		case ProtoValue::Type::Enum:
		{
			auto& enum_value = value.to_enum();
			if (ImGui::BeginCombo(name.c_str(), enum_value.repr().c_str()))
			{
				for (auto& [value, repr] : enum_value.values())
					ImGui::Selectable(repr.c_str(), false);
				
				ImGui::EndCombo();
			}
			break;
		}
		case ProtoValue::Type::ByteSequence:
		{
			auto& bsec = value.to_bytes();
			std::string encoded = util::base64_encode(reinterpret_cast<const BYTE*>(bsec.data()), bsec.size()); // Need optimization
			ImGui::InputText(name.c_str(), &encoded, ImGuiInputTextFlags_ReadOnly);
			break;
		}
		case ProtoValue::Type::List:
		{
			auto& list = value.to_list();
			if (list.empty())
			{
				ImGui::Text("%s [Empty list]", name.c_str());
			}
			else
			{
				if (ImGui::TreeNodeEx(name.c_str(), defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None))
				{
					RenderProtoList(list, options, defaultOpen);
					ImGui::TreePop();
				}
			}
			break;
		}
		case ProtoValue::Type::Map:
		{
			auto& map = value.to_map();
			if (map.empty())
			{
				ImGui::Text("%s [Empty map]", name.c_str());
			}
			else
			{
				if (ImGui::TreeNodeEx(name.c_str(), defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None))
				{
					RenderProtoMap(value.get<ProtoValue::map_type>(), options, defaultOpen);
					ImGui::TreePop();
				}
			}
			break;
		}
		case ProtoValue::Type::Node:
		{
			auto& node = value.get<ProtoNode>();
			if (!RenderPredefinedNode(name, node, options, defaultOpen))
			{
				if (ImGui::TreeNodeEx(name.c_str(), defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None))
				{
					RenderProtoNode(node, options, defaultOpen);
					ImGui::TreePop();
				}
			}
			break;
		}
		default:
			ImGui::Text("%s: null", name.c_str());
			break;
		}
	}

	void RenderProtoField(const ProtoField& field, PacketView::ShowOptions options, bool defaultOpen)
	{
		if (field.has_flag(ProtoField::Flag::Unknown) && !options.show_unknown)
			return;

		if (field.has_flag(ProtoField::Flag::Unsetted) && !options.show_unsetted)
			return;

		RenderProtoValue(field.name(), field.value(), options, defaultOpen && !field.has_flag(ProtoField::Flag::Unsetted));
	}

	void RenderProtoNode(const ProtoNode& node, PacketView::ShowOptions options, bool defaultOpen)
	{
		ImGui::Text("Node type: %s", node.type().empty() ? "<unknown>" : node.type().c_str());
		
		for (const auto& field : node.fields())
			RenderProtoField(field, options, defaultOpen);
	}

	void RenderProtoMessage(const std::string& name, const ProtoMessage& message, PacketView::ShowOptions options, bool defaultOpen)
	{
		if (ImGui::TreeNodeEx(name.c_str(), defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None))
		{
			RenderProtoNode(message, options, defaultOpen);
			ImGui::TreePop();
		}
	}

	void PacketWndBase::DrawPacketContent(PacketView* info)
	{
		auto id = ImGui::GetID(info);
		ImGui::PushID(id);

		auto& settings = Config::instance();

		PacketView::ShowOptions options;
		options.show_unsetted = settings.f_ShowUnsettedFields;
		options.show_unknown = settings.f_ShowUnknownFields;

		ImGui::Checkbox("## Header Beauty", &info->is_header_beauty);
		ImGui::SameLine();

		if (info->is_header_beauty)
			ImGui::InputTextMultiline("JSON Header", const_cast<std::string*>(&info->str_header(options)), ImVec2(0,0), ImGuiInputTextFlags_ReadOnly);
		else
			ImGui::InputText("JSON Header", const_cast<std::string*>(&info->str_header(options)), ImGuiInputTextFlags_ReadOnly);

		ImGui::Checkbox("## Content Beauty", &info->is_content_beauty);
		ImGui::SameLine();

		if (info->is_content_beauty)
			ImGui::InputTextMultiline("JSON Content", const_cast<std::string*>(&info->str_content(options)), ImVec2(0, 0), ImGuiInputTextFlags_ReadOnly);
		else
			ImGui::InputText("JSON Content", const_cast<std::string*>(&info->str_content(options)), ImGuiInputTextFlags_ReadOnly);

		RenderProtoMessage("Header", info->head(), options, false);
		RenderProtoMessage("Data", info->content(), options, true);
		
		ImGui::PopID();
	}



#define CREATE_SELECTOR(value, compareType, comparerCls, comparerType)											  	   \
	auto selector = new filter::FilterSelector();															           \
	{																												   \
	    auto comparer = new filter::comparer::comparerCls(filter::IFilterComparer::CompareType::compareType, value);   \
		selector->SetCurrentComparer(filter::FilterSelector::ComparerType::comparerType, comparer); 			       \
	}

#define RETURN_EQUAL_FILTER(value, comparerCls, comparerType)\
	auto fg = new filter::FilterGroup();					 \
	CREATE_SELECTOR(value, Equal, comparerCls, comparerType);\
	fg->AddFilter(selector);								 \
	return fg;

	static int64_t SelectTimeRange(bool canInfinite = false)
	{
		int64_t time = 0;
		if (canInfinite && ImGui::MenuItem("Infinite"))
			time = -1;

		if (ImGui::MenuItem("1 sec"))
			time = 1;

		if (ImGui::MenuItem("5 sec"))
			time = 5;

		if (ImGui::MenuItem("15 sec"))
			time = 15;

		if (ImGui::MenuItem("30 sec"))
			time = 30;

		if (ImGui::MenuItem("1 min"))
			time = 60;

		if (ImGui::MenuItem("5 min"))
			time = 300;

		return time;
	}

	static filter::FilterGroup* DrawTimeContextMenu(const PacketView& info)
	{
		if (!ImGui::BeginMenu("Time filter"))
			return nullptr;

		filter::FilterGroup* filterGroup = nullptr;
		if (ImGui::MenuItem("Equal"))
		{
			filterGroup = new filter::FilterGroup();
			CREATE_SELECTOR(info.timestamp(), Equal, PacketTime, Time);
			filterGroup->AddFilter(selector);
		}

		auto timestamp = info.timestamp() / 1000;
		if (ImGui::BeginMenu("About"))
		{
			auto timeRange = SelectTimeRange();
			if (timeRange > 0)
			{
				filterGroup = new filter::FilterGroup();
				{
					CREATE_SELECTOR(timestamp - timeRange, MoreEqual, PacketTime, Time);
					filterGroup->AddFilter(selector);
				}
				{
					CREATE_SELECTOR(timestamp + timeRange, LessEqual, PacketTime, Time);
					filterGroup->AddFilter(selector);
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Before"))
		{
			auto timeRange = SelectTimeRange(true);
			if (timeRange != 0)
			{
				filterGroup = new filter::FilterGroup();
				if (timeRange != -1)
				{
					CREATE_SELECTOR(timestamp - timeRange, MoreEqual, PacketTime, Time);
					filterGroup->AddFilter(selector);
				}
				CREATE_SELECTOR(timestamp, LessEqual, PacketTime, Time);
				filterGroup->AddFilter(selector);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("After"))
		{
			auto timeRange = SelectTimeRange(true);
			if (timeRange != 0)
			{
				filterGroup = new filter::FilterGroup();
				if (timeRange != -1)
				{
					CREATE_SELECTOR(timestamp + timeRange, LessEqual, PacketTime, Time);
					filterGroup->AddFilter(selector);
				}
				CREATE_SELECTOR(timestamp, MoreEqual, PacketTime, Time);
				filterGroup->AddFilter(selector);
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenu();
		return filterGroup;
	}

	static filter::FilterGroup* DrawSizeContextMenu(const PacketView& info)
	{
		if (!ImGui::MenuItem("Size filter"))
			return nullptr;

		RETURN_EQUAL_FILTER(info.size(), PacketSize, Size)
	}

	static filter::FilterGroup* DrawUIDContextMenu(const PacketView& info)
	{
		if (!ImGui::MenuItem("UID filter"))
			return nullptr;

		RETURN_EQUAL_FILTER(info.uid(), PacketID, UID)
	}

	static filter::FilterGroup* DrawNameContextMenu(const PacketView& info)
	{
		if (!ImGui::MenuItem("Name filter"))
			return nullptr;

		RETURN_EQUAL_FILTER(info.name(), PacketName, Name)
	}
}