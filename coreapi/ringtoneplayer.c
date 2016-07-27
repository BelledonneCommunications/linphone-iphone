/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)
Copyright (C) 2010  Belledonne Communications SARL

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "private.h"
#include <mediastreamer2/msfactory.h>

int linphone_ringtoneplayer_start(MSFactory *factory, LinphoneRingtonePlayer* rp, MSSndCard* card, const char* ringtone, int loop_pause_ms) {
	return linphone_ringtoneplayer_start_with_cb(factory, rp, card, ringtone, loop_pause_ms, NULL, NULL);
}

#ifdef __ios

#include "ringtoneplayer_ios.h"

LinphoneRingtonePlayer* linphone_ringtoneplayer_new() {
	return linphone_ringtoneplayer_ios_new();
}

void linphone_ringtoneplayer_destroy(LinphoneRingtonePlayer* rp) {
	linphone_ringtoneplayer_ios_destroy(rp);
}

int linphone_ringtoneplayer_start_with_cb(MSFactory* f, LinphoneRingtonePlayer* rp, MSSndCard* card, const char* ringtone, int loop_pause_ms, LinphoneRingtonePlayerFunc end_of_ringtone, void * user_data) {
	if (linphone_ringtoneplayer_is_started(rp)) {
		ms_message("the local ringtone is already started");
		return 2;
	}
	if (ringtone){
		return linphone_ringtoneplayer_ios_start_with_cb(rp, ringtone, loop_pause_ms, end_of_ringtone, user_data);
	}
	return 3;
}

bool_t linphone_ringtoneplayer_is_started(LinphoneRingtonePlayer* rp) {
	return linphone_ringtoneplayer_ios_is_started(rp);
}

int linphone_ringtoneplayer_stop(LinphoneRingtonePlayer* rp) {
	return linphone_ringtoneplayer_ios_stop(rp);
}


#else

struct _LinphoneRingtonePlayer {
	RingStream *ringstream;
	LinphoneRingtonePlayerFunc end_of_ringtone;
	void* end_of_ringtone_ud;
};

LinphoneRingtonePlayer* linphone_ringtoneplayer_new() {
	LinphoneRingtonePlayer* rp = ms_new0(LinphoneRingtonePlayer, 1);
	return rp;
}

void linphone_ringtoneplayer_destroy(LinphoneRingtonePlayer* rp) {
	if (rp->ringstream) {
		linphone_ringtoneplayer_stop(rp);
	}
	ms_free(rp);
}

static void notify_end_of_ringtone(void *ud, MSFilter *f, unsigned int event, void *arg){
	LinphoneRingtonePlayer *rp=(LinphoneRingtonePlayer*)ud;
	if (event==MS_PLAYER_EOF && rp->end_of_ringtone){
		rp->end_of_ringtone(rp, rp->end_of_ringtone_ud, 0);
	}
}

int linphone_ringtoneplayer_start_with_cb(MSFactory *factory, LinphoneRingtonePlayer* rp, MSSndCard* card, const char* ringtone, int loop_pause_ms, LinphoneRingtonePlayerFunc end_of_ringtone, void * user_data) {
	if (linphone_ringtoneplayer_is_started(rp)) {
		ms_message("the local ringtone is already started");
		return 2;
	}
	if (card!=NULL && ringtone){
		ms_message("Starting local ringtone...");
		rp->end_of_ringtone = end_of_ringtone;
		rp->end_of_ringtone_ud = user_data;
		rp->ringstream=ring_start_with_cb(factory, ringtone,loop_pause_ms,card,notify_end_of_ringtone,rp);
		return rp->ringstream != NULL ? 0 : 1;
	}
	return 3;
}

bool_t linphone_ringtoneplayer_is_started(LinphoneRingtonePlayer* rp) {
	return (rp->ringstream!=NULL);
}

int linphone_ringtoneplayer_stop(LinphoneRingtonePlayer* rp) {
	if (rp->ringstream) {
		ring_stop(rp->ringstream);
		rp->ringstream = NULL;
	}
	return 0;
}
#endif
