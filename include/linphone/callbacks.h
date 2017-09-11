/*
callbacks.h
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

#ifndef LINPHONE_CALLBACKS_H_
#define LINPHONE_CALLBACKS_H_


#include "linphone/types.h"


/**
 * @addtogroup chatroom
 * @{
 */

/**
 * Call back used to notify message delivery status
 * @param msg #LinphoneChatMessage object
 * @param status LinphoneChatMessageState
 * @param ud application user data
 * @deprecated Use LinphoneChatMessageCbsMsgStateChangedCb instead.
 * @donotwrap
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
 * @}
**/

/**
 * @addtogroup call_control
 * @{
**/

/**
 * Callback for being notified of received DTMFs.
 * @param call LinphoneCall object that received the dtmf
 * @param dtmf The ascii code of the dtmf
 */
typedef void (*LinphoneCallCbsDtmfReceivedCb)(LinphoneCall *call, int dtmf);

/**
 * Call encryption changed callback.
 * @param call LinphoneCall object whose encryption is changed.
 * @param on Whether encryption is activated.
 * @param authentication_token An authentication_token, currently set for ZRTP kind of encryption only.
 */
typedef void (*LinphoneCallCbsEncryptionChangedCb)(LinphoneCall *call, bool_t on, const char *authentication_token);

/**
 * Callback for receiving info messages.
 * @param call LinphoneCall whose info message belongs to.
 * @param msg LinphoneInfoMessage object.
 */
typedef void (*LinphoneCallCbsInfoMessageReceivedCb)(LinphoneCall *call, const LinphoneInfoMessage *msg);

/**
 * Call state notification callback.
 * @param call LinphoneCall whose state is changed.
 * @param cstate The new state of the call
 * @param message An informational message about the state.
 */
typedef void (*LinphoneCallCbsStateChangedCb)(LinphoneCall *call, LinphoneCallState cstate, const char *message);

/**
 * Callback for receiving quality statistics for calls.
 * @param call LinphoneCall object whose statistics are notified
 * @param stats LinphoneCallStats object
 */
typedef void (*LinphoneCallCbsStatsUpdatedCb)(LinphoneCall *call, const LinphoneCallStats *stats);

/**
 * Callback for notifying progresses of transfers.
 * @param call LinphoneCall that was transfered
 * @param cstate The state of the call to transfer target at the far end.
 */
typedef void (*LinphoneCallCbsTransferStateChangedCb)(LinphoneCall *call, LinphoneCallState cstate);

/**
 * Callback for notifying the processing SIP ACK messages.
 * @param call LinphoneCall for which an ACK is being received or sent
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
 * @}
**/

/**
 * @addtogroup initializing
 * @{
**/

/**
 * Callback notifying that a new LinphoneCall (either incoming or outgoing) has been created.
 * @param[in] lc LinphoneCore object that has created the call
 * @param[in] call The newly created LinphoneCall object
 */
typedef void (*LinphoneCoreCbsCallCreatedCb)(LinphoneCore *lc, LinphoneCall *call);

/**
 * Global state notification callback.
 * @param lc the #LinphoneCore.
 * @param gstate the global state
 * @param message informational message.
 */
typedef void (*LinphoneCoreCbsGlobalStateChangedCb)(LinphoneCore *lc, LinphoneGlobalState gstate, const char *message);

/**
 * Old name of #LinphoneCoreCbsGlobalStateChangedCb.
 */
typedef LinphoneCoreCbsGlobalStateChangedCb LinphoneCoreGlobalStateChangedCb;

/**
 * Call state notification callback.
 * @param lc the LinphoneCore
 * @param call the call object whose state is changed.
 * @param cstate the new state of the call
 * @param message a non NULL informational message about the state.
 */
typedef void (*LinphoneCoreCbsCallStateChangedCb)(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *message);

/**
 * Old name of #LinphoneCoreCbsCallStateChangedCb.
 */
typedef LinphoneCoreCbsCallStateChangedCb LinphoneCoreCallStateChangedCb;

/**
 * Call encryption changed callback.
 * @param lc the LinphoneCore
 * @param call the call on which encryption is changed.
 * @param on whether encryption is activated.
 * @param authentication_token an authentication_token, currently set for ZRTP kind of encryption only.
 */
