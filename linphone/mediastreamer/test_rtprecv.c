  /*
  The mediastreamer library aims at providing modular media processing and I/O
	for linphone, but also for any telephony application.
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

#include "msrtprecv.h"
#include "ms.h"
#include "mswrite.h"
#include "msosswrite.h"
#include "msMUlawdec.h"
#include "mstimer.h"
#include "msfdispatcher.h"

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

static int cond=1;

void stop_handler(int signum)
{
	cond=0;
}

int main()
{
	MSFilter *play,*dec,*rec,*filerec,*dis;
	MSSync *timer;
	RtpSession *rtps;
	
	/*create the rtp session */
	ortp_init();
	rtps=rtp_session_new(RTP_SESSION_RECVONLY);
	rtp_session_set_local_addr(rtps,"0.0.0.0",8000);
	rtp_session_set_scheduling_mode(rtps,0);
	rtp_session_set_blocking_mode(rtps,0);

	ms_init();
	signal(SIGINT,stop_handler);
	
	play=ms_rtp_recv_new();
	rec=ms_oss_write_new();
	ms_sound_write_set_device(MS_SOUND_WRITE(rec),0);
	dec=ms_MULAWdecoder_new();
	filerec=ms_write_new("/tmp/rtpstream");
	dis=ms_fdispatcher_new();
	timer=ms_timer_new();

	ms_rtp_recv_set_session(MS_RTP_RECV(play),rtps);
	
	ms_filter_add_link(play,dec);
	ms_filter_add_link(dec,dis);
	ms_filter_add_link(dis,rec);
	ms_filter_add_link(dis,filerec);
	ms_sync_attach(timer,play);
	printf("gran=%i\n",MS_SYNC(timer)->samples_per_tick);
	
	ms_start(timer);
	ms_sound_write_start(MS_SOUND_WRITE(rec));
	while(cond)
	{
		sleep(1);
	}
	
	printf("stoping sync...\n");
	ms_stop(timer);
	ms_sound_write_stop(MS_SOUND_WRITE(rec));
	printf("unlinking filters...\n");
	ms_filter_remove_links(play,dec);
	ms_filter_remove_links(dec,rec);
	printf("destroying filters...\n");
	ms_filter_destroy(play);
	ms_filter_destroy(dec);
	ms_filter_destroy(rec);
	ms_filter_destroy(dis);
	ms_filter_destroy(filerec);
	
	rtp_session_destroy(rtps);
	ms_sync_destroy(timer);
	ortp_global_stats_display();
	
	return 0;
}
