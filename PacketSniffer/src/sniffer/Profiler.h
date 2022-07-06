#pragma once

#include <cheat-base/ISerializable.h>
#include <cheat-base/events/event.hpp>

template<class TElement>
class Profiler : public ISerializable
{
public:
	Profiler();
	Profiler(const std::string& name, const TElement& element);
	Profiler(std::string&& name, TElement&& element);

	Profiler(Profiler<TElement>&& other) noexcept;
	Profiler(const Profiler<TElement>& other);

	Profiler<TElement>& operator=(Profiler<TElement>&& other) noexcept;
	Profiler<TElement>& operator=(const Profiler<TElement>& other);

	void add_profile(const std::string& name, const TElement& element);
	void add_profile(std::string&& name, TElement&& element);

	void rename_profile(const std::string& oldName, const std::string& newName);

	void remove_profile(const std::string& name);

	const std::list<std::pair<std::string, TElement>>& profiles() const;
	std::list<std::pair<std::string, TElement>>& profiles();

	bool has_profile(const std::string& name) const;
	const TElement& profile(const std::string& name) const;
	TElement& profile(const std::string& name);

	const std::string& current_name() const;
	const TElement& current() const;
	TElement& current();

	void switch_profile(const std::string& name);

	void to_json(nlohmann::json& j) const final;
	void from_json(const nlohmann::json& j) final;

	TEvent<Profiler<TElement>*> ChangedEvent;
	TEvent<Profiler<TElement>*, const std::string&, TElement&> NewProfileEvent;
	TEvent<Profiler<TElement>*, const std::string&, TElement&> PreRemoveProfileEvent;
	TEvent<Profiler<TElement>*, const std::string&, TElement&> ProfileSwitchEvent;

private:

	std::pair<std::string, TElement>* m_Current;
	std::unordered_map<std::string, TElement*> m_ProfileMap;
	std::list<std::pair<std::string, TElement>> m_Profiles;

	void Copy(const Profiler<TElement>& other);
};

template<class TElement>
std::list<std::pair<std::string, TElement>>& Profiler<TElement>::profiles()
{
	return m_Profiles;
}

template<class TElement>
Profiler<TElement>::Profiler() : m_Profiles(), m_ProfileMap(), m_Current(nullptr)
{ }

template<class TElement>
Profiler<TElement>::Profiler(const std::string& name, const TElement& element) : Profiler()
{
	add_profile(name, element);
}

template<class TElement>
Profiler<TElement>::Profiler(std::string&& name, TElement&& element) : Profiler()
{ 
	add_profile(name, element);
}

template<class TElement>
Profiler<TElement>::Profiler(Profiler<TElement>&& other) noexcept :
	m_Profiles(std::move(other.m_Profiles)), m_ProfileMap(std::move(other.m_ProfileMap)), m_Current(std::move(other.m_Current))
{ }

template<class TElement>
void Profiler<TElement>::Copy(const Profiler<TElement>& other)
{
	if (other.m_Current != nullptr)
	{
		auto& name = other.m_Current->first;
		auto it = std::find_if(m_Profiles.begin(), m_Profiles.end(),
			[&name](const std::pair<std::string, TElement>& profile) { return profile.first == name; });
		assert(it != m_Profiles.end());

		m_Current = &*it;
	}

	for (auto& [name, container] : m_Profiles)
	{
		m_ProfileMap[name] = &container;
	}
}

template<class TElement>
Profiler<TElement>::Profiler(const Profiler<TElement>& other) :
	m_Profiles(other.m_Profiles), m_ProfileMap()
{
	Copy(other);
}

template<class TElement>
Profiler<TElement>& Profiler<TElement>::operator=(Profiler<TElement>&& other) noexcept
{
	if (this == &other)
		return *this;

	m_Profiles = std::move(other.m_Profiles);
	m_ProfileMap = std::move(other.m_ProfileMap);
	m_Current = std::move(other.m_Current);

	return *this;
}

