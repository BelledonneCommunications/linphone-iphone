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

#include "ms.h"

#include "msosswrite.h"
#include "msossread.h"
#include "mscopy.h"
#include "msnosync.h"
#include "mstimer.h"
#include "msMUlawdec.h"
#include "msMUlawenc.h"

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
	MSFilter *play,*enc,*dec,*rec;
	MSSync *timer;	

	ms_init();
	signal(SIGINT,stop_handler);
	
	play=ms_oss_read_new();
	rec=ms_oss_write_new();
	ms_sound_read_set_device(MS_SOUND_READ(play),0);
	ms_sound_write_set_device(MS_SOUND_WRITE(rec),0);
	
	enc=ms_MULAWencoder_new();
	dec=ms_MULAWdecoder_new();
	timer=ms_timer_new();

	ms_filter_add_link(play,enc);
	ms_filter_add_link(enc,dec);
	ms_filter_add_link(dec,rec);
	ms_sync_attach(timer,play);

	ms_start(timer);
	ms_sound_read_start(MS_SOUND_READ(play));
	ms_sound_write_start(MS_SOUND_WRITE(rec));
	while(cond)
	{
		sleep(1);
	}
	ms_sound_read_stop(MS_SOUND_READ(play));
	ms_sound_write_stop(MS_SOUND_WRITE(rec));
	printf("stoping sync...\n");
	ms_stop(timer);
	printf("unlinking filters...\n");
	ms_filter_remove_links(play,enc);
	ms_filter_remove_links(enc,dec);
	ms_filter_remove_links(dec,rec);
	printf("destroying filters...\n");
	ms_filter_destroy(play);
	ms_filter_destroy(enc);
	ms_filter_destroy(dec);
	ms_filter_destroy(rec);
	ms_sync_destroy(timer);
	return 0;
}
