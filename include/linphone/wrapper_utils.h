/*
wrapper_utils.h
Copyright (C) 2017 Belledonne Communications SARL

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

/*
 * That file declares functions that are used by automatic API wrapper generators. These
 * should not be used by C API users.
 */

#ifndef _WRAPPER_UTILS_H
#define _WRAPPER_UTILS_H

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup wrapper
 * @{
 */

/**
 * Send a message to peer member of this chat room.
 *
 * The state of the sending message will be notified via the callbacks defined in the #LinphoneChatMessageCbs object that can be obtained
 * by calling linphone_chat_message_get_callbacks().
 * @note Unlike linphone_chat_room_send_chat_message(), that function only takes a reference on the #LinphoneChatMessage
 * instead of totaly takes ownership on it. Thus, the #LinphoneChatMessage object must be released by the API user after calling
 * that function.
 *
 * @param[in] cr A chat room.
 * @param[in] msg The message to send.
 */
LINPHONE_PUBLIC void linphone_chat_room_send_chat_message_2(LinphoneChatRoom *cr, LinphoneChatMessage *msg);

/**
 * Resend a chat message if it is in the 'not delivered' state for whatever reason.
 * @note Unlike linphone_chat_message_resend(), that function only takes a reference on the #LinphoneChatMessage
 * instead of totaly takes ownership on it. Thus, the #LinphoneChatMessage object must be released by the API user after calling
 * that function.
 *
 * @param[in] msg LinphoneChatMessage object
 */
LINPHONE_PUBLIC void linphone_chat_message_resend_2(LinphoneChatMessage *msg);

/**
 * Accessor for the shared_ptr&lt;BelCard&gt; stored by a #LinphoneVcard
 */
LINPHONE_PUBLIC void *linphone_vcard_get_belcard(LinphoneVcard *vcard);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // _WRAPPER_UTILS_H
