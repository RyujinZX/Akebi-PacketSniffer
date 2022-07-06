#include <pch.h>
#include "FilterGroup.h"
#include "FilterSelector.h"

#include <cheat-base/render/gui-util.h>

namespace sniffer::filter
{

	bool FilterGroup::Execute(const packet::Packet& packet)
	{
		if (m_Filters.empty())
			return true;

		for (auto& [filterType, filter] : m_Filters)
		{
			if (!filter->IsEnabled())
				continue;

			bool result = filter->Execute(packet);
			if (m_Rule == Rule::AND && !result)
				return false;

			if (m_Rule == Rule::NOT && result)
				return false;

			if (m_Rule == Rule::OR && result)
				return true;
		}
		return m_Rule != Rule::OR;
	}

	void FilterGroup::to_json(nlohmann::json& j) const
	{
		config::Enum<FilterGroup::Rule> rule = m_Rule;

		j = nlohmann::json({
			{ "enabled", m_Enabled },
			{ "rule", rule },
			{ "filters", nlohmann::json::array() }
			});

		for (auto& [filterType, filter] : m_Filters)
		{
			nlohmann::json filterJson;
			filter->to_json(filterJson);
			config::Enum<FilterType> filterTypeEnum = filterType;
			filterJson["type"] = filterTypeEnum;
			j["filters"].push_back(filterJson);
		}
	}

	void FilterGroup::from_json(const nlohmann::json& j)
	{
		config::Enum<FilterGroup::Rule> rule;
		j.at("enabled").get_to(m_Enabled);
		j.at("rule").get_to(rule);
		m_Rule = rule;

		for (auto& filterJson : j["filters"])
		{
			config::Enum<FilterType> filterType;
			filterJson.at("type").get_to(filterType);

			IFilterContainer* filter = nullptr;
			switch (filterType.value())
			{
			case FilterType::Selector:
				filter = new FilterSelector();
				break;
			case FilterType::Group:
				filter = new FilterGroup();
				break;
			case FilterType::Lua:
				filter = new FilterLua();
				break;
			default:
				break;
			}

			if (filter == nullptr)
				continue;

			filter->from_json(filterJson);
			AddFilter(filterType.value(), filter);
		}
	}

