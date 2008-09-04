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

#include "sndcard.h"
#include "mscopy.h"
#include "mstimer.h"
#include "msspeexdec.h"
#include "msspeexenc.h"

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

static int cond=1;

void stop_handler(int signum)
{
	cond=0;
}

int main()
{
	MSFilter *play,*enc,*dec,*rec;
	MSSync *timer;	
	SndCard *card;
	int rate=16000;

	ms_init();
	signal(SIGINT,stop_handler);
	/* get the first card */
	card=snd_card_manager_get_card(snd_card_manager,0);
	if (card==NULL) g_error("No sound card detected.");
	
	play=snd_card_create_read_filter(card);
	rec=snd_card_create_write_filter(card);
	
	enc=ms_speex_enc_new();
	dec=ms_speex_dec_new();
	
	ms_filter_set_property(play,MS_FILTER_PROPERTY_FREQ,&rate);
	ms_filter_set_property(rec,MS_FILTER_PROPERTY_FREQ,&rate);
	ms_filter_set_property(enc,MS_FILTER_PROPERTY_FREQ,&rate);
	ms_filter_set_property(dec,MS_FILTER_PROPERTY_FREQ,&rate);
	timer=ms_timer_new();

	ms_filter_add_link(play,enc);
	ms_filter_add_link(enc,dec);
	ms_filter_add_link(dec,rec);
	ms_sync_attach(timer,play);

	ms_start(timer);
	while(cond)
	{
		sleep(1);
	}
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
