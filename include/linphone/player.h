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
 * @addtogroup call_control
 * @{
 */

/**
 * Acquire a reference to the player.
 * @param[in] player LinphonePlayer object.
 * @return The same LinphonePlayer object.
**/
LINPHONE_PUBLIC LinphonePlayer * linphone_player_ref(LinphonePlayer *player);

/**
 * Release reference to the player.
 * @param[in] player LinphonePlayer object.
**/
LINPHONE_PUBLIC void linphone_player_unref(LinphonePlayer *player);

/**
 * Retrieve the user pointer associated with the player.
 * @param[in] player LinphonePlayer object.
 * @return The user pointer associated with the player.
**/
LINPHONE_PUBLIC void *linphone_player_get_user_data(const LinphonePlayer *player);

/**
 * Assign a user pointer to the player.
 * @param[in] player LinphonePlayer object.
 * @param[in] ud The user pointer to associate with the player.
**/
LINPHONE_PUBLIC void linphone_player_set_user_data(LinphonePlayer *player, void *ud);

/**
 * Get the LinphonePlayerCbs object associated with the LinphonePlayer.
 * @param[in] player LinphonePlayer object
 * @return The LinphonePlayerCbs object associated with the LinphonePlayer.
 */
LINPHONE_PUBLIC LinphonePlayerCbs * linphone_player_get_callbacks(const LinphonePlayer *player);

/**
 * Open a file for playing.
 * @param[in] obj LinphonePlayer object
 * @param[in] filename The path to the file to open
 */
LINPHONE_PUBLIC LinphoneStatus linphone_player_open(LinphonePlayer *obj, const char *filename);

/**
 * Start playing a file that has been opened with linphone_player_open().
 * @param[in] obj LinphonePlayer object
 * @return 0 on success, a negative value otherwise
 */
LINPHONE_PUBLIC LinphoneStatus linphone_player_start(LinphonePlayer *obj);

/**
 * Pause the playing of a file.
 * @param[in] obj LinphonePlayer object
 * @return 0 on success, a negative value otherwise
 */
LINPHONE_PUBLIC LinphoneStatus linphone_player_pause(LinphonePlayer *obj);

/**
 * Seek in an opened file.
 * @param[in] obj LinphonePlayer object
 * @param[in] time_ms The time we want to go to in the file (in milliseconds).
 * @return 0 on success, a negative value otherwise.
 */
LINPHONE_PUBLIC LinphoneStatus linphone_player_seek(LinphonePlayer *obj, int time_ms);

/**
 * Get the current state of a player.
 * @param[in] obj LinphonePlayer object
 * @return The current state of the player.
 */
LINPHONE_PUBLIC LinphonePlayerState linphone_player_get_state(LinphonePlayer *obj);

/**
 * Get the duration of the opened file.
 * @param[in] obj LinphonePlayer object
 * @return The duration of the opened file
 */
LINPHONE_PUBLIC int linphone_player_get_duration(LinphonePlayer *obj);

/**
 * Get the current position in the opened file.
 * @param[in] obj LinphonePlayer object
 * @return The current position in the opened file
 */
LINPHONE_PUBLIC int linphone_player_get_current_position(LinphonePlayer *obj);

/**
 * Close the opened file.
 * @param[in] obj LinphonePlayer object
 */
LINPHONE_PUBLIC void linphone_player_close(LinphonePlayer *obj);


/**
 * Acquire a reference to the LinphonePlayerCbs object.
 * @param[in] cbs LinphonePlayerCbs object.
 * @return The same LinphonePlayerCbs object.
 */
LINPHONE_PUBLIC LinphonePlayerCbs * linphone_player_cbs_ref(LinphonePlayerCbs *cbs);

/**
 * Release reference to the LinphonePlayerCbs object.
 * @param[in] cbs LinphonePlayerCbs object.
 */
LINPHONE_PUBLIC void linphone_player_cbs_unref(LinphonePlayerCbs *cbs);

/**
 * Retrieve the user pointer associated with the LinphonePlayerCbs object.
 * @param[in] cbs LinphonePlayerCbs object.
 * @return The user pointer associated with the LinphonePlayerCbs object.
 */
LINPHONE_PUBLIC void *linphone_player_cbs_get_user_data(const LinphonePlayerCbs *cbs);

/**
 * Assign a user pointer to the LinphonePlayerCbs object.
 * @param[in] cbs LinphonePlayerCbs object.
 * @param[in] ud The user pointer to associate with the LinphonePlayerCbs object.
 */
LINPHONE_PUBLIC void linphone_player_cbs_set_user_data(LinphonePlayerCbs *cbs, void *ud);

/**
 * Get the end-of-file reached callback.
 * @param[in] cbs LinphonePlayerCbs object.
 * @return The current end-of-file reached callback.
 */
LINPHONE_PUBLIC LinphonePlayerCbsEofReachedCb linphone_player_cbs_get_eof_reached(const LinphonePlayerCbs *cbs);

/**
 * Set the end-of-file reached callback.
 * @param[in] cbs LinphonePlayerCbs object.
 * @param[in] cb The end-of-file reached callback to be used.
 */
LINPHONE_PUBLIC void linphone_player_cbs_set_eof_reached(LinphonePlayerCbs *cbs, LinphonePlayerCbsEofReachedCb cb);

/**
 * @}
**/


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_PLAYER_H_ */
