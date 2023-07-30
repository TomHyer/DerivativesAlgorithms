#pragma once

#include "Time.h"

constexpr size_t LINE_BUF = 4 * PIPE_BUF;

#ifdef SINGLE_THREAD
bool get_line(char* line, unsigned linelen, DWORD)
{
	(void) fgets(line, linelen, stdin);
	return true;
}
template<class T_>
void uci_await_line(char* line, time_point* curr_time, T_)
{
	std::string in;
	getline(std::cin, in);
	auto size = Min(in.size(), LINE_BUF - 1);
	copy(in.begin(), in.begin() + size, line);
	line[size] = 0;
	*curr_time = now();
}
#else
static DWORD forward_input(void* param)
{
	char buf[4 * PIPE_BUF];
	HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE out = (HANDLE)param;
	while (true)
	{
		DWORD len;
		if (!ReadFile(in, buf, sizeof(buf), &len, NULL))
			error_msg("failed to read input (%d)", GetLastError());
		if (len == 0)
		{
			CloseHandle(out);
			return 0;
		}

		DWORD ptr = 0;
		while (ptr < len)
		{
			DWORD writelen;
			if (!WriteFile(out, buf + ptr, len - ptr, &writelen, NULL))
				error_msg("failed to forward input (%d)", GetLastError());
			ptr += writelen;
		}

		FlushFileBuffers(out);
	}
}

bool get_line(char* line, unsigned linelen, DWORD timeout)
{
	static char buf[4 * PIPE_BUF];
	static unsigned ptr = 0, end = 0;
	unsigned i = 0;

	static HANDLE handle = INVALID_HANDLE_VALUE;
	static event_t event = NULL;
	static bool init = false;
	if (!init)
	{
		handle = GetStdHandle(STD_INPUT_HANDLE);
		if (GetFileType(handle) == FILE_TYPE_PIPE)
		{
			char name[256];
			int res = snprintf(name, sizeof(name) - 1, "\\\\.\\pipe\\Roc%u_pipe", GetCurrentProcessId());
			if (res < 0 || res >= sizeof(name) - 1)
				error_msg("failed to create pipe name");
			HANDLE out = CreateNamedPipe(name,
				PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
				PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 2,
				4 * PIPE_BUF, 4 * PIPE_BUF, 0, NULL);
			if (out == INVALID_HANDLE_VALUE)
				error_msg("failed to create named pipe #1 (%d)", GetLastError());
			handle = CreateFile(name, GENERIC_READ, 0, NULL,
				OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
			if (handle == INVALID_HANDLE_VALUE)
				error_msg("failed to create named pipe #2 (%d)", GetLastError());
			HANDLE thread = CreateThread(NULL, 0, forward_input, (LPVOID)out, 0, NULL);
			if (thread == NULL)
				error_msg("failed to create thread (%d)", GetLastError());
		}
		event = event_init();
		init = true;
	}

	while (true)
	{
		bool space = false;
		while (ptr < end)
		{
			if (i >= linelen)
				error_msg("input buffer overflow");
			char c = buf[ptr++];
			switch (c)
			{
			case ' ': case '\r': case '\t': case '\n':
				if (!space)
				{
					line[i++] = ' ';
					space = true;
				}
				if (c == '\n')
				{
					line[i - 1] = '\0';
					return false;
				}
				continue;
			default:
				space = false;
				line[i++] = c;
				continue;
			}
		}

		OVERLAPPED overlapped;
		memset(&overlapped, 0, sizeof(overlapped));
		overlapped.hEvent = event;
		DWORD len;
		if (!ReadFile(handle, buf, sizeof(buf), &len, &overlapped))
		{
			if (GetLastError() != ERROR_IO_PENDING)
				error_msg("failed to read input (%d)", GetLastError());
			bool timedout = false;

			switch (WaitForSingleObject(event, timeout))
			{
			case WAIT_TIMEOUT:
				if (!CancelIo(handle))
					error_msg("failed to cancel input (%d)", GetLastError());
				timedout = true;
				break;
			case WAIT_OBJECT_0:
				break;
			default:
				error_msg("failed to wait for input (%d)", GetLastError());
			}
			if (!GetOverlappedResult(handle, &overlapped, &len, FALSE))
			{
				if (timedout && GetLastError() == ERROR_OPERATION_ABORTED)
					return true;
				error_msg("failed to get input result (%d)", GetLastError());
			}
		}

		if (len == 0)
		{
			line[0] = EOF;
			return false;
		}

		ptr = 0;
		end = len;
	}
}

template<class STOP_>
void uci_await_line(char* line, time_point* curr_time, STOP_ stop_handler)
{
	while (true)
	{
		DWORD timeout = std::numeric_limits<DWORD>::max();
		//			if (!SHARED->working.Empty())
		//				timeout = 100;          // 100ms

		bool timedout = get_line(line, sizeof(line) - 1, timeout);
		*curr_time = now();

		if (mutex_try_lock(&SHARED->mutex, 500))
			mutex_unlock(&SHARED->mutex);
		else
			stop_handler.emergency_stop();
		if (!SHARED->working.Empty() && millisecs(SHARED->startTime, *curr_time) >= SHARED->hardTimeLimit / 2)
			stop_handler.await_stop();
		if (!timedout)
			return;
	}
}

#endif

