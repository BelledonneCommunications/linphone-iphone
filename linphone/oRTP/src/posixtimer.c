/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc1889) stack.
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
#include "rtptimer.h"

#if	!defined(_WIN32) && !defined(_WIN32_WCE)

#ifdef __linux__
#include <sys/select.h>
#endif

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


static struct timeval orig,cur;
static uint32_t posix_timer_time=0;		/*in milisecond */

void posix_timer_init()
{
	posix_timer.state=RTP_TIMER_RUNNING;
	gettimeofday(&orig,NULL);
	posix_timer_time=0;
}




void posix_timer_do()
{
	int diff,time;
	struct timeval tv;
	gettimeofday(&cur,NULL);
	time=((cur.tv_usec-orig.tv_usec)/1000 ) + ((cur.tv_sec-orig.tv_sec)*1000 );
	if ( (diff=time-posix_timer_time)>50){
		ortp_warning("Must catchup %i miliseconds.",diff);
	}
	while((diff = posix_timer_time-time) > 0)
	{
		tv.tv_sec = diff/1000;
		tv.tv_usec = (diff%1000)*1000;
#if	defined(_WIN32) || defined(_WIN32_WCE)
        /* this kind of select is not supported on windows */
		Sleep(tv.tv_usec/1000 + tv.tv_sec * 1000);
#else
		select(0,NULL,NULL,NULL,&tv);
#endif
		gettimeofday(&cur,NULL);
		time=((cur.tv_usec-orig.tv_usec)/1000 ) + ((cur.tv_sec-orig.tv_sec)*1000 );
	}
	posix_timer_time+=POSIXTIMER_INTERVAL/1000;
	
}

void posix_timer_uninit()
{
	posix_timer.state=RTP_TIMER_STOPPED;
}

RtpTimer posix_timer={	0,
						posix_timer_init,
						posix_timer_do,
						posix_timer_uninit,
						{0,POSIXTIMER_INTERVAL}};
							
							
#else //WIN32


#include <windows.h>
#include <mmsystem.h>


MMRESULT timerId;
HANDLE   TimeEvent;
int      late_ticks;


static DWORD posix_timer_time;
static DWORD offset_time;


#define TIME_INTERVAL           50
#define TIME_RESOLUTION         10
#define TIME_TIMEOUT            100



void CALLBACK timerCb(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
        // Check timerId
        if (timerId == uID)
        {
                SetEvent(TimeEvent);
                posix_timer_time += TIME_INTERVAL;
        }
}


void win_timer_init(void)
{
        timerId = timeSetEvent(TIME_INTERVAL,10,timerCb,0,TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
        TimeEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

        late_ticks = 0;

        offset_time = GetTickCount();
        posix_timer_time=0;
}


void win_timer_do(void)
{
        DWORD diff;

        // If timer have expired while we where out of this method
        // Try to run after lost time.
        if (late_ticks > 0)
        {
                late_ticks--;
                posix_timer_time+=TIME_INTERVAL;
                return;
        }


        diff = GetTickCount() - posix_timer_time - offset_time;
        if( diff>TIME_INTERVAL && (diff<(1<<31)))
        {
                late_ticks = diff/TIME_INTERVAL;
                ortp_warning("we must catchup %i ticks.",late_ticks);
                return;
        }

        WaitForSingleObject(TimeEvent,TIME_TIMEOUT);
        return;
}


void win_timer_close(void)
{
	timeKillEvent(timerId); 
}

RtpTimer toto;

RtpTimer posix_timer={	0,
						win_timer_init,
						win_timer_do,
						win_timer_close,
						{0,TIME_INTERVAL * 1000}};
							

#endif // _WIN32
