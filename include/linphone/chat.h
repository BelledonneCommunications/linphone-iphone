/*
chat.h
Copyright (C) 2010-2016 Belledonne Communications SARL

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

#ifndef LINPHONE_CHAT_H_
#define LINPHONE_CHAT_H_


#include "linphone/callbacks.h"
#include "linphone/types.h"
#include "linphone/api/c-types.h"
#include "linphone/api/c-chat-message.h"
#include "linphone/api/c-chat-message-cbs.h"
#include "linphone/api/c-chat-room.h"
#include "linphone/api/c-chat-room-cbs.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup chatroom
 * @{
 */

/**
 * Returns an list of chat rooms
 * @param[in] lc #LinphoneCore object
 * @return \bctbx_list{LinphoneChatRoom}
**/
LINPHONE_PUBLIC const bctbx_list_t* linphone_core_get_chat_rooms(LinphoneCore *lc);



/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* LINPHONE_CHAT_H_ */
