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


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup chatroom
 * @{
 */

/**
 * An object to handle the callbacks for the handling a LinphoneChatMessage objects.
 */
typedef struct _LinphoneChatMessageCbs LinphoneChatMessageCbs;

/**
 * A chat room message to hold content to be sent.
 * <br> Can be created by linphone_chat_room_create_message().
 */
typedef struct _LinphoneChatMessage LinphoneChatMessage;

/**
 * A chat room is the place where text messages are exchanged.
 * <br> Can be created by linphone_core_create_chat_room().
 */
typedef struct _LinphoneChatRoom LinphoneChatRoom;

/**
 * LinphoneChatMessageState is used to notify if messages have been succesfully delivered or not.
 */
typedef enum _LinphoneChatMessageState {
	LinphoneChatMessageStateIdle, /**< Initial state */
	LinphoneChatMessageStateInProgress, /**< Delivery in progress */
	LinphoneChatMessageStateDelivered, /**< Message successfully delivered and acknowledged by server */
	LinphoneChatMessageStateNotDelivered, /**< Message was not delivered */
	LinphoneChatMessageStateFileTransferError, /**< Message was received(and acknowledged) but cannot get file from server */
	LinphoneChatMessageStateFileTransferDone, /**< File transfer has been completed successfully. */
	LinphoneChatMessageStateDeliveredToUser, /**< Message successfully delivered and acknowledged to destination */
	LinphoneChatMessageStateDisplayed /**< Message displayed to the remote user */
} LinphoneChatMessageState;

typedef enum _LinphoneLimeState {
	LinphoneLimeDisabled, /**< Lime is not used at all */
	LinphoneLimeMandatory, /**< Lime is always used */
	LinphoneLimePreferred, /**< Lime is used only if we already shared a secret with remote */
} LinphoneLimeState;

/**
 * Call back used to notify message delivery status
 * @param msg #LinphoneChatMessage object
 * @param status LinphoneChatMessageState
 * @param ud application user data
 * @deprecated Use LinphoneChatMessageCbsMsgStateChangedCb instead.
 */
typedef void (*LinphoneChatMessageStateChangedCb)(LinphoneChatMessage* msg,LinphoneChatMessageState state,void* ud);

/**
 * Call back used to notify message delivery status
 * @param msg #LinphoneChatMessage object
 * @param status LinphoneChatMessageState
 */
typedef void (*LinphoneChatMessageCbsMsgStateChangedCb)(LinphoneChatMessage* msg, LinphoneChatMessageState state);

/**
 * File transfer receive callback prototype. This function is called by the core upon an incoming File transfer is started. This function may be call several time for the same file in case of large file.
 * @param message #LinphoneChatMessage message from which the body is received.
 * @param content #LinphoneContent incoming content information
 * @param buffer #LinphoneBuffer holding the received data. Empty buffer means end of file.
 */
typedef void (*LinphoneChatMessageCbsFileTransferRecvCb)(LinphoneChatMessage *message, const LinphoneContent* content, const LinphoneBuffer *buffer);

/**
 * File transfer send callback prototype. This function is called by the core when an outgoing file transfer is started. This function is called until size is set to 0.
 * @param message #LinphoneChatMessage message from which the body is received.
 * @param content #LinphoneContent outgoing content
 * @param offset the offset in the file from where to get the data to be sent
 * @param size the number of bytes expected by the framework
 * @return A LinphoneBuffer object holding the data written by the application. An empty buffer means end of file.
 */
typedef LinphoneBuffer * (*LinphoneChatMessageCbsFileTransferSendCb)(LinphoneChatMessage *message,  const LinphoneContent* content, size_t offset, size_t size);

/**
 * File transfer progress indication callback prototype.
 * @param message #LinphoneChatMessage message from which the body is received.
 * @param content #LinphoneContent incoming content information
 * @param offset The number of bytes sent/received since the beginning of the transfer.
 * @param total The total number of bytes to be sent/received.
 */
typedef void (*LinphoneChatMessageCbsFileTransferProgressIndicationCb)(LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t total);

/**
 * Set the chat database path.
 * @param lc the linphone core
 * @param path the database path
 */
LINPHONE_PUBLIC void linphone_core_set_chat_database_path(LinphoneCore *lc, const char *path);

/**
 * Get path to the database file used for storing chat messages.
 * @param lc the linphone core
 * @return file path or NULL if not exist
 **/

LINPHONE_PUBLIC const char *linphone_core_get_chat_database_path(const LinphoneCore *lc);
/**
 * Get a chat room whose peer is the supplied address. If it does not exist yet, it will be created.
 * No reference is transfered to the application. The LinphoneCore keeps a reference on the chat room.
 * @param lc the linphone core
 * @param addr a linphone address.
 * @return #LinphoneChatRoom where messaging can take place.
**/
LINPHONE_PUBLIC LinphoneChatRoom *linphone_core_get_chat_room(LinphoneCore *lc, const LinphoneAddress *addr);
/**
 * Get a chat room for messaging from a sip uri like sip:joe@sip.linphone.org. If it does not exist yet, it will be created.
 * No reference is transfered to the application. The LinphoneCore keeps a reference on the chat room.
 * @param lc A #LinphoneCore object
 * @param to The destination address for messages.
 * @return #LinphoneChatRoom where messaging can take place.
**/
LINPHONE_PUBLIC LinphoneChatRoom *linphone_core_get_chat_room_from_uri(LinphoneCore *lc, const char *to);

/**
 * Removes a chatroom including all message history from the LinphoneCore.
 * @param lc A #LinphoneCore object
 * @param cr A #LinphoneChatRoom object
**/
LINPHONE_PUBLIC void linphone_core_delete_chat_room(LinphoneCore *lc, LinphoneChatRoom *cr);

/**
 * Inconditionnaly disable incoming chat messages.
 * @param lc A #LinphoneCore object
 * @param deny_reason the deny reason (#LinphoneReasonNone has no effect).
**/
LINPHONE_PUBLIC void linphone_core_disable_chat(LinphoneCore *lc, LinphoneReason deny_reason);
/**
 * Enable reception of incoming chat messages.
 * By default it is enabled but it can be disabled with linphone_core_disable_chat().
 * @param lc A #LinphoneCore object
**/
LINPHONE_PUBLIC void linphone_core_enable_chat(LinphoneCore *lc);
/**
 * Returns whether chat is enabled.
 * @param lc A #LinphoneCore object
**/
LINPHONE_PUBLIC bool_t linphone_core_chat_enabled(const LinphoneCore *lc);
/**
 * Destroy a LinphoneChatRoom.
 * @param cr #LinphoneChatRoom object
 * @deprecated Use linphone_chat_room_unref() instead.
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_chat_room_destroy(LinphoneChatRoom *cr);
/**
 * Create a message attached to a dedicated chat room;
 * @param cr the chat room.
 * @param message text message, NULL if absent.
 * @return a new #LinphoneChatMessage
 */
LINPHONE_PUBLIC	LinphoneChatMessage* linphone_chat_room_create_message(LinphoneChatRoom *cr,const char* message);
/**
 * Create a message attached to a dedicated chat room;
 * @param cr the chat room.
 * @param message text message, NULL if absent.
 * @param external_body_url the URL given in external body or NULL.
 * @param state the LinphoneChatMessage.State of the message.
 * @param time the time_t at which the message has been received/sent.
 * @param is_read TRUE if the message should be flagged as read, FALSE otherwise.
 * @param is_incoming TRUE if the message has been received, FALSE otherwise.
 * @return a new #LinphoneChatMessage
 */
LINPHONE_PUBLIC	LinphoneChatMessage* linphone_chat_room_create_message_2(LinphoneChatRoom *cr, const char* message, const char* external_body_url, LinphoneChatMessageState state, time_t time, bool_t is_read, bool_t is_incoming);

/**
 * Acquire a reference to the chat room.
 * @param[in] cr The chat room.
 * @return The same chat room.
**/
LINPHONE_PUBLIC LinphoneChatRoom *linphone_chat_room_ref(LinphoneChatRoom *cr);

/**
 * Release reference to the chat room.
 * @param[in] cr The chat room.
**/
LINPHONE_PUBLIC void linphone_chat_room_unref(LinphoneChatRoom *cr);

/**
 * Retrieve the user pointer associated with the chat room.
 * @param[in] cr The chat room.
 * @return The user pointer associated with the chat room.
**/
LINPHONE_PUBLIC void *linphone_chat_room_get_user_data(const LinphoneChatRoom *cr);

/**
 * Assign a user pointer to the chat room.
 * @param[in] cr The chat room.
 * @param[in] ud The user pointer to associate with the chat room.
**/
LINPHONE_PUBLIC void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void *ud);

 /**
 * Create a message attached to a dedicated chat room with a particular content.
 * Use #linphone_chat_room_send_message to initiate the transfer
 * @param cr the chat room.
 * @param initial_content #LinphoneContent initial content. #LinphoneCoreVTable.file_transfer_send is invoked later to notify file transfer progress and collect next chunk of the message if LinphoneContent.data is NULL.
 * @return a new #LinphoneChatMessage
 */
