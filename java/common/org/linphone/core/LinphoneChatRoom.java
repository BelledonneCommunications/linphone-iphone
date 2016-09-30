/*
LinphoneChatRoom.java
Copyright (C) 2010  Belledonne Communications, Grenoble, France

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
package org.linphone.core;

import org.linphone.core.LinphoneChatMessage.State;

/**
 *
 * A chat room is the place where text messages are exchanged.
Can be created by linphone_core_create_chat_room().
 *
 */
public interface LinphoneChatRoom {
	/**
	 * get peer address associated to this LinphoneChatRoom
	 *
	 * @return LinphoneAddress peer address
	 */
	LinphoneAddress getPeerAddress();
	
	/**
	* send a message to peer member of this chat room.
	* @param  	message to be sent
	*/
	void sendMessage(String message);
	
	/**
	 * Send a message to peer member of this chat room.
	 * @param message chat message
	 * @deprecated
	 */
	@Deprecated
	void sendMessage(LinphoneChatMessage message, LinphoneChatMessage.StateListener listener);

	/**
	 * Create a LinphoneChatMessage
	 * @param message message to send
	 * @return LinphoneChatMessage object
	 */
	LinphoneChatMessage createLinphoneChatMessage(String message);

	/**
	 * Returns the chat history associated with the peer address associated with this chat room
	 * @return an array of LinphoneChatMessage
	 */
	LinphoneChatMessage[] getHistory();

	/**
	 * Returns the chat history associated with the peer address associated with this chat room
	 * @param limit the maximum number of messages to fetch
	 * @return an array of LinphoneChatMessage
	 */
	LinphoneChatMessage[] getHistory(int limit);

	/**
	 * Returns the chat history associated with the peer address associated with this chat room for the given range, sorted from oldest to most recent
	 * @param begin the first (most recent) message to retrieve. Newest message has index 0. If negative, use value 0 instead.
	 * @param end the last (oldest) message to retrieve. Oldest message has value "history size" - 1 (equivalent to -1). If negative or lower than begin value,  value is given, use -1.
	 * @return an array of LinphoneChatMessage, empty if nothing has been found
	 */
	LinphoneChatMessage[] getHistoryRange(int begin, int end);

	/**
	 * Returns the amount of unread messages associated with the peer of this chatRoom.
	 * @return the amount of unread messages
	 */
	int getUnreadMessagesCount();

	/**
	 * Returns the amount of messages associated with the peer of this chatRoom.
	 * @return the amount of messages in the conversation
	 */
	int getHistorySize();

	/**
	 * Deletes all the messages associated with the peer of this chat room
	 */
	void deleteHistory();

	/**
	 * Notify the destination of the chat message being composed that the user is typing a new message.
	 */
	void compose();

	/**
	 * Tells whether the remote is currently composing a message.
	 * @return true if the remote is currently composing a message, false otherwise.
	 */
	boolean isRemoteComposing();

	/**
	 * Marks all the messages in this conversation as read
	 */
	void markAsRead();

	/**
	 * Deletes a message
	 * @param message the message to delete
	 */
	void deleteMessage(LinphoneChatMessage message);

	/**
	 * Create a LinphoneChatMessage
	 * @return LinphoneChatMessage object
	 */
	LinphoneChatMessage createLinphoneChatMessage(String message, String url, State state, long timestamp, boolean isRead, boolean isIncoming);
	
	/**
	 * Returns a back pointer to the core managing the chat room.
	 * @return the LinphoneCore
	 */
	LinphoneCore getCore();
	
	/**
	 * Create a message attached to a dedicated chat room with a particular content.
	 * @param content LinphoneContent initial content.
	 * @return a new LinphoneChatMessage
	 */
	LinphoneChatMessage createFileTransferMessage(LinphoneContent content);
	
	/**
	 * 
	 * @param message
	 */
	void sendChatMessage(LinphoneChatMessage message);
	
	/**
	 * get Curent Call associated to this chatroom if any
	 * To commit a message, use #linphone_chat_room_send_message
	 * @return LinphoneCall or NULL.
	 */
	public LinphoneCall getCall();
	/**
	 * When realtime text is enabled LinphoneCallParams.realTimeTextEnabled, LinphoneCoreListener.isComposingReceived is call every time a char is received from peer.
	 * At the end of remote typing a regular LinphoneChatMessage is received with committed data from LinphoneCoreListener.messageReceived .
	 * @return  RFC 4103/T.140 char
	 */
	long getChar();
	
	
}
