/*
 * c-chat-message.h
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _L_C_CHAT_MESSAGE_H_
#define _L_C_CHAT_MESSAGE_H_

#include "linphone/api/c-chat-message-cbs.h"
#include "linphone/api/c-types.h"

#ifdef SQLITE_STORAGE_ENABLED
	#include <sqlite3.h>
#endif // ifdef SQLITE_STORAGE_ENABLED

// =============================================================================

typedef enum _LinphoneChatMessageDir{
	LinphoneChatMessageIncoming,
	LinphoneChatMessageOutgoing
} LinphoneChatMessageDir;

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif // ifdef __cplusplus

/**
 * @addtogroup chatmessage
 * @{
 */

/**
 * Acquire a reference to the chat message.
 * @param[in] msg #LinphoneChatMessage object.
 * @return The same chat message.
 */
LINPHONE_PUBLIC LinphoneChatMessage *linphone_chat_message_ref (LinphoneChatMessage *msg);

/**
 * Release reference to the chat message.
 * @param[in] msg #LinphoneChatMessage object.
 */
LINPHONE_PUBLIC void linphone_chat_message_unref (LinphoneChatMessage *msg);

/**
 * Retrieve the user pointer associated with the chat message.
 * @param[in] msg #LinphoneChatMessage object.
 * @return The user pointer associated with the chat message.
 */
LINPHONE_PUBLIC void *linphone_chat_message_get_user_data (const LinphoneChatMessage *msg);

/**
 * Assign a user pointer to the chat message.
 * @param[in] msg #LinphoneChatMessage object.
 * @param[in] ud The user pointer to associate with the chat message.
 */
LINPHONE_PUBLIC void linphone_chat_message_set_user_data (LinphoneChatMessage *msg, void *ud);

// =============================================================================

LINPHONE_PUBLIC const char *linphone_chat_message_get_external_body_url (const LinphoneChatMessage *msg);

LINPHONE_PUBLIC void linphone_chat_message_set_external_body_url (LinphoneChatMessage *msg, const char *external_body_url);

/**
 * Get the time the message was sent.
 * @param[in] msg #LinphoneChatMessage object.
 */
LINPHONE_PUBLIC time_t linphone_chat_message_get_time (const LinphoneChatMessage *msg);

/**
 * Returns TRUE if the message has been sent, returns FALSE if the message has been received.
 * @param[in] msg #LinphoneChatMessage object.
 */
LINPHONE_PUBLIC bool_t linphone_chat_message_is_outgoing (LinphoneChatMessage *msg);

/**
 * Get origin of the message
 * @param[in] msg #LinphoneChatMessage object.
 * @return #LinphoneAddress
 */
LINPHONE_PUBLIC const LinphoneAddress *linphone_chat_message_get_from_address (LinphoneChatMessage *msg);

/**
 * Get destination of the message
 * @param[in] msg #LinphoneChatMessage object.
 * @return #LinphoneAddress
 */
LINPHONE_PUBLIC const LinphoneAddress *linphone_chat_message_get_to_address (LinphoneChatMessage *msg);

/**
 * Get the content type of a chat message.
 * @param[in] msg #LinphoneChatMessage object.
 * @return The content type of the chat message
 */
LINPHONE_PUBLIC const char *linphone_chat_message_get_content_type (LinphoneChatMessage *msg);

/**
 * Set the content type of a chat message.
 * This content type must match a content that is text representable, such as text/plain, text/html or image/svg+xml.
 * @param[in] msg #LinphoneChatMessage object.
 * @param[in] content_type The new content type of the chat message
 */
LINPHONE_PUBLIC void linphone_chat_message_set_content_type (LinphoneChatMessage *msg, const char *content_type);

/**
 * Get text part of this message
 * @param[in] msg #LinphoneChatMessage object.
 * @return text or NULL if no text.
 * @deprecated use getTextContent() instead
 */
LINPHONE_PUBLIC const char *linphone_chat_message_get_text (LinphoneChatMessage* msg);

/**
 * Get the message identifier.
 * It is used to identify a message so that it can be notified as delivered and/or displayed.
 * @param[in] msg #LinphoneChatMessage object.
 * @return The message identifier.
 */
LINPHONE_PUBLIC const char *linphone_chat_message_get_message_id (const LinphoneChatMessage *msg);

/**
 * Linphone message has an app-specific field that can store a text. The application might want
 * to use it for keeping data over restarts, like thumbnail image path.
 * @param[in] msg #LinphoneChatMessage object.
 * @return the application-specific data or NULL if none has been stored.
 */
LINPHONE_PUBLIC const char *linphone_chat_message_get_appdata (const LinphoneChatMessage *msg);

/**
 * Linphone message has an app-specific field that can store a text. The application might want
 * to use it for keeping data over restarts, like thumbnail image path.
 *
 * Invoking this function will attempt to update the message storage to reflect the changeif it is
 * enabled.
 *
 * @param[in] msg #LinphoneChatMessage object.
 * @param data the data to store into the message
 */
LINPHONE_PUBLIC void linphone_chat_message_set_appdata (LinphoneChatMessage *msg, const char *data);