LINPHONE_PUBLIC	LinphoneChatMessage* linphone_chat_room_create_file_transfer_message(LinphoneChatRoom *cr, const LinphoneContent* initial_content);

/**
 * get peer address \link linphone_core_get_chat_room() associated to \endlink this #LinphoneChatRoom
 * @param cr #LinphoneChatRoom object
 * @return #LinphoneAddress peer address
 */
LINPHONE_PUBLIC	const LinphoneAddress* linphone_chat_room_get_peer_address(LinphoneChatRoom *cr);

/**
 * Send a message to peer member of this chat room.
 * @deprecated Use linphone_chat_room_send_chat_message() instead.
 * @param cr #LinphoneChatRoom object
 * @param msg message to be sent
 */
LINPHONE_PUBLIC	LINPHONE_DEPRECATED void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg);
/**
 * Send a message to peer member of this chat room.
 * @param cr #LinphoneChatRoom object
 * @param msg #LinphoneChatMessage message to be sent
 * @param status_cb LinphoneChatMessageStateChangeCb status callback invoked when message is delivered or could not be delivered. May be NULL
 * @param ud user data for the status cb.
 * @deprecated Use linphone_chat_room_send_chat_message() instead.
 * @note The LinphoneChatMessage must not be destroyed until the the callback is called.
 * The LinphoneChatMessage reference is transfered to the function and thus doesn't need to be unref'd by the application.
 */
