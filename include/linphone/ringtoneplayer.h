/*
linphone
Copyright (C) 2000 - 2010 Simon MORLAT (simon.morlat@linphone.org)

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
#ifndef RINGPLAYER_H
#define RINGPLAYER_H

typedef void (*LinphoneRingtonePlayerFunc)(LinphoneRingtonePlayer* rp, void* user_data, int status);

LINPHONE_PUBLIC LinphoneRingtonePlayer* linphone_ringtoneplayer_new(void);
LINPHONE_PUBLIC void linphone_ringtoneplayer_destroy(LinphoneRingtonePlayer* rp);

LINPHONE_PUBLIC int linphone_ringtoneplayer_start(MSFactory *factory, LinphoneRingtonePlayer* rp, MSSndCard* card, const char* ringtone, int loop_pause_ms);
/**
 * Start a ringtone player
 * @param factory A MSFactory object
 * @param rp LinphoneRingtonePlayer object
 * @param card unused argument
 * @param ringtone path to the ringtone to play
 * @param loop_pause_ms pause interval in milliseconds to be observed between end of play and resuming at start. A value of -1 disables loop mode
 * @param end_of_ringtone A callback function called when the ringtone ends
 * @param user_data A user data passed to the callback function called when the ringtone ends
 * @return 0 if the player successfully started, positive error code otherwise
 */
LINPHONE_PUBLIC int linphone_ringtoneplayer_start_with_cb(MSFactory *factory, LinphoneRingtonePlayer* rp, MSSndCard* card,
														  const char* ringtone, int loop_pause_ms, LinphoneRingtonePlayerFunc end_of_ringtone, void * user_data);
LINPHONE_PUBLIC bool_t linphone_ringtoneplayer_is_started(LinphoneRingtonePlayer* rp);
LINPHONE_PUBLIC int linphone_ringtoneplayer_stop(LinphoneRingtonePlayer* rp);

#endif
