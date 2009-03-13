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


#if defined(WIN32) || defined(_WIN32_WCE)
#include "ortp-config-win32.h"
#else
#include "ortp-config.h"
#endif
#include "ortp/ortp.h"
#include "scheduler.h"

rtp_stats_t ortp_global_stats;

#ifdef ENABLE_MEMCHECK
int ortp_allocations=0;
#endif


RtpScheduler *__ortp_scheduler;



extern void av_profile_init(RtpProfile *profile);

static void init_random_number_generator(){
	struct timeval t;
	gettimeofday(&t,NULL);
	srandom(t.tv_usec+t.tv_sec);
}


#ifdef WIN32
static bool_t win32_init_sockets(void){
	WORD wVersionRequested;
	WSADATA wsaData;
	int i;
	
	wVersionRequested = MAKEWORD(2,0);
	if( (i = WSAStartup(wVersionRequested,  &wsaData))!=0)
	{
		ortp_error("Unable to initialize windows socket api, reason: %d (%s)",i,getWinSocketError(i));
		return FALSE;
	}
	return TRUE;
}
#endif

/**
 *	Initialize the oRTP library. You should call this function first before using
 *	oRTP API.
**/
void ortp_init()
{
	static bool_t initialized=FALSE;
	if (initialized) return;
	initialized=TRUE;

#ifdef WIN32
	win32_init_sockets();
#endif

	av_profile_init(&av_profile);
	ortp_global_stats_reset();
	init_random_number_generator();
	ortp_message("oRTP-" ORTP_VERSION " initialized.");
}


/**
 *	Initialize the oRTP scheduler. You only have to do that if you intend to use the
 *	scheduled mode of the #RtpSession in your application.
 *	
**/
void ortp_scheduler_init()
{
	static bool_t initialized=FALSE;
	if (initialized) return;
	initialized=TRUE;
#ifdef __hpux
	/* on hpux, we must block sigalrm on the main process, because signal delivery
	is ?random?, well, sometimes the SIGALRM goes to both the main thread and the 
	scheduler thread */
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set,SIGALRM);
	sigprocmask(SIG_BLOCK,&set,NULL);
#endif /* __hpux */

	__ortp_scheduler=rtp_scheduler_new();
	rtp_scheduler_start(__ortp_scheduler);
	//sleep(1);
}


/**
 * Gracefully uninitialize the library, including shutdowning the scheduler if it was started.
 *	
**/
void ortp_exit()
{
	if (__ortp_scheduler!=NULL)
	{
		rtp_scheduler_destroy(__ortp_scheduler);
		__ortp_scheduler=NULL;
	}
}

RtpScheduler * ortp_get_scheduler()
{
	if (__ortp_scheduler==NULL) ortp_error("Cannot use the scheduled mode: the scheduler is not "
									"started. Call ortp_scheduler_init() at the begginning of the application.");
	return __ortp_scheduler;
}


static FILE *__log_file=0;

/**
 *@param file a FILE pointer where to output the ortp logs.
 *
**/
void ortp_set_log_file(FILE *file)
{
	__log_file=file;
}

static void __ortp_logv_out(OrtpLogLevel lev, const char *fmt, va_list args);

OrtpLogFunc ortp_logv_out=__ortp_logv_out;

/**
 *@param func: your logging function, compatible with the OrtpLogFunc prototype.
 *
**/
void ortp_set_log_handler(OrtpLogFunc func){
	ortp_logv_out=func;
}


unsigned int __ortp_log_mask=ORTP_WARNING|ORTP_ERROR|ORTP_FATAL;

/**
 * @ param levelmask a mask of ORTP_DEBUG, ORTP_MESSAGE, ORTP_WARNING, ORTP_ERROR
 * ORTP_FATAL .
**/
void ortp_set_log_level_mask(int levelmask){
	__ortp_log_mask=levelmask;
}

char * ortp_strdup_vprintf(const char *fmt, va_list ap)
{
	/* Guess we need no more than 100 bytes. */
	int n, size = 200;
	char *p,*np;
#ifdef __linux
	va_list cap;/*copy of our argument list: a va_list cannot be re-used (SIGSEGV on linux 64 bits)*/
#endif
	if ((p = (char *) ortp_malloc (size)) == NULL)
		return NULL;
	while (1)
	{
		/* Try to print in the allocated space. */
#ifdef __linux
		va_copy(cap,ap);
		n = vsnprintf (p, size, fmt, cap);
		va_end(cap);
#else
		/*this works on 32 bits, luckily*/
		n = vsnprintf (p, size, fmt, ap);
#endif
		/* If that worked, return the string. */
		if (n > -1 && n < size)
			return p;
		//printf("Reallocing space.\n");
		/* Else try again with more space. */
		if (n > -1)	/* glibc 2.1 */
			size = n + 1;	/* precisely what is needed */
		else		/* glibc 2.0 */
			size *= 2;	/* twice the old size */
		if ((np = (char *) ortp_realloc (p, size)) == NULL)
		  {
		    free(p);
		    return NULL;
		  }
		else
		  {
		    p = np;
		  }
	}
}

char *ortp_strdup_printf(const char *fmt,...){
	char *ret;
	va_list args;
	va_start (args, fmt);
	ret=ortp_strdup_vprintf(fmt, args);
 	va_end (args);
	return ret;
}