LINPHONE_PUBLIC	LINPHONE_DEPRECATED void linphone_chat_room_send_message2(LinphoneChatRoom *cr, LinphoneChatMessage* msg,LinphoneChatMessageStateChangedCb status_cb,void* ud);
/**
 * Send a message to peer member of this chat room.
 * @param[in] cr LinphoneChatRoom object
 * @param[in] msg LinphoneChatMessage object
 * The state of the message sending will be notified via the callbacks defined in the LinphoneChatMessageCbs object that can be obtained
 * by calling linphone_chat_message_get_callbacks().
 * The LinphoneChatMessage reference is transfered to the function and thus doesn't need to be unref'd by the application.
 */
LINPHONE_PUBLIC void linphone_chat_room_send_chat_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg);

/**
 * Mark all messages of the conversation as read
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation.
 */
LINPHONE_PUBLIC void linphone_chat_room_mark_as_read(LinphoneChatRoom *cr);
/**
 * Delete a message from the chat room history.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation.
 * @param[in] msg The #LinphoneChatMessage object to remove.
 */

LINPHONE_PUBLIC void linphone_chat_room_delete_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg);
/**
 * Delete all messages from the history
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation.
 */
LINPHONE_PUBLIC void linphone_chat_room_delete_history(LinphoneChatRoom *cr);
/**
 * Gets the number of messages in a chat room.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which size has to be computed
 * @return the number of messages.
 */
LINPHONE_PUBLIC int linphone_chat_room_get_history_size(LinphoneChatRoom *cr);

