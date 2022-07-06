#include "pch.h"
#include "FilterWnd.h"

#include <imgui_internal.h>

#include <sniffer/gui/util.h>
#include <sniffer/gui/ScriptEditor.h>

#include <sniffer/script/ScriptManager.h>
#include <sniffer/filter/comparers.h>
#include <sniffer/filter/FilterSelector.h>
#include <sniffer/filter/FilterGroup.h>
#include <sniffer/filter/FilterLua.h>
#include <sniffer/filter/FilterManager.h>
namespace sniffer::gui
{

	static void DrawFilterComparer(filter::comparer::AnyKey* comparer)
	{
		ImGui::PushID(comparer);

		ImGui::Spacing(); ImGui::SameLine();

		if (ImGui::InputText("Key", &comparer->key))
			comparer->ChangedEvent(comparer); // Bad design! TO DO: Replace with getter/setter.

		ImGui::PopID();
	}

	static void DrawFilterComparer(filter::comparer::AnyValue* comparer)
	{
		ImGui::PushID(comparer);

		ImGui::Spacing(); ImGui::SameLine();

		if (ImGui::InputText("Value", &comparer->value))
			comparer->ChangedEvent(comparer); // Bad design! TO DO: Replace with getter/setter.

		ImGui::PopID();
	}