/**
 * Returns the chatroom this message belongs to.
 * @param[in] msg #LinphoneChatMessage object.
 */
LINPHONE_PUBLIC LinphoneChatRoom *linphone_chat_message_get_chat_room (const LinphoneChatMessage *msg);

/**
 * Get the path to the file to read from or write to during the file transfer.
 * @param[in] msg #LinphoneChatMessage object
 * @return The path to the file to use for the file transfer.
 */
LINPHONE_PUBLIC const char *linphone_chat_message_get_file_transfer_filepath (LinphoneChatMessage *msg);

// =============================================================================

/**
 * Get if a chat message is to be stored.
 * @param[in] msg #LinphoneChatMessage object.
 * @return Whether or not the message is to be stored
 */
LINPHONE_PUBLIC bool_t linphone_chat_message_get_to_be_stored (const LinphoneChatMessage *msg);

/**
 * Set if a chat message is to be stored.
 * This content type must match a content that is text representable, such as text/plain, text/html or image/svg+xml.
 * @param[in] msg #LinphoneChatMessage object.
 * @param[in] to_be_stored Whether or not the chat message is to be stored
 */
LINPHONE_PUBLIC void linphone_chat_message_set_to_be_stored (LinphoneChatMessage *msg, bool_t to_be_stored);

LINPHONE_PUBLIC unsigned int linphone_chat_message_store (LinphoneChatMessage *msg);

/**
 * Get the state of the message
 * @param[in] msg #LinphoneChatMessage object.
 * @return #LinphoneChatMessageState
 */
LINPHONE_PUBLIC LinphoneChatMessageState linphone_chat_message_get_state (const LinphoneChatMessage *msg);

/**
 * Get if the message was encrypted when transfered
 * @param[in] msg #LinphoneChatMessage object.
 * @return whether the message was encrypted when transfered or not
 */
LINPHONE_PUBLIC bool_t linphone_chat_message_is_secured (LinphoneChatMessage *msg);

/**
 * Linphone message can carry external body as defined by rfc2017
 * @param[in] msg #LinphoneChatMessage object.
 * @return external body url or NULL if not present.
 */
LINPHONE_PUBLIC const char *linphone_chat_message_get_external_body_url (const LinphoneChatMessage *msg);

/**
 * Linphone message can carry external body as defined by rfc2017
 *
 * @param[in] msg #LinphoneChatMessage object.
 * @param url ex: access-type=URL; URL="http://www.foo.com/file"
 */
LINPHONE_PUBLIC void linphone_chat_message_set_external_body_url (LinphoneChatMessage *msg,const char *url);

/**
 * Get the file_transfer_information (used by call backs to recover informations during a rcs file transfer)
 *
 * @param[in] msg #LinphoneChatMessage object.
 * @return a pointer to the #LinphoneContent structure or NULL if not present.
 */
LINPHONE_PUBLIC LinphoneContent *linphone_chat_message_get_file_transfer_information (LinphoneChatMessage *msg);

/**
 * Return whether or not a chat message is a file tranfer.
 * @param[in] msg #LinphoneChatMessage object
 * @return Whether or not the message is a file tranfer
 */
LINPHONE_PUBLIC bool_t linphone_chat_message_is_file_transfer (LinphoneChatMessage *msg);

/**
 * Return whether or not a chat message is a text.
 * @param[in] msg #LinphoneChatMessage object.
 * @return Whether or not the message is a text
 */
LINPHONE_PUBLIC bool_t linphone_chat_message_is_text (LinphoneChatMessage *msg);

/**
 * Start the download of the file from remote server
 *
 * @param[in] msg #LinphoneChatMessage object.
 * @param status_cb #LinphoneChatMessageStateChangeCb status callback invoked when file is downloaded or could not be downloaded
 * @param ud user data
 * @deprecated Use linphone_chat_message_download_file() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_chat_message_start_file_download (
	LinphoneChatMessage *msg,
	LinphoneChatMessageStateChangedCb status_cb,
	void *ud
);

/**
 * Start the download of the file referenced in a #LinphoneChatMessage from remote server.
 * @param[in] msg #LinphoneChatMessage object.
 */
LINPHONE_PUBLIC bool_t linphone_chat_message_download_file (LinphoneChatMessage *msg);

/**
 * Cancel an ongoing file transfer attached to this message.(upload or download)
 * @param[in] msg #LinphoneChatMessage object.
 */
LINPHONE_PUBLIC void linphone_chat_message_cancel_file_transfer (LinphoneChatMessage *msg);

/**
 * Send a chat message.
 * @param[in] msg #LinphoneChatMessage object.
 */
LINPHONE_PUBLIC void linphone_chat_message_send (LinphoneChatMessage *msg);

/**
 * Resend a chat message if it is in the 'not delivered' state for whatever reason.
 * @param[in] msg #LinphoneChatMessage object.
 * @deprecated Use linphone_chat_message_send instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_chat_message_resend (LinphoneChatMessage *msg);

LINPHONE_PUBLIC const LinphoneAddress *linphone_chat_message_get_peer_address (LinphoneChatMessage *msg);

/**
 * Returns the origin address of a message if it was a outgoing message, or the destination address if it was an incoming message.
 * @param[in] msg #LinphoneChatMessage object.
 * @return #LinphoneAddress
 */