/**
 * Gets nb_message most recent messages from cr chat room, sorted from oldest to most recent.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which messages should be retrieved
 * @param[in] nb_message Number of message to retrieve. 0 means everything.
 * @return \bctbx_list{LinphoneChatMessage}
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_chat_room_get_history(LinphoneChatRoom *cr,int nb_message);

/**
 * Gets the partial list of messages in the given range, sorted from oldest to most recent.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which messages should be retrieved
 * @param[in] begin The first message of the range to be retrieved. History most recent message has index 0.
 * @param[in] end The last message of the range to be retrieved. History oldest message has index of history size - 1 (use #linphone_chat_room_get_history_size to retrieve history size)
 * @return \bctbx_list{LinphoneChatMessage}
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_chat_room_get_history_range(LinphoneChatRoom *cr, int begin, int end);

LINPHONE_PUBLIC LinphoneChatMessage * linphone_chat_room_find_message(LinphoneChatRoom *cr, const char *message_id);

/**
 * Notifies the destination of the chat message being composed that the user is typing a new message.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which a new message is being typed.
 */
LINPHONE_PUBLIC void linphone_chat_room_compose(LinphoneChatRoom *cr);

/**
 * Tells whether the remote is currently composing a message.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation.
 * @return TRUE if the remote is currently composing a message, FALSE otherwise.
 */
LINPHONE_PUBLIC bool_t linphone_chat_room_is_remote_composing(const LinphoneChatRoom *cr);

/**
 * Gets the number of unread messages in the chatroom.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation.
 * @return the number of unread messages.
 */
LINPHONE_PUBLIC int linphone_chat_room_get_unread_messages_count(LinphoneChatRoom *cr);
/**
 * Returns back pointer to #LinphoneCore object.
 * @deprecated use linphone_chat_room_get_core()
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneCore* linphone_chat_room_get_lc(LinphoneChatRoom *cr);
/**
 * Returns back pointer to #LinphoneCore object.
**/
LINPHONE_PUBLIC LinphoneCore* linphone_chat_room_get_core(LinphoneChatRoom *cr);

/**
 * When realtime text is enabled #linphone_call_params_realtime_text_enabled, #LinphoneCoreIsComposingReceivedCb is call everytime a char is received from peer.
 * At the end of remote typing a regular #LinphoneChatMessage is received with committed data from #LinphoneCoreMessageReceivedCb.
 * @param[in] cr #LinphoneChatRoom object
 * @returns  RFC 4103/T.140 char
 */
LINPHONE_PUBLIC uint32_t linphone_chat_room_get_char(const LinphoneChatRoom *cr);

/**
 * Returns true if lime is available for given peer
 *
 * @return true if zrtp secrets have already been shared and ready to use
 */
LINPHONE_PUBLIC bool_t linphone_chat_room_lime_available(LinphoneChatRoom *cr);

/**
 * Returns an list of chat rooms
 * @param[in] lc #LinphoneCore object
 * @return \bctbx_list{LinphoneChatRoom}
**/
LINPHONE_PUBLIC const bctbx_list_t* linphone_core_get_chat_rooms(LinphoneCore *lc);


LINPHONE_PUBLIC unsigned int linphone_chat_message_store(LinphoneChatMessage *msg);

/**
 * Returns a #LinphoneChatMessageState as a string.
 */
LINPHONE_PUBLIC	const char* linphone_chat_message_state_to_string(const LinphoneChatMessageState state);
/**
 * Get the state of the message
 *@param message #LinphoneChatMessage obj
 *@return #LinphoneChatMessageState
 */
LINPHONE_PUBLIC	LinphoneChatMessageState linphone_chat_message_get_state(const LinphoneChatMessage* message);
/**
 * Duplicate a LinphoneChatMessage
**/
LINPHONE_PUBLIC LinphoneChatMessage* linphone_chat_message_clone(const LinphoneChatMessage* message);
/**
 * Acquire a reference to the chat message.
 * @param msg the chat message
 * @return the same chat message
**/
LINPHONE_PUBLIC LinphoneChatMessage * linphone_chat_message_ref(LinphoneChatMessage *msg);
/**
 * Release reference to the chat message.
 * @param msg the chat message.
**/
LINPHONE_PUBLIC void linphone_chat_message_unref(LinphoneChatMessage *msg);
/**
 * Destroys a LinphoneChatMessage.
**/
LINPHONE_PUBLIC void linphone_chat_message_destroy(LinphoneChatMessage* msg);
/** @deprecated Use linphone_chat_message_set_from_address() instead. */
#define linphone_chat_message_set_from(msg, addr) linphone_chat_message_set_from_address(msg, addr)
/**
 * Set origin of the message
 * @param[in] message #LinphoneChatMessage obj
 * @param[in] from #LinphoneAddress origin of this message (copied)
 */
