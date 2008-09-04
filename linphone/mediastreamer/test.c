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

#include "msringplayer.h"
#include "msosswrite.h"
#include "msossread.h"
#include "mscopy.h"
#include "mstimer.h"
#include <unistd.h>
#include <signal.h>

#define READFILE "../share/rings/orig.wav"
#define WRITEFILE "/tmp/mediaout"

static int cond=1;

void stop_handler(int signum)
{
	cond=0;
}


int main(int argc, char *argv[])
{
	MSFilter *play,*copy,*rec;
	MSSync *timer;
	int i=0;
	int tmp;
	char *ring;

	ms_init();

	if (argc>1){
		ring=argv[1];
	}else ring= READFILE;
	
	play=ms_ring_player_new(ring,2);
	//play=ms_oss_read_new(0);
	rec=snd_card_create_write_filter(snd_card_manager_get_card(snd_card_manager,1));
	copy=ms_copy_new();

	ms_filter_get_property(play,MS_FILTER_PROPERTY_FREQ,&tmp);
	g_message("Playing at rate %i.",tmp);
	ms_filter_set_property(rec,MS_FILTER_PROPERTY_FREQ,&tmp);
	ms_filter_get_property(play,MS_FILTER_PROPERTY_CHANNELS,&tmp);
	g_message("Playing with %i channels",tmp);
	ms_filter_set_property(rec,MS_FILTER_PROPERTY_CHANNELS,&tmp);
	
	timer=ms_timer_new();
	ms_sync_start(timer);

	ms_filter_add_link(play,copy);
	ms_filter_add_link(copy,rec);
	ms_sync_attach(timer,play);

	
	while(cond)
	{
		sleep(1);
	}
	ms_sync_detach(timer,play);
	ms_sync_stop(timer);
	ms_sync_destroy(timer);

	ms_filter_remove_links(play,copy);
	ms_filter_remove_links(copy,rec);
	ms_filter_destroy(play);
	ms_filter_destroy(copy);
	ms_filter_destroy(rec);
	
	return 0;
}
