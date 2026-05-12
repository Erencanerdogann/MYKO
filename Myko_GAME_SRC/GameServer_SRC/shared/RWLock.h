#pragma once

#include <shared_mutex>

class RWLock
{
public:
	void AcquireReadLock() { _mutex.lock_shared(); }
	void ReleaseReadLock() { _mutex.unlock_shared(); }
	void AcquireWriteLock() { _mutex.lock(); }
	void ReleaseWriteLock() { _mutex.unlock(); }

private:
	std::shared_mutex _mutex;
};