LINPHONE_PUBLIC const LinphoneAddress *linphone_chat_message_get_local_address (LinphoneChatMessage *msg);

/**
 * Add custom headers to the message.
 * @param[in] msg #LinphoneChatMessage object.
 * @param header_name name of the header
 * @param header_value header value
 */
LINPHONE_PUBLIC void linphone_chat_message_add_custom_header (
	LinphoneChatMessage *msg,
	const char *header_name,
	const char *header_value
);

/**
 * Retrieve a custom header value given its name.
 * @param[in] msg #LinphoneChatMessage object.
 * @param header_name header name searched
 */
LINPHONE_PUBLIC const char * linphone_chat_message_get_custom_header (LinphoneChatMessage *msg, const char *header_name);

/**
 * Removes a custom header from the message.
 * @param[in] msg #LinphoneChatMessage object.
 * @param header_name name of the header to remove
 */
LINPHONE_PUBLIC void linphone_chat_message_remove_custom_header (LinphoneChatMessage *msg, const char *header_name);

/**
 * Returns TRUE if the message has been read, otherwise returns FALSE.
 * @param[in] msg #LinphoneChatMessage object.
 */
LINPHONE_PUBLIC bool_t linphone_chat_message_is_read (LinphoneChatMessage *msg);

LINPHONE_PUBLIC LinphoneReason linphone_chat_message_get_reason (LinphoneChatMessage *msg);

/**
 * Get full details about delivery error of a chat message.
 * @param[in] msg #LinphoneChatMessage object.
 * @return a #LinphoneErrorInfo describing the details.
 */
LINPHONE_PUBLIC const LinphoneErrorInfo *linphone_chat_message_get_error_info (const LinphoneChatMessage *msg);

/**
 * Set the path to the file to read from or write to during the file transfer.
 * @param[in] msg #LinphoneChatMessage object.
 * @param[in] filepath The path to the file to use for the file transfer.
 */
LINPHONE_PUBLIC void linphone_chat_message_set_file_transfer_filepath (LinphoneChatMessage *msg, const char *filepath);

/**
 * Fulfill a chat message char by char. Message linked to a Real Time Text Call send char in realtime following RFC 4103/T.140
 * To commit a message, use #linphone_chat_room_send_message
 * @param[in] msg #LinphoneChatMessage object.
 * @param[in] character T.140 char
 * @returns 0 if succeed.
 */
LINPHONE_PUBLIC LinphoneStatus linphone_chat_message_put_char (LinphoneChatMessage *msg, uint32_t character);

/**
 * Get the #LinphoneChatMessageCbs object associated with the LinphoneChatMessage.
 * @param[in] msg #LinphoneChatMessage object.
 * @return The #LinphoneChatMessageCbs object associated with the LinphoneChatMessage.
 */
LINPHONE_PUBLIC LinphoneChatMessageCbs *linphone_chat_message_get_callbacks (const LinphoneChatMessage *msg);

/**
 * Adds a content to the ChatMessage
 * @param[in] msg #LinphoneChatMessage object.
 * @param[in] c_content #LinphoneContent object
 */
LINPHONE_PUBLIC void linphone_chat_message_add_text_content (LinphoneChatMessage *msg, const char *c_content);

/**
 * Returns true if the chat message has a text content
 * @param[in] msg #LinphoneChatMessage object.
 * @return true if it has one, false otherwise
 */
LINPHONE_PUBLIC bool_t linphone_chat_message_has_text_content (const LinphoneChatMessage *msg);

/**
 * Gets the text content if available as a string
 * @param[in] msg #LinphoneChatMessage object.
 * @return the #LinphoneContent buffer if available, null otherwise
 */
LINPHONE_PUBLIC const char *linphone_chat_message_get_text_content (const LinphoneChatMessage *msg);

/**
 * Gets whether or not a file is currently being downloaded or uploaded
 * @param[in] msg #LinphoneChatMessage object.
 * @return true if download or upload is in progress, false otherwise
 */
LINPHONE_PUBLIC bool_t linphone_chat_message_is_file_transfer_in_progress (LinphoneChatMessage *msg);

/**
 * Gets the list of participants for which the imdn state has reached the specified state and the time at which they did.
 * @param[in] msg #LinphoneChatMessage object.
 * @param[in] state The LinphoneChatMessageState the imdn have reached (only use LinphoneChatMessageStateDelivered,
 * LinphoneChatMessageStateDeliveredToUser, LinphoneChatMessageStateDisplayed and LinphoneChatMessageStateNotDelivered)
 * @return \bctbx_list{LinphoneParticipantImdnState}
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_chat_message_get_participants_by_imdn_state (const LinphoneChatMessage *msg, LinphoneChatMessageState state);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif // ifndef _L_C_CHAT_MESSAGE_H_
