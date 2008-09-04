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


#define READFILE "../share/rings/orig.wav"
#define WRITEFILE "/tmp/mediaout"

int main()
{
	MSFilter *play,*copy,*rec;
	MSSync *timer;
	int i=0;	
	SndCard *card;
	ms_init();
	
	card=snd_card_manager_get_card(snd_card_manager,2);
	play=ms_ring_player_new(READFILE,2);
	//play=ms_oss_read_new(0);
	rec=snd_card_create_write_filter(card);
	copy=ms_copy_new();
	timer=ms_timer_new();

	ms_filter_add_link(play,copy);
	ms_filter_add_link(copy,rec);
	ms_sync_attach(timer,play);

	ms_start(timer);
	
	while(1)
	{
		ms_sound_write_set_level(MS_SOUND_WRITE(rec),i);
		i+=10;
		sleep(1);
		if (i>100) i=0;
	}
	return 0;
}