LINPHONE_PUBLIC void linphone_chat_message_set_from_address(LinphoneChatMessage* message, const LinphoneAddress* from);
/** @deprecated Use linphone_chat_message_get_from_address() instead. */
#define linphone_chat_message_get_from(msg) linphone_chat_message_get_from_address(msg)
/**
 * Get origin of the message
 * @param[in] message #LinphoneChatMessage obj
 * @return #LinphoneAddress
 */
LINPHONE_PUBLIC	const LinphoneAddress* linphone_chat_message_get_from_address(const LinphoneChatMessage* message);
#define linphone_chat_message_set_to(msg, addr) linphone_chat_message_set_to_address(msg, addr)
/**
 * Set destination of the message
 * @param[in] message #LinphoneChatMessage obj
 * @param[in] addr #LinphoneAddress destination of this message (copied)
 */
LINPHONE_PUBLIC void linphone_chat_message_set_to_address(LinphoneChatMessage* message, const LinphoneAddress* addr);
/** @deprecated Use linphone_chat_message_get_to_address() instead. */
#define linphone_chat_message_get_to(msg) linphone_chat_message_get_to_address(msg)
/**
 * Get destination of the message
 * @param[in] message #LinphoneChatMessage obj
 * @return #LinphoneAddress
 */
LINPHONE_PUBLIC	const LinphoneAddress* linphone_chat_message_get_to_address(const LinphoneChatMessage* message);
/**
 * Linphone message can carry external body as defined by rfc2017
 * @param message #LinphoneChatMessage
 * @return external body url or NULL if not present.
 */
LINPHONE_PUBLIC	const char* linphone_chat_message_get_external_body_url(const LinphoneChatMessage* message);
/**
 * Linphone message can carry external body as defined by rfc2017
 *
 * @param message a LinphoneChatMessage
 * @param url ex: access-type=URL; URL="http://www.foo.com/file"
 */
LINPHONE_PUBLIC	void linphone_chat_message_set_external_body_url(LinphoneChatMessage* message,const char* url);
/**
 * Get the file_transfer_information (used by call backs to recover informations during a rcs file transfer)
 *
 * @param message #LinphoneChatMessage
 * @return a pointer to the LinphoneContent structure or NULL if not present.
 */
LINPHONE_PUBLIC	const LinphoneContent* linphone_chat_message_get_file_transfer_information(const LinphoneChatMessage* message);
/**
 * Start the download of the file from remote server
 *
 * @param message #LinphoneChatMessage
 * @param status_cb LinphoneChatMessageStateChangeCb status callback invoked when file is downloaded or could not be downloaded
 * @param ud user data
 * @deprecated Use linphone_chat_message_download_file() instead.
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_chat_message_start_file_download(LinphoneChatMessage* message, LinphoneChatMessageStateChangedCb status_cb, void* ud);
/**
 * Start the download of the file referenced in a LinphoneChatMessage from remote server.
 * @param[in] message LinphoneChatMessage object.
 */
LINPHONE_PUBLIC int linphone_chat_message_download_file(LinphoneChatMessage *message);
/**
 * Cancel an ongoing file transfer attached to this message.(upload or download)
 * @param msg	#LinphoneChatMessage
 */
LINPHONE_PUBLIC void linphone_chat_message_cancel_file_transfer(LinphoneChatMessage* msg);
/**
 * Linphone message has an app-specific field that can store a text. The application might want
 * to use it for keeping data over restarts, like thumbnail image path.
 * @param message #LinphoneChatMessage
 * @return the application-specific data or NULL if none has been stored.
 */
LINPHONE_PUBLIC	const char* linphone_chat_message_get_appdata(const LinphoneChatMessage* message);
/**
 * Linphone message has an app-specific field that can store a text. The application might want
 * to use it for keeping data over restarts, like thumbnail image path.
 *
 * Invoking this function will attempt to update the message storage to reflect the changeif it is
 * enabled.
 *
 * @param message #LinphoneChatMessage
 * @param data the data to store into the message
 */
