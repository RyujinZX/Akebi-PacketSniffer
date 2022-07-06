#pragma once
#include "IFilter.h"
#include "IFilterComparer.h"
#include "IFilterContainer.h"

namespace sniffer::filter
{
	class FilterSelector :
		public IFilterContainer
	{
	public:
		bool Execute(const packet::Packet& packet) final;

		FilterSelector& operator=(const FilterSelector& other);
		FilterSelector& operator=(FilterSelector&& other);

		FilterSelector(FilterSelector const& other);
		FilterSelector(FilterSelector&& other);

		FilterSelector();
		~FilterSelector();

		enum class ComparerType
		{
			Name, UID, Time, Size,
			KeyValue, AnyKey, AnyValue,
		};

		void SetComparerType(ComparerType comparerType);
		ComparerType GetComparerType();

		void SetCurrentComparer(ComparerType comparerType, IFilterComparer* comparer);
		IFilterComparer* GetCurrentComparer();

		void to_json(nlohmann::json& j) const;
		void from_json(const nlohmann::json& j);

		inline bool IsEnabled() const final { return true; };
		inline bool IsRemovable() const final { return true; };

	private:
		ComparerType m_ComparerType;

		void OnComparerChanged(IFilter* comparer);

		std::map<ComparerType, IFilterComparer*> m_Comparers;
		IFilterComparer* m_CurrentComparer;
	};
}