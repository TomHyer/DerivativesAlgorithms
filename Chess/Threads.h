#pragma once

#include "Shared.h"

constexpr int PIPE_BUF = 4096;

static void put_line(const char* line, size_t linelen)
{
	if (linelen > PIPE_BUF)
	{
		log_msg("warning: output \"%s\" too long (max is %u, got %u)\n", line, PIPE_BUF, linelen);
		return;
	}
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD len;
	if (!WriteFile(handle, line, (DWORD)linelen, &len, NULL) || len != linelen)
		error_msg("failed to write output (%d)", GetLastError());
	FlushFileBuffers(handle);
}

#ifdef SINGLE_THREAD
template<class T_> void Say(const T_& what) { std::cout << what << "\n"; }
#define LOCK_SHARED
#else
mutex_t mutex_init()
{
	SECURITY_ATTRIBUTES attr;
	memset(&attr, 0, sizeof(attr));
	attr.nLength = sizeof(attr);
	attr.bInheritHandle = TRUE;
	mutex_t retval = CreateMutex(&attr, FALSE, NULL);
	if (retval == NULL)
		error_msg("failed to create mutex (%d)", GetLastError());
	return retval;
}

void mutex_lock0(mutex_t* mutex)
{
	if (WaitForSingleObject(*mutex, INFINITE) != WAIT_OBJECT_0)
		error_msg("failed to lock mutex (%d)", GetLastError());
}
static bool mutex_try_lock(mutex_t* mutex, uint64_t timeout)
{
	switch (WaitForSingleObject(*mutex, (DWORD)timeout))
	{
	case WAIT_OBJECT_0:
		return true;
	case WAIT_TIMEOUT:
		return false;
	default:
		error_msg("failed to lock mutex (%d)", GetLastError());
	}
}

void mutex_unlock0(mutex_t* mutex)
{
	if (!ReleaseMutex(*mutex))
		error_msg("failed to unlock mutex (%d)", GetLastError());
}

void mutex_discard(mutex_t* mutex)
{
	mutex_unlock0(mutex);
	//	CloseHandle(mutex);
}

void Say(const std::string& what)
{
	if (SHARED)
		mutex_lock0(&SHARED->outMutex);
	put_line(what.c_str(), what.size());
	if (SHARED)
		mutex_unlock0(&SHARED->outMutex);
}

event_t event_init()
{
	SECURITY_ATTRIBUTES attr;
	memset(&attr, 0, sizeof(attr));
	attr.nLength = sizeof(attr);
	attr.bInheritHandle = TRUE;
	event_t retval = CreateEvent(&attr, TRUE, FALSE, NULL);
	if (retval == NULL)
		error_msg("failed to create event (%d)", GetLastError());
	return retval;
}

void event_discard(event_t* event)
{
	if (event)
		CloseHandle(event);
}

void mutex_lock1(mutex_t* mutex, int lineno)
{
	mutex_lock0(mutex);
	if (SHARED && !SHARED->working.Empty())
	{
		std::string what = "id name locked to " + Str(GetCurrentProcessId()) + " at " + Str(lineno) + "\n";
		put_line(what.c_str(), what.size());
	}
}
void mutex_unlock1(mutex_t* mutex, int lineno)
{
	if (SHARED && !SHARED->stopAll)
	{
		char line[64];
		int len = snprintf(line, sizeof(line) - 1, "id name unlocked by %d at %d\n", GetCurrentProcessId(), lineno);
		put_line(line, len);
	}
	mutex_unlock0(mutex);
}

#define mutex_lock(M) mutex_lock1(M, __LINE__)
#define mutex_unlock(M) mutex_unlock1(M, __LINE__)

struct SharedLock_
{
	int line_;
	SharedLock_(int line) : line_(line)
	{
		mutex_lock0(&SHARED->mutex);
	}
	~SharedLock_()
	{
		mutex_unlock0(&SHARED->mutex);
	}
};
#define LOCK_SHARED SharedLock_ _(__LINE__);
#endif

