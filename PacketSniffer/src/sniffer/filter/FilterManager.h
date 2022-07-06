#pragma once
#include <cheat-base/config/Config.h>
#include <cheat-base/config/Field.h>

#include <sniffer/filter/FilterGroup.h>
#include <sniffer/Profiler.h>

namespace sniffer::filter
{
	class FilterManager
	{
	public:
		static void Run();

		static bool IsOverrideModeEnabled();
		static void SetOverrideModeEnabled(bool enabled);

		static bool IsIntermediateGroupEnabled();
		static void SetIntermediateGroupEnabled(bool enabled);

		// Intermediate filter for adding generated conditions.
		static filter::FilterGroup& GetIntermediateFilter();

		// Filter what user will change directly.
		static filter::FilterGroup& GetUserFilter();
		static Profiler<filter::FilterGroup>& GetUserFilterProfiler();

		static void MoveToUserGroup();
		static void ApplyUserGroup();

		static bool Execute(const packet::Packet& packet);

		static void AddIGFilter(FilterGroup* filter, bool copy = false);
		static void AddIGFilter(FilterSelector* filter, bool copy = false);
		static void AddIGFilter(FilterLua* filter, bool copy = false);

		static void AddIGFilter(FilterGroup::FilterType type, IFilterContainer* filter, bool copy = false);

	private:
		inline static config::Field<bool> f_IGOverrideMode;
		inline static config::Field<bool> f_IntermediateGroupEnabled;

		inline static config::Field<filter::FilterGroup> f_IntermediateGroup;
		inline static config::Field<Profiler<filter::FilterGroup>> f_UserGroupProfiler;
		inline static filter::FilterGroup s_AppliedGroup;

		static void OnIGChanged(IFilter* filter);
		static void OnUGChanged(IFilter* filter);

		static void OnProfilerChanged(Profiler<FilterGroup>* profiler);
		static void OnUGProfileAdded(Profiler<FilterGroup>* profiler, const std::string& name, FilterGroup& fg);

		inline static TEvent<> s_MainFilterChangedEvent;

	public:

		inline static IEvent<>& MainFilterChangedEvent = s_MainFilterChangedEvent;
	};
}