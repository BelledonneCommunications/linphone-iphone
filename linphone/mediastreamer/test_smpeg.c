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
#include "msfilter.h"
#include "mssmpeg.h"
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include "mstimer.h"
#include "msread.h"

int cond=1;

void stop_handler(int signum)
{
	cond=0;
}

int main()
{
	MSFilter *source;
	MSFilter *smpeg_filter;
	MSSync *timer;
	
	timer=ms_timer_new();
	ms_timer_set_interval(MS_TIMER(timer),100);
	
	smpeg_filter=ms_smpeg_new();
	source=ms_read_new("/cdrom/Videos/Queen - Bohemian Rhapsody.mpg");
	ms_filter_add_link(source,smpeg_filter);
	ms_sync_attach(timer,source);
	signal(SIGINT,stop_handler);
	
	ms_start(timer);
	ms_smpeg_start(MS_SMPEG(smpeg_filter));
	while (cond){
		sleep (1);
	}
	g_message("Exiting...");
	ms_smpeg_stop(MS_SMPEG(smpeg_filter));
	ms_stop(timer);
	ms_sync_detach(timer,source);
	ms_filter_remove_links(source,smpeg_filter);
	ms_sync_destroy(timer);
	ms_filter_destroy(smpeg_filter);
	ms_filter_destroy(source);
	g_message("End of test program.");
	return 0;
}