	void FilterGroup::SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
		ChangedEvent(this);
	}

	bool FilterGroup::IsEnabled() const
	{
		return m_Enabled;
	}

	bool FilterGroup::operator==(const FilterGroup& fg)
	{
		return false;
	}

	FilterGroup::FilterGroup(FilterGroup const& other)
	{
		CopyFrom(other);
	}

	FilterGroup::FilterGroup(FilterGroup&& other) noexcept
	{
		MoveFrom(std::move(other));
	}

	FilterGroup::FilterGroup() :
		m_Rule(Rule::AND),
		m_Enabled(true),
		m_Removable(true),
		m_Filters()
	{}

	FilterGroup::~FilterGroup()
	{
		for (auto& [filterType, filter] : m_Filters)
		{
			delete filter;
		}
	}

	void FilterGroup::SetRule(Rule rule)
	{
		m_Rule = rule;
	}

	filter::FilterGroup::Rule FilterGroup::GetRule()
	{
		return m_Rule;
	}

	std::list<std::pair<FilterGroup::FilterType, IFilterContainer*>> FilterGroup::GetFilters()
	{
		return m_Filters;
	}

	void FilterGroup::AddFilter(FilterSelector* filterSelector, bool copy)
	{
		AddFilter(FilterType::Selector, filterSelector, copy);
	}

	void FilterGroup::AddFilter(FilterGroup* filterGroup, bool copy)
	{
		AddFilter(FilterType::Group, filterGroup, copy);
	}

	void FilterGroup::AddFilter(FilterLua* filterLua, bool copy)
	{
		AddFilter(FilterType::Lua, filterLua, copy);
	}

	void FilterGroup::AddFilter(FilterType filterType, IFilterContainer* filter, bool copy)
	{
		IFilterContainer* newFilter = filter;

		if (copy)
		{
			newFilter = CopyFilter(filterType, filter);
			if (newFilter == nullptr)
				return;
		}

		newFilter->ChangedEvent += MY_METHOD_HANDLER(FilterGroup::OnNestedFilterChanged);
		m_Filters.push_back({ filterType, newFilter });
		ChangedEvent(this);
	}


	void FilterGroup::RemoveFilter(FilterSelector* filterSelector, bool freeMemory )
	{
		m_Filters.remove({ FilterType::Selector, filterSelector });
		if (freeMemory)
			delete filterSelector;
		ChangedEvent(this);
	}

	void FilterGroup::RemoveFilter(FilterGroup* filterGroup, bool freeMemory)
	{
		m_Filters.remove({ FilterType::Group, filterGroup });
		if (freeMemory)
			delete filterGroup;
		ChangedEvent(this);
	}

	void FilterGroup::RemoveFilter(IFilterContainer* filter, bool freeMemory)
	{
		m_Filters.remove_if([&filter](const std::pair<FilterType, IFilterContainer*>& filterPair) { return filter == filterPair.second; });
		if (freeMemory)
			delete filter;
		ChangedEvent(this);
	}

	bool FilterGroup::IsEmpty()
	{
		return m_Filters.empty();
	}

	FilterGroup& FilterGroup::operator=(const FilterGroup& other)
	{
		Clear();
		CopyFrom(other);
		ChangedEvent(this);
		return *this;
	}

	FilterGroup& FilterGroup::operator=(FilterGroup&& other) noexcept
	{
		if (this == &other)
			return *this;

		Clear();
		MoveFrom(std::move(other));
		ChangedEvent(this);
		return *this;
	}

	IFilterContainer* FilterGroup::CopyFilter(FilterType filterType, IFilterContainer* filter)
	{
		switch (filterType)
		{
		case filter::FilterGroup::FilterType::Group:
			return new FilterGroup(*reinterpret_cast<FilterGroup*>(filter));
		case filter::FilterGroup::FilterType::Selector:
			return new FilterSelector(*reinterpret_cast<FilterSelector*>(filter));
		case filter::FilterGroup::FilterType::Lua:
			return new FilterLua(*reinterpret_cast<FilterLua*>(filter));
		default:
			return nullptr;
		}
	}

	void FilterGroup::CopyFrom(const FilterGroup& fg)
	{
		m_Rule = fg.m_Rule;
		m_Enabled = fg.m_Enabled;
		m_Removable = fg.m_Removable;
		for (auto& [filterType, filter] : fg.m_Filters)
		{
			AddFilter(filterType, filter, true);
		}
	}

	void FilterGroup::MoveFrom(FilterGroup&& fg)
	{
		m_Rule = std::move(fg.m_Rule);
		m_Enabled = std::move(fg.m_Enabled);
		m_Removable = std::move(fg.m_Removable);
		m_Filters = std::move(fg.m_Filters);

		for (auto& [FilterType, filter] : m_Filters)
		{
			filter->ChangedEvent += MY_METHOD_HANDLER(FilterGroup::OnNestedFilterChanged);
			filter->ChangedEvent -= METHOD_HANDLER(fg, FilterGroup::OnNestedFilterChanged);
		}
		fg.m_Filters.clear(); // Should be cleared before deleting
	}

	void FilterGroup::OnNestedFilterChanged(IFilter* filter)
	{
		ChangedEvent(filter);
	}

	void FilterGroup::Clear(bool removePtrs /*= true*/)
	{
		if (removePtrs)
		{
			for (auto& [type, filter] : m_Filters)
				delete filter;
		}

		m_Filters.clear();
		ChangedEvent(this);
	}

	bool FilterGroup::IsRemovable() const
	{
		return m_Removable;
	}

	void FilterGroup::SetRemovable(bool removable)
	{
		m_Removable = removable;
		ChangedEvent(this);
	}

}