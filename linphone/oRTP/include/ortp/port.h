/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/* this file is responsible of the portability of the stack */

#ifndef ORTP_PORT_H
#define ORTP_PORT_H


#if !defined(WIN32) && !defined(_WIN32_WCE)
/********************************/
/* definitions for UNIX flavour */
/********************************/

#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __linux
#include <stdint.h>
#endif


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#if defined(_XOPEN_SOURCE_EXTENDED) || !defined(__hpux)
#include <arpa/inet.h>
#endif



#include <sys/time.h>

#ifdef ORTP_INET6
#include <netdb.h>
#endif

typedef int ortp_socket_t;
typedef pthread_t ortp_thread_t;
typedef pthread_mutex_t ortp_mutex_t;
typedef pthread_cond_t ortp_cond_t;

#ifdef __INTEL_COMPILER
#pragma warning(disable : 111)		// statement is unreachable
#pragma warning(disable : 181)		// argument is incompatible with corresponding format string conversion
#pragma warning(disable : 188)		// enumerated type mixed with another type
#pragma warning(disable : 593)		// variable "xxx" was set but never used
#pragma warning(disable : 810)		// conversion from "int" to "unsigned short" may lose significant bits
#pragma warning(disable : 869)		// parameter "xxx" was never referenced
#pragma warning(disable : 981)		// operands are evaluated in unspecified order
#pragma warning(disable : 1418)		// external function definition with no prior declaration
#pragma warning(disable : 1419)		// external declaration in primary source file
#pragma warning(disable : 1469)		// "cc" clobber ignored
#endif

#ifdef __cplusplus
extern "C"
{
#endif

int __ortp_thread_join(ortp_thread_t thread, void **ptr);
int __ortp_thread_create(pthread_t *thread, pthread_attr_t *attr, void * (*routine)(void*), void *arg);

#ifdef __cplusplus
}
#endif

#define ortp_thread_create	__ortp_thread_create
#define ortp_thread_join	__ortp_thread_join
#define ortp_thread_exit	pthread_exit
#define ortp_mutex_init		pthread_mutex_init
#define ortp_mutex_lock		pthread_mutex_lock
#define ortp_mutex_unlock	pthread_mutex_unlock
#define ortp_mutex_destroy	pthread_mutex_destroy
#define ortp_cond_init		pthread_cond_init
#define ortp_cond_signal	pthread_cond_signal
#define ortp_cond_broadcast	pthread_cond_broadcast
#define ortp_cond_wait		pthread_cond_wait
#define ortp_cond_destroy	pthread_cond_destroy

#define SOCKET_OPTION_VALUE	void *
#define SOCKET_BUFFER		void *

#define getSocketError() strerror(errno)
#define getSocketErrorCode() (errno)

#else
/*********************************/
/* definitions for WIN32 flavour */
/*********************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <winsock2.h>
#include <ws2tcpip.h>


#ifdef _MSC_VER
#pragma push_macro("_WINSOCKAPI_")
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

typedef  unsigned __int64 uint64_t;
typedef  __int64 int64_t;
typedef  unsigned short uint16_t;
typedef  unsigned int uint32_t;
typedef  int int32_t;
typedef  unsigned char uint8_t;
typedef __int16 int16_t;
#else
#include <stdint.h> /*provided by mingw32*/
#endif

#define vsnprintf	_vsnprintf
#define srandom		srand
#define random		rand


typedef SOCKET ortp_socket_t;
typedef HANDLE ortp_cond_t;
typedef HANDLE ortp_mutex_t;
typedef HANDLE ortp_thread_t;

#define ortp_thread_create	WIN_thread_create
#define ortp_thread_join	WIN_thread_join
#define ortp_thread_exit(arg)		
#define ortp_mutex_init		WIN_mutex_init
#define ortp_mutex_lock		WIN_mutex_lock
#define ortp_mutex_unlock	WIN_mutex_unlock
#define ortp_mutex_destroy	WIN_mutex_destroy
#define ortp_cond_init		WIN_cond_init
#define ortp_cond_signal	WIN_cond_signal
#define ortp_cond_broadcast	WIN_cond_broadcast
#define ortp_cond_wait		WIN_cond_wait
#define ortp_cond_destroy	WIN_cond_destroy


