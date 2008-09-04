/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msticker.h"

#include <signal.h>

static int run=1;

static void stop(int signum){
	run=0;
}

int main(int argc, char *argv[]){
	MSFilter *f1,*f2;
	MSSndCard *card;
	MSTicker *ticker;
	char *card_id=NULL;
	ortp_init();
	ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
	ms_init();
	
	signal(SIGINT,stop);

	if (argc>1)
		card_id=argv[1];

	if (card_id!=NULL)
	  {
		card=ms_snd_card_manager_get_card(ms_snd_card_manager_get(),card_id);
#ifdef __linux
		if (card==NULL)
		  card = ms_alsa_card_new_custom(card_id, card_id);
#endif
	  }
	else card=ms_snd_card_manager_get_default_card(ms_snd_card_manager_get());

	if (card==NULL){
		ms_error("No card.");
		return -1;
	}
	f1=ms_snd_card_create_reader(card);
	f2=ms_snd_card_create_writer(card);
	ticker=ms_ticker_new();
	ms_filter_link(f1,0,f2,0);
	ms_ticker_attach(ticker,f1);
	while(run)
		sleep(1);
	ms_ticker_detach(ticker,f1);
	ms_ticker_destroy(ticker);
	ms_filter_unlink(f1,0,f2,0);
	ms_filter_destroy(f1);
	ms_filter_destroy(f2);
	return 0;
}
