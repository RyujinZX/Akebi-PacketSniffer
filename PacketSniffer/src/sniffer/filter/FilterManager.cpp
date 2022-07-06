#include <pch.h>
#include "FilterManager.h"

namespace sniffer::filter
{

	void FilterManager::Run()
	{
		f_UserGroupProfiler = SNF(f_UserGroupProfiler, "User group", "sniffer::filter", Profiler<FilterGroup>("default", FilterGroup()));
		f_IntermediateGroup = SNF(f_IntermediateGroup, "Intermediate group", "sniffer::filter", FilterGroup());
		f_IGOverrideMode = SNF(f_IGOverrideMode, "Override mode", "sniffer::filter", false);
		f_IntermediateGroupEnabled = SNF(f_IntermediateGroupEnabled, "Intermediate group enabled", "sniffer::filter", true);

		s_AppliedGroup = f_UserGroupProfiler.value().current();
		
		for (auto& [name, userGroup] : f_UserGroupProfiler.value().profiles())
			userGroup.ChangedEvent += FUNCTION_HANDLER(FilterManager::OnUGChanged);

		f_IntermediateGroup.value().ChangedEvent += FUNCTION_HANDLER(FilterManager::OnIGChanged);
		f_UserGroupProfiler.value().ChangedEvent += FUNCTION_HANDLER(FilterManager::OnProfilerChanged);
		f_UserGroupProfiler.value().NewProfileEvent += FUNCTION_HANDLER(FilterManager::OnUGProfileAdded);
	}

	bool FilterManager::IsOverrideModeEnabled()
	{
		return f_IGOverrideMode.value();
	}

	void FilterManager::SetOverrideModeEnabled(bool enabled)
	{
		f_IGOverrideMode = enabled;
	}

	bool FilterManager::IsIntermediateGroupEnabled()
	{
		return f_IntermediateGroupEnabled.value();
	}

	void FilterManager::SetIntermediateGroupEnabled(bool enabled)
	{
		f_IntermediateGroupEnabled = enabled;
	}

	sniffer::filter::FilterGroup& FilterManager::GetIntermediateFilter()
	{
		return f_IntermediateGroup.value();
	}

	sniffer::filter::FilterGroup& FilterManager::GetUserFilter()
	{
		return f_UserGroupProfiler.value().current();
	}

	Profiler<sniffer::filter::FilterGroup>& FilterManager::GetUserFilterProfiler()
	{
		return f_UserGroupProfiler.value();
	}

	void FilterManager::OnIGChanged(IFilter* filter)
	{
		f_IntermediateGroup.FireChanged();
		if (f_IntermediateGroupEnabled)
			s_MainFilterChangedEvent();
	}

	void FilterManager::OnUGChanged(IFilter* filter)
	{
		f_UserGroupProfiler.FireChanged();
	}

	void FilterManager::OnProfilerChanged(Profiler<filter::FilterGroup>* profiler)
	{
		f_UserGroupProfiler.FireChanged();
	}

	void FilterManager::OnUGProfileAdded(Profiler<FilterGroup>* profiler, const std::string& name, FilterGroup& fg)
	{
		fg.ChangedEvent += FUNCTION_HANDLER(FilterManager::OnUGChanged);
	}

	void FilterManager::MoveToUserGroup()
	{
		auto filters = f_IntermediateGroup.value().GetFilters();
		if (filters.size() == 1)
		{
			auto& filterInfo = filters.front();

			// We should add both to don't apply changes in current filter
			GetUserFilter().AddFilter(filterInfo.first, filterInfo.second, true);
			s_AppliedGroup.AddFilter(filterInfo.first, filterInfo.second, true);
		}
		else
		{
			auto& filterInfo = filters.front();
			s_AppliedGroup.AddFilter(f_IntermediateGroup, true);
			GetUserFilter().AddFilter(filterInfo.first, filterInfo.second, true);
		}

		f_IntermediateGroup = FilterGroup();
	}

	void FilterManager::ApplyUserGroup()
	{
		s_AppliedGroup = f_UserGroupProfiler.value().current();
		s_MainFilterChangedEvent();
	}

	bool FilterManager::Execute(const packet::Packet& packet)
	{
		bool result = true;

		if (!f_IntermediateGroupEnabled || !f_IGOverrideMode)
			result &= s_AppliedGroup.Execute(packet);

		if (f_IntermediateGroupEnabled)
			result &= f_IntermediateGroup.value().Execute(packet);

		return result;
	}

	void FilterManager::AddIGFilter(FilterGroup* filter, bool copy)
	{
		AddIGFilter(FilterGroup::FilterType::Group, filter, copy);
	}

	void FilterManager::AddIGFilter(FilterSelector* filter, bool copy)
	{
		AddIGFilter(FilterGroup::FilterType::Selector, filter, copy);
	}

	void FilterManager::AddIGFilter(FilterLua* filter, bool copy)
	{
		AddIGFilter(FilterGroup::FilterType::Lua, filter, copy);
	}

	void FilterManager::AddIGFilter(const FilterGroup::FilterType type, IFilterContainer* filter, bool copy)
	{
		if (f_IntermediateGroupEnabled)
			f_IntermediateGroup.value().AddFilter(type, filter, copy);
		else
			GetUserFilter().AddFilter(type, filter, copy);
	}

}