typedef void (*LinphoneCoreCbsCallEncryptionChangedCb)(LinphoneCore *lc, LinphoneCall *call, bool_t on, const char *authentication_token);

/**
 * Old name of #LinphoneCoreCbsCallEncryptionChangedCb.
 */
typedef LinphoneCoreCbsCallEncryptionChangedCb LinphoneCoreCallEncryptionChangedCb;

/**
 * Registration state notification callback prototype
 * @ingroup Proxies
 */
typedef void (*LinphoneCoreCbsRegistrationStateChangedCb)(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message);

/**
 * Old name of #LinphoneCoreCbsRegistrationStateChangedCb.
 */
typedef LinphoneCoreCbsRegistrationStateChangedCb LinphoneCoreRegistrationStateChangedCb;

/**
 * Report status change for a friend previously \link linphone_core_add_friend() added \endlink to #LinphoneCore.
 * @param lc #LinphoneCore object .
 * @param lf Updated #LinphoneFriend .
 */
typedef void (*LinphoneCoreCbsNotifyPresenceReceivedCb)(LinphoneCore *lc, LinphoneFriend * lf);

/**
 * Old name of #LinphoneCoreCbsNotifyPresenceReceivedCb.
 */
typedef LinphoneCoreCbsNotifyPresenceReceivedCb LinphoneCoreNotifyPresenceReceivedCb;

/**
 * Reports presence model change for a specific URI or phone number of a friend
 * @param lc #LinphoneCore object
 * @param lf #LinphoneFriend object
 * @param uri_or_tel The URI or phone number for which teh presence model has changed
 * @param presence_model The new presence model
 */
typedef void (*LinphoneCoreCbsNotifyPresenceReceivedForUriOrTelCb)(LinphoneCore *lc, LinphoneFriend *lf, const char *uri_or_tel, const LinphonePresenceModel *presence_model);

/**
 * Old name of #LinphoneCoreCbsNotifyPresenceReceivedForUriOrTelCb.
 */
typedef LinphoneCoreCbsNotifyPresenceReceivedForUriOrTelCb LinphoneCoreNotifyPresenceReceivedForUriOrTelCb;

/**
 * Reports that a new subscription request has been received and wait for a decision.
 * Status on this subscription request is notified by \link linphone_friend_set_inc_subscribe_policy() changing policy \endlink for this friend
 * @param lc #LinphoneCore object
 * @param lf #LinphoneFriend corresponding to the subscriber
 * @param url of the subscriber
 */
typedef void (*LinphoneCoreCbsNewSubscriptionRequestedCb)(LinphoneCore *lc, LinphoneFriend *lf, const char *url);

/**
 * Old name of #LinphoneCoreCbsNewSubscriptionRequestedCb.
 */
typedef LinphoneCoreCbsNewSubscriptionRequestedCb LinphoneCoreNewSubscriptionRequestedCb;

/**
 * Callback for requesting authentication information to application or user.
 * @param lc the LinphoneCore
 * @param realm the realm (domain) on which authentication is required.
 * @param username the username that needs to be authenticated.
 * @param domain the domain on which authentication is required.
 * Application shall reply to this callback using linphone_core_add_auth_info().
 */
typedef void (*LinphoneCoreAuthInfoRequestedCb)(LinphoneCore *lc, const char *realm, const char *username, const char *domain);

/**
 * Callback for requesting authentication information to application or user.
 * @param lc the LinphoneCore
 * @param auth_info a LinphoneAuthInfo pre-filled with username, realm and domain values as much as possible
 * @param method the type of authentication requested
 * Application shall reply to this callback using linphone_core_add_auth_info().
 */
typedef void (*LinphoneCoreCbsAuthenticationRequestedCb)(LinphoneCore *lc, LinphoneAuthInfo *auth_info, LinphoneAuthMethod method);

/**
 * Old name of #LinphoneCoreCbsAuthenticationRequestedCb.
 */
typedef LinphoneCoreCbsAuthenticationRequestedCb LinphoneCoreAuthenticationRequestedCb;

/**
 * Callback to notify a new call-log entry has been added.
 * This is done typically when a call terminates.
 * @param lc the LinphoneCore
 * @param newcl the new call log entry added.
 */