	static bool InputTextLast(const char* label, std::string* str)
	{
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
		const ImVec2 frame_size = ImGui::CalcItemSize(ImVec2(-1, 0), 0.0f, label_size.y + style.FramePadding.y * 2.0f); // Arbitrary default of 8 lines high for multi-line
		const ImVec2 total_size = ImVec2(frame_size.x - (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_size.y);

		ImGui::PushItemWidth(total_size.x);
		bool result = ImGui::InputText(label, str);
		ImGui::PopItemWidth();
		return result;
	}

	static void DrawFilterComparer(filter::comparer::KeyValue* comparer)
	{
		ImGui::PushID(comparer);

		ImGui::Spacing(); ImGui::SameLine();

		bool internalChanged = false;
		internalChanged |= ComboEnum("## KeyCT", &comparer->keyCT, &comparer->keyCTs); ImGui::SameLine();
		internalChanged |= InputTextLast("Key", &comparer->key);

		ImGui::Spacing(); ImGui::SameLine();
		internalChanged |= ComboEnum("## ValueCT", &comparer->valueCT, &comparer->ValueCTs);
		float itemWidth = ImGui::GetItemRectSize().x;
		ImGui::SameLine();
		internalChanged |= InputTextLast("Value", &comparer->value);

		if (internalChanged)
			comparer->ChangedEvent(comparer); // Bad design! TO DO: Replace with getter/setter.

		ImGui::PopID();
	}

	static void DrawFilterComparer(filter::comparer::PacketID* comparer)
	{
		ImGui::PushID(comparer);

		ImGui::Spacing(); ImGui::SameLine();

		if (ImGui::InputInt("ID", &comparer->id))
			comparer->ChangedEvent(comparer); // Bad design! TO DO: Replace with getter/setter.

		ImGui::PopID();
	}

	static void DrawFilterComparer(filter::comparer::PacketName* comparer)
	{
		ImGui::PushID(comparer);

		ImGui::Spacing(); ImGui::SameLine();

		if (ImGui::InputText("Name", &comparer->name))
			comparer->ChangedEvent(comparer); // Bad design! TO DO: Replace with getter/setter.

		ImGui::PopID();
	}

	static void DrawFilterComparer(filter::comparer::PacketSize* comparer)
	{
		ImGui::PushID(comparer);

		ImGui::Spacing(); ImGui::SameLine();

		if (ImGui::InputInt("Size", &comparer->size))
			comparer->ChangedEvent(comparer); // Bad design! TO DO: Replace with getter/setter.

		ImGui::PopID();
	}

	static void DrawFilterComparer(filter::comparer::PacketTime* comparer)
	{
		ImGui::PushID(comparer);

		ImGui::Spacing(); ImGui::SameLine();
		bool timeChanged = ImGui::InputText("Time", &comparer->time);
		if (timeChanged)
			comparer->ParseTime();

		if (comparer->IsInvalid())
			ImGui::TextColored({ 0.88f, 0.08f, 0.11f, 1.0f }, "Invalid time value.\nDD/MM/YYYY hh:mm:ss");

		if (timeChanged)
			comparer->ChangedEvent(comparer); // Bad design! TO DO: Replace with getter/setter.

		ImGui::PopID();
	}

	static void DrawFilterContainer(filter::FilterSelector* selector, bool* removed = nullptr)
	{
		ImGui::PushID(selector);

		auto comparerType = selector->GetComparerType();
		if (ComboEnum("## ComparerType", &comparerType))
			selector->SetComparerType(comparerType);

		auto comparer = selector->GetCurrentComparer();

		auto compareType = comparer->GetCompareType();
		auto& compareTypes = comparer->GetCompareTypes();
		if (!compareTypes.empty() && (ImGui::SameLine(), ComboEnum("## CompareType", &compareType, &compareTypes)))
			comparer->SetCompareType(compareType);


		if (selector->IsRemovable() && removed != nullptr && (ImGui::SameLine(), ImGui::Button("Remove")))
			*removed = true;

		switch (comparerType)
		{
		case filter::FilterSelector::ComparerType::AnyKey:
			DrawFilterComparer(reinterpret_cast<filter::comparer::AnyKey*>(comparer));
			break;
		case filter::FilterSelector::ComparerType::AnyValue:
			DrawFilterComparer(reinterpret_cast<filter::comparer::AnyValue*>(comparer));
			break;
		case filter::FilterSelector::ComparerType::KeyValue:
			DrawFilterComparer(reinterpret_cast<filter::comparer::KeyValue*>(comparer));
			break;
		case filter::FilterSelector::ComparerType::Name:
			DrawFilterComparer(reinterpret_cast<filter::comparer::PacketName*>(comparer));
			break;
		case filter::FilterSelector::ComparerType::Size:
			DrawFilterComparer(reinterpret_cast<filter::comparer::PacketSize*>(comparer));
			break;
		case filter::FilterSelector::ComparerType::Time:
			DrawFilterComparer(reinterpret_cast<filter::comparer::PacketTime*>(comparer));
			break;
		case filter::FilterSelector::ComparerType::UID:
			DrawFilterComparer(reinterpret_cast<filter::comparer::PacketID*>(comparer));
			break;
		default:
			throw std::logic_error("Unsupported comparer type for drawing.");
		}

		ImGui::PopID();
	}

	static void DrawFilterContainer(filter::FilterLua* filter, bool* removed = nullptr)
	{
		ImGui::PushID(filter);

		auto script = filter->GetScript();
		ImGui::Text("Current script: %s", script == nullptr ? "None" : script->GetName().c_str());
		ImGui::SameLine();

		bool editDisabled = script == nullptr || ScriptEditorManager::IsOpened(script);
		if (editDisabled)
			ImGui::BeginDisabled();

		if (ImGui::Button("Edit"))
			ScriptEditorManager::Open(script);

		if (editDisabled)
			ImGui::EndDisabled();

		if (script != nullptr && !script->GetLua().HasFunction("on_filter"))
			ImGui::TextColored(ImVec4(0.94f, 0.11f, 0.07f, 1.0f), "SErr: Script have not function `on_filter`.");

		if (script != nullptr && script->HasError())
			ImGui::TextColored(ImVec4(0.94f, 0.11f, 0.07f, 1.0f), "PErr: %s", script->GetError().c_str());

		static script::Script* selected = nullptr;
		if (ImGui::Button("Select..."))
		{
			ImGui::OpenPopup("SelectScriptPopup");
			selected = nullptr;
		}

		if (ImGui::BeginPopup("SelectScriptPopup"))
		{
			ImGui::BeginListBox("Scripts");
			auto& scriptList = script::ScriptManager::GetScripts();
			for (auto& script : scriptList)
			{
				if (!script->GetLua().HasFunction("on_filter"))
					continue;

				std::string name = script->GetName();
				if (ImGui::Selectable(name.c_str(), selected))
				{
					selected = script;
				}
			}
			ImGui::EndListBox();

			if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
			{
				if (selected != nullptr)
					filter->SetScript(selected);

				ImGui::CloseCurrentPopup();
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::SameLine();
		if (filter->IsRemovable() && removed != nullptr && ImGui::Button("Remove"))
			*removed = true;

		ImGui::PopID();
	}

	static void DrawFilterContainer(filter::FilterGroup* filter, bool* removed = nullptr)
	{
		ImGui::PushID(filter);

		bool enabled = filter->IsEnabled();
		bool enableChanged = false;
		bool groupOpened = ImGui::BeginSelectableGroupPanel("Group", enabled, enableChanged, true, ImVec2(-1.0f, 0.0f), "## Enabled");

		if (enableChanged)
			filter->SetEnabled(enabled);

		ImGui::NextGroupPanelHeaderItem({ GetMaxEnumWidth<filter::FilterGroup::Rule>(), ImGui::GetFrameHeight() });

		auto rule = filter->GetRule();
		if (ComboEnum("## Rule", &rule))
			filter->SetRule(rule);

		if (filter->IsRemovable() && removed != nullptr)
		{
			const char* text = "Remove";
			ImGui::NextGroupPanelHeaderItem(ImGui::CalcButtonSize(text), true);
			*removed = ImGui::Button(text);
		}

		if (groupOpened)
		{
			if (!enabled)
				ImGui::BeginDisabled();

			filter::IFilterContainer* removeFilter = nullptr;
			for (auto& [filterType, container] : filter->GetFilters())
			{
				bool filterRemoved = false;
				switch (filterType)
				{
				case filter::FilterGroup::FilterType::Group:
					DrawFilterContainer(reinterpret_cast<filter::FilterGroup*>(container), &filterRemoved);
					break;
				case filter::FilterGroup::FilterType::Lua:
					DrawFilterContainer(reinterpret_cast<filter::FilterLua*>(container), &filterRemoved);
					break;
				case filter::FilterGroup::FilterType::Selector:
					DrawFilterContainer(reinterpret_cast<filter::FilterSelector*>(container), &filterRemoved);
					break;
				}

				if (filterRemoved)
					removeFilter = container;
			}

			if (removeFilter != nullptr)
				filter->RemoveFilter(removeFilter);

			if (ImGui::Button("+ Group", ImVec2(70, 0)))
				filter->AddFilter(new filter::FilterGroup());

			ImGui::SameLine();

			if (ImGui::Button("+ Filter", ImVec2(70, 0)))
				filter->AddFilter(new filter::FilterSelector());

			ImGui::SameLine();

			if (ImGui::Button("+ Lua", ImVec2(70, 0)))
				filter->AddFilter(new filter::FilterLua());

			if (!enabled)
				ImGui::EndDisabled();
		}
		ImGui::EndSelectableGroupPanel();

		ImGui::PopID();
	}

	static void DrawIntermediateGroup()
	{
		if (ImGui::BeginGroupPanel("Intermediate", true))
		{
			bool isIntermediateEnabled = filter::FilterManager::IsIntermediateGroupEnabled();
			if (ImGui::Checkbox("Enabled", &isIntermediateEnabled))
				filter::FilterManager::SetIntermediateGroupEnabled(isIntermediateEnabled);
			ImGui::SameLine();
			HelpMarker("If enabled, context menu filters will be add to intermediate group, if not - to the main.");

			bool isOverrideEnabled = filter::FilterManager::IsOverrideModeEnabled();
			if (ImGui::Checkbox("Override mode", &isOverrideEnabled))
				filter::FilterManager::SetOverrideModeEnabled(isOverrideEnabled);
			ImGui::SameLine();
			HelpMarker("The intermediate filter will override the main filter if no empty.");

			auto& intermediateGroup = filter::FilterManager::GetIntermediateFilter();
			auto disabled = !isIntermediateEnabled || intermediateGroup.IsEmpty();
			if (disabled)
				ImGui::BeginDisabled();

			if (ImGui::Button("Move to main"))
				filter::FilterManager::MoveToUserGroup();

			ImGui::SameLine();

			if (ImGui::Button("Clear"))
				intermediateGroup.Clear();

			if (ImGui::TreeNode("Filter"))
			{
				DrawFilterContainer(&intermediateGroup);
				ImGui::TreePop();
			}

			if (disabled)
				ImGui::EndDisabled();
		}
		ImGui::EndGroupPanel();
	}

	static void DrawUserGroup()
	{
		DrawProfiler("Profiles", filter::FilterManager::GetUserFilterProfiler(), 200.0f);
		DrawFilterContainer(&filter::FilterManager::GetUserFilter());

		if (ImGui::Button("Apply"))
			filter::FilterManager::ApplyUserGroup();
	}

	void FilterWnd::Draw()
	{
		DrawIntermediateGroup();
		DrawUserGroup();
	}

	sniffer::gui::FilterWnd& FilterWnd::instance()
	{
		static FilterWnd instance;
		return instance;
	}

	FilterWnd::FilterWnd() { }

	sniffer::gui::WndInfo& FilterWnd::GetInfo()
	{
		static WndInfo info = { "Packet filter", true };
		return info;
	}
}