#if	defined(WIN32) || defined(_WIN32_WCE)
#define ENDLINE "\r\n"
#else
#define ENDLINE "\n"
#endif

#if	defined(WIN32) || defined(_WIN32_WCE)
void ortp_logv(int level, const char *fmt, va_list args)
{
	if (ortp_logv_out!=NULL && ortp_log_level_enabled(level))
		ortp_logv_out(level,fmt,args);
#if !defined(_WIN32_WCE)
	if ((level)==ORTP_FATAL) abort();
#endif
}
#endif

static void __ortp_logv_out(OrtpLogLevel lev, const char *fmt, va_list args){
	const char *lname="undef";
	char *msg;
	if (__log_file==NULL) __log_file=stderr;
	switch(lev){
		case ORTP_DEBUG:
			lname="debug";
			break;
		case ORTP_MESSAGE:
			lname="message";
			break;
		case ORTP_WARNING:
			lname="warning";
			break;
		case ORTP_ERROR:
			lname="error";
			break;
		case ORTP_FATAL:
			lname="fatal";
			break;
		default:
			ortp_fatal("Bad level !");
	}
	msg=ortp_strdup_vprintf(fmt,args);
#ifdef _MSC_VER
 	OutputDebugString(msg);
  	OutputDebugString("\r\n");
#else
	fprintf(__log_file,"ortp-%s-%s" ENDLINE,lname,msg);
	fflush(__log_file);
#endif

	ortp_free(msg);
}

/**
 * Display global statistics (cumulative for all RtpSession)
**/
void ortp_global_stats_display()
{
	rtp_stats_display(&ortp_global_stats,"Global statistics");
#ifdef ENABLE_MEMCHECK	
	printf("Unfreed allocations: %i\n",ortp_allocations);
#endif
}

/**
 * Print RTP statistics.
**/
void rtp_stats_display(const rtp_stats_t *stats, const char *header)
{
#ifndef WIN32
  ortp_log(ORTP_MESSAGE,
	   "oRTP-stats:\n   %s :",
	   header);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp packet sent=%lld",
	   (long long)stats->packet_sent);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp bytes sent=%lld bytes",
	   (long long)stats->sent);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp packet received=%lld",
	   (long long)stats->packet_recv);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp bytes received=%lld bytes",
	   (long long)stats->hw_recv);
  ortp_log(ORTP_MESSAGE,
	   " number of incoming rtp bytes successfully delivered to the application=%lld ",
	   (long long)stats->recv);
  ortp_log(ORTP_MESSAGE,
	   " number of times the application queried a packet that didn't exist=%lld ",
	   (long long)stats->unavaillable);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp packet lost=%lld",
	   (long long) stats->cum_packet_loss);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp packets received too late=%lld",
	   (long long)stats->outoftime);
  ortp_log(ORTP_MESSAGE,
	   " number of bad formatted rtp packets=%lld",
	   (long long)stats->bad);
  ortp_log(ORTP_MESSAGE,
	   " number of packet discarded because of queue overflow=%lld",
	   (long long)stats->discarded);
#else
  ortp_log(ORTP_MESSAGE,
	   "oRTP-stats:\n   %s :",
	   header);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp packet sent=%I64d",
	   (uint64_t)stats->packet_sent);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp bytes sent=%I64d bytes",
	   (uint64_t)stats->sent);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp packet received=%I64d",
	   (uint64_t)stats->packet_recv);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp bytes received=%I64d bytes",
	   (uint64_t)stats->hw_recv);
  ortp_log(ORTP_MESSAGE,
	   " number of incoming rtp bytes successfully delivered to the application=%I64d ",
	   (uint64_t)stats->recv);
  ortp_log(ORTP_MESSAGE,
	   " number of times the application queried a packet that didn't exist=%I64d ",
	   (uint64_t)stats->unavaillable);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp packet lost=%I64d",
	   (uint64_t) stats->cum_packet_loss);
  ortp_log(ORTP_MESSAGE,
	   " number of rtp packets received too late=%I64d",
	   (uint64_t)stats->outoftime);
  ortp_log(ORTP_MESSAGE,
		 " number of bad formatted rtp packets=%I64d",
	   (uint64_t)stats->bad);
  ortp_log(ORTP_MESSAGE,
	   " number of packet discarded because of queue overflow=%I64d",
	   (uint64_t)stats->discarded);
#endif
}

void ortp_global_stats_reset(){
	memset(&ortp_global_stats,0,sizeof(rtp_stats_t));
}

rtp_stats_t *ortp_get_global_stats(){
	return &ortp_global_stats;
}

void rtp_stats_reset(rtp_stats_t *stats){
	memset((void*)stats,0,sizeof(rtp_stats_t));
}


/**
 * This function give the opportunity to programs to check if the libortp they link to
 * has the minimum version number they need.
 *
 * Returns: true if ortp has a version number greater or equal than the required one.
**/
bool_t ortp_min_version_required(int major, int minor, int micro){
	return ((major*1000000) + (minor*1000) + micro) <= 
		   ((ORTP_MAJOR_VERSION*1000000) + (ORTP_MINOR_VERSION*1000) + ORTP_MICRO_VERSION);
}
