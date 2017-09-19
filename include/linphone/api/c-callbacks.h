/*
 * c-callbacks.h
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _C_CALLBACKS_H_
#define _C_CALLBACKS_H_

// TODO: Remove me in the future.
#include "linphone/callbacks.h"
#include "linphone/api/c-types.h"

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif // ifdef __cplusplus

/**
 * @addtogroup chatroom
 * @{
 */

/**
 * Is composing notification callback prototype.
 * @param[in] cr #LinphoneChatRoom involved in the conversation
 * @param[in] participant The #LinphoneParticipant that has sent the is-composing notification
 */
typedef void (*LinphoneChatRoomCbsIsComposingReceivedCb) (LinphoneChatRoom *cr, const LinphoneParticipant *participant);

/**
 * Callback used to notify a chat room that a message has been received.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] msg The #LinphoneChatMessage that has been received
 */
typedef void (*LinphoneChatRoomCbsMessageReceivedCb) (LinphoneChatRoom *cr, LinphoneChatMessage *msg);

/**
 * Callback used to notify a chat room state has changed.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] newState The new state of the chat room
 */
typedef void (*LinphoneChatRoomCbsStateChangedCb) (LinphoneChatRoom *cr, LinphoneChatRoomState newState);

/**
 * Callback used to notify a chat room that a message has been received but we were unable to decrypt it
 * @param cr #LinphoneChatRoom involved in this conversation
 * @param msg The #LinphoneChatMessage that has been received
 */
typedef void (*LinphoneChatRoomCbsUndecryptableMessageReceivedCb) (LinphoneChatRoom *cr, LinphoneChatMessage *msg);

/**
 * @}
**/

#ifdef __cplusplus
	}
#endif // ifdef __cplusplus

#endif // ifndef _C_CALLBACKS_H_