LINPHONE_PUBLIC	void linphone_chat_message_set_appdata(LinphoneChatMessage* message, const char* data);
/**
 * Get text part of this message
 * @return text or NULL if no text.
 */
LINPHONE_PUBLIC	const char* linphone_chat_message_get_text(const LinphoneChatMessage* message);
/**
 * Get the time the message was sent.
 */
LINPHONE_PUBLIC	time_t linphone_chat_message_get_time(const LinphoneChatMessage* message);
/**
 * User pointer get function
 */
LINPHONE_PUBLIC	void* linphone_chat_message_get_user_data(const LinphoneChatMessage* message);
/**
 *User pointer set function
 */
LINPHONE_PUBLIC	void linphone_chat_message_set_user_data(LinphoneChatMessage* message,void*);
/**
 * Returns the chatroom this message belongs to.
**/
LINPHONE_PUBLIC LinphoneChatRoom* linphone_chat_message_get_chat_room(LinphoneChatMessage *msg);

LINPHONE_PUBLIC	const LinphoneAddress* linphone_chat_message_get_peer_address(LinphoneChatMessage *msg);
/**
 * Returns the origin address of a message if it was a outgoing message, or the destination address if it was an incoming message.
 *@param message #LinphoneChatMessage obj
 *@return #LinphoneAddress
 */
LINPHONE_PUBLIC	LinphoneAddress *linphone_chat_message_get_local_address(const LinphoneChatMessage* message);
/**
 * Add custom headers to the message.
 * @param message the message
 * @param header_name name of the header
 * @param header_value header value
**/
LINPHONE_PUBLIC	void linphone_chat_message_add_custom_header(LinphoneChatMessage* message, const char *header_name, const char *header_value);
/**
 * Retrieve a custom header value given its name.
 * @param message the message
 * @param header_name header name searched
**/
LINPHONE_PUBLIC	const char * linphone_chat_message_get_custom_header(LinphoneChatMessage* message, const char *header_name);
/**
 * Removes a custom header from the message.
 * @param msg the message
 * @param header_name name of the header to remove
**/
LINPHONE_PUBLIC	void linphone_chat_message_remove_custom_header(LinphoneChatMessage *msg, const char *header_name);
/**
 * Returns TRUE if the message has been read, otherwise returns FALSE.
 * @param message the message
**/
LINPHONE_PUBLIC bool_t linphone_chat_message_is_read(LinphoneChatMessage* message);
/**
 * Returns TRUE if the message has been sent, returns FALSE if the message has been received.
 * @param message the message
**/
LINPHONE_PUBLIC bool_t linphone_chat_message_is_outgoing(LinphoneChatMessage* message);
/**
 * Returns the id used to identify this message in the storage database
 * @param message the message
 * @return the id
 */
LINPHONE_PUBLIC unsigned int linphone_chat_message_get_storage_id(LinphoneChatMessage* message);
LINPHONE_PUBLIC LinphoneReason linphone_chat_message_get_reason(LinphoneChatMessage* msg);
/**
 * Get full details about delivery error of a chat message.
 * @param msg a LinphoneChatMessage
 * @return a LinphoneErrorInfo describing the details.
**/
LINPHONE_PUBLIC const LinphoneErrorInfo *linphone_chat_message_get_error_info(const LinphoneChatMessage *msg);
/**
 * Set the path to the file to read from or write to during the file transfer.
 * @param[in] msg LinphoneChatMessage object
 * @param[in] filepath The path to the file to use for the file transfer.
 */
LINPHONE_PUBLIC void linphone_chat_message_set_file_transfer_filepath(LinphoneChatMessage *msg, const char *filepath);
/**
 * Get the path to the file to read from or write to during the file transfer.
 * @param[in] msg LinphoneChatMessage object
 * @return The path to the file to use for the file transfer.
 */
LINPHONE_PUBLIC const char * linphone_chat_message_get_file_transfer_filepath(LinphoneChatMessage *msg);

/**
 * Fulfill a chat message char by char. Message linked to a Real Time Text Call send char in realtime following RFC 4103/T.140
 * To commit a message, use #linphone_chat_room_send_message
 * @param[in] msg LinphoneChatMessage
 * @param[in] character T.140 char
 * @returns 0 if succeed.
 */