#ifdef __cplusplus
extern "C"
{
#endif
	
int WIN_mutex_init(ortp_mutex_t *m, void *attr_unused);
int WIN_mutex_lock(ortp_mutex_t *mutex);
int WIN_mutex_unlock(ortp_mutex_t *mutex);
int WIN_mutex_destroy(ortp_mutex_t *mutex);
int WIN_thread_create(ortp_thread_t *t, void *attr_unused, void *(*func)(void*), void *arg); 
int WIN_thread_join(ortp_thread_t thread, void **unused);
int WIN_cond_init(ortp_cond_t *cond, void *attr_unused);
int WIN_cond_wait(ortp_cond_t * cond, ortp_mutex_t * mutex);
int WIN_cond_signal(ortp_cond_t * cond);
int WIN_cond_broadcast(ortp_cond_t * cond);
int WIN_cond_destroy(ortp_cond_t * cond);

#ifdef __cplusplus
}
#endif

#define SOCKET_OPTION_VALUE	char *
#define inline			__inline

const char *getWinSocketError(int error);
#define getSocketErrorCode() WSAGetLastError()
#define getSocketError() getWinSocketError(WSAGetLastError())

#define snprintf _snprintf
#define strcasecmp _stricmp

#if 0
struct timeval {
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* and microseconds */
};
#endif

int gettimeofday (struct timeval *tv, void* tz);
#ifdef _WORKAROUND_MINGW32_BUGS
char * WSAAPI gai_strerror(int errnum);
#endif


#endif

typedef unsigned char bool_t;
#undef TRUE
#undef FALSE
#define TRUE 1
#define FALSE 0

#ifdef __cplusplus
extern "C"{
#endif

void* ortp_malloc(size_t sz);
void ortp_free(void *ptr);
void* ortp_realloc(void *ptr, size_t sz);
void* ortp_malloc0(size_t sz);
char * ortp_strdup(const char *tmp);

/*override the allocator with this method, to be called BEFORE ortp_init()*/
typedef struct _OrtpMemoryFunctions{
	void *(*malloc_fun)(size_t sz);
	void *(*realloc_fun)(void *ptr, size_t sz);
	void (*free_fun)(void *ptr);
}OrtpMemoryFunctions;

void ortp_set_memory_functions(OrtpMemoryFunctions *functions);

#define ortp_new(type,count)	ortp_malloc(sizeof(type)*(count))
#define ortp_new0(type,count)	ortp_malloc0(sizeof(type)*(count))

int close_socket(ortp_socket_t sock);
int set_non_blocking_socket(ortp_socket_t sock);

char *ortp_strndup(const char *str,int n);
char *ortp_strdup_printf(const char *fmt,...);
char *ortp_strdup_vprintf(const char *fmt, va_list ap);

/* portable named pipes */
#if !defined(_WIN32_WCE)
#ifdef WIN32
typedef HANDLE ortp_pipe_t;
#define ORTP_PIPE_INVALID INVALID_HANDLE_VALUE
#else
typedef int ortp_pipe_t;
#define ORTP_PIPE_INVALID (-1)
#endif

ortp_pipe_t ortp_server_pipe_create(const char *name);
/*
 * warning: on win32 ortp_server_pipe_accept_client() might return INVALID_HANDLE_VALUE without
 * any specific error, this happens when ortp_server_pipe_close() is called on another pipe.
 * This pipe api is not thread-safe.
*/
ortp_pipe_t ortp_server_pipe_accept_client(ortp_pipe_t server);
int ortp_server_pipe_close(ortp_pipe_t spipe);
int ortp_server_pipe_close_client(ortp_pipe_t client);

ortp_pipe_t ortp_client_pipe_connect(const char *name);
int ortp_client_pipe_close(ortp_pipe_t sock);

int ortp_pipe_read(ortp_pipe_t p, uint8_t *buf, int len);
int ortp_pipe_write(ortp_pipe_t p, const uint8_t *buf, int len);
#endif

#ifdef __cplusplus
}
#endif


#if (defined(WIN32) || defined(_WIN32_WCE)) && !defined(ORTP_STATIC)
#ifdef ORTP_EXPORTS
   #define VAR_DECLSPEC    __declspec(dllexport)
#else
   #define VAR_DECLSPEC    __declspec(dllimport)
#endif
#else
   #define VAR_DECLSPEC    extern
#endif


#endif


