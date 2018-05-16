/*
 * c-callbacks.h
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

#ifndef _L_C_CALLBACKS_H_
#define _L_C_CALLBACKS_H_

// TODO: Remove me in the future.
#include "linphone/callbacks.h"
#include "linphone/api/c-types.h"

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif // ifdef __cplusplus

/**
 * @addtogroup call_control
 * @{
**/

/**
 * Callback for being notified of received DTMFs.
 * @param call #LinphoneCall object that received the dtmf
 * @param dtmf The ascii code of the dtmf
 */
typedef void (*LinphoneCallCbsDtmfReceivedCb)(LinphoneCall *call, int dtmf);

/**
 * Call encryption changed callback.
 * @param call #LinphoneCall object whose encryption is changed.
 * @param on Whether encryption is activated.
 * @param authentication_token An authentication_token, currently set for ZRTP kind of encryption only.
 */
typedef void (*LinphoneCallCbsEncryptionChangedCb)(LinphoneCall *call, bool_t on, const char *authentication_token);

/**
 * Callback for receiving info messages.
 * @param call #LinphoneCall whose info message belongs to.
 * @param msg #LinphoneInfoMessage object.
 */
typedef void (*LinphoneCallCbsInfoMessageReceivedCb)(LinphoneCall *call, const LinphoneInfoMessage *msg);

/**
 * Call state notification callback.
 * @param call #LinphoneCall whose state is changed.
 * @param cstate The new state of the call
 * @param message An informational message about the state.
 */
typedef void (*LinphoneCallCbsStateChangedCb)(LinphoneCall *call, LinphoneCallState cstate, const char *message);

/**
 * Callback for receiving quality statistics for calls.
 * @param call #LinphoneCall object whose statistics are notified
 * @param stats #LinphoneCallStats object
 */
typedef void (*LinphoneCallCbsStatsUpdatedCb)(LinphoneCall *call, const LinphoneCallStats *stats);

/**
 * Callback for notifying progresses of transfers.
 * @param call #LinphoneCall that was transfered
 * @param cstate The state of the call to transfer target at the far end.
 */
typedef void (*LinphoneCallCbsTransferStateChangedCb)(LinphoneCall *call, LinphoneCallState cstate);

/**
 * Callback for notifying the processing SIP ACK messages.
 * @param call #LinphoneCall for which an ACK is being received or sent
 * @param ack the ACK message
 * @param is_received if TRUE this ACK is an incoming one, otherwise it is an ACK about to be sent.
 */
typedef void (*LinphoneCallCbsAckProcessingCb)(LinphoneCall *call, LinphoneHeaders *ack, bool_t is_received);

/**
 * Callback for notifying a received TMMBR.
 * @param call LinphoneCall for which the TMMBR has changed
 * @param stream_index the index of the current stream
 * @param tmmbr the value of the received TMMBR
 */
typedef void (*LinphoneCallCbsTmmbrReceivedCb)(LinphoneCall *call, int stream_index, int tmmbr);

/**
 * Callback for notifying a snapshot taken.
 * @param call LinphoneCall for which the snapshot was taken
 * @param filepath the name of the saved file
 */
typedef void (*LinphoneCallCbsSnapshotTakenCb)(LinphoneCall *call, const char *filepath);

 /**
 * Callback to notify a next video frame has been decoded
 * @param call LinphoneCall for which the next video frame has been decoded
 */
typedef void (*LinphoneCallCbsNextVideoFrameDecodedCb)(LinphoneCall *call);

/**
 * @}
**/


/**
 * @addtogroup chatroom
 * @{
 */

 /**
 * Call back used to notify message delivery status
 * @param msg #LinphoneChatMessage object
 * @param status #LinphoneChatMessageState
 * @param ud application user data
 * @deprecated Use #LinphoneChatMessageCbsMsgStateChangedCb instead.
 * @donotwrap
 */
typedef void (*LinphoneChatMessageStateChangedCb)(LinphoneChatMessage* msg, LinphoneChatMessageState state, void* ud);

/**
 * Call back used to notify message delivery status
 * @param msg #LinphoneChatMessage object
 * @param status #LinphoneChatMessageState
 */
typedef void (*LinphoneChatMessageCbsMsgStateChangedCb)(LinphoneChatMessage* msg, LinphoneChatMessageState state);

/**
 * Call back used to notify participant IMDN state
 * @param msg #LinphoneChatMessage object
 * @param state #LinphoneParticipantImdnState
 */
typedef void (*LinphoneChatMessageCbsParticipantImdnStateChangedCb)(LinphoneChatMessage* msg, const LinphoneParticipantImdnState *state);

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
 * @return A #LinphoneBuffer object holding the data written by the application. An empty buffer means end of file.
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
 * Is composing notification callback prototype.
 * @param[in] cr #LinphoneChatRoom involved in the conversation
 * @param[in] remoteAddr The address that has sent the is-composing notification
 * @param[in] isComposing A boolean value telling whether the remote is composing or not
 */
typedef void (*LinphoneChatRoomCbsIsComposingReceivedCb) (LinphoneChatRoom *cr, const LinphoneAddress *remoteAddr, bool_t isComposing);

/**
 * Callback used to notify a chat room that a message has been received.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] msg The #LinphoneChatMessage that has been received
 */
typedef void (*LinphoneChatRoomCbsMessageReceivedCb) (LinphoneChatRoom *cr, LinphoneChatMessage *msg);

/**
 * Callback used to notify a chat room that a chat message has been received.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] event_log The #LinphoneChatMessage event log that has been received
 */