typedef void (*LinphoneCoreCbsCallLogUpdatedCb)(LinphoneCore *lc, LinphoneCallLog *newcl);

/**
 * Old name of #LinphoneCoreCbsCallLogUpdatedCb.
 */
typedef LinphoneCoreCbsCallLogUpdatedCb LinphoneCoreCallLogUpdatedCb;

/**
 * Callback prototype
 * @param lc #LinphoneCore object
 * @param room #LinphoneChatRoom involved in this conversation. Can be be created by the framework in case \link #LinphoneAddress the from \endlink is not present in any chat room.
 * @param from #LinphoneAddress from
 * @param message incoming message
 * @deprecated use #LinphoneCoreMessageReceivedCb instead.
 * @donotwrap
 */
typedef void (*LinphoneCoreTextMessageReceivedCb)(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message);

/**
 * Chat message callback prototype
 * @param lc #LinphoneCore object
 * @param room #LinphoneChatRoom involved in this conversation. Can be be created by the framework in case \link #LinphoneAddress the from \endlink is not present in any chat room.
 * @param LinphoneChatMessage incoming message
 */
typedef void (*LinphoneCoreCbsMessageReceivedCb)(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *message);

/**
 * Old name of #LinphoneCoreCbsMessageReceivedCb.
 */
typedef LinphoneCoreCbsMessageReceivedCb LinphoneCoreMessageReceivedCb;

/**
 * Chat message not decrypted callback prototype
 * @param lc #LinphoneCore object
 * @param room #LinphoneChatRoom involved in this conversation. Can be be created by the framework in case \link #LinphoneAddress the from \endlink is not present in any chat room.
 * @param LinphoneChatMessage incoming message
 */
typedef void (*LinphoneCoreCbsMessageReceivedUnableDecryptCb)(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *message);

/**
 * File transfer receive callback prototype. This function is called by the core upon an incoming File transfer is started. This function may be call several time for the same file in case of large file.
 * @param lc #LinphoneCore object
 * @param message #LinphoneChatMessage message from which the body is received.
 * @param content #LinphoneContent incoming content information
 * @param buff pointer to the received data
 * @param size number of bytes to be read from buff. 0 means end of file.
 */
typedef void (*LinphoneCoreFileTransferRecvCb)(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, const char* buff, size_t size);

/**
 * File transfer send callback prototype. This function is called by the core upon an outgoing file transfer is started. This function is called until size is set to 0.
 * @param lc #LinphoneCore object
 * @param message #LinphoneChatMessage message from which the body is received.
 * @param content #LinphoneContent outgoing content
 * @param buff pointer to the buffer where data chunk shall be written by the app
 * @param size as input value, it represents the number of bytes expected by the framework. As output value, it means the number of bytes wrote by the application in the buffer. 0 means end of file.
 *
 */
typedef void (*LinphoneCoreFileTransferSendCb)(LinphoneCore *lc, LinphoneChatMessage *message,  const LinphoneContent* content, char* buff, size_t* size);

/**
 * File transfer progress indication callback prototype.
 * @param lc #LinphoneCore object
 * @param message #LinphoneChatMessage message from which the body is received.
 * @param content #LinphoneContent incoming content information
 * @param offset The number of bytes sent/received since the beginning of the transfer.
 * @param total The total number of bytes to be sent/received.
 */
typedef void (*LinphoneCoreFileTransferProgressIndicationCb)(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t total);

/**
 * Is composing notification callback prototype.
 * @param[in] lc #LinphoneCore object
 * @param[in] room #LinphoneChatRoom involved in the conversation.
 */
typedef void (*LinphoneCoreCbsIsComposingReceivedCb)(LinphoneCore *lc, LinphoneChatRoom *room);

/**
 * Old name of #LinphoneCoreCbsIsComposingReceivedCb.
 */
typedef LinphoneCoreCbsIsComposingReceivedCb LinphoneCoreIsComposingReceivedCb;

/**
 * Callback for being notified of DTMFs received.
 * @param lc the linphone core
 * @param call the call that received the dtmf
 * @param dtmf the ascii code of the dtmf
 */
