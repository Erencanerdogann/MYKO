#pragma once
#include <queue>

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define THREADCALL WINAPI
#define STRCASECMP _stricmp

#define I64FMT "%016I64X"
#define I64FMTD "%I64u"
#define SI64FMTD "%I64d"

#define CLOSEDEBUGMODE 1

#if CLOSEDEBUGMODE==0

#if defined(_DEBUG) || defined(DEBUG)
#	include <cassert>
#	include "DebugUtils.h"

#	define ASSERT assert
#	define TRACE FormattedDebugString

//	Enables tracing to stdout. 
//	Preferable with the VS debugger (is thrown in the "output" window), but
//	it can be spammy otherwise (especially if you don't need it enabled).
#	define USE_SQL_TRACE

//	Ensure both typically used debug defines behave as intended
#	ifndef DEBUG
#		define DEBUG
#	endif

#	ifndef _DEBUG
#		define _DEBUG
#	endif

#else
#	define TRACE(...) ((void)0)
#endif

#else

#define ASSERT
#define TRACE(...) ((void)0)
#endif

// Ignore the warning "nonstandard extension used: enum '<enum name>' used in qualified name"
// Sometimes it's necessary to avoid collisions, but aside from that, specifying the enumeration helps make code intent clearer.
#pragma warning(disable: 4482)

#define STR(str) #str
#define STRINGIFY(str) STR(str)

#include <thread>
#include <future>
#include <chrono>
#include <atomic>
#include <mutex>
#include <shared_mutex>


class SRWLock
{
	std::atomic<unsigned> w_tid; // only tracks exclusive (write) owner

public:
	SRWLock()
	{
		w_tid.store(0xffffffff, std::memory_order_relaxed);
		InitializeSRWLock(&b_lock);
	}
	void w_lock()
	{
		unsigned me = GetCurrentThreadId();
		// Detect recursive write lock (would deadlock on Windows SRWLOCK)
		if (w_tid.load(std::memory_order_relaxed) == me)
		{
			printf("#Error: Recursive write lock is not suitable for SRWLock, owner %d\n", me);
			ASSERT(0);
		}

		AcquireSRWLockExclusive(&b_lock);
		w_tid.store(me, std::memory_order_relaxed);
	}
	void w_unlock()
	{
		w_tid.store(0xffffffff, std::memory_order_relaxed);
		ReleaseSRWLockExclusive(&b_lock);
	}

	void r_lock()
	{
		unsigned me = GetCurrentThreadId();
		// Detect read-while-write-held on same thread (would deadlock on Windows SRWLOCK)
		if (w_tid.load(std::memory_order_relaxed) == me)
		{
			printf("#Error: Read lock while holding write lock is not suitable for SRWLock, thread %d\n", me);
			ASSERT(0);
		}
		AcquireSRWLockShared(&b_lock);
		// NOTE: Do NOT set w_tid here - multiple readers can co-exist,
		// and setting a single tid from readers was a data race.
	}
	void r_unlock()
	{
		// NOTE: Do NOT clear w_tid here - readers never owned it.
		ReleaseSRWLockShared(&b_lock);
	}
public:
	SRWLOCK b_lock;
};

class Guard
{
public:
	Guard(std::recursive_mutex& mutex) : target(mutex) { target.lock(); }
	Guard(std::recursive_mutex* mutex) : target(*mutex) { target.lock(); }
	~Guard() { target.unlock(); }

protected:
	std::recursive_mutex& target;
};

#define sleep(ms) Sleep(ms)

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

// define compiler-specific types
#include "types.h"

#include <random>
#include <memory>
#include <map>
#include <list>
#include <vector>

#include "tstring.h"
#include "globals.h"
#include "Atomic.h"
#include "Thread.h"
#include "Network.h"
#include "TimeThread.h"
#include "LogSystem.h"
#include "ServerConfig.h"
#include "MemoryPool.h"