typedef void (*LinphoneChatRoomCbsChatMessageReceivedCb) (LinphoneChatRoom *cr, const LinphoneEventLog *event_log);

/**
 * Callback used to notify a chat room that a chat message is being sent.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] event_log The #LinphoneChatMessage event log that is being sent
 */
typedef void (*LinphoneChatRoomCbsChatMessageSentCb) (LinphoneChatRoom *cr, const LinphoneEventLog *event_log);

/**
 * Callback used to notify a chat room that a participant has been added.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] participant The #LinphoneParticipant that has been added to the chat room
 */
typedef void (*LinphoneChatRoomCbsParticipantAddedCb) (LinphoneChatRoom *cr, const LinphoneEventLog *event_log);

/**
 * Callback used to notify a chat room that a participant has been removed.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] participant The #LinphoneParticipant that has been removed from the chat room
 */
typedef void (*LinphoneChatRoomCbsParticipantRemovedCb) (LinphoneChatRoom *cr, const LinphoneEventLog *event_log);

/**
 * Callback used to notify a chat room that the admin status of a participant has been changed.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] participant The #LinphoneParticipant for which the admin status has been changed
 * @param[in] isAdmin The new admin status of the participant
 */
typedef void (*LinphoneChatRoomCbsParticipantAdminStatusChangedCb) (LinphoneChatRoom *cr, const LinphoneEventLog *event_log);

/**
 * Callback used to notify a chat room state has changed.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] newState The new state of the chat room
 */
typedef void (*LinphoneChatRoomCbsStateChangedCb) (LinphoneChatRoom *cr, LinphoneChatRoomState newState);

/**
 * Callback used to notify that the subject of a chat room has changed.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] subject The new subject of the chat room
 */
typedef void (*LinphoneChatRoomCbsSubjectChangedCb) (LinphoneChatRoom *cr, const LinphoneEventLog *event_log);

/**
 * Callback used to notify a chat room that a message has been received but we were unable to decrypt it
 * @param cr #LinphoneChatRoom involved in this conversation
 * @param msg The #LinphoneChatMessage that has been received
 */
typedef void (*LinphoneChatRoomCbsUndecryptableMessageReceivedCb) (LinphoneChatRoom *cr, LinphoneChatMessage *msg);

/**
 * Callback used to notify a chat room that a participant has been added.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] participant The #LinphoneParticipant that has been added to the chat room
 */
typedef void (*LinphoneChatRoomCbsParticipantDeviceAddedCb) (LinphoneChatRoom *cr, const LinphoneEventLog *event_log);

/**
 * Callback used to notify a chat room that a participant has been removed.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] participant The #LinphoneParticipant that has been removed from the chat room
 */
typedef void (*LinphoneChatRoomCbsParticipantDeviceRemovedCb) (LinphoneChatRoom *cr, const LinphoneEventLog *event_log);

/**
 * Callback used to notify a chat room has been joined.
 * @param[in] cr #LinphoneChatRoom object
 */
typedef void (*LinphoneChatRoomCbsConferenceJoinedCb) (LinphoneChatRoom *cr, const LinphoneEventLog *eventLog);

/**
 * Callback used to notify a chat room has been left.
 * @param[in] cr #LinphoneChatRoom object
 */
typedef void (*LinphoneChatRoomCbsConferenceLeftCb) (LinphoneChatRoom *cr, const LinphoneEventLog *eventLog);

/**
 * Callback used when a group chat room is created server-side to generate the address of the chat room.
 * The function linphone_chat_room_set_conference_address() needs to be called by this callback.
 * @param[in] cr #LinphoneChatRoom object
 */
typedef void (*LinphoneChatRoomCbsConferenceAddressGenerationCb) (LinphoneChatRoom *cr);

/**
 * Callback used when a group chat room server is adding participant to fetch all device information from participant.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] participantAddr #LinphoneAddress object
 */
typedef void (*LinphoneChatRoomCbsParticipantDeviceFetchRequestedCb) (LinphoneChatRoom *cr, const LinphoneAddress *participantAddr);

/**
 * Callback used when a group chat room server is checking participants capabilities.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] deviceAddr #LinphoneAddress object
 * @param[in] participantsAddr \bctbx_list{LinphoneAddress}
 */
typedef void (*LinphoneChatRoomCbsParticipantsCapabilitiesCheckedCb) (LinphoneChatRoom *cr, const LinphoneAddress *deviceAddr, const bctbx_list_t *participantsAddr);

/**
 * Callback used when a group chat room server is subscribing to registration state of a participant.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] participantAddr #LinphoneAddress object
 */
typedef void (*LinphoneChatRoomCbsParticipantRegistrationSubscriptionRequestedCb) (LinphoneChatRoom *cr, const LinphoneAddress *participantAddr);

/**
 * Callback used when a group chat room server is unsubscribing to registration state of a participant.
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] participantAddr #LinphoneAddress object
 */
typedef void (*LinphoneChatRoomCbsParticipantRegistrationUnsubscriptionRequestedCb) (LinphoneChatRoom *cr, const LinphoneAddress *participantAddr);

/**
 * Callback used to tell the core whether or not to store the incoming message in db or not using linphone_chat_message_set_to_be_stored().
 * @param[in] cr #LinphoneChatRoom object
 * @param[in] msg The #LinphoneChatMessage that is being received
 */
typedef void (*LinphoneChatRoomCbsShouldChatMessageBeStoredCb) (LinphoneChatRoom *cr, LinphoneChatMessage *msg);

/**
 * @}
**/

#ifdef __cplusplus
	}
#endif // ifdef __cplusplus

#endif // ifndef _L_C_CALLBACKS_H_
