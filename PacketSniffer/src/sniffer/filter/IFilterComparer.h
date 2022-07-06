#pragma once
#include "IFilter.h"
namespace sniffer::filter
{
	class IFilterComparer :
		public IFilter
	{
	public:
		enum class CompareType
		{
			Regex, Contains,
			Equal,
			Less, LessEqual,
			More, MoreEqual
		};

		virtual std::vector<CompareType>& GetCompareTypes() = 0;

		virtual CompareType GetCompareType() = 0;
		virtual void SetCompareType(CompareType compareType) = 0;
	};
}