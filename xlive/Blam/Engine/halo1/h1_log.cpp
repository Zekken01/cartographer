#include "stdafx.h"
#include "h1_log.h"

static FILE* g_h1_log_file = NULL;
static CRITICAL_SECTION g_h1_log_lock;
static bool g_h1_log_lock_initialized = false;

void h1_log(const char* format, ...)
{
	if (!g_h1_log_lock_initialized)
	{
		InitializeCriticalSection(&g_h1_log_lock);
		g_h1_log_lock_initialized = true;
	}

	EnterCriticalSection(&g_h1_log_lock);
	if (!g_h1_log_file)
	{
		g_h1_log_file = _fsopen("h1_maps.log", "w", _SH_DENYWR);
	}

	if (g_h1_log_file)
	{
		SYSTEMTIME time;
		GetLocalTime(&time);
		fprintf(g_h1_log_file, "%02d:%02d:%02d.%03d ", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

		va_list args;
		va_start(args, format);
		vfprintf(g_h1_log_file, format, args);
		va_end(args);

		fputc('\n', g_h1_log_file);
		fflush(g_h1_log_file);
	}
	LeaveCriticalSection(&g_h1_log_lock);
	return;
}

void h1_log_flush(void)
{
	if (g_h1_log_file)
	{
		fflush(g_h1_log_file);
	}
	return;
}
