#pragma once
#include <sniffer/filter/IFilter.h>

namespace sniffer::filter
{
	class IFilterContainer : public IFilter
	{
	public:
		virtual bool IsRemovable() const = 0;
		virtual bool IsEnabled() const = 0;
	};
}