typedef void (*LinphoneCoreCbsDtmfReceivedCb)(LinphoneCore* lc, LinphoneCall *call, int dtmf);

/**
 * Old name of #LinphoneCoreCbsDtmfReceivedCb.
 */
typedef LinphoneCoreCbsDtmfReceivedCb LinphoneCoreDtmfReceivedCb;

/** Callback prototype */
typedef void (*LinphoneCoreCbsReferReceivedCb)(LinphoneCore *lc, const char *refer_to);

/**
 * Old name of #LinphoneCoreCbsReferReceivedCb.
 */
typedef LinphoneCoreCbsReferReceivedCb LinphoneCoreReferReceivedCb;

/** Callback prototype */
typedef void (*LinphoneCoreCbsBuddyInfoUpdatedCb)(LinphoneCore *lc, LinphoneFriend *lf);

/**
 * Old name of #LinphoneCoreCbsBuddyInfoUpdatedCb.
 */
typedef LinphoneCoreCbsBuddyInfoUpdatedCb LinphoneCoreBuddyInfoUpdatedCb;

/**
 * Callback for notifying progresses of transfers.
 * @param lc the LinphoneCore
 * @param transfered the call that was transfered
 * @param new_call_state the state of the call to transfer target at the far end.
 */
typedef void (*LinphoneCoreCbsTransferStateChangedCb)(LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state);

/**
 * Old name of LinphoneCoreCbsTransferStateChangedCb.
 */
typedef LinphoneCoreCbsTransferStateChangedCb LinphoneCoreTransferStateChangedCb;

/**
 * Callback for receiving quality statistics for calls.
 * @param lc the LinphoneCore
 * @param call the call
 * @param stats the call statistics.
 */
typedef void (*LinphoneCoreCbsCallStatsUpdatedCb)(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *stats);

/**
 * Old name of #LinphoneCoreCbsCallStatsUpdatedCb.
 */
typedef LinphoneCoreCbsCallStatsUpdatedCb LinphoneCoreCallStatsUpdatedCb;

/**
 * Callback prototype for receiving info messages.
 * @param lc the LinphoneCore
 * @param call the call whose info message belongs to.
 * @param msg the info message.
 */
typedef void (*LinphoneCoreCbsInfoReceivedCb)(LinphoneCore *lc, LinphoneCall *call, const LinphoneInfoMessage *msg);

/**
 * Old name of #LinphoneCoreCbsInfoReceivedCb.
 */
typedef LinphoneCoreCbsInfoReceivedCb LinphoneCoreInfoReceivedCb;

/**
 * Callback prototype for configuring status changes notification
 * @param lc the LinphoneCore
 * @param message informational message.
 */
typedef void (*LinphoneCoreCbsConfiguringStatusCb)(LinphoneCore *lc, LinphoneConfiguringState status, const char *message);

/**
 * Old name of #LinphoneCoreCbsConfiguringStatusCb.
 */
typedef LinphoneCoreCbsConfiguringStatusCb LinphoneCoreConfiguringStatusCb;

/**
 * Callback prototype for reporting network change either automatically detected or notified by #linphone_core_set_network_reachable.
 * @param lc the LinphoneCore
 * @param reachable true if network is reachable.
 */
typedef void (*LinphoneCoreCbsNetworkReachableCb)(LinphoneCore *lc, bool_t reachable);

/**
 * Old name of #LinphoneCoreCbsNetworkReachableCb.
 */
typedef LinphoneCoreCbsNetworkReachableCb LinphoneCoreNetworkReachableCb;

/**
 * Callback prototype for reporting log collection upload state change.
 * @param[in] lc LinphoneCore object
 * @param[in] state The state of the log collection upload
 * @param[in] info Additional information: error message in case of error state, URL of uploaded file in case of success.
 */
typedef void (*LinphoneCoreCbsLogCollectionUploadStateChangedCb)(LinphoneCore *lc, LinphoneCoreLogCollectionUploadState state, const char *info);

/**
 * Old name of #LinphoneCoreCbsLogCollectionUploadStateChangedCb.
 */
typedef LinphoneCoreCbsLogCollectionUploadStateChangedCb LinphoneCoreLogCollectionUploadStateChangedCb;

