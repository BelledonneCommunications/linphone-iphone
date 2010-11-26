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

#ifndef LINPHONECORE_UTILS_H
#define LINPHONECORE_UTILS_H

#ifdef IN_LINPHONE
#include "linphonecore.h"
#else
#include "linphone/linphonecore.h"
#endif

typedef struct _LsdPlayer LsdPlayer;
typedef struct _LinphoneSoundDaemon LinphoneSoundDaemon;

typedef void (*LsdEndOfPlayCallback)(LsdPlayer *p);

void lsd_player_set_callback(LsdPlayer *p, LsdEndOfPlayCallback cb);
void lsd_player_set_user_pointer(LsdPlayer *p, void *up);
void *lsd_player_get_user_pointer(const LsdPlayer *p);
int lsd_player_play(LsdPlayer *p, const char *filename);
int lsd_player_stop(LsdPlayer *p);
void lsd_player_enable_loop(LsdPlayer *p, bool_t loopmode);
bool_t lsd_player_loop_enabled(const LsdPlayer *p);
void lsd_player_set_gain(LsdPlayer *p, float gain);
LinphoneSoundDaemon *lsd_player_get_daemon(const LsdPlayer *p);

LinphoneSoundDaemon * linphone_sound_daemon_new(const char *cardname, int rate, int nchannels);
LsdPlayer * linphone_sound_daemon_get_player(LinphoneSoundDaemon *lsd);
void linphone_sound_daemon_release_player(LinphoneSoundDaemon *lsd, LsdPlayer *lsdplayer);
void linphone_sound_daemon_stop_all_players(LinphoneSoundDaemon *obj);
void linphone_sound_daemon_release_all_players(LinphoneSoundDaemon *obj);
void linphone_core_use_sound_daemon(LinphoneCore *lc, LinphoneSoundDaemon *lsd);
void linphone_sound_daemon_destroy(LinphoneSoundDaemon *obj);

#endif
