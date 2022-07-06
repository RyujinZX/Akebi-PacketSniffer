#pragma once
#include "IFilter.h"
#include "FilterSelector.h"
#include "FilterLua.h"

namespace sniffer::filter
{
	class FilterGroup :
		public IFilterContainer
	{
	public:

		enum class Rule
		{
			AND, OR, NOT
		};

		enum class FilterType
		{
			Group,
			Selector,
			Lua
		};

		bool Execute(const packet::Packet& info) final;
		
		void to_json(nlohmann::json& j) const final;
		void from_json(const nlohmann::json& j) final;

		void SetEnabled(bool enabled);
		bool IsEnabled() const final;
		
		void SetRemovable(bool removable);
		bool IsRemovable() const final;
		bool IsEmpty();

		void SetRule(Rule rule);
		Rule GetRule();

		std::list<std::pair<FilterType, IFilterContainer*>> GetFilters();

		void AddFilter(FilterSelector* filterSelector, bool copy = false);
		void AddFilter(FilterGroup* filterGroup, bool copy = false);
		void AddFilter(FilterLua* filterLua, bool copy = false);
		void AddFilter(FilterType filterType, IFilterContainer* filter, bool copy = false);

		void RemoveFilter(FilterSelector* filterSelector, bool freeMemory = true);
		void RemoveFilter(FilterGroup* filterGroup, bool freeMemory = true);
		void RemoveFilter(IFilterContainer* filter, bool freeMemory = true);

		void Clear(bool removePtrs = true);

		bool operator==(const FilterGroup& fg);

		FilterGroup(FilterGroup const& other);
		FilterGroup(FilterGroup&& other) noexcept;
		FilterGroup();
		~FilterGroup();
		
		FilterGroup& operator=(const FilterGroup& other);
		FilterGroup& operator=(FilterGroup&& other) noexcept;

	private:

		IFilterContainer* CopyFilter(FilterType filterType, IFilterContainer* filter);

		void CopyFrom(const FilterGroup& fg);
		void MoveFrom(FilterGroup&& fg);

		void OnNestedFilterChanged(IFilter* filter);

		bool m_Enabled;
		bool m_Removable;
		Rule m_Rule;
		std::list<std::pair<FilterType, IFilterContainer*>> m_Filters;
	};
}