/**
 * Callback prototype for reporting log collection upload progress indication.
 * @param[in] lc LinphoneCore object
 */
typedef void (*LinphoneCoreCbsLogCollectionUploadProgressIndicationCb)(LinphoneCore *lc, size_t offset, size_t total);

/**
 * Old name of #LinphoneCoreCbsLogCollectionUploadProgressIndicationCb.
 */
typedef LinphoneCoreCbsLogCollectionUploadProgressIndicationCb LinphoneCoreLogCollectionUploadProgressIndicationCb;

/**
 * Callback prototype for reporting when a friend list has been added to the core friends list.
 * @param[in] lc LinphoneCore object
 * @param[in] list LinphoneFriendList object
 */
typedef void (*LinphoneCoreCbsFriendListCreatedCb) (LinphoneCore *lc, LinphoneFriendList *list);

/**
 * Old name of #LinphoneCoreCbsFriendListCreatedCb.
 */
typedef LinphoneCoreCbsFriendListCreatedCb LinphoneCoreFriendListCreatedCb;

/**
 * Callback prototype for reporting when a friend list has been removed from the core friends list.
 * @param[in] lc LinphoneCore object
 * @param[in] list LinphoneFriendList object
 */
typedef void (*LinphoneCoreCbsFriendListRemovedCb) (LinphoneCore *lc, LinphoneFriendList *list);

/**
 * Old name of #LinphoneCoreCbsFriendListRemovedCb.
 */
typedef LinphoneCoreCbsFriendListRemovedCb LinphoneCoreFriendListRemovedCb;

/**
 * Callback prototype for reporting the result of a version update check.
 * @param[in] lc LinphoneCore object
 * @param[in] result The result of the version update check
 * @param[in] url The url where to download the new version if the result is LinphoneVersionUpdateCheckNewVersionAvailable
 */
typedef void (*LinphoneCoreCbsVersionUpdateCheckResultReceivedCb) (LinphoneCore *lc, LinphoneVersionUpdateCheckResult result, const char *version, const char *url);

/**
 * @}
**/

/**
 * @addtogroup event_api
 * @{
**/

/**
 * Callback prototype for notifying the application about notification received from the network.
**/
typedef void (*LinphoneCoreCbsNotifyReceivedCb)(LinphoneCore *lc, LinphoneEvent *lev, const char *notified_event, const LinphoneContent *body);

/**
 * Old name of #LinphoneCoreCbsNotifyReceivedCb.
 */
typedef LinphoneCoreCbsNotifyReceivedCb LinphoneCoreNotifyReceivedCb;

/**
 * Callback prototype for notifying the application about changes of subscription states, including arrival of new subscriptions.
**/
typedef void (*LinphoneCoreCbsSubscriptionStateChangedCb)(LinphoneCore *lc, LinphoneEvent *lev, LinphoneSubscriptionState state);

/**
 * Old name of #LinphoneCoreCbsSubscriptionStateChangedCb.
 */
typedef LinphoneCoreCbsSubscriptionStateChangedCb LinphoneCoreSubscriptionStateChangedCb;

/**
 * Callback prototype for notifying the application about changes of publish states.
**/
typedef void (*LinphoneCoreCbsPublishStateChangedCb)(LinphoneCore *lc, LinphoneEvent *lev, LinphonePublishState state);

/**
 * Old name of LinphoneCoreCbsPublishStateChangedCb.
 */
typedef LinphoneCoreCbsPublishStateChangedCb LinphoneCorePublishStateChangedCb;

/**
 * @}
**/

/**
 * @addtogroup buddy_list
 * @{
 */

/**
 * Callback used to notify a new contact has been created on the CardDAV server and downloaded locally
 * @param list The LinphoneFriendList object the new contact is added to
 * @param lf The LinphoneFriend object that has been created
**/
typedef void (*LinphoneFriendListCbsContactCreatedCb)(LinphoneFriendList *list, LinphoneFriend *lf);

/**
 * Callback used to notify a contact has been deleted on the CardDAV server
 * @param list The LinphoneFriendList object a contact has been removed from
 * @param lf The LinphoneFriend object that has been deleted
**/
typedef void (*LinphoneFriendListCbsContactDeletedCb)(LinphoneFriendList *list, LinphoneFriend *lf);

