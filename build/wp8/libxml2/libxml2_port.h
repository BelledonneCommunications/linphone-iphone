
#ifndef LIBXML2_PORT_H
#define LIBXML2_PORT_H

#define CreateMutex(a, b, c)	CreateMutexExW(a, c, ((b) ? CREATE_MUTEX_INITIAL_OWNER : 0), 0)

#define GetVersionEx(osvi)		(((osvi)->dwPlatformId = 0) != 0)

#define InitializeCriticalSection(cs)	InitializeCriticalSectionEx(cs, 0, 0)

#define WaitForSingleObject(hHandle, dwMilliseconds)	WaitForSingleObjectEx(hHandle, dwMilliseconds, 0)

#define Sleep(ms)	{ \
	HANDLE sleepEvent = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS); \
	if (!sleepEvent) return; \
	WaitForSingleObjectEx(sleepEvent, ms, FALSE); \
}

#endif /* LIBXML2_PORT_H */
