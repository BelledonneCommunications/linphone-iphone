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

#include "mediastream.h"
#include <signal.h>

static gboolean cond=TRUE;

static void stop_handler(int signum){
	cond=FALSE;
}

int main()
{
	VideoStream *v;
	ortp_init();
	rtp_profile_set_payload(&av_profile,98,&payload_type_h263_1998);
	ms_init();
	signal(SIGINT,stop_handler);
	v=video_stream_start(&av_profile,6000,"127.0.0.1",6000, 98, 60, TRUE, "Video4Linux","/dev/video0");
	while(cond) {
		ortp_global_stats_display();
		sleep(1);
	}
	video_stream_stop(v);
	ortp_exit();
	return 0;
}
