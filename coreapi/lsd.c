/*
linphone
Copyright (C) 2010 Simon MORLAT (simon.morlat@linphone.org)

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

/* Linphone Sound Daemon: is a lightweight utility to play sounds to speaker during a conversation.
 This is useful for embedded platforms, where sound apis are not performant enough to allow
 simultaneous sound access.
*/

#include "linphonecore_utils.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msvolume.h"


#define MAX_BRANCHES 10


struct _LsdPlayer{
	struct _LinphoneSoundDaemon *lsd;
	MSFilter *player;
	MSFilter *rateconv;
	MSFilter *chanadapter;
	MSFilter *volumectl;
	LsdEndOfPlayCallback eop_cb;
	void *user_data;
	bool_t loopmode;
};

struct _LinphoneSoundDaemon {
	int out_rate;
	int out_nchans;
	MSFilter *mixer;
	MSFilter *soundout;
	LsdPlayer branches[MAX_BRANCHES];
};


LsdPlayer *linphone_sound_daemon_get_player(LinphoneSoundDaemon *obj){
	int i;
	for(i=0;i<MAX_BRANCHES;++i){
		LsdPlayer *b=&obj->branches[i];
		MSFilter *p=b->player;
		int state;
		ms_filter_call_method(p,MS_PLAYER_GET_STATE,&state);
		if (state==MSPlayerClosed){
			return b;
		}
	}
	ms_warning("No more free players !");
	return NULL;
}

void linphone_sound_daemon_release_player(LinphoneSoundDaemon *obj, LsdPlayer * player){
	int state;
	float gain=1;
	ms_filter_call_method(player->player,MS_PLAYER_GET_STATE,&state);
	if (state!=MSPlayerClosed){
		ms_filter_call_method(player->player,MS_PLAYER_CLOSE,&state);
	}
	ms_filter_call_method(player->volumectl,MS_VOLUME_SET_GAIN,&gain);
}

int lsd_player_stop(LsdPlayer *p){
	ms_filter_call_method_noarg(p->player,MS_PLAYER_PAUSE);
	return 0;
}

MSFilter *linphone_sound_daemon_get_proxy(LinphoneSoundDaemon *obj);
void linphone_sound_daemon_destroy(LinphoneSoundDaemon *obj);

static void lsd_player_init(LsdPlayer *p, MSConnectionPoint mixer, MSFilterId playerid, LinphoneSoundDaemon *lsd){
	MSConnectionHelper h;
	p->player=ms_filter_new(playerid);
	p->rateconv=ms_filter_new(MS_RESAMPLE_ID);
	p->chanadapter=NULL;
	p->volumectl=ms_filter_new(MS_VOLUME_ID);

	ms_connection_helper_start(&h);
	ms_connection_helper_link(&h,p->player,-1,0);
	ms_connection_helper_link(&h,p->rateconv,0,0);
	ms_connection_helper_link(&h,p->chanadapter,0,0);
	ms_connection_helper_link(&h,p->volumectl,0,0);
	ms_connection_helper_link(&h,mixer.filter,mixer.pin,-1);
	p->lsd=lsd;
}

static void lsd_player_uninit(LsdPlayer *p, MSConnectionPoint mixer){
	MSConnectionHelper h;

	ms_connection_helper_start(&h);
	ms_connection_helper_unlink (&h,p->player,-1,0);
	ms_connection_helper_unlink(&h,p->rateconv,0,0);
	ms_connection_helper_unlink(&h,p->chanadapter,0,0);
	ms_connection_helper_unlink(&h,p->volumectl,0,0);
	ms_connection_helper_unlink(&h,mixer.filter,mixer.pin,-1);

	ms_filter_destroy(p->player);
	ms_filter_destroy(p->rateconv);
	ms_filter_destroy(p->chanadapter);
	ms_filter_destroy(p->volumectl);
}

void lsd_player_set_callback(LsdPlayer *p, LsdEndOfPlayCallback cb){
	p->eop_cb=cb;
}

void lsd_player_set_user_pointer(LsdPlayer *p, void *up){
	p->user_data=up;
}

void *lsd_player_get_user_pointer(LsdPlayer *p){
	return p->user_data;
}

static void lsd_player_on_eop(void * userdata, unsigned int id, void *arg){
}

int lsd_player_play(LsdPlayer *b, const char *filename ){
	int rate,chans;
	int state;
	LinphoneSoundDaemon *lsd=b->lsd;
	
	ms_filter_call_method(b->player,MS_PLAYER_GET_STATE,&state);
	if (state!=MSPlayerClosed){
		ms_filter_call_method_noarg(b->player,MS_PLAYER_CLOSE);
	}
	
	if (ms_filter_call_method(b->player,MS_PLAYER_OPEN,(void*)filename)!=0){
		return -1;
	}
	ms_filter_call_method(b->player,MS_FILTER_GET_SAMPLE_RATE,&rate);
	ms_filter_call_method(b->player,MS_FILTER_GET_NCHANNELS,&chans);
	ms_filter_set_notify_callback (b->player,lsd_player_on_eop,b);
	
	ms_filter_call_method(b->rateconv,MS_FILTER_SET_SAMPLE_RATE,&rate);
	ms_filter_call_method(b->rateconv,MS_FILTER_SET_NCHANNELS,&chans);
	ms_filter_call_method(b->rateconv,MS_FILTER_SET_OUTPUT_SAMPLE_RATE,&lsd->out_rate);

	ms_filter_call_method(b->chanadapter,MS_FILTER_SET_NCHANNELS,&chans);
	ms_filter_call_method(b->chanadapter,MS_FILTER_SET_OUTPUT_NCHANNELS,&lsd->out_nchans);
	return 0;
}

int lsd_player_stop(LsdPlayer *p);
void lsd_player_enable_loop(LsdPlayer *p, bool_t loopmode);

LinphoneSoundDaemon * linphone_sound_daemon_new(const char *cardname){
	int i;
	MSConnectionPoint mp;
	LinphoneSoundDaemon *lsd;
	MSSndCard *card=ms_snd_card_manager_get_card(
	                                             ms_snd_card_manager_get(),
	                                             cardname);
	if (card==NULL){
		card=ms_snd_card_manager_get_default_playback_card (
		                                                    ms_snd_card_manager_get());
		if (card==NULL){
			ms_error("linphone_sound_daemon_new(): No playback soundcard available");
			return NULL;
		}
	}
	
	lsd=ms_new0(LinphoneSoundDaemon,1);
	lsd->soundout=ms_snd_card_create_writer(card);
	lsd->out_rate=44100;
	lsd->out_nchans=2;
	ms_filter_call_method(lsd->soundout,MS_FILTER_SET_SAMPLE_RATE,&lsd->out_rate);
	ms_filter_call_method(lsd->soundout,MS_FILTER_SET_NCHANNELS,&lsd->out_nchans);

	mp.filter=lsd->filter;
	mp.pin=0;

	lsd_player_init(&lsd->branches[0],mp,MS_ITC_SINK_ID,lsd);
	for(i=1;i<MAX_BRANCHES;++i){
		mp.pin=i;
		lsd_player_init(&lsd->branches[i],mp,MS_FILE_PLAYER_ID);
	}
	
	return lsd;
}


#endif
