#include <pch.h>
#include "FilterSelector.h"

#include <cheat-base/render/gui-util.h>

#include <sniffer/filter/comparers.h>

namespace sniffer::filter
{
	void FilterSelector::to_json(nlohmann::json& j) const
	{
		config::Enum<ComparerType> comparerType = m_ComparerType;

		nlohmann::json comparerJson;
		m_CurrentComparer->to_json(comparerJson);

		j = nlohmann::json(
			{
				{ "comparer", comparerJson },
				{ "comparerType", comparerType }
			}
		);
	}

	void FilterSelector::from_json(const nlohmann::json& j)
	{
		config::Enum<ComparerType> comparerType;

		j.at("comparerType").get_to(comparerType);

		m_Comparers[comparerType]->from_json(j["comparer"]);
		m_ComparerType = comparerType;
		m_CurrentComparer = m_Comparers[comparerType];
	}

	bool FilterSelector::Execute(const packet::Packet& packet)
	{
		return m_CurrentComparer->Execute(packet);
	}

#define COPY_COMPARER(type, cmpClass) { ComparerType::type, new comparer::cmpClass(*reinterpret_cast<comparer::cmpClass*>(other.m_Comparers.at(ComparerType::type))) }

	FilterSelector& FilterSelector::operator=(const FilterSelector& other)
	{
		for (auto& [comparerType, comparer] : m_Comparers)
			delete comparer;

		m_Comparers.clear();

		m_Comparers = {
			COPY_COMPARER(Name, PacketName),
			COPY_COMPARER(UID, PacketID),
			COPY_COMPARER(Size, PacketSize),
			COPY_COMPARER(Time, PacketTime),
			COPY_COMPARER(AnyKey, AnyKey),
			COPY_COMPARER(AnyValue, AnyValue),
			COPY_COMPARER(KeyValue, KeyValue)
		};
		m_ComparerType = other.m_ComparerType;
		m_CurrentComparer = m_Comparers[m_ComparerType];

		for (auto& [comparerType, comparer] : m_Comparers)
			comparer->ChangedEvent += MY_METHOD_HANDLER(FilterSelector::OnComparerChanged);

		ChangedEvent(this);
		return *this;
	}

	FilterSelector& FilterSelector::operator=(FilterSelector&& other)
	{
		if (this == &other)
			return *this;

		for (auto& [comparerType, comparer] : m_Comparers)
			delete comparer;

		m_Comparers.clear();

		m_ComparerType = std::move(other.m_ComparerType);
		m_CurrentComparer = std::move(other.m_CurrentComparer);
		m_Comparers = std::move(other.m_Comparers);

		for (auto& [comparerType, comparer] : m_Comparers)
		{
			comparer->ChangedEvent += MY_METHOD_HANDLER(FilterSelector::OnComparerChanged);
			comparer->ChangedEvent -= METHOD_HANDLER(other, FilterSelector::OnComparerChanged);
		}
		ChangedEvent(this);
		return *this;
	}

	FilterSelector::FilterSelector(FilterSelector const& other) :
		m_ComparerType(other.m_ComparerType),
		m_Comparers({
			COPY_COMPARER(Name, PacketName),
			COPY_COMPARER(UID, PacketID),
			COPY_COMPARER(Size, PacketSize),
			COPY_COMPARER(Time, PacketTime),
			COPY_COMPARER(AnyKey, AnyKey),
			COPY_COMPARER(AnyValue, AnyValue),
			COPY_COMPARER(KeyValue, KeyValue)
		})
	{
		m_CurrentComparer = m_Comparers[m_ComparerType];
		for (auto& [comparerType, comparer] : m_Comparers)
			comparer->ChangedEvent += MY_METHOD_HANDLER(FilterSelector::OnComparerChanged);
	}

	FilterSelector::FilterSelector(FilterSelector&& other)
		: m_ComparerType(std::move(other.m_ComparerType)),
		m_CurrentComparer(std::move(other.m_CurrentComparer)),
		m_Comparers(std::move(other.m_Comparers))
	{
		for (auto& [comparerType, comparer] : m_Comparers)
		{
			comparer->ChangedEvent -= METHOD_HANDLER(other, FilterSelector::OnComparerChanged);
			comparer->ChangedEvent += MY_METHOD_HANDLER(FilterSelector::OnComparerChanged);
		}
	}

#undef COPY_COMPARER

	FilterSelector::FilterSelector() :
		m_ComparerType(ComparerType::Name),
		m_Comparers({
			{ ComparerType::Name, new comparer::PacketName() },
			{ ComparerType::UID, new comparer::PacketID() },
			{ ComparerType::Size, new comparer::PacketSize() },
			{ ComparerType::Time, new comparer::PacketTime() },

			{ ComparerType::AnyKey, new comparer::AnyKey() },
			{ ComparerType::AnyValue, new comparer::AnyValue() },
			{ ComparerType::KeyValue, new comparer::KeyValue() }
			})
	{ 
		m_CurrentComparer = m_Comparers[m_ComparerType];
		for (auto& [comparerType, comparer] : m_Comparers)
			comparer->ChangedEvent += MY_METHOD_HANDLER(FilterSelector::OnComparerChanged);
	}

	FilterSelector::~FilterSelector()
	{
		for (auto& [comparerType, comparer] : m_Comparers)
			delete comparer;
	}

	void FilterSelector::SetComparerType(ComparerType comparerType)
	{
		m_ComparerType = comparerType;
		m_CurrentComparer = m_Comparers[m_ComparerType];
	}

	filter::FilterSelector::ComparerType FilterSelector::GetComparerType()
	{
		return m_ComparerType;
	}

	void FilterSelector::SetCurrentComparer(ComparerType comparerType, IFilterComparer* comparer)
	{
		delete m_Comparers[comparerType];
		comparer->ChangedEvent += MY_METHOD_HANDLER(FilterSelector::OnComparerChanged);

		m_Comparers[comparerType] = comparer;
		m_ComparerType = comparerType;
		m_CurrentComparer = comparer;
	}

	filter::IFilterComparer* FilterSelector::GetCurrentComparer()
	{
		return m_CurrentComparer;
	}

	void FilterSelector::OnComparerChanged(IFilter* comparer)
	{
		ChangedEvent(comparer);
	}
}