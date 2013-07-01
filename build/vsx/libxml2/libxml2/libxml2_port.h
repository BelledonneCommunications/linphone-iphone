
#ifndef LIBXML2_PORT_H
#define LIBXML2_PORT_H

#define CreateMutex(a, b, c)	CreateMutexEx(a, c, ((b) ? CREATE_MUTEX_INITIAL_OWNER : 0), 0)

#define GetVersionEx(osvi)		(((osvi)->dwPlatformId = 0) != 0)

#endif /* LIBXML2_PORT_H */