/**
 * Callback used to notify a contact has been updated on the CardDAV server
 * @param list The LinphoneFriendList object in which a contact has been updated
 * @param new_friend The new LinphoneFriend object corresponding to the updated contact
 * @param old_friend The old LinphoneFriend object before update
**/
typedef void (*LinphoneFriendListCbsContactUpdatedCb)(LinphoneFriendList *list, LinphoneFriend *new_friend, LinphoneFriend *old_friend);

/**
 * Callback used to notify the status of the synchronization has changed
 * @param list The LinphoneFriendList object for which the status has changed
 * @param status The new synchronisation status
 * @param msg An additional information on the status update
**/
typedef void (*LinphoneFriendListCbsSyncStateChangedCb)(LinphoneFriendList *list, LinphoneFriendListSyncStatus status, const char *msg);

/**
 * @}
**/

/**
 * @addtogroup misc
 * @{
 */

/**
 * Callback to decrypt incoming LinphoneChatMessage
 * @param engine ImEncryptionEngine object
 * @param room LinphoneChatRoom object
 * @param msg LinphoneChatMessage object
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineCbsIncomingMessageCb)(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg);

/**
 * Callback to encrypt outgoing LinphoneChatMessage
 * @param engine LinphoneImEncryptionEngine object
 * @param room LinphoneChatRoom object
 * @param msg LinphoneChatMessage object
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineCbsOutgoingMessageCb)(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg);

/**
 * Callback to know whether or not the engine will encrypt files before uploading them
 * @param engine LinphoneImEncryptionEngine object
 * @param room LinphoneChatRoom object
 * @return TRUE if files will be encrypted, FALSE otherwise
*/
typedef bool_t (*LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb)(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room);

/**
 * Callback to generate the key used to encrypt the files before uploading them
 * Key can be stored in the LinphoneContent object inside the LinphoneChatMessage using linphone_content_set_key
 * @param engine LinphoneImEncryptionEngine object
 * @param room LinphoneChatRoom object
 * @param msg LinphoneChatMessage object
*/
typedef void (*LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb)(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg);

/**
 * Callback to decrypt downloading file
 * @param engine LinphoneImEncryptionEngine object
 * @param msg LinphoneChatMessage object
 * @param offset The current offset of the upload
 * @param[in] buffer Encrypted data buffer
 * @param[in] size Size of the encrypted data buffer and maximum size of the decrypted data buffer
 * @param[out] decrypted_buffer Buffer in which to write the decrypted data which maximum size is size
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineCbsDownloadingFileCb)(LinphoneImEncryptionEngine *engine, LinphoneChatMessage *msg, size_t offset, const uint8_t *buffer, size_t size, uint8_t *decrypted_buffer);

/**
 * Callback to encrypt uploading file
 * @param engine LinphoneImEncryptionEngine object
 * @param msg LinphoneChatMessage object
 * @param offset The current offset of the upload
 * @param[in] buffer Encrypted data buffer
 * @param[in,out] size Size of the plain data buffer and the size of the encrypted data buffer once encryption is done
 * @param[out] encrypted_buffer Buffer in which to write the encrypted data which maxmimum size is size
 * @return -1 if nothing to be done, 0 on success or an integer > 0 for error
*/
typedef int (*LinphoneImEncryptionEngineCbsUploadingFileCb)(LinphoneImEncryptionEngine *engine, LinphoneChatMessage *msg, size_t offset, const uint8_t *buffer, size_t *size, uint8_t *encrypted_buffer);

/**
 * Callback used to notify the response to an XML-RPC request.
 * @param[in] request LinphoneXmlRpcRequest object
**/
typedef void (*LinphoneXmlRpcRequestCbsResponseCb)(LinphoneXmlRpcRequest *request);

/**
 * @}
**/

/**
 * @addtogroup call_control
 * @{
 */

/**
 * Callback for notifying end of play (file).
 * @param[in] player The LinphonePlayer object
**/
typedef void (*LinphonePlayerCbsEofReachedCb)(LinphonePlayer *obj);


/**
 * @}
**/

#endif /* LINPHONE_CALLBACKS_H_ */