LINPHONE_PUBLIC int linphone_chat_message_put_char(LinphoneChatMessage *msg,uint32_t character);

/**
 * Get the message identifier.
 * It is used to identify a message so that it can be notified as delivered and/or displayed.
 * @param[in] cm LinphoneChatMessage object
 * @return The message identifier.
 */
LINPHONE_PUBLIC	const char* linphone_chat_message_get_message_id(const LinphoneChatMessage *cm);

/**
 * get Curent Call associated to this chatroom if any
 * To commit a message, use #linphone_chat_room_send_message
 * @param[in] room LinphoneChatRomm
 * @returns LinphoneCall or NULL.
 */
LINPHONE_PUBLIC LinphoneCall *linphone_chat_room_get_call(const LinphoneChatRoom *room);


/**
 * Get the LinphoneChatMessageCbs object associated with the LinphoneChatMessage.
 * @param[in] msg LinphoneChatMessage object
 * @return The LinphoneChatMessageCbs object associated with the LinphoneChatMessage.
 */
LINPHONE_PUBLIC LinphoneChatMessageCbs * linphone_chat_message_get_callbacks(const LinphoneChatMessage *msg);

/**
 * Acquire a reference to the LinphoneChatMessageCbs object.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The same LinphoneChatMessageCbs object.
 */
LINPHONE_PUBLIC LinphoneChatMessageCbs * linphone_chat_message_cbs_ref(LinphoneChatMessageCbs *cbs);

/**
 * Release reference to the LinphoneChatMessageCbs object.
 * @param[in] cbs LinphoneChatMessageCbs object.
 */
LINPHONE_PUBLIC void linphone_chat_message_cbs_unref(LinphoneChatMessageCbs *cbs);

/**
 * Retrieve the user pointer associated with the LinphoneChatMessageCbs object.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The user pointer associated with the LinphoneChatMessageCbs object.
 */
LINPHONE_PUBLIC void *linphone_chat_message_cbs_get_user_data(const LinphoneChatMessageCbs *cbs);

/**
 * Assign a user pointer to the LinphoneChatMessageCbs object.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] ud The user pointer to associate with the LinphoneChatMessageCbs object.
 */
LINPHONE_PUBLIC void linphone_chat_message_cbs_set_user_data(LinphoneChatMessageCbs *cbs, void *ud);

/**
 * Get the message state changed callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The current message state changed callback.
 */
LINPHONE_PUBLIC LinphoneChatMessageCbsMsgStateChangedCb linphone_chat_message_cbs_get_msg_state_changed(const LinphoneChatMessageCbs *cbs);

/**
 * Set the message state changed callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The message state changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_message_cbs_set_msg_state_changed(LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsMsgStateChangedCb cb);

/**
 * Get the file transfer receive callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The current file transfer receive callback.
 */
LINPHONE_PUBLIC LinphoneChatMessageCbsFileTransferRecvCb linphone_chat_message_cbs_get_file_transfer_recv(const LinphoneChatMessageCbs *cbs);

/**
 * Set the file transfer receive callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The file transfer receive callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_message_cbs_set_file_transfer_recv(LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsFileTransferRecvCb cb);

/**
 * Get the file transfer send callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The current file transfer send callback.
 */
LINPHONE_PUBLIC LinphoneChatMessageCbsFileTransferSendCb linphone_chat_message_cbs_get_file_transfer_send(const LinphoneChatMessageCbs *cbs);

/**
 * Set the file transfer send callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The file transfer send callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_message_cbs_set_file_transfer_send(LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsFileTransferSendCb cb);

/**
 * Get the file transfer progress indication callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The current file transfer progress indication callback.
 */
LINPHONE_PUBLIC LinphoneChatMessageCbsFileTransferProgressIndicationCb linphone_chat_message_cbs_get_file_transfer_progress_indication(const LinphoneChatMessageCbs *cbs);

/**
 * Set the file transfer progress indication callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The file transfer progress indication callback to be used.
 */
LINPHONE_PUBLIC void linphone_chat_message_cbs_set_file_transfer_progress_indication(LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsFileTransferProgressIndicationCb cb);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* LINPHONE_CHAT_H_ */
