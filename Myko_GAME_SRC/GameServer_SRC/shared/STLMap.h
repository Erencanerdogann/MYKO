#pragma once

#include <map>

// RAII helper for optional locking - acquires lock only if condition is true,
// but stays alive for the full scope (unlike "if (x) Guard lock(m);" which
// destroys the guard immediately).
class OptionalGuard
{
public:
	OptionalGuard(std::recursive_mutex& mutex, bool doLock)
		: target(mutex), locked(doLock)
	{
		if (locked) target.lock();
	}
	~OptionalGuard() { if (locked) target.unlock(); }

	OptionalGuard(const OptionalGuard&) = delete;
	OptionalGuard& operator=(const OptionalGuard&) = delete;

private:
	std::recursive_mutex& target;
	bool locked;
};

template <class T, class Key = uint32>
class CSTLMap
{
public:
	typedef typename std::map<Key, T*>::iterator Iterator;
	std::map<Key, T*> m_UserTypeMap;
	std::recursive_mutex m_lock;
	SRWLock b_lock;

	int GetSize(bool mutex = true)
	{
		// FIX: "if (mutex) Guard lock(m_lock);" was a scoping bug -
		// Guard was destroyed at end of if-statement, not end of function.
		OptionalGuard lock(m_lock, mutex);
		return (int)m_UserTypeMap.size();
	}

	bool IsExist(Key key, bool mutex = true)
	{
		OptionalGuard lock(m_lock, mutex);
		return (m_UserTypeMap.find(key) != m_UserTypeMap.end());
	}

	bool IsEmpty(bool mutex = true) {
		OptionalGuard lock(m_lock, mutex);
		return m_UserTypeMap.empty();
	}

	bool PutData(Key key_value, T* pData, bool mutex = true)
	{
		OptionalGuard lock(m_lock, mutex);
		return m_UserTypeMap.insert(std::make_pair(key_value, pData)).second;
	}

	T* GetData(Key key_value, bool mutex = true)
	{
		// FIX: "if(mutex)Guard lock(m_lock);" was a scoping bug -
		// Guard was destroyed at end of if-statement, map accessed unlocked.
		OptionalGuard lock(m_lock, mutex);
		auto itr = m_UserTypeMap.find(key_value);
		return (itr != m_UserTypeMap.end() ? itr->second : nullptr);
	}

	void DeleteData(Key key_value, bool mutex = true)
	{
		OptionalGuard lock(m_lock, mutex);
		auto itr = m_UserTypeMap.find(key_value);
		if (itr != m_UserTypeMap.end())
		{
			if (itr->second)
				delete itr->second;
			m_UserTypeMap.erase(itr);
		}
	}

	void DeleteAllData(bool mutex = true)
	{
		OptionalGuard lock(m_lock, mutex);

		if (m_UserTypeMap.empty())
			return;

		foreach(itr, m_UserTypeMap)
			delete itr->second;

		m_UserTypeMap.clear();
	}

	~CSTLMap() { DeleteAllData(); }
};