template<class TElement>
Profiler<TElement>& Profiler<TElement>::operator=(const Profiler<TElement>& other)
{
	m_Profiles = other.m_Profiles;
	Copy(other);
	return *this;
}

template<class TElement>
void Profiler<TElement>::add_profile(const std::string& name, const TElement& element)
{
	if (m_ProfileMap.count(name) > 0)
		return;

	auto& placedElement = m_Profiles.emplace_back(name, element);
	m_ProfileMap[placedElement.first] = &placedElement.second;

	if (m_Current == nullptr)
		m_Current = &placedElement;

	NewProfileEvent(this, placedElement.first, placedElement.second);
	ChangedEvent(this);
}

template<class TElement>
void Profiler<TElement>::add_profile(std::string&& name, TElement&& element)
{
	if (m_ProfileMap.count(name) > 0)
		return;

	auto& placedElement = m_Profiles.emplace_back(std::move(name), std::move(element));
	m_ProfileMap[placedElement.first] = &placedElement.second;

	if (m_Current == nullptr)
		m_Current = &placedElement;
	
	NewProfileEvent(this, placedElement.first, placedElement.second);
	ChangedEvent(this);
}

template<class TElement>
void Profiler<TElement>::remove_profile(const std::string& name)
{
	if (m_Profiles.size() <= 1)
		return;

	auto it = std::find_if(m_Profiles.begin(), m_Profiles.end(),
		[&name](const std::pair<std::string, TElement>& profile) { return profile.first == name; });

	if (it == m_Profiles.end())
		return;

	if (&*it == m_Current)
		switch_profile(m_Profiles.begin()->first);

	PreRemoveProfileEvent(this, name, it->second);

	m_ProfileMap.erase(name);
	m_Profiles.erase(it);

	ChangedEvent(this);
}


template<class TElement>
const std::list<std::pair<std::string, TElement>>& Profiler<TElement>::profiles() const
{
	return m_Profiles;
}

template<class TElement>
void Profiler<TElement>::rename_profile(const std::string& oldName, const std::string& newName)
{
	if (has_profile(newName))
		return;

	auto it = std::find_if(m_Profiles.begin(), m_Profiles.end(),
		[&oldName](const std::pair<std::string, TElement>& profile) { return profile.first == oldName; });

	if (it == m_Profiles.end())
		return;

	it->first = newName;

	m_ProfileMap.erase(oldName);
	m_ProfileMap[newName] = &it->second;
	ChangedEvent(this);
}

template<class TElement>
bool Profiler<TElement>::has_profile(const std::string& name) const
{
	return m_ProfileMap.count(name) > 0;
}

template<class TElement>
const TElement& Profiler<TElement>::profile(const std::string& name) const
{
	return *m_ProfileMap.at(name);
}

template<class TElement>
TElement& Profiler<TElement>::profile(const std::string& name)
{
	return *m_ProfileMap.at(name);
}

template<class TElement>
const std::string& Profiler<TElement>::current_name() const
{
	assert(m_Current != nullptr);
	return m_Current->first;
}

template<class TElement>
const TElement& Profiler<TElement>::current() const
{
	assert(m_Current != nullptr);
	return m_Current->second;
}

template<class TElement>
TElement& Profiler<TElement>::current()
{
	assert(m_Current != nullptr);
	return m_Current->second;
}

template<class TElement>
void Profiler<TElement>::switch_profile(const std::string& name)
{
	auto it = std::find_if(m_Profiles.begin(), m_Profiles.end(),
		[&name](const std::pair<std::string, TElement>& profile) { return profile.first == name; });
	
	if (it != m_Profiles.end())
	{
		m_Current = &*it;
		ProfileSwitchEvent(this, name, m_Current->second);
	}
}

template<class TElement>
void Profiler<TElement>::to_json(nlohmann::json& j) const
{
	j = {
		{ "current", current_name() },
		{ "profiles" , m_Profiles }
	};
}

template<class TElement>
void Profiler<TElement>::from_json(const nlohmann::json& j)
{
	j.at("profiles").get_to(m_Profiles);

	auto current = j.at("current").get<std::string>();
	switch_profile(current);
}