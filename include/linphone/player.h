/*
player.h
Copyright (C) 2010-2017 Belledonne Communications SARL

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

#ifndef LINPHONE_PLAYER_H_
#define LINPHONE_PLAYER_H_


#include "linphone/types.h"
#include "mediastreamer2/msinterfaces.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Callback for notifying end of play (file).
 * @param obj the LinphonePlayer
 * @param user_data the user_data provided when calling linphone_player_open().
 * @ingroup call_control
**/
typedef void (*LinphonePlayerEofCallback)(LinphonePlayer *obj, void *user_data);

LINPHONE_PUBLIC int linphone_player_open(LinphonePlayer *obj, const char *filename, LinphonePlayerEofCallback, void *user_data);

LINPHONE_PUBLIC int linphone_player_start(LinphonePlayer *obj);

LINPHONE_PUBLIC int linphone_player_pause(LinphonePlayer *obj);

LINPHONE_PUBLIC int linphone_player_seek(LinphonePlayer *obj, int time_ms);

LINPHONE_PUBLIC MSPlayerState linphone_player_get_state(LinphonePlayer *obj);

LINPHONE_PUBLIC int linphone_player_get_duration(LinphonePlayer *obj);

LINPHONE_PUBLIC int linphone_player_get_current_position(LinphonePlayer *obj);

LINPHONE_PUBLIC void linphone_player_close(LinphonePlayer *obj);

LINPHONE_PUBLIC void linphone_player_destroy(LinphonePlayer *obj);


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_PLAYER_H_ */
