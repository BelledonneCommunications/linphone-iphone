/*
linphone
Copyright (C) 2000 - 2010 Simon MORLAT (simon.morlat@linphone.org)

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

#ifndef LINPHONECORE_H
#define LINPHONECORE_H

#include "ortp/ortp.h"
#include "ortp/payloadtype.h"
#include "mediastreamer2/mscommon.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/bitratecontrol.h"

#include "linphone/defs.h"
#include "linphone/types.h"
#include "linphone/callbacks.h"
#include "linphone/sipsetup.h"

#include "linphone/account_creator.h"
#include "linphone/account_creator_service.h"
#include "linphone/address.h"
#include "linphone/auth_info.h"
#include "linphone/buffer.h"
#include "linphone/call.h"
#include "linphone/call_log.h"
#include "linphone/call_params.h"
#include "linphone/call_stats.h"
#include "linphone/chat.h"
#include "linphone/conference.h"
#include "linphone/content.h"
#include "linphone/dictionary.h"
#include "linphone/error_info.h"
#include "linphone/event.h"
#include "linphone/factory.h"
#include "linphone/friend.h"
#include "linphone/friendlist.h"
#include "linphone/im_encryption_engine.h"
#include "linphone/im_notif_policy.h"
#include "linphone/info_message.h"
#include "linphone/lpconfig.h"
#include "linphone/misc.h"
#include "linphone/nat_policy.h"
#include "linphone/payload_type.h"
#include "linphone/player.h"
#include "linphone/presence.h"
#include "linphone/proxy_config.h"
#include "linphone/ringtoneplayer.h"
#include "linphone/vcard.h"
#include "linphone/video_definition.h"
#include "linphone/xmlrpc.h"
#include "linphone/headers.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Safely down-cast a belle_sip_object_t into LinphoneCore
 * @ingroup initializing
 */
#define LINPHONE_CORE(object) BELLE_SIP_CAST(object, LinphoneCore)


/**
 * Create a #LinphoneAddress object by parsing the user supplied address, given as a string.
 * @param[in] lc #LinphoneCore object
 * @param[in] address String containing the user supplied address
 * @return The create #LinphoneAddress object
 * @ingroup linphone_address
 */
LINPHONE_PUBLIC LinphoneAddress * linphone_core_create_address(LinphoneCore *lc, const char *address);


/**
 * @addtogroup misc
 * @{
 */

/**
 * Create an independent media file player.
 * This player support WAVE and MATROSKA formats.
 * @param lc A LinphoneCore object
 * @param sound_card_name Playback sound card. If NULL, the ringer sound card set in LinphoneCore will be used
 * @param video_display_name Video display. If NULL, the video display set in LinphoneCore will be used
 * @param window_id Id of the drawing window. Depend of video out
 * @return A pointer on the new instance. NULL if faild.
 */
LINPHONE_PUBLIC LinphonePlayer *linphone_core_create_local_player(LinphoneCore *lc, const char *sound_card_name, const char *video_display_name, void *window_id);

/**
 * Creates an empty info message.
 * @param lc the LinphoneCore
 * @return a new LinphoneInfoMessage.
 *
 * The info message can later be filled with information using linphone_info_message_add_header() or linphone_info_message_set_content(),
 * and finally sent with linphone_core_send_info_message().
**/
LINPHONE_PUBLIC LinphoneInfoMessage *linphone_core_create_info_message(LinphoneCore*lc);

/**
 * Checks if a new version of the application is available.
 * @param lc LinphoneCore object
 * @param current_version The current version of the application
 */
LINPHONE_PUBLIC void linphone_core_check_for_update(LinphoneCore *lc, const char *current_version);

/**
 * @}
 */

/**
 * Get the remote address of the current call.
 * @param[in] lc LinphoneCore object.
 * @return The remote address of the current call or NULL if there is no current call.
 * @ingroup call_control
 */
LINPHONE_PUBLIC const LinphoneAddress * linphone_core_get_current_call_remote_address(LinphoneCore *lc);


/**
 * @addtogroup initializing
 * @{
**/

/**
 * Callback prototype
 * @deprecated
 * @donotwrap
 */
typedef void (*ShowInterfaceCb)(LinphoneCore *lc);

/**
 * Callback prototype
 * @deprecated
 * @donotwrap
 */
typedef void (*DisplayStatusCb)(LinphoneCore *lc, const char *message);

/**
 * Callback prototype
 * @deprecated
 * @donotwrap
 */
typedef void (*DisplayMessageCb)(LinphoneCore *lc, const char *message);

/**
 * Callback prototype
 * @deprecated
 * @donotwrap
 */
typedef void (*DisplayUrlCb)(LinphoneCore *lc, const char *message, const char *url);

/**
 * Callback prototype
 */
typedef void (*LinphoneCoreCbFunc)(LinphoneCore *lc,void * user_data);

/**
 * This structure holds all callbacks that the application should implement.
 * None is mandatory.
 * @donotwrap
**/
typedef struct _LinphoneCoreVTable{
	LinphoneCoreGlobalStateChangedCb global_state_changed; /**<Notifies global state changes*/
	LinphoneCoreRegistrationStateChangedCb registration_state_changed;/**<Notifies registration state changes*/
	LinphoneCoreCallStateChangedCb call_state_changed;/**<Notifies call state changes*/
	LinphoneCoreNotifyPresenceReceivedCb notify_presence_received; /**< Notify received presence events*/
	LinphoneCoreNotifyPresenceReceivedForUriOrTelCb notify_presence_received_for_uri_or_tel; /**< Notify received presence events*/
	LinphoneCoreNewSubscriptionRequestedCb new_subscription_requested; /**< Notify about pending presence subscription request */
	LINPHONE_DEPRECATED LinphoneCoreAuthInfoRequestedCb auth_info_requested; /** @brief Ask the application some authentication information.
																				 @deprecated Use authentication_requested instead. Deprecated since 2016-09-21 */
	LinphoneCoreAuthenticationRequestedCb authentication_requested; /**< Ask the application some authentication information */
	LinphoneCoreCallLogUpdatedCb call_log_updated; /**< Notifies that call log list has been updated */
	LinphoneCoreMessageReceivedCb message_received; /**< a message is received, can be text or external body*/
	LinphoneCoreCbsMessageReceivedUnableDecryptCb message_received_unable_decrypt; /**< an encrypted message is received but we can't decrypt it*/
	LinphoneCoreIsComposingReceivedCb is_composing_received; /**< An is-composing notification has been received */
	LinphoneCoreDtmfReceivedCb dtmf_received; /**< A dtmf has been received received */
	LinphoneCoreReferReceivedCb refer_received; /**< An out of call refer was received */
	LinphoneCoreCallEncryptionChangedCb call_encryption_changed; /**<Notifies on change in the encryption of call streams */
	LinphoneCoreTransferStateChangedCb transfer_state_changed; /**<Notifies when a transfer is in progress */
	LinphoneCoreBuddyInfoUpdatedCb buddy_info_updated; /**< a LinphoneFriend's BuddyInfo has changed*/
	LinphoneCoreCallStatsUpdatedCb call_stats_updated; /**<Notifies on refreshing of call's statistics. */
	LinphoneCoreInfoReceivedCb info_received; /**<Notifies an incoming informational message received.*/
	LinphoneCoreSubscriptionStateChangedCb subscription_state_changed; /**<Notifies subscription state change */
	LinphoneCoreNotifyReceivedCb notify_received; /**< Notifies a an event notification, see linphone_core_subscribe() */
	LinphoneCorePublishStateChangedCb publish_state_changed;/**Notifies publish state change (only from #LinphoneEvent api)*/
	LinphoneCoreConfiguringStatusCb configuring_status; /** Notifies configuring status changes */
	LINPHONE_DEPRECATED DisplayStatusCb display_status; /**< @brief Callback that notifies various events with human readable text.
															 @deprecated Deprecated since 2015-11-19. */
	LINPHONE_DEPRECATED DisplayMessageCb display_message;/**< @brief Callback to display a message to the user.
															  @deprecated Deprecated since 2015-11-19. */
	LINPHONE_DEPRECATED DisplayMessageCb display_warning;/**< @brief Callback to display a warning to the user.
															  @deprecated Deprecated since 2015-11-19. */
	LINPHONE_DEPRECATED DisplayUrlCb display_url; /**< @deprecated Deprecated since 2015-11-19. */
	LINPHONE_DEPRECATED ShowInterfaceCb show; /**< @brief Notifies the application that it should show up.
	                                               @deprecated Deprecated since 2015-11-19. */
	LINPHONE_DEPRECATED LinphoneCoreTextMessageReceivedCb text_received; /**< @brief A text message has been received.
	                                                                          @deprecated Use #message_received instead. Deprecated since 2015-11-19.  */
	LINPHONE_DEPRECATED LinphoneCoreFileTransferRecvCb file_transfer_recv; /**< @brief Callback to store file received attached to a #LinphoneChatMessage.
																				@deprecated Deprecated since 2015-11-19. */
	LINPHONE_DEPRECATED LinphoneCoreFileTransferSendCb file_transfer_send; /**< @brief Callback to collect file chunk to be sent for a #LinphoneChatMessage.
																				@deprecated Deprecated since 2015-11-19. */
	LINPHONE_DEPRECATED LinphoneCoreFileTransferProgressIndicationCb file_transfer_progress_indication; /**< @brief Callback to indicate file transfer progress.
	                                                                                                         @deprecated Deprecated since 2015-11-19. */
	LinphoneCoreNetworkReachableCb network_reachable; /**< Callback to report IP network status (I.E up/down )*/
	LinphoneCoreLogCollectionUploadStateChangedCb log_collection_upload_state_changed; /**< Callback to upload collected logs */
	LinphoneCoreLogCollectionUploadProgressIndicationCb log_collection_upload_progress_indication; /**< Callback to indicate log collection upload progress */
	LinphoneCoreFriendListCreatedCb friend_list_created;
	LinphoneCoreFriendListRemovedCb friend_list_removed;
	LinphoneCoreCbsCallCreatedCb call_created;
	LinphoneCoreCbsVersionUpdateCheckResultReceivedCb version_update_check_result_received;
	void *user_data; /**<User data associated with the above callbacks */
} LinphoneCoreVTable;

/**
 * Instantiate a vtable with all arguments set to NULL
 * @return newly allocated vtable
 */
LINPHONE_PUBLIC LinphoneCoreVTable *linphone_core_v_table_new(void);

/**
 * Sets a user data pointer in the vtable.
 * @param table the vtable
 * @param data the user data to attach
 */
LINPHONE_PUBLIC void linphone_core_v_table_set_user_data(LinphoneCoreVTable *table, void *data);

/**
 * Gets a user data pointer in the vtable.
 * @param table the vtable
 * @return the data attached to the vtable
 */
LINPHONE_PUBLIC void* linphone_core_v_table_get_user_data(const LinphoneCoreVTable *table);

/**
 * Gets the current VTable.
 * This is meant only to be called from a callback to be able to get the user_data associated with the vtable that called the callback.
 * @param lc the linphonecore
 * @return the vtable that called the last callback
 * @donotwrap
 */
LINPHONE_PUBLIC LinphoneCoreVTable *linphone_core_get_current_vtable(LinphoneCore *lc);

/**
 * Destroy a vtable.
 * @param table to be destroyed
 */
LINPHONE_PUBLIC void linphone_core_v_table_destroy(LinphoneCoreVTable* table);

/**
 * Increment the reference counter.
 */
LINPHONE_PUBLIC LinphoneCoreCbs *linphone_core_cbs_ref(LinphoneCoreCbs *cbs);

/**
 * Decrement the reference counter.
 */
LINPHONE_PUBLIC void linphone_core_cbs_unref(LinphoneCoreCbs *cbs);

/**
 * Set private data to be get from each callbacks.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_user_data(LinphoneCoreCbs *cbs, void *user_data);

/**
 * Get the user pointer.
 */
LINPHONE_PUBLIC void *linphone_core_cbs_get_user_data(const LinphoneCoreCbs *cbs);

/**
 * Gets the current #LinphoneCoreCbs.
 * This is meant only to be called from a callback to be able to get the user_data associated with the #LinphoneCoreCbs that is calling the callback.
 * @param lc the linphonecore
 * @return the #LinphoneCoreCbs that has called the last callback
 */
LINPHONE_PUBLIC LinphoneCoreCbs *linphone_core_get_current_callbacks(const LinphoneCore *lc);

/**
 * Set the #LinphoneCoreGlobalStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_global_state_changed(LinphoneCoreCbs *cbs, LinphoneCoreCbsGlobalStateChangedCb cb);

/**
 * Get the # callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsGlobalStateChangedCb linphone_core_cbs_get_global_state_changed(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsRegistrationStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_registration_state_changed(LinphoneCoreCbs *cbs, LinphoneCoreCbsRegistrationStateChangedCb cb);

/**
 * Get the #LinphoneCoreCbsRegistrationStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsRegistrationStateChangedCb linphone_core_cbs_get_registration_state_changed(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsCallStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_call_state_changed(LinphoneCoreCbs *cbs, LinphoneCoreCbsCallStateChangedCb cb);

/**
 * Get the #LinphoneCoreCbsCallStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsCallStateChangedCb linphone_core_cbs_get_call_state_changed(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsNotifyPresenceReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_notify_presence_received(LinphoneCoreCbs *cbs, LinphoneCoreCbsNotifyPresenceReceivedCb cb);

/**
 * Get the #LinphoneCoreCbsNotifyPresenceReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsNotifyPresenceReceivedCb linphone_core_cbs_get_notify_presence_received(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsNotifyPresenceReceivedForUriOrTelCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_notify_presence_received_for_uri_or_tel(LinphoneCoreCbs *cbs, LinphoneCoreCbsNotifyPresenceReceivedForUriOrTelCb cb);

/**
 * Get the #LinphoneCoreCbsNotifyPresenceReceivedForUriOrTelCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsNotifyPresenceReceivedForUriOrTelCb linphone_core_cbs_get_notify_presence_received_for_uri_or_tel(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsNewSubscriptionRequestedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_new_subscription_requested(LinphoneCoreCbs *cbs, LinphoneCoreCbsNewSubscriptionRequestedCb cb);

/**
 * Get the #LinphoneCoreCbsNewSubscriptionRequestedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsNewSubscriptionRequestedCb linphone_core_cbs_get_new_subscription_requested(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsAuthenticationRequestedCb callback.'
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_authentication_requested(LinphoneCoreCbs *cbs, LinphoneCoreCbsAuthenticationRequestedCb cb);

/**
 * Get the #LinphoneCoreCbsAuthenticationRequestedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsAuthenticationRequestedCb linphone_core_cbs_get_authentication_requested(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsCallLogUpdatedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_call_log_updated(LinphoneCoreCbs *cbs, LinphoneCoreCbsCallLogUpdatedCb cb);

/**
 * Get the #LinphoneCoreCbsCallLogUpdatedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsCallLogUpdatedCb linphone_core_cbs_get_call_log_updated(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsMessageReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_message_received(LinphoneCoreCbs *cbs, LinphoneCoreCbsMessageReceivedCb cb);

/**
 * Get the #LinphoneCoreCbsMessageReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsMessageReceivedCb linphone_core_cbs_get_message_received(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsMessageReceivedUnableDecryptCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_message_received_unable_decrypt(LinphoneCoreCbs *cbs, LinphoneCoreCbsMessageReceivedUnableDecryptCb cb);

/**
 * Get the #LinphoneCoreCbsMessageReceivedUnableDecryptCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsMessageReceivedUnableDecryptCb linphone_core_cbs_get_message_received_unable_decrypt(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsIsComposingReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_is_composing_received(LinphoneCoreCbs *cbs, LinphoneCoreCbsIsComposingReceivedCb cb);

/**
 * Get the #LinphoneCoreCbsIsComposingReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsIsComposingReceivedCb linphone_core_cbs_get_is_composing_received(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsDtmfReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_dtmf_received(LinphoneCoreCbs *cbs, LinphoneCoreCbsDtmfReceivedCb cb);

/**
 * Get the #LinphoneCoreCbsDtmfReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsDtmfReceivedCb linphone_core_cbs_get_dtmf_received(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsReferReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_refer_received(LinphoneCoreCbs *cbs, LinphoneCoreCbsReferReceivedCb cb);

/**
 * Get the #LinphoneCoreCbsReferReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsReferReceivedCb linphone_core_cbs_get_refer_received(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsCallEncryptionChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_call_encryption_changed(LinphoneCoreCbs *cbs, LinphoneCoreCbsCallEncryptionChangedCb cb);

/**
 * Get the #LinphoneCoreCbsCallEncryptionChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsCallEncryptionChangedCb linphone_core_cbs_get_call_encryption_changed(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsTransferStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_transfer_state_changed(LinphoneCoreCbs *cbs, LinphoneCoreCbsTransferStateChangedCb cb);

/**
 * Get the #LinphoneCoreCbsTransferStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsTransferStateChangedCb linphone_core_cbs_get_transfer_state_changed(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsBuddyInfoUpdatedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_buddy_info_updated(LinphoneCoreCbs *cbs, LinphoneCoreCbsBuddyInfoUpdatedCb cb);

/**
 * Get the #LinphoneCoreCbsBuddyInfoUpdatedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsBuddyInfoUpdatedCb linphone_core_cbs_get_buddy_info_updated(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsCallStatsUpdatedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_call_stats_updated(LinphoneCoreCbs *cbs, LinphoneCoreCbsCallStatsUpdatedCb cb);

/**
 * Get the #LinphoneCoreCbsCallStatsUpdatedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsCallStatsUpdatedCb linphone_core_cbs_get_call_stats_updated(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsInfoReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_info_received(LinphoneCoreCbs *cbs, LinphoneCoreCbsInfoReceivedCb cb);

/**
 * Get the #LinphoneCoreCbsInfoReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsInfoReceivedCb linphone_core_cbs_get_info_received(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsSubscriptionStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_subscription_state_changed(LinphoneCoreCbs *cbs, LinphoneCoreCbsSubscriptionStateChangedCb cb);

/**
 * Get the #LinphoneCoreCbsSubscriptionStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsSubscriptionStateChangedCb linphone_core_cbs_get_subscription_state_changed(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsNotifyReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_notify_received(LinphoneCoreCbs *cbs, LinphoneCoreCbsNotifyReceivedCb cb);

/**
 * Get the #LinphoneCoreCbsNotifyReceivedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsNotifyReceivedCb linphone_core_cbs_get_notify_received(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsPublishStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_publish_state_changed(LinphoneCoreCbs *cbs, LinphoneCoreCbsPublishStateChangedCb cb);

/**
 * Get the #LinphoneCoreCbsPublishStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsPublishStateChangedCb linphone_core_cbs_get_publish_state_changed(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsConfiguringStatusCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_configuring_status(LinphoneCoreCbs *cbs, LinphoneCoreCbsConfiguringStatusCb cb);

/**
 * Get the #LinphoneCoreCbsConfiguringStatusCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsConfiguringStatusCb linphone_core_cbs_get_configuring_status(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsNetworkReachableCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_network_reachable(LinphoneCoreCbs *cbs, LinphoneCoreCbsNetworkReachableCb cb);

/**
 * Get the #LinphoneCoreCbsNetworkReachableCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsNetworkReachableCb linphone_core_cbs_get_network_reachable(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsLogCollectionUploadStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_log_collection_upload_state_changed(LinphoneCoreCbs *cbs, LinphoneCoreCbsLogCollectionUploadStateChangedCb cb);

/**
 * Get the #LinphoneCoreCbsLogCollectionUploadStateChangedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsLogCollectionUploadStateChangedCb linphone_core_cbs_get_log_collection_upload_state_changed(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsLogCollectionUploadProgressIndicationCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_log_collection_upload_progress_indication(LinphoneCoreCbs *cbs, LinphoneCoreCbsLogCollectionUploadProgressIndicationCb cb);

/**
 * Get the #LinphoneCoreCbsLogCollectionUploadProgressIndicationCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsLogCollectionUploadProgressIndicationCb linphone_core_cbs_get_log_collection_upload_progress_indication(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsFriendListCreatedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_friend_list_created(LinphoneCoreCbs *cbs, LinphoneCoreCbsFriendListCreatedCb cb);

/**
 * Get the #LinphoneCoreCbsFriendListCreatedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsFriendListCreatedCb linphone_core_cbs_get_friend_list_created(LinphoneCoreCbs *cbs);

/**
 * Set the #LinphoneCoreCbsFriendListRemovedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @param[in] cb The callback.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_friend_list_removed(LinphoneCoreCbs *cbs, LinphoneCoreCbsFriendListRemovedCb cb);

/**
 * Get the #LinphoneCoreCbsFriendListRemovedCb callback.
 * @param[in] cbs A #LinphoneCoreCbs.
 * @return The callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsFriendListRemovedCb linphone_core_cbs_get_friend_list_removed(LinphoneCoreCbs *cbs);

/**
 * Set the call created callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The call created callback to be used.
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_call_created(LinphoneCoreCbs *cbs, LinphoneCoreCbsCallCreatedCb cb);

/**
 * Get the call created callback.
 * @param[in] cbs LinphoneCoreCbs object.
 * @return The current call created callback.
 */
LINPHONE_PUBLIC LinphoneCoreCbsCallCreatedCb linphone_core_cbs_get_call_created(LinphoneCoreCbs *cbs);

/**
 * Set the version update check result callback.
 * @param[in] cbs LinphoneCoreCbs object
 * @param[in] cb The callback to use
 */
LINPHONE_PUBLIC void linphone_core_cbs_set_version_update_check_result_received(LinphoneCoreCbs *cbs, LinphoneCoreCbsVersionUpdateCheckResultReceivedCb cb);

/**
 * Get the version update check result callback.
 * @param[in] cbs LinphoneCoreCbs object
 * @return The current callback
 */
LINPHONE_PUBLIC LinphoneCoreCbsVersionUpdateCheckResultReceivedCb linphone_core_cbs_get_version_update_check_result_received(LinphoneCoreCbs *cbs);

/**
 * @}
**/

typedef void * (*LinphoneCoreWaitingCallback)(LinphoneCore *lc, void *context, LinphoneWaitingState ws, const char *purpose, float progress);


/**
 * @addtogroup initializing
 * @{
**/

/**
 * Tells whether the linphone core log collection is enabled.
 * @return The state of the linphone core log collection.
 */
LINPHONE_PUBLIC LinphoneLogCollectionState linphone_core_log_collection_enabled(void);

/**
 * Enable the linphone core log collection to upload logs on a server.
 * @param[in] state LinphoneLogCollectionState value telling whether to enable log collection or not.
 */
LINPHONE_PUBLIC void linphone_core_enable_log_collection(LinphoneLogCollectionState state);

/**
 * Get the path where the log files will be written for log collection.
 * @return The path where the log files will be written.
 */
LINPHONE_PUBLIC const char * linphone_core_get_log_collection_path(void);

/**
 * Set the path where the log files will be written for log collection.
 * @param[in] path The path where the log files will be written.
 */
LINPHONE_PUBLIC void linphone_core_set_log_collection_path(const char *path);

/**
 * Get the prefix of the filenames that will be used for log collection.
 * @return The prefix of the filenames used for log collection.
 */
LINPHONE_PUBLIC const char * linphone_core_get_log_collection_prefix(void);

/**
 * Set the prefix of the filenames that will be used for log collection.
 * @param[in] prefix The prefix to use for the filenames for log collection.
 */
LINPHONE_PUBLIC void linphone_core_set_log_collection_prefix(const char *prefix);

/**
 * Get the max file size in bytes of the files used for log collection.
 * @return The max file size in bytes of the files used for log collection.
 */
LINPHONE_PUBLIC size_t linphone_core_get_log_collection_max_file_size(void);

/**
 * Set the max file size in bytes of the files used for log collection.
 * Warning: this function should only not be used to change size
 * dynamically but instead only before calling @see
 * linphone_core_enable_log_collection. If you increase max size
  * on runtime, logs chronological order COULD be broken.
 * @param[in] size The max file size in bytes of the files used for log collection.
 */
LINPHONE_PUBLIC void linphone_core_set_log_collection_max_file_size(size_t size);

/**
 * Set the url of the server where to upload the collected log files.
 * @param[in] core LinphoneCore object
 * @param[in] server_url The url of the server where to upload the collected log files.
 */
LINPHONE_PUBLIC void linphone_core_set_log_collection_upload_server_url(LinphoneCore *core, const char *server_url);

/**
 * Gets the url of the server where to upload the collected log files.
 * @param[in] core LinphoneCore object
 * @return The url of the server where to upload the collected log files.
 */
LINPHONE_PUBLIC const char * linphone_core_get_log_collection_upload_server_url(LinphoneCore *core);

/**
 * Upload the log collection to the configured server url.
 * @param[in] core LinphoneCore object
 */
LINPHONE_PUBLIC void linphone_core_upload_log_collection(LinphoneCore *core);

/**
 * Compress the log collection in a single file.
 * @return The path of the compressed log collection file (to be freed calling ms_free()).
 */
LINPHONE_PUBLIC char * linphone_core_compress_log_collection(void);

/**
 * Reset the log collection by removing the log files.
 */
LINPHONE_PUBLIC void linphone_core_reset_log_collection(void);

/**
 * Define a log handler.
 *
 * @param logfunc The function pointer of the log handler.
 * @deprecated Use #linphone_log_service_set_log_handler() instead. Deprecated since 2017-10-10.
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_log_handler(OrtpLogFunc logfunc);

/**
 * Define a log file.
 *
 * If the file pointer passed as an argument is NULL, stdout is used instead.
 *
 * @param file A pointer to the FILE structure of the file to write to.
 * @deprecated Use #linphone_log_service_set_file() instead. Deprecated since 2017-10-10.
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_log_file(FILE *file);

/**
 * Define the minimum level for logging.
 *
 * @param loglevel Minimum level for logging messages.
 * @deprecated Use #linphone_logging_service_set_log_level() instead. Deprecated since 2017-10-10.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_log_level(OrtpLogLevel loglevel);

/**
 * Define the log level using mask.
 *
 * The loglevel parameter is a bitmask parameter. Therefore to enable only warning and error
 * messages, use ORTP_WARNING | ORTP_ERROR. To disable logs, simply set loglevel to 0.
 *
 * @param mask A bitmask of the log levels to set.
 * @deprecated Use #linphone_logging_service_set_log_level() instead. Deprecated since 2017-10-10.
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_log_level_mask(unsigned int mask);

/**
 * Get defined log level mask.
 *
 * @return The loglevel parameter is a bitmask parameter. Therefore to enable only warning and error
 * messages, use ORTP_WARNING | ORTP_ERROR. To disable logs, simply set loglevel to 0.
 * @deprecated Use #linphone_logging_service_get_log_level_mask() instead. Deprecated since 2017-10-10.
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED unsigned int linphone_core_get_log_level_mask(void);

/**
 * Enable logs in supplied FILE*.
 * @param file a C FILE* where to fprintf logs. If null stdout is used.
 * @deprecated Use #linphone_core_set_log_file and #linphone_core_set_log_level() instead.
 * Deprecated since 2017-01-12.
 * @donotwrap
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC void linphone_core_enable_logs(FILE *file);

/**
 * Enable logs through the user's supplied log callback.
 * @param logfunc The address of a OrtpLogFunc callback whose protoype is
 *            	  typedef void (*OrtpLogFunc)(OrtpLogLevel lev, const char *fmt, va_list args);
 * @deprecated Use #linphone_core_set_log_handler and #linphone_core_set_log_level instead.
 * Deprecated since 2017-01-12.
 * @donotwrap
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC void linphone_core_enable_logs_with_cb(OrtpLogFunc logfunc);

/**
 * Entirely disable logging.
 * @deprecated Use #linphone_core_set_log_level() instead. Deprecated since 2017-01-12.
 * @donotwrap
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC void linphone_core_disable_logs(void);

/**
 * Enable logs serialization (output logs from either the thread that creates the linphone core or the thread that calls linphone_core_iterate()).
 * Must be called before creating the linphone core.
 */
LINPHONE_PUBLIC void linphone_core_serialize_logs(void);

/**
 * Returns liblinphone's version as a string.
**/
LINPHONE_PUBLIC const char *linphone_core_get_version(void);

/**
 * @return liblinphone's user agent as a string.
**/
LINPHONE_PUBLIC const char *linphone_core_get_user_agent(LinphoneCore *lc);

/**
 * @deprecated Use #linphone_core_get_user_agent() instead.
 * Deprecated since 2015-11-19.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED const char *linphone_core_get_user_agent_name(void);

/**
 * @deprecated Use #linphone_core_get_user_agent instead.
 * Deprecated since 2015-11-19.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED const char *linphone_core_get_user_agent_version(void);

/**
 * @}
**/

/**
 * Instanciates a LinphoneCore object.
 * @ingroup initializing
 *
 * The LinphoneCore object is the primary handle for doing all phone actions.
 * It should be unique within your application.
 * @param vtable a LinphoneCoreVTable structure holding your application callbacks
 * @param config_path a path to a config file. If it does not exists it will be created.
 *        The config file is used to store all settings, call logs, friends, proxies... so that all these settings
 *	       become persistent over the life of the LinphoneCore object.
 *	       It is allowed to set a NULL config file. In that case LinphoneCore will not store any settings.
 * @param factory_config_path a path to a read-only config file that can be used to
 *        to store hard-coded preference such as proxy settings or internal preferences.
 *        The settings in this factory file always override the one in the normal config file.
 *        It is OPTIONAL, use NULL if unneeded.
 * @param userdata an opaque user pointer that can be retrieved at any time (for example in
 *        callbacks) using linphone_core_get_user_data().
 * @see linphone_core_new_with_config
 * @deprecated Use #linphone_factory_create_core() instead. Deprecated since 2017-01-12.
 * @donotwrap
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC LinphoneCore *linphone_core_new(const LinphoneCoreVTable *vtable,
						const char *config_path, const char *factory_config_path, void* userdata);

/**
 * Instantiates a LinphoneCore object with a given LpConfig.
 * @ingroup initializing
 *
 * The LinphoneCore object is the primary handle for doing all phone actions.
 * It should be unique within your application.
 * @param vtable a LinphoneCoreVTable structure holding your application callbacks
 * @param config a pointer to an LpConfig object holding the configuration of the LinphoneCore to be instantiated.
 * @param userdata an opaque user pointer that can be retrieved at any time (for example in
 *        callbacks) using linphone_core_get_user_data().
 * @see linphone_core_new
 * @deprecated Use #linphone_factory_create_core_with_config() instead. Deprecated since 2017-01-12.
 * @donotwrap
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC LinphoneCore *linphone_core_new_with_config(const LinphoneCoreVTable *vtable, LpConfig *config, void *userdata);

/**
 * Increment the reference counter of a #LinphoneCore object.
 * @param lc The #LinphoneCore which the ref counter is to be incremented.
 * @return A pointer on the #LinphoneCore passed as parameter.
 * @ingroup initializing
 */
LINPHONE_PUBLIC LinphoneCore *linphone_core_ref(LinphoneCore *lc);

/**
 * Decrement the ref counter of a #LinphoneCore object and destroy it
 * if the counter reach 0.
 * @param lc The #LinphoneCore which the reference counter is to be decreased.
 * @ingroup initializing
 */
LINPHONE_PUBLIC void linphone_core_unref(LinphoneCore *lc);

/**
 * Main loop function. It is crucial that your application call it periodically.
 *
 * linphone_core_iterate() performs various backgrounds tasks:
 * - receiving of SIP messages
 * - handles timers and timeout
 * - performs registration to proxies
 * - authentication retries
 * The application MUST call this function periodically, in its main loop.
 * Be careful that this function must be called from the same thread as
 * other liblinphone methods. If it is not the case make sure all liblinphone calls are
 * serialized with a mutex.
 * For ICE to work properly it should be called every 20ms.
 * @param[in] lc LinphoneCore object
 * @ingroup initializing
**/
LINPHONE_PUBLIC void linphone_core_iterate(LinphoneCore *lc);

/**
 * @ingroup initializing
 * add a listener to be notified of linphone core events. Once events are received, registered vtable are invoked in order.
 * @param vtable a LinphoneCoreVTable structure holding your application callbacks. Object is owned by linphone core until linphone_core_remove_listener.
 * @param lc object
 * @deprecated Use linphone_core_add_callbacks() instead. Deprecated since 2017-01-12.
 * @donotwrap
 */
LINPHONE_DEPRECATED LINPHONE_PUBLIC void linphone_core_add_listener(LinphoneCore *lc, LinphoneCoreVTable *vtable);

/**
 * @ingroup initializing
 * Add a listener in order to be notified of #LinphoneCore events. Once an event is received, registred #LinphoneCoreCbs are
 * invoked sequencially.
 * @param lc The #LinphoneCore object to monitor.
 * @param cbs A #LinphoneCoreCbs object holding the callbacks you need. A reference is taken by #LinphoneCore until you invoke linphone_core_remove_callbacks().
 */
LINPHONE_PUBLIC void linphone_core_add_callbacks(LinphoneCore *lc, LinphoneCoreCbs *cbs);

/**
 * @ingroup initializing
 * remove a listener registred by linphone_core_add_listener.
 * @param lc object
 * @param vtable a LinphoneCoreVTable structure holding your application callbacks.
 * @deprecated Use linphone_core_remove_callbacks() instead. Deprecated since 2017-01-12.
 * @donotwrap
 */
LINPHONE_DEPRECATED LINPHONE_PUBLIC void linphone_core_remove_listener(LinphoneCore *lc, const LinphoneCoreVTable *vtable);

/**
 * @ingroup initializing
 * Remove a listener from a #LinphoneCore
 * @param lc The #LinphoneCore
 * @param cbs The pointer on the #LinphoneCoreCbs to remove.
 */
LINPHONE_PUBLIC void linphone_core_remove_callbacks(LinphoneCore *lc, const LinphoneCoreCbs *cbs);

/**
 * Sets the user agent string used in SIP messages, ideally called just after linphone_core_new() or linphone_core_init().
 * @param[in] lc LinphoneCore object
 * @param[in] ua_name The user agent name
 * @param[in] version The user agent version
 * @ingroup misc
**/
LINPHONE_PUBLIC void linphone_core_set_user_agent(LinphoneCore *lc, const char *ua_name, const char *version);

/**
 * See linphone_proxy_config_normalize_sip_uri for documentation. Default proxy config is used to parse
 * the address.
 * @ingroup misc
 */
LINPHONE_PUBLIC LinphoneAddress * linphone_core_interpret_url(LinphoneCore *lc, const char *url);

/**
 * @brief Initiates an outgoing call.
 *
 * The application doesn't own a reference to the returned LinphoneCall object.
 * Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
 *
 * @param[in] lc LinphoneCore object
 * @param[in] url The destination of the call (sip address, or phone number).
 * @return A LinphoneCall object or NULL in case of failure
 * @ingroup call_control
**/
LINPHONE_PUBLIC LinphoneCall * linphone_core_invite(LinphoneCore *lc, const char *url);

/**
 * Initiates an outgoing call given a destination LinphoneAddress
 * The LinphoneAddress can be constructed directly using linphone_address_new(), or
 * created by linphone_core_interpret_url().
 * The application doesn't own a reference to the returned LinphoneCall object.
 * Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
 * @param[in] lc LinphoneCore object
 * @param[in] addr The destination of the call (sip address).
 * @return A LinphoneCall object or NULL in case of failure
 * @ingroup call_control
**/
LINPHONE_PUBLIC LinphoneCall * linphone_core_invite_address(LinphoneCore *lc, const LinphoneAddress *addr);

/**
 * Initiates an outgoing call according to supplied call parameters
 * The application doesn't own a reference to the returned LinphoneCall object.
 * Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
 * @param[in] lc LinphoneCore object
 * @param[in] url The destination of the call (sip address, or phone number).
 * @param[in] params Call parameters
 * @return A LinphoneCall object or NULL in case of failure
 * @ingroup call_control
**/
LINPHONE_PUBLIC LinphoneCall * linphone_core_invite_with_params(LinphoneCore *lc, const char *url, const LinphoneCallParams *params);

/**
 * Initiates an outgoing call given a destination LinphoneAddress
 * The LinphoneAddress can be constructed directly using linphone_address_new(), or
 * created by linphone_core_interpret_url().
 * The application doesn't own a reference to the returned LinphoneCall object.
 * Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
 * @param[in] lc LinphoneCore object
 * @param[in] addr The destination of the call (sip address).
 * @param[in] params Call parameters
 * @return A LinphoneCall object or NULL in case of failure
 * @ingroup call_control
**/
LINPHONE_PUBLIC LinphoneCall * linphone_core_invite_address_with_params(LinphoneCore *lc, const LinphoneAddress *addr, const LinphoneCallParams *params);

/**
 * @brief Performs a simple call transfer to the specified destination.
 *
 * The remote endpoint is expected to issue a new call to the specified destination.
 * The current call remains active and thus can be later paused or terminated.
 * It is possible to follow the progress of the transfer provided that transferee sends notification about it.
 * In this case, the transfer_state_changed callback of the #LinphoneCoreVTable is invoked to notify of the state of the new call at the other party.
 * The notified states are #LinphoneCallOutgoingInit , #LinphoneCallOutgoingProgress, #LinphoneCallOutgoingRinging and #LinphoneCallConnected.
 * @param[in] lc LinphoneCore object
 * @param[in] call The call to be transfered
 * @param[in] refer_to The destination the call is to be refered to
 * @return 0 on success, -1 on failure
 * @ingroup call_control
 * @deprecated Use #linphone_call_transfer() instead. Deprecated since 2017-02-13.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_transfer_call(LinphoneCore *lc, LinphoneCall *call, const char *refer_to);

/**
 * @brief Transfers a call to destination of another running call. This is used for "attended transfer" scenarios.
 *
 * The transfered call is supposed to be in paused state, so that it is able to accept the transfer immediately.
 * The destination call is a call previously established to introduce the transfered person.
 * This method will send a transfer request to the transfered person. The phone of the transfered is then
 * expected to automatically call to the destination of the transfer. The receiver of the transfer will then automatically
 * close the call with us (the 'dest' call).
 * It is possible to follow the progress of the transfer provided that transferee sends notification about it.
 * In this case, the transfer_state_changed callback of the #LinphoneCoreVTable is invoked to notify of the state of the new call at the other party.
 * The notified states are #LinphoneCallOutgoingInit , #LinphoneCallOutgoingProgress, #LinphoneCallOutgoingRinging and #LinphoneCallConnected.
 * @param[in] lc LinphoneCore object
 * @param[in] call A running call you want to transfer
 * @param[in] dest A running call whose remote person will receive the transfer
 * @return 0 on success, -1 on failure
 * @ingroup call_control
 * @deprecated Use #linphone_call_transfer_to_another() instead. Deprecated since 2017-02-13.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_transfer_call_to_another(LinphoneCore *lc, LinphoneCall *call, LinphoneCall *dest);

/**
 * @brief Start a new call as a consequence of a transfer request received from a call.
 *
 * This function is for advanced usage: the execution of transfers is automatically managed by the LinphoneCore. However if an application
 * wants to have control over the call parameters for the new call, it should call this function immediately during the LinphoneCallRefered notification.
 * @see LinphoneCoreVTable::call_state_changed
 * @param[in] lc LinphoneCore object
 * @param[in] call A call that has just been notified about LinphoneCallRefered state event.
 * @param[in] params The call parameters to be applied to the new call.
 * @return A LinphoneCall corresponding to the new call that is attempted to the transfer destination.
**/
LINPHONE_PUBLIC LinphoneCall * linphone_core_start_refered_call(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params);

/** @deprecated Use linphone_core_is_incoming_invite_pending() instead. */
#define linphone_core_inc_invite_pending(lc) linphone_core_is_incoming_invite_pending(lc)

/**
 * @brief Tells whether there is an incoming invite pending.
 *
 * @ingroup call_control
 * @param[in] lc LinphoneCore object
 * @return A boolean telling whether an incoming invite is pending or not.
 */
LINPHONE_PUBLIC bool_t linphone_core_is_incoming_invite_pending(LinphoneCore*lc);

/**
 * Tells whether there is a call running.
 * @param[in] lc LinphoneCore object
 * @return A boolean value telling whether a call is currently running or not
 * @ingroup call_control
**/
LINPHONE_PUBLIC bool_t linphone_core_in_call(const LinphoneCore *lc);

/**
 * Gets the current call.
 * @param[in] lc LinphoneCore object
 * @return The current call or NULL if no call is running
 * @ingroup call_control
**/
LINPHONE_PUBLIC LinphoneCall *linphone_core_get_current_call(const LinphoneCore *lc);

/**
 * @brief Accept an incoming call.
 *
 * Basically the application is notified of incoming calls within the
 * call_state_changed callback of the #LinphoneCoreVTable structure, where it will receive
 * a #LinphoneCallIncoming event with the associated #LinphoneCall object.
 * The application can later accept the call using this method.
 * @param[in] lc #LinphoneCore object.
 * @param[in] call The #LinphoneCall object representing the call to be answered.
 * @return 0 on success, -1 on failure.
 * @ingroup call_control
 * @deprecated Use #linphone_call_accept() instead. Deprecated since 2017-02-13.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_accept_call(LinphoneCore *lc, LinphoneCall *call);

/**
 * @brief Accept an incoming call, with parameters.
 *
 * Basically the application is notified of incoming calls within the
 * call_state_changed callback of the #LinphoneCoreVTable structure, where it will receive
 * a #LinphoneCallIncoming event with the associated #LinphoneCall object.
 * The application can later accept the call using this method.
 * @param[in] lc #LinphoneCore object.
 * @param[in] call The #LinphoneCall object representing the call to be answered.
 * @param[in] params The specific parameters for this call, for example whether video is accepted or not. Use NULL to use default parameters.
 * @return 0 on success, -1 on failure
 * @ingroup call_control
 * @deprecated Use #linphone_call_accept_with_params() instead. Deprecated since 2017-02-13.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_accept_call_with_params(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params);

/**
 * @brief When receiving an incoming, accept to start a media session as early-media.
 *
 * This means the call is not accepted but audio & video streams can be established if the remote party supports early media.
 * However, unlike after call acceptance, mic and camera input are not sent during early-media, though received audio & video are played normally.
 * The call can then later be fully accepted using linphone_core_accept_call() or linphone_core_accept_call_with_params().
 * @param[in] lc LinphoneCore object
 * @param[in] call The call to accept
 * @param[in] params The call parameters to use (can be NULL)
 * @return 0 if successful, -1 otherwise
 * @ingroup call_control
 * @deprecated Use linphone_call_accept_early_media_with_params() instead.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_accept_early_media_with_params(LinphoneCore* lc, LinphoneCall* call, const LinphoneCallParams* params);

/**
 * @brief Accept an early media session for an incoming call.
 *
 * This is identical as calling linphone_core_accept_early_media_with_params() with NULL call parameters.
 * @param[in] lc LinphoneCore object
 * @param[in] call The incoming call to accept
 * @return 0 if successful, -1 otherwise
 * @ingroup call_control
 * @see linphone_core_accept_early_media_with_params()
 * @deprecated Use #linphone_call_accept_early_media() instead. Deprecated since 2017-02-13.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_accept_early_media(LinphoneCore* lc, LinphoneCall* call);

/**
 * @brief Terminates a call.
 *
 * @param[in] lc LinphoneCore object
 * @param[in] call The LinphoneCall object representing the call to be terminated
 * @return 0 on success, -1 on failure
 * @ingroup call_control
 * @deprecated Use #linphone_call_terminate() instead. Deprecated since 2017-02-13.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_terminate_call(LinphoneCore *lc, LinphoneCall *call);

/**
 * Redirect the specified call to the given redirect URI.
 * @param[in] lc #LinphoneCore object
 * @param[in] call The #LinphoneCall to redirect
 * @param[in] redirect_uri The URI to redirect the call to
 * @return 0 if successful, -1 on error.
 * @ingroup call_control
 * @deprecated Use #linphone_call_redirect() instead. Deprecated since 2017-02-13.
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_redirect_call(LinphoneCore *lc, LinphoneCall *call, const char *redirect_uri);

/**
 * @brief Decline a pending incoming call, with a reason.
 * @param[in] lc #LinphoneCore object
 * @param[in] call The #LinphoneCall to decline, must be in the IncomingReceived state
 * @param[in] reason The reason for rejecting the call: #LinphoneReasonDeclined or #LinphoneReasonBusy
 * @return 0 on success, -1 on failure
 * @ingroup call_control
 * @deprecated Use #linphone_call_decline() instead. Deprecated since 2017-02-13.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_decline_call(LinphoneCore *lc, LinphoneCall * call, LinphoneReason reason);

/**
 * Terminates all the calls.
 * @param[in] lc LinphoneCore object
 * @return 0
 * @ingroup call_control
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_terminate_all_calls(LinphoneCore *lc);

/**
 * @biref Pauses the call. If a music file has been setup using linphone_core_set_play_file(),
 * this file will be played to the remote user.
 *
 * The only way to resume a paused call is to call linphone_core_resume_call().
 * @param[in] lc LinphoneCore object
 * @param[in] call The call to pause
 * @return 0 on success, -1 on failure
 * @ingroup call_control
 * @see linphone_core_resume_call()
 * @deprecated Use #linphone_call_pause() instead. Deprecated since 2017-02-13.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_pause_call(LinphoneCore *lc, LinphoneCall *call);

/**
 * Pause all currently running calls.
 * @param[in] lc LinphoneCore object
 * @return 0
 * @ingroup call_control
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_pause_all_calls(LinphoneCore *lc);

/**
 * @brief Resumes a call.
 *
 * The call needs to have been paused previously with linphone_core_pause_call().
 * @param[in] lc LinphoneCore object
 * @param[in] call The call to resume
 * @return 0 on success, -1 on failure
 * @ingroup call_control
 * @see linphone_core_pause_call()
 * @deprecated Use #linphone_call_resume() instead. Deprecated since 2017-02-13.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_resume_call(LinphoneCore *lc, LinphoneCall *call);

/**
 * @brief Updates a running call according to supplied call parameters or parameters changed in the LinphoneCore.
 *
 * In this version this is limited to the following use cases:
 * - setting up/down the video stream according to the video parameter of the LinphoneCallParams (see linphone_call_params_enable_video() ).
 * - changing the size of the transmitted video after calling linphone_core_set_preferred_video_size()
 * In case no changes are requested through the LinphoneCallParams argument, then this argument can be omitted and set to NULL.
 * WARNING: Updating a call in the LinphoneCallPaused state will still result in a paused call even if the media directions set in the
 * params are sendrecv. To resume a paused call, you need to call linphone_core_resume_call().
 *
 * @param[in] lc LinphoneCore object
 * @param[in] call The call to be updated
 * @param[in] params The new call parameters to use (may be NULL)
 * @return 0 if successful, -1 otherwise.
 * @ingroup call_control
 * @deprecated Use #linphone_call_update() instead. Deprecated since 2017-02-13.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_update_call(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params);

/**
 * When receiving a #LinphoneCallUpdatedByRemote state notification, prevent LinphoneCore from performing an automatic answer.
 *
 * When receiving a #LinphoneCallUpdatedByRemote state notification (ie an incoming reINVITE), the default behaviour of
 * LinphoneCore is defined by the "defer_update_default" option of the "sip" section of the config. If this option is 0 (the default)
 * then the LinphoneCore automatically answers the reINIVTE with call parameters unchanged.
 * However when for example when the remote party updated the call to propose a video stream, it can be useful
 * to prompt the user before answering. This can be achieved by calling linphone_core_defer_call_update() during
 * the call state notification, to deactivate the automatic answer that would just confirm the audio but reject the video.
 * Then, when the user responds to dialog prompt, it becomes possible to call linphone_core_accept_call_update() to answer
 * the reINVITE, with eventually video enabled in the LinphoneCallParams argument.
 *
 * The #LinphoneCallUpdatedByRemote notification can also arrive when receiving an INVITE without SDP. In such case, an unchanged offer is made
 * in the 200Ok, and when the ACK containing the SDP answer is received, #LinphoneCallUpdatedByRemote is triggered to notify the application of possible
 * changes in the media session. However in such case defering the update has no meaning since we just generating an offer.
 *
 * @param[in] lc LinphoneCore object
 * @param[in] call The call for which to defer the update
 * @return 0 if successful, -1 if the linphone_core_defer_call_update() was done outside a valid #LinphoneCallUpdatedByRemote notification
 * @ingroup call_control
 * @deprecated Use linphone_call_defer_update() instead
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_defer_call_update(LinphoneCore *lc, LinphoneCall *call);

/**
 * @brief Accept call modifications initiated by other end.
 *
 * This call may be performed in response to a #LinphoneCallUpdatedByRemote state notification.
 * When such notification arrives, the application can decide to call #linphone_core_defer_call_update() so that it can
 * have the time to prompt the user. #linphone_call_get_remote_params() can be used to get information about the call parameters
 * requested by the other party, such as whether a video stream is requested.
 *
 * When the user accepts or refuse the change, #linphone_core_accept_call_update() can be done to answer to the other party.
 * If params is NULL, then the same call parameters established before the update request will continue to be used (no change).
 * If params is not NULL, then the update will be accepted according to the parameters passed.
 * Typical example is when a user accepts to start video, then params should indicate that video stream should be used
 * (see #linphone_call_params_enable_video()).
 * @param[in] lc #LinphoneCore object
 * @param[in] call The call for which to accept an update
 * @param[in] params A #LinphoneCallParams object describing the call parameters to accept
 * @return 0 if successful, -1 otherwise (actually when this function call is performed outside of #LinphoneCallUpdatedByRemote state)
 * @ingroup call_control
 * @deprecated Use #linphone_call_accept_update() instead. Deprecated since 2017-02-13.
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_accept_call_update(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params);

/**
 * Create a LinphoneCallParams suitable for linphone_core_invite_with_params(), linphone_core_accept_call_with_params(), linphone_core_accept_early_media_with_params(),
 * linphone_core_accept_call_update().
 * The parameters are initialized according to the current LinphoneCore configuration and the current state of the LinphoneCall.
 * @param[in] lc LinphoneCore object
 * @param[in] call LinphoneCall for which the parameters are to be build, or NULL in the case where the parameters are to be used for a new outgoing call.
 * @return A new LinphoneCallParams object
 * @ingroup call_control
 */
LINPHONE_PUBLIC LinphoneCallParams *linphone_core_create_call_params(LinphoneCore *lc, LinphoneCall *call);

/**
 * Get the call with the remote_address specified
 * @param[in] lc LinphoneCore object
 * @param[in] remote_address The remote address of the call that we want to get
 * @return The call if it has been found, NULL otherwise
 * @ingroup call_control
 */
LINPHONE_PUBLIC LinphoneCall *linphone_core_get_call_by_remote_address(const LinphoneCore *lc, const char *remote_address);

/**
 * Get the call with the remote_address specified
 * @param lc
 * @param remote_address
 * @return the LinphoneCall of the call if found
 *
 * @ingroup call_control
 */
LINPHONE_PUBLIC LinphoneCall *linphone_core_get_call_by_remote_address2(const LinphoneCore *lc, const LinphoneAddress *remote_address);


/**
 * @brief Send the specified dtmf.
 *
 * This function only works during calls. The dtmf is automatically played to the user.
 * @param lc The LinphoneCore object
 * @param dtmf The dtmf name specified as a char, such as '0', '#' etc...
 * @deprecated Use #linphone_call_send_dtmf instead. Deprecated since 2015-11-23.
 * @ingroup media_parameters
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_send_dtmf(LinphoneCore *lc, char dtmf);

/**
 * Sets the local "from" identity.
 *
 * @ingroup proxies
 * This data is used in absence of any proxy configuration or when no
 * default proxy configuration is set. See LinphoneProxyConfig
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_primary_contact(LinphoneCore *lc, const char *contact);

/**
 * Returns the default identity when no proxy configuration is used.
 *
 * @ingroup proxies
**/
LINPHONE_PUBLIC const char *linphone_core_get_primary_contact(LinphoneCore *lc);

/**
 * Gets the default identity SIP address.
 * This is an helper function.
 * If no default proxy is set, this will return the primary contact (
 * see linphone_core_get_primary_contact() ). If a default proxy is set
 * it returns the registered identity on the proxy.
 * @param[in] lc LinphoneCore object
 * @return The default identity SIP address
 * @ingroup proxies
**/
LINPHONE_PUBLIC const char * linphone_core_get_identity(LinphoneCore *lc);

/**
 * Tells LinphoneCore to guess local hostname automatically in primary contact.
 * @ingroup proxies
**/
LINPHONE_PUBLIC void linphone_core_set_guess_hostname(LinphoneCore *lc, bool_t val);

/**
 * Returns TRUE if hostname part of primary contact is guessed automatically.
 * @ingroup proxies
**/
LINPHONE_PUBLIC bool_t linphone_core_get_guess_hostname(LinphoneCore *lc);

/**
 * Tells to LinphoneCore to use Linphone Instant Messaging encryption
 * @param[in] lc LinphoneCore object
 * @param[in] val The new lime state
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_enable_lime(LinphoneCore *lc, LinphoneLimeState val);


/**
 * Returns the lime state
 * @param[in] lc LinphoneCore object
 * @return The current lime state
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC LinphoneLimeState linphone_core_lime_enabled(const LinphoneCore *lc);


/**
 * Tells if lime is available
 * @param[in] lc LinphoneCore object
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_lime_available(const LinphoneCore *lc);

/**
 * Tells whether IPv6 is enabled or not.
 * @param[in] lc LinphoneCore object
 * @return A boolean value telling whether IPv6 is enabled or not
 * @ingroup network_parameters
 * @see linphone_core_enable_ipv6() for more details on how IPv6 is supported in liblinphone.
**/
LINPHONE_PUBLIC bool_t linphone_core_ipv6_enabled(LinphoneCore *lc);

/**
 * Turns IPv6 support on or off.
 * @param[in] lc LinphoneCore object
 * @param[in] val A boolean value telling whether to enable IPv6 support
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_ipv6(LinphoneCore *lc, bool_t val);

/**
 * Same as linphone_core_get_primary_contact() but the result is a LinphoneAddress object
 * instead of const char*
 *
 * @ingroup proxies
**/
LINPHONE_PUBLIC LinphoneAddress *linphone_core_get_primary_contact_parsed(LinphoneCore *lc);

LINPHONE_PUBLIC const char * linphone_core_get_identity(LinphoneCore *lc);

/**
 * Sets maximum available download bandwidth
 * This is IP bandwidth, in kbit/s.
 * This information is used signaled to other parties during
 * calls (within SDP messages) so that the remote end can have
 * sufficient knowledge to properly configure its audio & video
 * codec output bitrate to not overflow available bandwidth.
 *
 * @ingroup media_parameters
 *
 * @param lc the LinphoneCore object
 * @param bw the bandwidth in kbits/s, 0 for infinite
 */
LINPHONE_PUBLIC void linphone_core_set_download_bandwidth(LinphoneCore *lc, int bw);

/**
 * Sets maximum available upload bandwidth
 * This is IP bandwidth, in kbit/s.
 * This information is used by liblinphone together with remote
 * side available bandwidth signaled in SDP messages to properly
 * configure audio & video codec's output bitrate.
 *
 * @param lc the LinphoneCore object
 * @param bw the bandwidth in kbits/s, 0 for infinite
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_upload_bandwidth(LinphoneCore *lc, int bw);

/**
 * Sets expected available upload bandwidth
 * This is IP bandwidth, in kbit/s.
 * This information is used by liblinphone together with remote
 * side available bandwidth signaled in SDP messages to properly
 * configure audio & video codec's output bitrate.
 *
 * @param lc the LinphoneCore object
 * @param bw the bandwidth in kbits/s, 0 for infinite
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_expected_bandwidth(LinphoneCore *lc, int bw);

/**
 * Retrieve the maximum available download bandwidth.
 * This value was set by linphone_core_set_download_bandwidth().
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_download_bandwidth(const LinphoneCore *lc);

/**
 * Retrieve the maximum available upload bandwidth.
 * This value was set by linphone_core_set_upload_bandwidth().
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_upload_bandwidth(const LinphoneCore *lc);

/**
 * Enable adaptive rate control.
 *
 * Adaptive rate control consists in using RTCP feedback provided information to dynamically
 * control the output bitrate of the audio and video encoders, so that we can adapt to the network conditions and
 * available bandwidth. Control of the audio encoder is done in case of audio-only call, and control of the video encoder is done for audio & video calls.
 * Adaptive rate control feature is enabled by default.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_adaptive_rate_control(LinphoneCore *lc, bool_t enabled);

/**
 * Returns whether adaptive rate control is enabled.
 * @see linphone_core_enable_adaptive_rate_control()
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_adaptive_rate_control_enabled(const LinphoneCore *lc);

/**
 * Sets adaptive rate algorithm. It will be used for each new calls starting from
 * now. Calls already started will not be updated.
 * @param lc the core
 * @param algorithm the adaptive rate control algorithm. Currently two values are supported: 'advanced', which is the default value, or 'basic'.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_adaptive_rate_algorithm(LinphoneCore *lc, const char *algorithm);

/**
 * Returns which adaptive rate algorithm is currently configured for future calls.
 * @see linphone_core_set_adaptive_rate_algorithm()
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char* linphone_core_get_adaptive_rate_algorithm(const LinphoneCore *lc);

/**
 * Set audio packetization time linphone expects to receive from peer.
 * A value of zero means that ptime is not specified.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_download_ptime(LinphoneCore *lc, int ptime);

/**
 * Get audio packetization time linphone expects to receive from peer.
 * A value of zero means that ptime is not specified.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC int  linphone_core_get_download_ptime(LinphoneCore *lc);

/**
 * Set audio packetization time linphone will send (in absence of requirement from peer)
 * A value of 0 stands for the current codec default packetization time.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_upload_ptime(LinphoneCore *lc, int ptime);

/**
 * Set audio packetization time linphone will send (in absence of requirement from peer)
 * A value of 0 stands for the current codec default packetization time.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_upload_ptime(LinphoneCore *lc);

/**
 * Set the SIP transport timeout.
 * @param[in] lc #LinphoneCore object.
 * @param[in] timeout_ms The SIP transport timeout in milliseconds.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_sip_transport_timeout(LinphoneCore *lc, int timeout_ms);

/**
 * Get the SIP transport timeout.
 * @param[in] lc #LinphoneCore object.
 * @return The SIP transport timeout in milliseconds.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC int linphone_core_get_sip_transport_timeout(LinphoneCore *lc);

/**
 * Enable or disable DNS SRV resolution.
 * @param[in] lc #LinphoneCore object.
 * @param[in] enable TRUE to enable DNS SRV resolution, FALSE to disable it.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_enable_dns_srv(LinphoneCore *lc, bool_t enable);

/**
 * Tells whether DNS SRV resolution is enabled.
 * @param[in] lc #LinphoneCore object.
 * @return TRUE if DNS SRV resolution is enabled, FALSE if disabled.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_dns_srv_enabled(const LinphoneCore *lc);

/**
 * Enable or disable DNS search (use of local domain if the fully qualified name did return results).
 * @param[in] lc #LinphoneCore object.
 * @param[in] enable TRUE to enable DNS search, FALSE to disable it.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_enable_dns_search(LinphoneCore *lc, bool_t enable);

/**
 * Tells whether DNS search (use of local domain if the fully qualified name did return results) is enabled.
 * @param[in] lc #LinphoneCore object.
 * @return TRUE if DNS search is enabled, FALSE if disabled.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_dns_search_enabled(const LinphoneCore *lc);

/**
 * Tells if the DNS was set by an application
 * @param[in] lc #LinphoneCore object.
 * @return TRUE if DNS was set by app, FALSE otherwise.
 *@ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_get_dns_set_by_app(LinphoneCore *lc);

/**
 * Forces liblinphone to use the supplied list of dns servers, instead of system's ones
 * and set dns_set_by_app at true or false according to value of servers list.
 * @param[in] lc #LinphoneCore object.
 * @param[in] servers \bctbx_list{const char *} A list of strings containing the IP addresses of DNS servers to be used.
 * Setting to NULL restores default behaviour, which is to use the DNS server list provided by the system.
 * The list is copied internally.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_dns_servers_app(LinphoneCore *lc, const bctbx_list_t *servers);

/**
 * Forces liblinphone to use the supplied list of dns servers, instead of system's ones.
 * @param[in] lc #LinphoneCore object.
 * @param[in] servers \bctbx_list{const char *} A list of strings containing the IP addresses of DNS servers to be used.
 * Setting to NULL restores default behaviour, which is to use the DNS server list provided by the system.
 * The list is copied internally.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_dns_servers(LinphoneCore *lc, const bctbx_list_t *servers);

/**
 * Return the list of the available audio payload types.
 * @param[in] lc The core.
 * @return \bctbx_list{LinphonePayloadType} A freshly allocated list of the available payload types. The list
 * must be destroyed with bctbx_list_free() after usage. The elements of the list haven't to be unref.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_core_get_audio_payload_types(LinphoneCore *lc);

/**
 * Redefine the list of the available payload types.
 * @param[in] lc The core.
 * @param[in] payload_types \bctbx_list{LinphonePayloadType} The new list of payload types. The core does not take
 * ownership on it.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_audio_payload_types(LinphoneCore *lc, const bctbx_list_t *payload_types);

/**
 * Returns the list of available audio codecs.
 * @param[in] lc The LinphoneCore object
 * @return \bctbx_list{OrtpPayloadType}
 *
 * This list is unmodifiable. The ->data field of the bctbx_list_t points a PayloadType
 * structure holding the codec information.
 * It is possible to make copy of the list with bctbx_list_copy() in order to modify it
 * (such as the order of codecs).
 * @ingroup media_parameters
 * @deprecated Use linphone_core_get_audio_payload_types() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED const bctbx_list_t *linphone_core_get_audio_codecs(const LinphoneCore *lc);

/**
 * Sets the list of audio codecs.
 * @param[in] lc The LinphoneCore object
 * @param[in] codecs \bctbx_list{OrtpPayloadType}
 * @return 0
 * The list is taken by the LinphoneCore thus the application should not free it.
 * This list is made of struct PayloadType describing the codec parameters.
 * @ingroup media_parameters
 * @deprecated Use linphone_core_set_audio_payload_types() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_set_audio_codecs(LinphoneCore *lc, bctbx_list_t *codecs);

/**
 * Return the list of the available video payload types.
 * @param[in] lc The core.
 * @return \bctbx_list{LinphonePayloadType} A freshly allocated list of the available payload types. The list
 * must be destroyed with bctbx_list_free() after usage. The elements of the list haven't to be unref.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_core_get_video_payload_types(LinphoneCore *lc);

/**
 * Redefine the list of the available video payload types.
 * @param[in] lc The core.
 * @param[in] payload_types \bctbx_list{LinphonePayloadType} The new list of codecs. The core does not take
 * ownership on it.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_video_payload_types(LinphoneCore *lc, const bctbx_list_t *payload_types);

/**
 * Returns the list of available video codecs.
 * @param[in] lc The LinphoneCore object
 * @return \bctbx_list{OrtpPayloadType}
 *
 * This list is unmodifiable. The ->data field of the bctbx_list_t points a PayloadType
 * structure holding the codec information.
 * It is possible to make copy of the list with bctbx_list_copy() in order to modify it
 * (such as the order of codecs).
 * @ingroup media_parameters
 * @deprecated Use linphone_core_get_video_payload_types() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED const bctbx_list_t *linphone_core_get_video_codecs(const LinphoneCore *lc);

/**
 * Sets the list of video codecs.
 * @param[in] lc The LinphoneCore object
 * @param[in] codecs \bctbx_list{OrtpPayloadType}
 * @return 0
 *
 * The list is taken by the LinphoneCore thus the application should not free it.
 * This list is made of struct PayloadType describing the codec parameters.
 * @ingroup media_parameters
 * @deprecated Use linphone_core_set_video_payload_types() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_set_video_codecs(LinphoneCore *lc, bctbx_list_t *codecs);

/**
 * Return the list of the available text payload types.
 * @param[in] lc The core.
 * @return \bctbx_list{LinphonePayloadType} A freshly allocated list of the available payload types. The list
 * must be destroyed with bctbx_list_free() after usage. The elements of the list haven't to be unref.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_core_get_text_payload_types(LinphoneCore *lc);

/**
 * Redefine the list of the available payload types.
 * @param[in] lc The core.
 * @param[in] payload_types \bctbx_list{LinphonePayloadType} The new list of payload types. The core does not take
 * ownership on it.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_text_payload_types(LinphoneCore *lc, const bctbx_list_t *payload_types);

/**
 * Returns the list of available text codecs.
 * @param[in] lc The LinphoneCore object
 * @return \bctbx_list{OrtpPayloadType}
 *
 * This list is unmodifiable. The ->data field of the bctbx_list_t points a PayloadType
 * structure holding the codec information.
 * It is possible to make copy of the list with bctbx_list_copy() in order to modify it
 * (such as the order of codecs).
 * @ingroup media_parameters
 * @deprecated Use linphone_core_get_text_payload_types() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED const bctbx_list_t *linphone_core_get_text_codecs(const LinphoneCore *lc);

/**
 * Sets the list of text codecs.
 * @param[in] lc The LinphoneCore object
 * @param[in] codecs \bctbx_list{LinphonePayloadType}
 * @return 0
 *
 * The list is taken by the LinphoneCore thus the application should not free it.
 * This list is made of struct PayloadType describing the codec parameters.
 * @ingroup media_parameters
 * @deprecated Use linphone_core_set_text_payload_types() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_set_text_codecs(LinphoneCore *lc, bctbx_list_t *codecs);

/**
 * Enable RFC3389 generic comfort noise algorithm (CN payload type).
 * It is disabled by default, because this algorithm is only relevant for legacy codecs (PCMU, PCMA, G722).
 * @param[in] lc LinphoneCore object
 * @param[in] enabled TRUE if enabled, FALSE otherwise.
 * @deprecated Use linphone_core_enable_generic_comfort_noise() instead
 */
#define linphone_core_enable_generic_confort_noise(lc, enabled) linphone_core_enable_generic_comfort_noise(lc, enabled)

/**
 * Returns enablement of RFC3389 generic comfort noise algorithm.
 * @param[in] lc LinphoneCore object
 * @return TRUE or FALSE.
 * @deprecated Use linphone_core_generic_comfort_noise_enabled() instead
 */
#define linphone_core_generic_confort_noise_enabled(lc) linphone_core_generic_comfort_noise_enabled(lc)

/**
 * Enable RFC3389 generic comfort noise algorithm (CN payload type).
 * It is disabled by default, because this algorithm is only relevant for legacy codecs (PCMU, PCMA, G722).
 * @param[in] lc LinphoneCore object
 * @param[in] enabled TRUE if enabled, FALSE otherwise.
**/
LINPHONE_PUBLIC void linphone_core_enable_generic_comfort_noise(LinphoneCore *lc, bool_t enabled);

/**
 * Returns enablement of RFC3389 generic comfort noise algorithm.
 * @param[in] lc LinphoneCore object
 * @return TRUE or FALSE.
**/
LINPHONE_PUBLIC bool_t linphone_core_generic_comfort_noise_enabled(const LinphoneCore *lc);

/**
 * Tells whether the specified payload type is enabled.
 * @param[in] lc #LinphoneCore object.
 * @param[in] pt The payload type to check.
 * @return TRUE if the payload type is enabled, FALSE if disabled.
 * @ingroup media_parameters
 * @deprecated Use linphone_payload_type_enabled() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED bool_t linphone_core_payload_type_enabled(const LinphoneCore *lc, const OrtpPayloadType *pt);

/**
 * Tells whether the specified payload type represents a variable bitrate codec.
 * @param[in] lc #LinphoneCore object.
 * @param[in] pt The payload type to check.
 * @return TRUE if the payload type represents a VBR codec, FALSE if disabled.
 * @ingroup media_parameters
 * @deprecated Use linphone_payload_type_is_vbr() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED bool_t linphone_core_payload_type_is_vbr(const LinphoneCore *lc, const OrtpPayloadType *pt);

/**
 * Set an explicit bitrate (IP bitrate, not codec bitrate) for a given codec, in kbit/s.
 * @param[in] lc the #LinphoneCore object
 * @param[in] pt the payload type to modify.
 * @param[in] bitrate the IP bitrate in kbit/s.
 * @ingroup media_parameters
 * @deprecated Use linphone_payload_type_set_normal_bitrate() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_payload_type_bitrate(LinphoneCore *lc, OrtpPayloadType *pt, int bitrate);

/**
 * Get the bitrate explicitely set with linphone_core_set_payload_type_bitrate().
 * @param[in] lc the #LinphoneCore object
 * @param[in] pt the payload type to modify.
 * @return bitrate the IP bitrate in kbit/s, or -1 if an error occured.
 * @ingroup media_parameters
 * @deprecated Use linphone_payload_type_get_bitrate().
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED int linphone_core_get_payload_type_bitrate(LinphoneCore *lc, const OrtpPayloadType *pt);

/**
 * Enable or disable the use of the specified payload type.
 * @param[in] lc #LinphoneCore object.
 * @param[in] pt The payload type to enable or disable. It can be retrieved using #linphone_core_find_payload_type
 * @param[in] enable TRUE to enable the payload type, FALSE to disable it.
 * @return 0 if successful, any other value otherwise.
 * @ingroup media_parameters
 * @deprecated Use linphone_payload_type_enable().
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneStatus linphone_core_enable_payload_type(LinphoneCore *lc, OrtpPayloadType *pt, bool_t enable);

/**
 * Wildcard value used by #linphone_core_find_payload_type to ignore rate in search algorithm
 * @ingroup media_parameters
 */
#define LINPHONE_FIND_PAYLOAD_IGNORE_RATE -1

/**
 * Wildcard value used by #linphone_core_find_payload_type to ignore channel in search algorithm
 * @ingroup media_parameters
 */
#define LINPHONE_FIND_PAYLOAD_IGNORE_CHANNELS -1

/**
 * Get payload type from mime type and clock rate.
 * @ingroup media_parameters
 * This function searches in audio and video codecs for the given payload type name and clockrate.
 * @param lc #LinphoneCore object
 * @param type payload mime type (I.E SPEEX, PCMU, VP8)
 * @param rate can be #LINPHONE_FIND_PAYLOAD_IGNORE_RATE
 * @param channels  number of channels, can be #LINPHONE_FIND_PAYLOAD_IGNORE_CHANNELS
 * @return Returns NULL if not found.
 * @deprecated Use linphone_core_get_payload_type() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED OrtpPayloadType *linphone_core_find_payload_type(LinphoneCore* lc, const char* type, int rate, int channels);

/**
 * Get payload type from mime type and clock rate.
 * @ingroup media_parameters
 * This function searches in audio and video codecs for the given payload type name and clockrate.
 * @param lc #LinphoneCore object
 * @param type payload mime type (I.E SPEEX, PCMU, VP8)
 * @param rate can be #LINPHONE_FIND_PAYLOAD_IGNORE_RATE
 * @param channels  number of channels, can be #LINPHONE_FIND_PAYLOAD_IGNORE_CHANNELS
 * @return Returns NULL if not found. If a #LinphonePayloadType is returned, it must be released with
 * linphone_payload_type_unref() after using it.
 * @warning The returned payload type is allocated as a floating reference i.e. the reference counter is initialized to 0.
 */
LINPHONE_PUBLIC LinphonePayloadType *linphone_core_get_payload_type(LinphoneCore *lc, const char *type, int rate, int channels);

/**
 * Returns the payload type number assigned for this codec.
 * @ingroup media_parameters
 * @deprecated Use linphone_payload_type_get_number() instead
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED int linphone_core_get_payload_type_number(LinphoneCore *lc, const OrtpPayloadType *pt);

/**
 * Force a number for a payload type. The LinphoneCore does payload type number assignment automatically. THis function is to be used mainly for tests, in order
 * to override the automatic assignment mechanism.
 * @ingroup media_parameters
 * @deprecated Use linphone_payload_type_set_number() instead
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_payload_type_number(LinphoneCore *lc, OrtpPayloadType *pt, int number);

/**
 * Get a description of the encoder used to supply a payload type.
 * @param[in] lc The core.
 * @param[in] pt The payload type.
 * @return The description of the encoder. Can be NULL if the format is not supported by Mediastreamer2.
 * @deprecated Use linphone_payload_type_get_encoder_description() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED const char *linphone_core_get_payload_type_description(LinphoneCore *lc, const OrtpPayloadType *pt);

/**
 * Return TRUE if codec can be used with bandwidth, FALSE else
 * @ingroup media_parameters
 * @deprecated Use linphone_payload_type_is_usable() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED bool_t linphone_core_check_payload_type_usability(LinphoneCore *lc, const OrtpPayloadType *pt);

/**
 * @addtogroup proxies
 * @{
 */

/**
 * Create a proxy config with default values from Linphone core.
 * @param[in] lc #LinphoneCore object
 * @return #LinphoneProxyConfig with default values set
 */
LINPHONE_PUBLIC LinphoneProxyConfig * linphone_core_create_proxy_config(LinphoneCore *lc);

/**
 * Add a proxy configuration.
 * This will start registration on the proxy, if registration is enabled.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_add_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *config);

/**
 * Erase all proxies from config.
**/
LINPHONE_PUBLIC void linphone_core_clear_proxy_config(LinphoneCore *lc);

/**
 * Removes a proxy configuration.
 *
 * LinphoneCore will then automatically unregister and place the proxy configuration
 * on a deleted list. For that reason, a removed proxy does NOT need to be freed.
**/
LINPHONE_PUBLIC void linphone_core_remove_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *config);

/**
 * Returns an unmodifiable list of entered proxy configurations.
 * @param[in] lc The LinphoneCore object
 * @return \bctbx_list{LinphoneProxyConfig}
**/
LINPHONE_PUBLIC const bctbx_list_t *linphone_core_get_proxy_config_list(const LinphoneCore *lc);

/** @deprecated Use linphone_core_set_default_proxy_config() instead. */
#define linphone_core_set_default_proxy(lc, config) linphone_core_set_default_proxy_config(lc, config)

LINPHONE_PUBLIC void linphone_core_set_default_proxy_index(LinphoneCore *lc, int index);

/**
 * @return the default proxy configuration, that is the one used to determine the current identity.
 * @deprecated Use linphone_core_get_default_proxy_config() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED int linphone_core_get_default_proxy(LinphoneCore *lc, LinphoneProxyConfig **config);

/**
 * @return the default proxy configuration, that is the one used to determine the current identity.
 * @param[in] lc LinphoneCore object
 * @return The default proxy configuration.
**/
LINPHONE_PUBLIC LinphoneProxyConfig * linphone_core_get_default_proxy_config(LinphoneCore *lc);

/**
 * Sets the default proxy.
 *
 * This default proxy must be part of the list of already entered LinphoneProxyConfig.
 * Toggling it as default will make LinphoneCore use the identity associated with
 * the proxy configuration in all incoming and outgoing calls.
 * @param[in] lc LinphoneCore object
 * @param[in] config The proxy configuration to use as the default one.
**/
LINPHONE_PUBLIC void linphone_core_set_default_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *config);

/**
 * @}
 */

/**
 * Create an authentication information with default values from Linphone core.
 * @param[in] lc #LinphoneCore object
 * @param[in] username String containing the username part of the authentication credentials
 * @param[in] userid String containing the username to use to calculate the authentication digest (optional)
 * @param[in] passwd String containing the password of the authentication credentials (optional, either passwd or ha1 must be set)
 * @param[in] ha1 String containing a ha1 hash of the password (optional, either passwd or ha1 must be set)
 * @param[in] realm String used to discriminate different SIP authentication domains (optional)
 * @param[in] domain String containing the SIP domain for which this authentication information is valid, if it has to be restricted for a single SIP domain.
 * @return #LinphoneAuthInfo with default values set
 * @ingroup authentication
 */
LINPHONE_PUBLIC LinphoneAuthInfo * linphone_core_create_auth_info(LinphoneCore *lc, const char *username, const char *userid, const char *passwd, const char *ha1, const char *realm, const char *domain);

/**
 * Adds authentication information to the #LinphoneCore.
 * That piece of information will be used during all SIP transactions that require authentication.
 * @param[in] lc The #LinphoneCore.
 * @param[in] info The #LinphoneAuthInfo to add.
 * @ingroup authentication
 */
LINPHONE_PUBLIC void linphone_core_add_auth_info(LinphoneCore *lc, const LinphoneAuthInfo *info);

/**
 * Removes an authentication information object.
 * @param[in] lc The #LinphoneCore from which the #LinphoneAuthInfo will be removed.
 * @param[in] info The #LinphoneAuthInfo to remove.
 * @ingroup authentication
 */
LINPHONE_PUBLIC void linphone_core_remove_auth_info(LinphoneCore *lc, const LinphoneAuthInfo *info);

/**
 * Returns an unmodifiable list of currently entered #LinphoneAuthInfo.
 * @param[in] lc The LinphoneCore object.
 * @return \bctbx_list{LinphoneAuthInfo}
 * @ingroup authentication
 */
LINPHONE_PUBLIC const bctbx_list_t *linphone_core_get_auth_info_list(const LinphoneCore *lc);

/**
 * Find authentication info matching realm, username, domain criteria.
 * First of all, (realm,username) pair are searched. If multiple results (which should not happen because realm are supposed to be unique), then domain is added to the search.
 * @param lc the LinphoneCore
 * @param realm the authentication 'realm' (optional)
 * @param username the SIP username to be authenticated (mandatory)
 * @param sip_domain the SIP domain name (optional)
 * @return a #LinphoneAuthInfo
 * @ingroup authentication
**/
LINPHONE_PUBLIC const LinphoneAuthInfo *linphone_core_find_auth_info(LinphoneCore *lc, const char *realm, const char *username, const char *sip_domain);

/**
 * This method is used to abort a user authentication request initiated by LinphoneCore
 * from the auth_info_requested callback of LinphoneCoreVTable.
 * @note That function does nothing for now.
**/
LINPHONE_PUBLIC void linphone_core_abort_authentication(LinphoneCore *lc, LinphoneAuthInfo *info);

/**
 * Clear all authentication information.
 * @ingroup authentication
 **/
LINPHONE_PUBLIC void linphone_core_clear_all_auth_info(LinphoneCore *lc);

/**
 * Sets an default account creator service in the core
 * @param lc LinphoneCore object
 * @param cbs LinphoneAccountCreatorService object
**/
LINPHONE_PUBLIC void linphone_core_set_account_creator_service(LinphoneCore *lc, LinphoneAccountCreatorService *service);

/**
 * Get default account creator service from the core
 * @param lc LinphoneCore object
 * @return LinphoneAccountCreatorService object
**/
LINPHONE_PUBLIC LinphoneAccountCreatorService * linphone_core_get_account_creator_service(LinphoneCore *lc);

/**
 * Enable or disable the audio adaptive jitter compensation.
 * @param[in] lc #LinphoneCore object
 * @param[in] enable TRUE to enable the audio adaptive jitter compensation, FALSE to disable it.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_enable_audio_adaptive_jittcomp(LinphoneCore *lc, bool_t enable);

/**
 * Tells whether the audio adaptive jitter compensation is enabled.
 * @param[in] lc #LinphoneCore object
 * @return TRUE if the audio adaptive jitter compensation is enabled, FALSE otherwise.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_audio_adaptive_jittcomp_enabled(LinphoneCore *lc);

/**
 * Returns the nominal audio jitter buffer size in milliseconds.
 * @param[in] lc LinphoneCore object
 * @return The nominal audio jitter buffer size in milliseconds
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_audio_jittcomp(LinphoneCore *lc);

/**
 * Sets the nominal audio jitter buffer size in milliseconds.
 * The value takes effect immediately for all running and pending calls, if any.
 * A value of 0 disables the jitter buffer.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_audio_jittcomp(LinphoneCore *lc, int milliseconds);

/**
 * Enable or disable the video adaptive jitter compensation.
 * @param[in] lc #LinphoneCore object
 * @param[in] enable TRUE to enable the video adaptive jitter compensation, FALSE to disable it.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_enable_video_adaptive_jittcomp(LinphoneCore *lc, bool_t enable);

/**
 * Tells whether the video adaptive jitter compensation is enabled.
 * @param[in] lc #LinphoneCore object
 * @return TRUE if the video adaptive jitter compensation is enabled, FALSE otherwise.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_video_adaptive_jittcomp_enabled(LinphoneCore *lc);

/**
 * Returns the nominal video jitter buffer size in milliseconds.
 * @param[in] lc LinphoneCore object
 * @return The nominal video jitter buffer size in milliseconds
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_video_jittcomp(LinphoneCore *lc);

/**
 * Sets the nominal video jitter buffer size in milliseconds.
 * The value takes effect immediately for all running and pending calls, if any.
 * A value of 0 disables the jitter buffer.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_video_jittcomp(LinphoneCore *lc, int milliseconds);

/**
 * Gets the UDP port used for audio streaming.
 * @param[in] lc LinphoneCore object
 * @return The UDP port used for audio streaming
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_audio_port(const LinphoneCore *lc);

/**
 * Get the audio port range from which is randomly chosen the UDP port used for audio streaming.
 * @param[in] lc LinphoneCore object
 * @param[out] min_port The lower bound of the audio port range being used
 * @param[out] max_port The upper bound of the audio port range being used
 * @ingroup network_parameters
 * @donotwrap
 */
LINPHONE_PUBLIC void linphone_core_get_audio_port_range(const LinphoneCore *lc, int *min_port, int *max_port);

/**
 * Get the audio port range from which is randomly chosen the UDP port used for audio streaming.
 * @param[in] lc LinphoneCore object
 * @return a LinphoneRange object
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC LinphoneRange *linphone_core_get_audio_ports_range(const LinphoneCore *lc);

/**
 * Gets the UDP port used for video streaming.
 * @param[in] lc LinphoneCore object
 * @return The UDP port used for video streaming
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_video_port(const LinphoneCore *lc);

/**
 * Get the video port range from which is randomly chosen the UDP port used for video streaming.
 * @param[in] lc LinphoneCore object
 * @param[out] min_port The lower bound of the video port range being used
 * @param[out] max_port The upper bound of the video port range being used
 * @ingroup network_parameters
 * @donotwrap
 */
LINPHONE_PUBLIC void linphone_core_get_video_port_range(const LinphoneCore *lc, int *min_port, int *max_port);

/**
 * Get the video port range from which is randomly chosen the UDP port used for video streaming.
 * @param[in] lc LinphoneCore object
 * @return a LinphoneRange object
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC LinphoneRange *linphone_core_get_video_ports_range(const LinphoneCore *lc);

/**
 * Gets the UDP port used for text streaming.
 * @param[in] lc LinphoneCore object
 * @return The UDP port used for text streaming
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_text_port(const LinphoneCore *lc);

/**
 * Get the video port range from which is randomly chosen the UDP port used for text streaming.
 * @param[in] lc LinphoneCore object
 * @param[out] min_port The lower bound of the text port range being used
 * @param[out] max_port The upper bound of the text port range being used
 * @ingroup network_parameters
 * @donotwrap
 */
LINPHONE_PUBLIC void linphone_core_get_text_port_range(const LinphoneCore *lc, int *min_port, int *max_port);

/**
 * Get the text port range from which is randomly chosen the UDP port used for text streaming.
 * @param[in] lc LinphoneCore object
 * @return a LinphoneRange object
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC LinphoneRange *linphone_core_get_text_ports_range(const LinphoneCore *lc);

/**
 * Gets the value of the no-rtp timeout.
 *
 * When no RTP or RTCP packets have been received for a while
 * LinphoneCore will consider the call is broken (remote end crashed or
 * disconnected from the network), and thus will terminate the call.
 * The no-rtp timeout is the duration above which the call is considered broken.
 * @param[in] lc LinphoneCore object
 * @return The value of the no-rtp timeout in seconds
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_nortp_timeout(const LinphoneCore *lc);

/**
 * Sets the UDP port used for audio streaming.
 * A value of -1 will request the system to allocate the local port randomly.
 * This is recommended in order to avoid firewall warnings.
 * @param[in] lc LinphoneCore object
 * @param[in] port The UDP port to use for audio streaming
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_audio_port(LinphoneCore *lc, int port);

/**
 * Sets the UDP port range from which to randomly select the port used for audio streaming.
 * @param[in] lc LinphoneCore object
 * @param[in] min_port The lower bound of the audio port range to use
 * @param[in] max_port The upper bound of the audio port range to use
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_audio_port_range(LinphoneCore *lc, int min_port, int max_port);

/**
 * Sets the UDP port used for video streaming.
 * A value of -1 will request the system to allocate the local port randomly.
 * This is recommended in order to avoid firewall warnings.
 * @param[in] lc LinphoneCore object
 * @param[in] port The UDP port to use for video streaming
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_video_port(LinphoneCore *lc, int port);

/**
 * Sets the UDP port range from which to randomly select the port used for video streaming.
 * @param[in] lc LinphoneCore object
 * @param[in] min_port The lower bound of the video port range to use
 * @param[in] max_port The upper bound of the video port range to use
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_video_port_range(LinphoneCore *lc, int min_port, int max_port);

/**
 * Sets the UDP port used for text streaming.
 * A value if -1 will request the system to allocate the local port randomly.
 * This is recommended in order to avoid firewall warnings.
 * @param[in] lc LinphoneCore object
 * @param[in] port The UDP port to use for text streaming
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_text_port(LinphoneCore *lc, int port);

/**
 * Sets the UDP port range from which to randomly select the port used for text streaming.
 * @param[in] lc LinphoneCore object
 * @param[in] min_port The lower bound of the text port range to use
 * @param[in] max_port The upper bound of the text port range to use
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_text_port_range(LinphoneCore *lc, int min_port, int max_port);

/**
 * Sets the no-rtp timeout value in seconds.
 * @param[in] lc LinphoneCore object
 * @param[in] seconds The no-rtp timeout value to use in seconds
 * @ingroup media_parameters
 * @see linphone_core_get_nortp_timeout() for details.
**/
LINPHONE_PUBLIC void linphone_core_set_nortp_timeout(LinphoneCore *lc, int seconds);

/**
 * Sets whether SIP INFO is to be used to send digits.
 * @param[in] lc LinphoneCore object
 * @param[in] use_info A boolean value telling whether to use SIP INFO to send digits
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_use_info_for_dtmf(LinphoneCore *lc, bool_t use_info);

/**
 * Indicates whether SIP INFO is used to send digits.
 * @param[in] lc LinphoneCore object
 * @return A boolean value telling whether SIP INFO is used to send digits
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_get_use_info_for_dtmf(LinphoneCore *lc);

/**
 * Sets whether RFC2833 is to be used to send digits.
 * @param[in] lc LinphoneCore object
 * @param[in] use_rfc2833 A boolean value telling whether to use RFC2833 to send digits
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_use_rfc2833_for_dtmf(LinphoneCore *lc,bool_t use_rfc2833);

/**
 * Indicates whether RFC2833 is used to send digits.
 * @param[in] lc LinphoneCore object
 * @return A boolean value telling whether RFC2833 is used to send digits
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_get_use_rfc2833_for_dtmf(LinphoneCore *lc);

/**
 * Sets the UDP port to be used by SIP.
 * @param[in] lc LinphoneCore object
 * @param[in] port The UDP port to be used by SIP
 * @ingroup network_parameters
 * @deprecated use linphone_core_set_sip_transports() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_sip_port(LinphoneCore *lc, int port);

/**
 * Gets the UDP port used by SIP.
 * @param[in] lc LinphoneCore object
 * @return The UDP port used by SIP
 * @ingroup network_parameters
 * @deprecated use linphone_core_get_sip_transports() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED int linphone_core_get_sip_port(LinphoneCore *lc);

/**
 * Sets the ports to be used for each of transport (UDP or TCP)
 * A zero value port for a given transport means the transport
 * is not used. A value of LC_SIP_TRANSPORT_RANDOM (-1) means the port is to be choosen randomly by the system.
 * @param[in] lc LinphoneCore object
 * @param[in] transports A LinphoneSipTransports structure giving the ports to use
 * @return 0
 * @ingroup network_parameters
 * @deprecated Use linphone_core_set_transports instead
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_sip_transports(LinphoneCore *lc, const LinphoneSipTransports *transports);

/**
 * Retrieves the port configuration used for each transport (udp, tcp, tls).
 * A zero value port for a given transport means the transport
 * is not used. A value of LC_SIP_TRANSPORT_RANDOM (-1) means the port is to be chosen randomly by the system.
 * @param[in] lc LinphoneCore object
 * @param[out] transports A #LinphoneSipTransports structure that will receive the configured ports
 * @return 0
 * @ingroup network_parameters
 * @deprecated
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_get_sip_transports(LinphoneCore *lc, LinphoneSipTransports *transports);

/**
 * Retrieves the real port number assigned for each sip transport (udp, tcp, tls).
 * A zero value means that the transport is not activated.
 * If LC_SIP_TRANSPORT_RANDOM was passed to linphone_core_set_sip_transports(), the random port choosed by the system is returned.
 * @param[in] lc LinphoneCore object
 * @param[out] tr A #LinphoneSipTransports structure that will receive the ports being used
 * @ingroup network_parameters
 * @deprecated Use linphone_core_get_transports_used instead
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_core_get_sip_transports_used(LinphoneCore *lc, LinphoneSipTransports *tr);

/**
 * Sets the ports to be used for each of transport (UDP or TCP)
 * A zero value port for a given transport means the transport
 * is not used. A value of LC_SIP_TRANSPORT_RANDOM (-1) means the port is to be choosen randomly by the system.
 * @param[in] lc LinphoneCore object
 * @param[in] transports A LinphoneSipTransports structure giving the ports to use
 * @return 0
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_transports(LinphoneCore *lc, const LinphoneTransports *transports);

/**
 * Retrieves the port configuration used for each transport (udp, tcp, tls).
 * A zero value port for a given transport means the transport
 * is not used. A value of LC_SIP_TRANSPORT_RANDOM (-1) means the port is to be chosen randomly by the system.
 * @param[in] lc LinphoneCore object
 * @return A #LinphoneTransports structure with the configured ports
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC LinphoneTransports *linphone_core_get_transports(LinphoneCore *lc);

/**
 * Retrieves the real port number assigned for each sip transport (udp, tcp, tls).
 * A zero value means that the transport is not activated.
 * If LC_SIP_TRANSPORT_RANDOM was passed to linphone_core_set_sip_transports(), the random port choosed by the system is returned.
 * @param[in] lc LinphoneCore object
 * @return A #LinphoneTransports structure with the ports being used
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC LinphoneTransports *linphone_core_get_transports_used(LinphoneCore *lc);

/**
 * Increment refcount.
 * @param[in] transports LinphoneTransports object
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC LinphoneTransports *linphone_transports_ref(LinphoneTransports *transports);

/**
 * Decrement refcount and possibly free the object.
 * @param[in] transports LinphoneTransports object
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC void linphone_transports_unref(LinphoneTransports *transports);

/**
 * Gets the user data in the LinphoneTransports object
 * @param[in] transports the LinphoneTransports
 * @return the user data
 * @ingroup network_parameters
*/
LINPHONE_PUBLIC void *linphone_transports_get_user_data(const LinphoneTransports *transports);

/**
 * Sets the user data in the LinphoneTransports object
 * @param[in] transports the LinphoneTransports object
 * @param[in] data the user data
 * @ingroup network_parameters
*/
LINPHONE_PUBLIC void linphone_transports_set_user_data(LinphoneTransports *transports, void *data);

/**
 * Gets the UDP port in the LinphoneTransports object
 * @param[in] transports the LinphoneTransports object
 * @return the UDP port
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC int linphone_transports_get_udp_port(const LinphoneTransports* transports);

/**
 * Gets the TCP port in the LinphoneTransports object
 * @param[in] transports the LinphoneTransports object
 * @return the TCP port
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC int linphone_transports_get_tcp_port(const LinphoneTransports* transports);

/**
 * Gets the TLS port in the LinphoneTransports object
 * @param[in] transports the LinphoneTransports object
 * @return the TLS port
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC int linphone_transports_get_tls_port(const LinphoneTransports* transports);

/**
 * Gets the DTLS port in the LinphoneTransports object
 * @param[in] transports the LinphoneTransports object
 * @return the DTLS port
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC int linphone_transports_get_dtls_port(const LinphoneTransports* transports);

/**
 * Sets the UDP port in the LinphoneTransports object
 * @param[in] transports the LinphoneTransports object
 * @param[in] port the UDP port
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_transports_set_udp_port(LinphoneTransports *transports, int port);

/**
 * Sets the TCP port in the LinphoneTransports object
 * @param[in] transports the LinphoneTransports object
 * @param[in] port the TCP port
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_transports_set_tcp_port(LinphoneTransports *transports, int port);

/**
 * Sets the TLS port in the LinphoneTransports object
 * @param[in] transports the LinphoneTransports object
 * @param[in] port the TLS port
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_transports_set_tls_port(LinphoneTransports *transports, int port);

/**
 * Sets the DTLS port in the LinphoneTransports object
 * @param[in] transports the LinphoneTransports object
 * @param[in] port the DTLS port
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_transports_set_dtls_port(LinphoneTransports *transports, int port);

/**
 * Tells whether the given transport type is supported by the library.
 * @param[in] lc LinphoneCore object
 * @param[in] tp LinphoneTranportType to check for support
 * @return A boolean value telling whether the given transport type is supported by the library
**/
LINPHONE_PUBLIC bool_t linphone_core_sip_transport_supported(const LinphoneCore *lc, LinphoneTransportType tp);

LINPHONE_PUBLIC bool_t linphone_core_content_encoding_supported(const LinphoneCore *lc, const char *content_encoding);

/**
 *
 * Give access to the UDP sip socket. Can be useful to configure this socket as persistent I.E kCFStreamNetworkServiceType set to kCFStreamNetworkServiceTypeVoIP)
 * @param lc #LinphoneCore
 * @return socket file descriptor
 */
ortp_socket_t linphone_core_get_sip_socket(LinphoneCore *lc);

/**
 * Set the incoming call timeout in seconds.
 * If an incoming call isn't answered for this timeout period, it is
 * automatically declined.
 * @param[in] lc LinphoneCore object
 * @param[in] seconds The new timeout in seconds
 * @ingroup call_control
**/
LINPHONE_PUBLIC void linphone_core_set_inc_timeout(LinphoneCore *lc, int seconds);

/**
 * Returns the incoming call timeout
 * See linphone_core_set_inc_timeout() for details.
 * @param[in] lc LinphoneCore object
 * @return The current incoming call timeout in seconds
 * @ingroup call_control
**/
LINPHONE_PUBLIC int linphone_core_get_inc_timeout(LinphoneCore *lc);

/**
 * Set the in call timeout in seconds.
 * After this timeout period, the call is automatically hangup.
 * @param[in] lc LinphoneCore object
 * @param[in] seconds The new timeout in seconds
 * @ingroup call_control
**/
LINPHONE_PUBLIC void linphone_core_set_in_call_timeout(LinphoneCore *lc, int seconds);

/**
 * Gets the in call timeout
 * See linphone_core_set_in_call_timeout() for details.
 * @param[in] lc LinphoneCore object
 * @return The current in call timeout in seconds
 * @ingroup call_control
**/
LINPHONE_PUBLIC int linphone_core_get_in_call_timeout(LinphoneCore *lc);

/**
 * Set the in delayed timeout in seconds.
 * After this timeout period, a delayed call (internal call initialisation or resolution) is resumed.
 * @param[in] lc LinphoneCore object
 * @param[in] seconds The new delayed timeout
 * @ingroup call_control
**/
LINPHONE_PUBLIC void linphone_core_set_delayed_timeout(LinphoneCore *lc, int seconds);

/**
 * Gets the delayed timeout
 * See linphone_core_set_delayed_timeout() for details.
 * @param[in] lc LinphoneCore object
 * @return The current delayed timeout in seconds
 * @ingroup call_control
**/
LINPHONE_PUBLIC int linphone_core_get_delayed_timeout(LinphoneCore *lc);

/**
 * Set the STUN server address to use when the firewall policy is set to STUN.
 * @param[in] lc #LinphoneCore object
 * @param[in] server The STUN server address to use.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_stun_server(LinphoneCore *lc, const char *server);

/**
 * Get the STUN server address being used.
 * @param[in] lc #LinphoneCore object
 * @return The STUN server address being used.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC const char * linphone_core_get_stun_server(const LinphoneCore *lc);

/**
 * Return the availability of uPnP.
 * @return true if uPnP is available otherwise return false.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_upnp_available(void);

/**
 * Return the internal state of uPnP.
 * @param lc #LinphoneCore
 * @return an LinphoneUpnpState.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC LinphoneUpnpState linphone_core_get_upnp_state(const LinphoneCore *lc);

/**
 * Return the external ip address of router.
 * In some cases the uPnP can have an external ip address but not a usable uPnP
 * (state different of Ok).
 *
 * @param lc #LinphoneCore
 * @return a null terminated string containing the external ip address. If the
 * the external ip address is not available return null.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC const char * linphone_core_get_upnp_external_ipaddress(const LinphoneCore *lc);

/**
 * Set the public IP address of NAT when using the firewall policy is set to use NAT.
 * @param[in] lc #LinphoneCore object.
 * @param[in] addr The public IP address of NAT to use.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_nat_address(LinphoneCore *lc, const char *addr);

/**
 * Get the public IP address of NAT being used.
 * @param[in] lc #LinphoneCore object.
 * @return The public IP address of NAT being used.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC const char *linphone_core_get_nat_address(const LinphoneCore *lc);

/**
 * Set the policy to use to pass through firewalls.
 * @param[in] lc #LinphoneCore object.
 * @param[in] pol The #LinphoneFirewallPolicy to use.
 * @ingroup network_parameters
 * @deprecated Use linphone_core_set_nat_policy() instead.
 * @donotwrap
 */
LINPHONE_DEPRECATED LINPHONE_PUBLIC void linphone_core_set_firewall_policy(LinphoneCore *lc, LinphoneFirewallPolicy pol);

/**
 * Get the policy that is used to pass through firewalls.
 * @param[in] lc #LinphoneCore object.
 * @return The #LinphoneFirewallPolicy that is being used.
 * @ingroup network_parameters
 * @deprecated Use linphone_core_get_nat_policy() instead
 * @donotwrap
 */
LINPHONE_DEPRECATED LINPHONE_PUBLIC LinphoneFirewallPolicy linphone_core_get_firewall_policy(const LinphoneCore *lc);

/**
 * Set the policy to use to pass through NATs/firewalls.
 * It may be overridden by a NAT policy for a specific proxy config.
 * @param[in] lc #LinphoneCore object
 * @param[in] policy LinphoneNatPolicy object
 * @ingroup network_parameters
 * @see linphone_proxy_config_set_nat_policy()
 */
LINPHONE_PUBLIC void linphone_core_set_nat_policy(LinphoneCore *lc, LinphoneNatPolicy *policy);

/**
 * Get The policy that is used to pass through NATs/firewalls.
 * It may be overridden by a NAT policy for a specific proxy config.
 * @param[in] lc #LinphoneCore object
 * @return LinphoneNatPolicy object in use.
 * @ingroup network_parameters
 * @see linphone_proxy_config_get_nat_policy()
 */
LINPHONE_PUBLIC LinphoneNatPolicy * linphone_core_get_nat_policy(const LinphoneCore *lc);

/**
 * Gets the list of the available sound devices.
 * @param[in] lc LinphoneCore object
 * @return An unmodifiable array of strings contanining the names of the available sound devices that is NULL terminated
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char** linphone_core_get_sound_devices(LinphoneCore *lc);

/**
 * Use this function when you want to set the default sound devices
 **/
LINPHONE_PUBLIC void linphone_core_set_default_sound_devices(LinphoneCore *lc);

/**
 * Update detection of sound devices.
 *
 * Use this function when the application is notified of USB plug events, so that
 * list of available hardwares for sound playback and capture is updated.
 * @param[in] lc #LinphoneCore object.
 * @ingroup media_parameters
 **/
LINPHONE_PUBLIC void linphone_core_reload_sound_devices(LinphoneCore *lc);

/**
 * Tells whether a specified sound device can capture sound.
 * @param[in] lc LinphoneCore object
 * @param[in] device the device name as returned by linphone_core_get_sound_devices()
 * @return A boolean value telling whether the specified sound device can capture sound
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_sound_device_can_capture(LinphoneCore *lc, const char *device);

/**
 * Tells whether a specified sound device can play sound.
 * @param[in] lc LinphoneCore object
 * @param[in] device the device name as returned by linphone_core_get_sound_devices()
 * @return A boolean value telling whether the specified sound device can play sound
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_sound_device_can_playback(LinphoneCore *lc, const char *device);

/**
 * Get ring sound level in 0-100 scale.
 * @ingroup media_parameters
 * @deprecated
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED int linphone_core_get_ring_level(LinphoneCore *lc);

/**
 * Get playback sound level in 0-100 scale.
 * @ingroup media_parameters
 * @deprecated
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED int linphone_core_get_play_level(LinphoneCore *lc);

/**
 * Get sound capture level in 0-100 scale.
 * @ingroup media_parameters
 * @deprecated
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED int linphone_core_get_rec_level(LinphoneCore *lc);

/**
 * Set sound ring level in 0-100 scale.
 * @ingroup media_parameters
 * @deprecated
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_ring_level(LinphoneCore *lc, int level);

/**
 * Set sound playback level in 0-100 scale.
 * @deprecated
 * @ingroup media_parameters
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_play_level(LinphoneCore *lc, int level);

/**
 * Set sound capture level in 0-100 scale.
 * @deprecated
 * @ingroup media_parameters
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_rec_level(LinphoneCore *lc, int level);

LINPHONE_DEPRECATED char linphone_core_get_sound_source(LinphoneCore *lc);

LINPHONE_DEPRECATED void linphone_core_set_sound_source(LinphoneCore *lc, char source);

/**
 * Allow to control microphone level: gain in db.
 * @param[in] lc LinphoneCore object
 * @param[in] level The new microphone level
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_mic_gain_db(LinphoneCore *lc, float level);

/**
 * Get microphone gain in db.
 * @param[in] lc LinphoneCore object
 * @return The current microphone gain
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC float linphone_core_get_mic_gain_db(LinphoneCore *lc);

/**
 * Allow to control play level before entering sound card:  gain in db
 * @param[in] lc LinphoneCore object
 * @param[in] level The new play level
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_playback_gain_db(LinphoneCore *lc, float level);

/**
 * Get playback gain in db before entering  sound card.
 * @param[in] lc LinphoneCore object
 * @return The current playback gain
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC float linphone_core_get_playback_gain_db(LinphoneCore *lc);

/**
 * Gets the name of the currently assigned sound device for ringing.
 * @param[in] lc LinphoneCore object
 * @return The name of the currently assigned sound device for ringing
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char * linphone_core_get_ringer_device(LinphoneCore *lc);

/**
 * Gets the name of the currently assigned sound device for playback.
 * @param[in] lc LinphoneCore object
 * @return The name of the currently assigned sound device for playback
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char * linphone_core_get_playback_device(LinphoneCore *lc);

/**
 * Gets the name of the currently assigned sound device for capture.
 * @param[in] lc LinphoneCore object
 * @return The name of the currently assigned sound device for capture
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char * linphone_core_get_capture_device(LinphoneCore *lc);

/**
 * Sets the sound device used for ringing.
 * @param[in] lc LinphoneCore object
 * @param[in] devid The device name as returned by linphone_core_get_sound_devices()
 * @return 0
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_ringer_device(LinphoneCore *lc, const char * devid);

/**
 * Sets the sound device used for playback.
 * @param[in] lc LinphoneCore object
 * @param[in] devid The device name as returned by linphone_core_get_sound_devices()
 * @return 0
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_playback_device(LinphoneCore *lc, const char * devid);

/**
 * Sets the sound device used for capture.
 * @param[in] lc LinphoneCore object
 * @param[in] devid The device name as returned by linphone_core_get_sound_devices()
 * @return 0
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_capture_device(LinphoneCore *lc, const char * devid);

/**
 * Whenever the liblinphone is playing a ring to advertise an incoming call or ringback of an outgoing call, this function stops
 * the ringing. Typical use is to stop ringing when the user requests to ignore the call.
 * @param[in] lc LinphoneCore object
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_stop_ringing(LinphoneCore *lc);

/**
 * Sets the path to a wav file used for ringing. The file must be a wav 16bit linear. Local ring is disabled if null.
 * @param[in] lc LinphoneCore object
 * @param[in] path The path to a wav file to be used for ringing
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_ring(LinphoneCore *lc, const char *path);

/**
 * Returns the path to the wav file used for ringing.
 * @param[in] lc LinphoneCore object
 * @return The path to the wav file used for ringing
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char *linphone_core_get_ring(const LinphoneCore *lc);

/**
 * Specify whether the tls server certificate must be verified when connecting to a SIP/TLS server.
 * @param[in] lc LinphoneCore object
 * @param[in] yesno A boolean value telling whether the tls server certificate must be verified
 * @ingroup initializing
**/
LINPHONE_PUBLIC void linphone_core_verify_server_certificates(LinphoneCore *lc, bool_t yesno);

/**
 * Specify whether the tls server certificate common name must be verified when connecting to a SIP/TLS server.
 * @param[in] lc LinphoneCore object
 * @param[in] yesno A boolean value telling whether the tls server certificate common name must be verified
 * @ingroup initializing
**/
LINPHONE_PUBLIC void linphone_core_verify_server_cn(LinphoneCore *lc, bool_t yesno);

/**
 * Gets the path to a file or folder containing the trusted root CAs (PEM format)
 * @param[in] lc LinphoneCore object
 * @return The path to a file or folder containing the trusted root CAs
 * @ingroup initializing
**/
LINPHONE_PUBLIC const char *linphone_core_get_root_ca(LinphoneCore *lc);

/**
 * Sets the path to a file or folder containing trusted root CAs (PEM format)
 * @param[in] lc LinphoneCore object
 * @param[in] path The path to a file or folder containing trusted root CAs
 * @ingroup initializing
**/
LINPHONE_PUBLIC void linphone_core_set_root_ca(LinphoneCore *lc, const char *path);

/**
 * Sets the trusted root CAs (PEM format)
 * @param[in] lc LinphoneCore object
 * @param[in] data The trusted root CAs as a string
 * @ingroup initializing
**/
LINPHONE_PUBLIC void linphone_core_set_root_ca_data(LinphoneCore *lc, const char *data);

/**
 * @internal
 * Set the pointer to an externally provided ssl configuration for the crypto library
 * @param lc #LinphoneCore object
 * @param[in] ssl_config A pointer to an opaque structure which will be provided directly to the crypto library used in bctoolbox. Use with extra care.
 * This ssl_config structure is responsibility of the caller and will not be freed at the connection's end.
 * @ingroup initializing
 * @endinternal
 */
LINPHONE_PUBLIC void linphone_core_set_ssl_config(LinphoneCore *lc, void *ssl_config);

/**
 * Sets the path to a wav file used for ringing back.
 * Ringback means the ring that is heard when it's ringing at the remote party.
 * The file must be a wav 16bit linear.
 * @param[in] lc LinphoneCore object
 * @param[in] path The path to a wav file to be used for ringing back
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_ringback(LinphoneCore *lc, const char *path);

/**
 * Returns the path to the wav file used for ringing back.
 * @param[in] lc LinphoneCore object
 * @return The path to the wav file used for ringing back
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char * linphone_core_get_ringback(const LinphoneCore *lc);

/**
 * Specify a ring back tone to be played to far end during incoming calls.
 * @param[in] lc #LinphoneCore object
 * @param[in] ring The path to the ring back tone to be played.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_remote_ringback_tone(LinphoneCore *lc, const char *ring);

/**
 * Get the ring back tone played to far end during incoming calls.
 * @param[in] lc #LinphoneCore object
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char *linphone_core_get_remote_ringback_tone(const LinphoneCore *lc);

/**
 * Enable or disable the ring play during an incoming early media call.
 * @param[in] lc #LinphoneCore object
 * @param[in] enable A boolean value telling whether to enable ringing during an incoming early media call.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_ring_during_incoming_early_media(LinphoneCore *lc, bool_t enable);

/**
 * Tells whether the ring play is enabled during an incoming early media call.
 * @param[in] lc #LinphoneCore object
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_get_ring_during_incoming_early_media(const LinphoneCore *lc);

LINPHONE_PUBLIC LinphoneStatus linphone_core_preview_ring(LinphoneCore *lc, const char *ring,LinphoneCoreCbFunc func,void * userdata);

/**
 * Returns the MSFactory (mediastreamer2 factory) used by the LinphoneCore to control mediastreamer2 library.
**/
LINPHONE_PUBLIC MSFactory* linphone_core_get_ms_factory(LinphoneCore* lc);

/**
 * Plays an audio file to the local user.
 * This function works at any time, during calls, or when no calls are running.
 * It doesn't request the underlying audio system to support multiple playback streams.
 * @param[in] lc LinphoneCore object
 * @param[in] audiofile The path to an audio file in wav PCM 16 bit format
 * @return 0 on success, -1 on error
 * @ingroup misc
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_play_local(LinphoneCore *lc, const char *audiofile);

/**
 * Enables or disable echo cancellation. Value is saved and used for subsequent calls.
 * This actually controls software echo cancellation. If hardware echo cancellation is available,
 * it will be always used and activated for calls, regardless of the value passed to this function.
 * When hardware echo cancellation is available, the software one is of course not activated.
 * @param[in] lc LinphoneCore object
 * @param[in] val A boolean value telling whether echo cancellation is to be enabled or disabled.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_echo_cancellation(LinphoneCore *lc, bool_t val);

/**
 * Returns TRUE if echo cancellation is enabled.
 * @param[in] lc LinphoneCore object
 * @return A boolean value telling whether echo cancellation is enabled or disabled
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_echo_cancellation_enabled(const LinphoneCore *lc);

/**
 * Enables or disable echo limiter.
 * @param[in] lc #LinphoneCore object.
 * @param[in] val TRUE to enable echo limiter, FALSE to disable it.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_echo_limiter(LinphoneCore *lc, bool_t val);

/**
 * Tells whether echo limiter is enabled.
 * @param[in] lc #LinphoneCore object.
 * @return TRUE if the echo limiter is enabled, FALSE otherwise.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_echo_limiter_enabled(const LinphoneCore *lc);

void linphone_core_enable_agc(LinphoneCore *lc, bool_t val);

bool_t linphone_core_agc_enabled(const LinphoneCore *lc);

/**
 * @deprecated Use #linphone_core_enable_mic instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_mute_mic(LinphoneCore *lc, bool_t muted);

/**
 * Get mic state.
 * @deprecated Use #linphone_core_mic_enabled instead
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED bool_t linphone_core_is_mic_muted(LinphoneCore *lc);

/**
 * Enable or disable the microphone.
 * @param[in] lc #LinphoneCore object
 * @param[in] enable TRUE to enable the microphone, FALSE to disable it.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_mic(LinphoneCore *lc, bool_t enable);

/**
 * Tells whether the microphone is enabled.
 * @param[in] lc #LinphoneCore object
 * @return TRUE if the microphone is enabled, FALSE if disabled.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_mic_enabled(LinphoneCore *lc);

/**
 * Returns the RTP transmission status for an active stream.
 * If audio is muted and config parameter rtp_no_xmit_on_audio_mute
 * has been set on then the RTP transmission is also muted.
 * @param lc The #LinphoneCore.
 * @return TRUE if the RTP transmisison is muted.
 */
LINPHONE_PUBLIC bool_t linphone_core_is_rtp_muted(LinphoneCore *lc);

LINPHONE_PUBLIC bool_t linphone_core_get_rtp_no_xmit_on_audio_mute(const LinphoneCore *lc);

LINPHONE_PUBLIC void linphone_core_set_rtp_no_xmit_on_audio_mute(LinphoneCore *lc, bool_t val);


/*******************************************************************************
 * Call log related functions                                                  *
 ******************************************************************************/

/**
 * @addtogroup call_logs
 * @{
**/

/**
 * Get the list of call logs (past calls).
 * @param[in] lc LinphoneCore object
 * @return \bctbx_list{LinphoneCallLog}
**/
LINPHONE_PUBLIC const bctbx_list_t * linphone_core_get_call_logs(LinphoneCore *lc);

/**
 * Get the list of call logs (past calls) that matches the given #LinphoneAddress.
 * At the contrary of linphone_core_get_call_logs, it is your responsibility to unref the logs and free this list once you are done using it.
 * @param[in] lc LinphoneCore object
 * @param[in] addr LinphoneAddress object
 * @return \bctbx_list{LinphoneCallLog}
**/
LINPHONE_PUBLIC bctbx_list_t * linphone_core_get_call_history_for_address(LinphoneCore *lc, const LinphoneAddress *addr);

/**
 * Get the latest outgoing call log.
 * @param[in] lc LinphoneCore object
 * @return {LinphoneCallLog}
**/
LINPHONE_PUBLIC LinphoneCallLog * linphone_core_get_last_outgoing_call_log(LinphoneCore *lc);

/**
 * Get the call log matching the call id, or NULL if can't be found.
 * @param[in] lc LinphoneCore object
 * @param[in] call_id Call id of the call log to find
 * @return {LinphoneCallLog}
**/
LINPHONE_PUBLIC LinphoneCallLog * linphone_core_find_call_log_from_call_id(LinphoneCore *lc, const char *call_id);

/**
 * Erase the call log.
 * @param[in] lc LinphoneCore object
**/
LINPHONE_PUBLIC void linphone_core_clear_call_logs(LinphoneCore *lc);

/**
 * Get the number of missed calls.
 * Once checked, this counter can be reset with linphone_core_reset_missed_calls_count().
 * @param[in] lc #LinphoneCore object.
 * @return The number of missed calls.
**/
LINPHONE_PUBLIC int linphone_core_get_missed_calls_count(LinphoneCore *lc);

/**
 * Reset the counter of missed calls.
 * @param[in] lc #LinphoneCore object.
**/
LINPHONE_PUBLIC void linphone_core_reset_missed_calls_count(LinphoneCore *lc);

/**
 * Remove a specific call log from call history list.
 * This function destroys the call log object. It must not be accessed anymore by the application after calling this function.
 * @param[in] lc #LinphoneCore object
 * @param[in] call_log #LinphoneCallLog object to remove.
**/
LINPHONE_PUBLIC void linphone_core_remove_call_log(LinphoneCore *lc, LinphoneCallLog *call_log);

/**
 * Sets the database filename where call logs will be stored.
 * If the file does not exist, it will be created.
 * @ingroup initializing
 * @param lc the linphone core
 * @param path filesystem path
**/
LINPHONE_PUBLIC void linphone_core_set_call_logs_database_path(LinphoneCore *lc, const char *path);

/**
 * Gets the database filename where call logs will be stored.
 * @ingroup initializing
 * @param lc the linphone core
 * @return filesystem path
**/
LINPHONE_PUBLIC const char * linphone_core_get_call_logs_database_path(LinphoneCore *lc);

/**
 * Migrates the call logs from the linphonerc to the database if not done yet
 * @ingroup initializing
 * @param lc the linphone core
**/
LINPHONE_PUBLIC void linphone_core_migrate_logs_from_rc_to_db(LinphoneCore *lc);

/**
 * @}
**/

/**
 * Tells whether VCARD support is builtin.
 * @return TRUE if VCARD is supported, FALSE otherwise.
 * @ingroup misc
 */
LINPHONE_PUBLIC bool_t linphone_core_vcard_supported(void);

/**
 * Test if video is supported
 * @ingroup misc
**/
LINPHONE_PUBLIC bool_t linphone_core_video_supported(LinphoneCore *lc);

/**
 * Enables video globally.
 *
 * This function does not have any effect during calls. It just indicates LinphoneCore to
 * initiate future calls with video or not. The two boolean parameters indicate in which
 * direction video is enabled. Setting both to false disables video entirely.
 *
 * @param lc The LinphoneCore object
 * @param vcap_enabled indicates whether video capture is enabled
 * @param display_enabled indicates whether video display should be shown
 * @ingroup media_parameters
 * @deprecated Use #linphone_core_enable_video_capture and #linphone_core_enable_video_display instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_enable_video(LinphoneCore *lc, bool_t vcap_enabled, bool_t display_enabled);

/**
 * Returns TRUE if either capture or display is enabled, FALSE otherwise.
 * same as  ( #linphone_core_video_capture_enabled | #linphone_core_video_display_enabled )
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC  bool_t linphone_core_video_enabled(LinphoneCore *lc);

/**
 * Enable or disable video capture.
 *
 * This function does not have any effect during calls. It just indicates the #LinphoneCore to
 * initiate future calls with video capture or not.
 * @param[in] lc #LinphoneCore object.
 * @param[in] enable TRUE to enable video capture, FALSE to disable it.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_video_capture(LinphoneCore *lc, bool_t enable);

/**
 * Enable or disable video display.
 *
 * This function does not have any effect during calls. It just indicates the #LinphoneCore to
 * initiate future calls with video display or not.
 * @param[in] lc #LinphoneCore object.
 * @param[in] enable TRUE to enable video display, FALSE to disable it.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_video_display(LinphoneCore *lc, bool_t enable);

/**
 * Enable or disable video source reuse when switching from preview to actual video call.
 *
 * This source reuse is useful when you always display the preview, even before calls are initiated.
 * By keeping the video source for the transition to a real video call, you will smooth out the
 * source close/reopen cycle.
 *
 * This function does not have any effect durfing calls. It just indicates the #LinphoneCore to
 * initiate future calls with video source reuse or not.
 * Also, at the end of a video call, the source will be closed whatsoever for now.
 * @param[in] lc #LinphoneCore object
 * @param[in] enable TRUE to enable video source reuse. FALSE to disable it for subsequent calls.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_enable_video_source_reuse(LinphoneCore* lc, bool_t enable);

/**
 * Tells whether video capture is enabled.
 * @param[in] lc #LinphoneCore object.
 * @return TRUE if video capture is enabled, FALSE if disabled.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_video_capture_enabled(LinphoneCore *lc);

/**
 * Tells whether video display is enabled.
 * @param[in] lc #LinphoneCore object.
 * @return TRUE if video display is enabled, FALSE if disabled.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_video_display_enabled(LinphoneCore *lc);

/**
 * Sets the default policy for video.
 * This policy defines whether:
 * - video shall be initiated by default for outgoing calls
 * - video shall be accepter by default for incoming calls
 *
 * @param[in] lc LinphoneCore object
 * @param[in] policy The video policy to use
 * @ingroup media_parameters
 * @deprecated
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_video_policy(LinphoneCore *lc, const LinphoneVideoPolicy *policy);

/**
 * Get the default policy for video.
 * See linphone_core_set_video_policy() for more details.
 * @param[in] lc LinphoneCore object
 * @return The video policy being used
 * @ingroup media_parameters
 * @deprecated
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED const LinphoneVideoPolicy *linphone_core_get_video_policy(const LinphoneCore *lc);

/**
 * Increment refcount.
 * @param[in] policy LinphoneVideoActivationPolicy object
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneVideoActivationPolicy *linphone_video_activation_policy_ref(LinphoneVideoActivationPolicy *policy);

/**
 * Decrement refcount and possibly free the object.
 * @param[in] policy LinphoneVideoActivationPolicy object
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_video_activation_policy_unref(LinphoneVideoActivationPolicy *policy);

/**
 * Gets the user data in the LinphoneVideoActivationPolicy object
 * @param[in] policy the LinphoneVideoActivationPolicy
 * @return the user data
 * @ingroup media_parameters
*/
LINPHONE_PUBLIC void *linphone_video_activation_policy_get_user_data(const LinphoneVideoActivationPolicy *policy);

/**
 * Sets the user data in the LinphoneVideoActivationPolicy object
 * @param[in] policy the LinphoneVideoActivationPolicy object
 * @param[in] data the user data
 * @ingroup media_parameters
*/
LINPHONE_PUBLIC void linphone_video_activation_policy_set_user_data(LinphoneVideoActivationPolicy *policy, void *data);

/**
 * Gets the value for the automatically accept video policy
 * @param[in] policy the LinphoneVideoActivationPolicy object
 * @return whether or not to automatically accept video requests is enabled
 * @ingroup media_parameters
*/
LINPHONE_PUBLIC bool_t linphone_video_activation_policy_get_automatically_accept(const LinphoneVideoActivationPolicy *policy);

/**
 * Gets the value for the automatically initiate video policy
 * @param[in] policy the LinphoneVideoActivationPolicy object
 * @return whether or not to automatically initiate video calls is enabled
 * @ingroup media_parameters
*/
LINPHONE_PUBLIC bool_t linphone_video_activation_policy_get_automatically_initiate(const LinphoneVideoActivationPolicy *policy);

/**
 * Sets the value for the automatically accept video policy
 * @param[in] policy the LinphoneVideoActivationPolicy object
 * @param[in] enable whether or not to enable automatically accept video requests
 * @ingroup media_parameters
*/
LINPHONE_PUBLIC void linphone_video_activation_policy_set_automatically_accept(LinphoneVideoActivationPolicy *policy, bool_t enable);

/**
 * Sets the value for the automatically initiate video policy
 * @param[in] policy the LinphoneVideoActivationPolicy object
 * @param[in] enable whether or not to enable automatically initiate video calls
 * @ingroup media_parameters
*/
LINPHONE_PUBLIC void linphone_video_activation_policy_set_automatically_initiate(LinphoneVideoActivationPolicy *policy, bool_t enable);

/**
 * Sets the default policy for video.
 * This policy defines whether:
 * - video shall be initiated by default for outgoing calls
 * - video shall be accepted by default for incoming calls
 * @param[in] lc LinphoneCore object
 * @param[in] policy The video policy to use
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_video_activation_policy(LinphoneCore *lc, const LinphoneVideoActivationPolicy *policy);

/**
 * Get the default policy for video.
 * See linphone_core_set_video_activation_policy() for more details.
 * @param[in] lc LinphoneCore object
 * @return The video policy being used
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneVideoActivationPolicy *linphone_core_get_video_activation_policy(const LinphoneCore *lc);

/**
 * Returns the zero terminated table of supported video resolutions.
 * @ingroup media_parameters
 * @deprecated Use linphone_factory_get_supported_video_definitions() instead
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED const MSVideoSizeDef *linphone_core_get_supported_video_sizes(LinphoneCore *lc);

/**
 * Set the preferred video definition for the stream that is captured and sent to the remote party.
 * All standard video definitions are accepted on the receive path.
 * @param[in] lc LinphoneCore object
 * @param[in] vdef LinphoneVideoDefinition object
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_preferred_video_definition(LinphoneCore *lc, LinphoneVideoDefinition *vdef);

/**
 * Sets the preferred video size.
 *
 * This applies only to the stream that is captured and sent to the remote party,
 * since we accept all standard video size on the receive path.
 * @ingroup media_parameters
 * @deprecated Use linphone_core_set_preferred_video_definition() instead
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_preferred_video_size(LinphoneCore *lc, MSVideoSize vsize);

/**
 * Set the video definition for the captured (preview) video.
 * This method is for advanced usage where a video capture must be set independently of the definition of the stream actually sent through the call.
 * This allows for example to have the preview window in High Definition  even if due to bandwidth constraint the sent video definition is small.
 * Using this feature increases the CPU consumption, since a rescaling will be done internally.
 * @param[in] lc LinphoneCore object
 * @param[in] vdef LinphoneVideoDefinition object
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_preview_video_definition(LinphoneCore *lc, LinphoneVideoDefinition *vdef);

/**
 * Sets the video size for the captured (preview) video.
 * This method is for advanced usage where a video capture must be set independently of the size of the stream actually sent through the call.
 * This allows for example to have the preview window with HD resolution even if due to bandwidth constraint the sent video size is small.
 * Using this feature increases the CPU consumption, since a rescaling will be done internally.
 * @ingroup media_parameters
 * @param lc the linphone core
 * @param vsize the video resolution choosed for capuring and previewing. It can be (0,0) to not request any specific preview size and let the core optimize the processing.
 * @deprecated Use linphone_core_set_preview_video_definition() instead
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_preview_video_size(LinphoneCore *lc, MSVideoSize vsize);

/**
 * Sets the preview video size by its name. See linphone_core_set_preview_video_size() for more information about this feature.
 *
 * Video resolution names are: qcif, svga, cif, vga, 4cif, svga ...
 * @ingroup media_parameters
 * @deprecated Use linphone_factory_create_video_definition_from_name() and linphone_core_set_preview_video_definition() instead
**/
LINPHONE_PUBLIC void linphone_core_set_preview_video_size_by_name(LinphoneCore *lc, const char *name);

/**
 * Get the definition of the captured video.
 * @param[in] lc LinphoneCore object
 * @return The captured LinphoneVideoDefinition if it was previously set by linphone_core_set_preview_video_definition(), otherwise a 0x0 LinphoneVideoDefinition.
 * @see linphone_core_set_preview_video_definition()
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC const LinphoneVideoDefinition * linphone_core_get_preview_video_definition(const LinphoneCore *lc);

/**
 * Returns video size for the captured video if it was previously set by linphone_core_set_preview_video_size(), otherwise returns a 0,0 size.
 * @see linphone_core_set_preview_video_size()
 * @ingroup media_parameters
 * @param lc the core
 * @return a MSVideoSize
 * @deprecated Use linphone_core_get_preview_video_definition() instead
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED MSVideoSize linphone_core_get_preview_video_size(const LinphoneCore *lc);

/**
 * Get the effective video definition provided by the camera for the captured video.
 * When preview is disabled or not yet started this function returns a 0x0 video definition.
 * @param[in] lc LinphoneCore object
 * @return The captured LinphoneVideoDefinition
 * @ingroup media_parameters
 * @see linphone_core_set_preview_video_definition()
 */
LINPHONE_PUBLIC LinphoneVideoDefinition * linphone_core_get_current_preview_video_definition(const LinphoneCore *lc);

/**
 * Returns the effective video size for the captured video as provided by the camera.
 * When preview is disabled or not yet started, this function returns a zeroed video size.
 * @see linphone_core_set_preview_video_size()
 * @ingroup media_parameters
 * @param lc the core
 * @return a MSVideoSize
 * @deprecated Use linphone_core_get_current_preview_video_definition() instead
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED MSVideoSize linphone_core_get_current_preview_video_size(const LinphoneCore *lc);

/**
 * Get the preferred video definition for the stream that is captured and sent to the remote party.
 * @param[in] lc LinphoneCore object
 * @return The preferred LinphoneVideoDefinition
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC const LinphoneVideoDefinition * linphone_core_get_preferred_video_definition(const LinphoneCore *lc);

/**
 * Returns the current preferred video size for sending.
 * @ingroup media_parameters
 * @deprecated Use linphone_core_get_preferred_video_definition() instead
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED MSVideoSize linphone_core_get_preferred_video_size(const LinphoneCore *lc);

/**
 * Get the name of the current preferred video size for sending.
 * @param[in] lc #LinphoneCore object.
 * @return A string containing the name of the current preferred video size (to be freed with ms_free()).
 * @deprecated Use linphone_core_get_preferred_video_defintion() and linphone_video_definition_get_name() instead
 */
LINPHONE_PUBLIC char * linphone_core_get_preferred_video_size_name(const LinphoneCore *lc);

/**
 * Sets the preferred video size by its name.
 *
 * This is identical to linphone_core_set_preferred_video_size() except
 * that it takes the name of the video resolution as input.
 * Video resolution names are: qcif, svga, cif, vga, 4cif, svga ...
 * @ingroup media_parameters
 * @deprecated Use linphone_factory_create_video_definition_from_name() and linphone_core_set_preferred_video_definition() instead
**/
LINPHONE_PUBLIC void linphone_core_set_preferred_video_size_by_name(LinphoneCore *lc, const char *name);

/**
 * Set the preferred frame rate for video.
 * Based on the available bandwidth constraints and network conditions, the video encoder
 * remains free to lower the framerate. There is no warranty that the preferred frame rate be the actual framerate.
 * used during a call. Default value is 0, which means "use encoder's default fps value".
 * @param lc the LinphoneCore
 * @param fps the target frame rate in number of frames per seconds.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_preferred_framerate(LinphoneCore *lc, float fps);

/**
 * Returns the preferred video framerate, previously set by linphone_core_set_preferred_framerate().
 * @param lc the linphone core
 * @return frame rate in number of frames per seconds.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC float linphone_core_get_preferred_framerate(LinphoneCore *lc);

/**
 * Call generic OpenGL render for a given core.
 * @param lc The core.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_preview_ogl_render(const LinphoneCore *lc);

/**
 * Controls video preview enablement.
 * @param[in] lc LinphoneCore object
 * @param[in] val A boolean value telling whether the video preview is to be shown
 * Video preview refers to the action of displaying the local webcam image
 * to the user while not in call.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_video_preview(LinphoneCore *lc, bool_t val);

/**
 * Tells whether video preview is enabled.
 * @param[in] lc LinphoneCore object
 * @return A boolean value telling whether video preview is enabled
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_video_preview_enabled(const LinphoneCore *lc);

/**
 * Take a photo of currently from capture device and write it into a jpeg file.
 * Note that the snapshot is asynchronous, an application shall not assume that the file is created when the function returns.
 * @ingroup misc
 * @param lc the linphone core
 * @param file a path where to write the jpeg content.
 * @return 0 if successfull, -1 otherwise (typically if jpeg format is not supported).
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_take_preview_snapshot(LinphoneCore *lc, const char *file);

/**
 * Enables or disable self view during calls.
 * @param[in] lc LinphoneCore object
 * @param[in] val A boolean value telling whether to enable self view
 * Self-view refers to having local webcam image inserted in corner
 * of the video window during calls.
 * This function works at any time, including during calls.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_self_view(LinphoneCore *lc, bool_t val);

/**
 * Tells whether video self view during call is enabled or not.
 * @param[in] lc LinphoneCore object
 * @return A boolean value telling whether self view is enabled
 * @see linphone_core_enable_self_view() for details.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_self_view_enabled(const LinphoneCore *lc);

/**
 * Update detection of camera devices.
 *
 * Use this function when the application is notified of USB plug events, so that
 * list of available hardwares for video capture is updated.
 * @param[in] lc #LinphoneCore object.
 * @ingroup media_parameters
 **/
LINPHONE_PUBLIC void linphone_core_reload_video_devices(LinphoneCore *lc);

/**
 * Gets the list of the available video capture devices.
 * @param[in] lc LinphoneCore object
 * @return An unmodifiable array of strings contanining the names of the available video capture devices that is NULL terminated
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char**  linphone_core_get_video_devices(const LinphoneCore *lc);

/**
 * Sets the active video device.
 * @param[in] lc LinphoneCore object
 * @param id The name of the video device to use as returned by linphone_core_get_video_devices()
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_video_device(LinphoneCore *lc, const char *id);

/**
 * Returns the name of the currently active video device.
 * @param[in] lc LinphoneCore object
 * @return The name of the currently active video device
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char *linphone_core_get_video_device(const LinphoneCore *lc);

/**
 * Set the path to the image file to stream when "Static picture" is set as the video device.
 * @param[in] lc #LinphoneCore object.
 * @param[in] path The path to the image file to use.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_static_picture(LinphoneCore *lc, const char *path);

/**
 * Get the path to the image file streamed when "Static picture" is set as the video device.
 * @param[in] lc #LinphoneCore object.
 * @return The path to the image file streamed when "Static picture" is set as the video device.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC const char *linphone_core_get_static_picture(LinphoneCore *lc);

/**
 * Set the frame rate for static picture.
 * @param[in] lc #LinphoneCore object.
 * @param[in] fps The new frame rate to use for static picture.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_static_picture_fps(LinphoneCore *lc, float fps);

/**
 * Get the frame rate for static picture
 * @param[in] lc #LinphoneCore object.
 * @return The frame rate used for static picture.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC float linphone_core_get_static_picture_fps(LinphoneCore *lc);

/**
 * Get the native window handle of the video window.
 * @param[in] lc LinphoneCore object
 * @return The native window handle of the video window
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void * linphone_core_get_native_video_window_id(const LinphoneCore *lc);

/**
 * @ingroup media_parameters
 * For MacOS, Linux, Windows: core will create its own window
 */
#define LINPHONE_VIDEO_DISPLAY_AUTO (void*)((unsigned long) 0)

/**
 * @ingroup media_parameters
 * For MacOS, Linux, Windows: do nothing
 */
#define LINPHONE_VIDEO_DISPLAY_NONE (void*)((unsigned long) -1)

/**
 * @ingroup media_parameters
 * Set the native video window id where the video is to be displayed.
 * For MacOS, Linux, Windows: if not set or LINPHONE_VIDEO_DISPLAY_AUTO the core will create its own window, unless the special id LINPHONE_VIDEO_DISPLAY_NONE is given.
**/
LINPHONE_PUBLIC void linphone_core_set_native_video_window_id(LinphoneCore *lc, void *id);

/**
 * Get the native window handle of the video preview window.
 * @param[in] lc LinphoneCore object
 * @return The native window handle of the video preview window
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void * linphone_core_get_native_preview_window_id(const LinphoneCore *lc);

/**
 * Set the native window id where the preview video (local camera) is to be displayed.
 * This has to be used in conjonction with linphone_core_use_preview_window().
 * MacOS, Linux, Windows: if not set or zero the core will create its own window, unless the special id -1 is given.
 * @param[in] lc LinphoneCore object
 * @param[in] id The native window id where the preview video is to be displayed
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_native_preview_window_id(LinphoneCore *lc, void *id);

/**
 * Tells the core to use a separate window for local camera preview video, instead of
 * inserting local view within the remote video window.
 * @param[in] lc #LinphoneCore object.
 * @param[in] yesno TRUE to use a separate window, FALSE to insert the preview in the remote video window.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_use_preview_window(LinphoneCore *lc, bool_t yesno);

/**
 * Gets the current device orientation.
 * @param[in] lc LinphoneCore object
 * @return The current device orientation
 * @ingroup media_parameters
 * @see linphone_core_set_device_rotation()
 */
LINPHONE_PUBLIC int linphone_core_get_device_rotation(LinphoneCore *lc);

/**
 * Tells the core the device current orientation. This can be used by capture filters
 * on mobile devices to select between portrait/landscape mode and to produce properly
 * oriented images. The exact meaning of the value in rotation if left to each device
 * specific implementations.
 * IOS supported values are 0 for UIInterfaceOrientationPortrait and 270 for UIInterfaceOrientationLandscapeRight.
 * @param[in] lc LinphoneCore object
 * @param[in] rotation The orientation to use
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_device_rotation(LinphoneCore *lc, int rotation);

/**
 * Get the camera sensor rotation.
 *
 * This is needed on some mobile platforms to get the number of degrees the camera sensor
 * is rotated relative to the screen.
 * @param lc The linphone core related to the operation
 * @ingroup media_parameters
 * @return The camera sensor rotation in degrees (0 to 360) or -1 if it could not be retrieved
 */
LINPHONE_PUBLIC int linphone_core_get_camera_sensor_rotation(LinphoneCore *lc);

/**
 * Start or stop streaming video in case of embedded window.
 * Can be used to disable video showing to free XV port
**/
void linphone_core_show_video(LinphoneCore *lc, bool_t show);

/** @deprecated Use linphone_core_set_use_files() instead. */
#define linphone_core_use_files(lc, yesno) linphone_core_set_use_files(lc, yesno)

/**
 * Ask the core to stream audio from and to files, instead of using the soundcard.
 * @param[in] lc LinphoneCore object
 * @param[in] yesno A boolean value asking to stream audio from and to files or not.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_use_files(LinphoneCore *lc, bool_t yesno);

/**
 * Gets whether linphone is currently streaming audio from and to files, rather
 * than using the soundcard.
 * @param[in] lc LinphoneCore object
 * @return A boolean value representing whether linphone is streaming audio from and to files or not.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_get_use_files(LinphoneCore *lc);

/**
 * Get the wav file that is played when putting somebody on hold,
 * or when files are used instead of soundcards (see linphone_core_set_use_files()).
 *
 * The file is a 16 bit linear wav file.
 * @param[in] lc LinphoneCore object
 * @return The path to the file that is played when putting somebody on hold.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC const char * linphone_core_get_play_file(const LinphoneCore *lc);

/**
 * Sets a wav file to be played when putting somebody on hold,
 * or when files are used instead of soundcards (see linphone_core_set_use_files()).
 *
 * The file must be a 16 bit linear wav file.
 * @param[in] lc LinphoneCore object
 * @param[in] file The path to the file to be played when putting somebody on hold.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_play_file(LinphoneCore *lc, const char *file);

/**
 * Get the wav file where incoming stream is recorded,
 * when files are used instead of soundcards (see linphone_core_set_use_files()).
 *
 * This feature is different from call recording (linphone_call_params_set_record_file())
 * The file is a 16 bit linear wav file.
 * @param[in] lc LinphoneCore object
 * @return The path to the file where incoming stream is recorded.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char * linphone_core_get_record_file(const LinphoneCore *lc);

/**
 * Sets a wav file where incoming stream is to be recorded,
 * when files are used instead of soundcards (see linphone_core_set_use_files()).
 *
 * This feature is different from call recording (linphone_call_params_set_record_file())
 * The file will be a 16 bit linear wav file.
 * @param[in] lc LinphoneCore object
 * @param[in] file The path to the file where incoming stream is to be recorded.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_record_file(LinphoneCore *lc, const char *file);

/**
 * Plays a dtmf sound to the local user.
 * @param[in] lc LinphoneCore object
 * @param[in] dtmf DTMF to play ['0'..'16'] | '#' | '#'
 * @param[in] duration_ms Duration in ms, -1 means play until next further call to #linphone_core_stop_dtmf()
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_play_dtmf(LinphoneCore *lc, char dtmf, int duration_ms);

/**
 * Stops playing a dtmf started by linphone_core_play_dtmf().
 * @param[in] lc LinphoneCore object
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_stop_dtmf(LinphoneCore *lc);

LINPHONE_PUBLIC int linphone_core_get_current_call_duration(const LinphoneCore *lc);

/**
 * Returns the maximum transmission unit size in bytes.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_mtu(const LinphoneCore *lc);

/**
 * Sets the maximum transmission unit size in bytes.
 * This information is useful for sending RTP packets.
 * Default value is 1500.
 * @param[in] lc LinphoneCore object
 * @param[in] mtu The MTU in bytes
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_mtu(LinphoneCore *lc, int mtu);

/**
 * This method is called by the application to notify the linphone core library when network is reachable.
 * Calling this method with true trigger linphone to initiate a registration process for all proxies.
 * Calling this method disables the automatic network detection mode. It means you must call this method after each network state changes.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_network_reachable(LinphoneCore* lc,bool_t value);

/**
 * return network state either as positioned by the application or by linphone itself.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_is_network_reachable(LinphoneCore* lc);

/**
 * This method is called by the application to notify the linphone core library when the SIP network is reachable.
 * This is for advanced usage, when SIP and RTP layers are required to use different interfaces.
 * Most applications just need linphone_core_set_network_reachable().
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_sip_network_reachable(LinphoneCore* lc,bool_t value);

/**
 * This method is called by the application to notify the linphone core library when the media (RTP) network is reachable.
 * This is for advanced usage, when SIP and RTP layers are required to use different interfaces.
 * Most applications just need linphone_core_set_network_reachable().
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_media_network_reachable(LinphoneCore* lc,bool_t value);

/**
 * Enables signaling keep alive, small udp packet sent periodically to keep udp NAT association.
 * @param[in] lc LinphoneCore object
 * @param[in] enable A boolean value telling whether signaling keep alive is to be enabled
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_enable_keep_alive(LinphoneCore* lc, bool_t enable);

/**
 * Is signaling keep alive enabled.
 * @param[in] lc LinphoneCore object
 * @return A boolean value telling whether signaling keep alive is enabled
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_keep_alive_enabled(LinphoneCore* lc);

/**
 * Retrieves the user pointer that was given to linphone_core_new()
 * @param[in] lc LinphoneCore object
 * @return The user data associated with the LinphoneCore object
 * @ingroup initializing
**/
LINPHONE_PUBLIC void *linphone_core_get_user_data(const LinphoneCore *lc);

/**
 * Associate a user pointer to the linphone core.
 * @param[in] lc LinphoneCore object
 * @param[in] userdata The user data to associate with the LinphoneCore object
 * @ingroup initializing
**/
LINPHONE_PUBLIC void linphone_core_set_user_data(LinphoneCore *lc, void *userdata);

/**
 * Returns the LpConfig object used to manage the storage (config) file.
 * @param[in] lc #LinphoneCore object
 * The application can use the LpConfig object to insert its own private
 * sections and pairs of key=value in the configuration file.
 * @ingroup misc
**/
LINPHONE_PUBLIC LinphoneConfig * linphone_core_get_config(LinphoneCore *lc);

/**
 * Create a LpConfig object from a user config file.
 * @param[in] lc #LinphoneCore object
 * @param[in] filename The filename of the config file to read to fill the instantiated LpConfig
 * @ingroup misc
 * @deprecated Use linphone_core_create_config() instead.
 * @donotwrap
 */
LINPHONE_DEPRECATED LINPHONE_PUBLIC LinphoneConfig * linphone_core_create_lp_config(LinphoneCore *lc, const char *filename);

/**
 * Create a #LinphoneConfig object from a user config file.
 * @param[in] lc #LinphoneCore object
 * @param[in] filename The filename of the config file to read to fill the instantiated #LinphoneConfig
 * @ingroup misc
 */
LINPHONE_PUBLIC LinphoneConfig * linphone_core_create_config(LinphoneCore *lc, const char *filename);

/**
 * Set a callback for some blocking operations, it takes you informed of the progress of the operation
 */
LINPHONE_PUBLIC void linphone_core_set_waiting_callback(LinphoneCore *lc, LinphoneCoreWaitingCallback cb, void *user_context);

/**
 * Returns the list of registered SipSetup (linphonecore plugins)
 */
LINPHONE_PUBLIC const bctbx_list_t * linphone_core_get_sip_setups(LinphoneCore *lc);

/**
 * Destroys a LinphoneCore
 * @param[in] lc LinphoneCore object
 * @ingroup initializing
 * @deprecated Use linphone_core_unref() instead.
 * @donotwrap
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC void linphone_core_destroy(LinphoneCore *lc);

/*for advanced users:*/
typedef RtpTransport * (*LinphoneCoreRtpTransportFactoryFunc)(void *data, int port);
struct _LinphoneRtpTransportFactories{
	LinphoneCoreRtpTransportFactoryFunc audio_rtp_func;
	void *audio_rtp_func_data;
	LinphoneCoreRtpTransportFactoryFunc audio_rtcp_func;
	void *audio_rtcp_func_data;
	LinphoneCoreRtpTransportFactoryFunc video_rtp_func;
	void *video_rtp_func_data;
	LinphoneCoreRtpTransportFactoryFunc video_rtcp_func;
	void *video_rtcp_func_data;
};
typedef struct _LinphoneRtpTransportFactories LinphoneRtpTransportFactories;

void linphone_core_set_rtp_transport_factories(LinphoneCore* lc, LinphoneRtpTransportFactories *factories);

/**
 * Retrieve RTP statistics regarding current call.
 * @param[in] lc LinphoneCore object
 * @param[out] local RTP statistics computed locally.
 * @param[out] remote RTP statistics computed by far end (obtained via RTCP feedback).
 * @return 0 or -1 if no call is running.
 * @note Remote RTP statistics is not implemented yet.
**/
int linphone_core_get_current_call_stats(LinphoneCore *lc, rtp_stats_t *local, rtp_stats_t *remote);

/**
 * Get the number of Call
 * @param[in] lc LinphoneCore object
 * @return The current number of calls
 * @ingroup call_control
**/
LINPHONE_PUBLIC int linphone_core_get_calls_nb(const LinphoneCore *lc);

/**
 * Gets the current list of calls.
 * Note that this list is read-only and might be changed by the core after a function call to linphone_core_iterate().
 * Similarly the LinphoneCall objects inside it might be destroyed without prior notice.
 * To hold references to LinphoneCall object into your program, you must use linphone_call_ref().
 * @param[in] lc The LinphoneCore object
 * @return \bctbx_list{LinphoneCall}
 * @ingroup call_control
**/
LINPHONE_PUBLIC const bctbx_list_t *linphone_core_get_calls(LinphoneCore *lc);

LINPHONE_PUBLIC LinphoneGlobalState linphone_core_get_global_state(const LinphoneCore *lc);

/**
 * force registration refresh to be initiated upon next iterate
 * @ingroup proxies
 */
LINPHONE_PUBLIC void linphone_core_refresh_registers(LinphoneCore* lc);

/**
 * Set the path to the file storing the zrtp secrets cache.
 * @param[in] lc #LinphoneCore object
 * @param[in] file The path to the file to use to store the zrtp secrets cache.
 * @ingroup initializing
 */
LINPHONE_PUBLIC void linphone_core_set_zrtp_secrets_file(LinphoneCore *lc, const char* file);

/**
 * Get the path to the file storing the zrtp secrets cache.
 * @param[in] lc #LinphoneCore object.
 * @return The path to the file storing the zrtp secrets cache.
 * @ingroup initializing
 */
LINPHONE_PUBLIC const char *linphone_core_get_zrtp_secrets_file(LinphoneCore *lc);

/**
 * Get a pointer to the sqlite db holding zrtp/lime cache
 * @param[in] lc #LinphoneCore object.
 * @return An sqlite3 pointer cast to a void one or NULL if cache is not available(not enabled at compile or access failed)
 * @ingroup initializing
 */
LINPHONE_PUBLIC void *linphone_core_get_zrtp_cache_db(LinphoneCore *lc);

/**
 * Set the path to the directory storing the user's x509 certificates (used by dtls)
 * @param[in] lc #LinphoneCore object
 * @param[in] path The path to the directory to use to store the user's certificates.
 * @ingroup initializing
 */
LINPHONE_PUBLIC void linphone_core_set_user_certificates_path(LinphoneCore *lc, const char* path);

/**
 * Get the path to the directory storing the user's certificates.
 * @param[in] lc #LinphoneCore object.
 * @returns The path to the directory storing the user's certificates.
 * @ingroup initializing
 */
LINPHONE_PUBLIC const char *linphone_core_get_user_certificates_path(LinphoneCore *lc);

/**
 * Reload mediastreamer2 plugins from specified directory.
 * @param[in] lc #LinphoneCore object.
 * @param[in] path the path from where plugins are to be loaded, pass NULL to use default (compile-time determined) plugin directory.
 * @ingroup initializing
 */
LINPHONE_PUBLIC void linphone_core_reload_ms_plugins(LinphoneCore *lc, const char *path);

/**
 * Search from the list of current calls if a remote address match uri
 * @ingroup call_control
 * @param lc
 * @param uri which should match call remote uri
 * @return LinphoneCall or NULL is no match is found
 */
LINPHONE_PUBLIC LinphoneCall* linphone_core_find_call_from_uri(const LinphoneCore *lc, const char *uri);

/**
 * @addtogroup call_control
 * @{
 */

/**
 * Create some default conference parameters for instanciating a a conference with linphone_core_create_conference_with_params().
 * @param lc the core
 * @return conference parameters.
**/
LINPHONE_PUBLIC LinphoneConferenceParams * linphone_core_create_conference_params(LinphoneCore *lc);


/**
 * Create a conference
 * @param lc The #LinphoneCore instance where the conference will be created inside.
 * @param params Parameters of the conference. See #LinphoneConferenceParams.
 * @return A pointer on the freshly created conference. That object will be automatically
 * freed by the core after calling linphone_core_terminate_conference().
 */
LINPHONE_PUBLIC LinphoneConference *linphone_core_create_conference_with_params(LinphoneCore *lc, const LinphoneConferenceParams *params);


/**
 * Add a participant to the conference. If no conference is going on
 * a new internal conference context is created and the participant is
 * added to it.
 * @param lc #LinphoneCore
 * @param call The current call with the participant to add
 * @return 0 if succeeded. Negative number if failed
 */
LINPHONE_PUBLIC LinphoneStatus linphone_core_add_to_conference(LinphoneCore *lc, LinphoneCall *call);

/**
 * Add all current calls into the conference. If no conference is running
 * a new internal conference context is created and all current calls
 * are added to it.
 * @param lc #LinphoneCore
 * @return 0 if succeeded. Negative number if failed
 */
LINPHONE_PUBLIC LinphoneStatus linphone_core_add_all_to_conference(LinphoneCore *lc);

/**
 * Remove a call from the conference.
 * @param lc the linphone core
 * @param call a call that has been previously merged into the conference.
 *
 * After removing the remote participant belonging to the supplied call, the call becomes a normal call in paused state.
 * If one single remote participant is left alone together with the local user in the conference after the removal, then the conference is
 * automatically transformed into a simple call in StreamsRunning state.
 * The conference's resources are then automatically destroyed.
 *
 * In other words, unless linphone_core_leave_conference() is explicitly called, the last remote participant of a conference is automatically
 * put in a simple call in running state.
 *
 * @return 0 if successful, -1 otherwise.
 **/
LINPHONE_PUBLIC LinphoneStatus linphone_core_remove_from_conference(LinphoneCore *lc, LinphoneCall *call);

/**
 * Indicates whether the local participant is part of a conference.
 * @warning That function automatically fails in the case of conferences using a
 * conferencet server (focus). If you use such a conference, you should use
 * linphone_conference_remove_participant() instead.
 * @param lc the linphone core
 * @return TRUE if the local participant is in a conference, FALSE otherwise.
*/
LINPHONE_PUBLIC bool_t linphone_core_is_in_conference(const LinphoneCore *lc);

/**
 * Join the local participant to the running conference
 * @param lc #LinphoneCore
 * @return 0 if succeeded. Negative number if failed
 */
LINPHONE_PUBLIC LinphoneStatus linphone_core_enter_conference(LinphoneCore *lc);

/**
 * Make the local participant leave the running conference
 * @param lc #LinphoneCore
 * @return 0 if succeeded. Negative number if failed
 */
LINPHONE_PUBLIC LinphoneStatus linphone_core_leave_conference(LinphoneCore *lc);

/**
 * Get the set input volume of the local participant
 * @param lc #LinphoneCore
 * @return A value inside [0.0 ; 1.0]
 */
LINPHONE_PUBLIC float linphone_core_get_conference_local_input_volume(LinphoneCore *lc);

/**
 * Terminate the running conference. If it is a local conference, all calls
 * inside it will become back separate calls and will be put in #LinphoneCallPaused state.
 * If it is a conference involving a focus server, all calls inside the conference
 * will be terminated.
 * @param lc #LinphoneCore
 * @return 0 if succeeded. Negative number if failed
 */
LINPHONE_PUBLIC LinphoneStatus linphone_core_terminate_conference(LinphoneCore *lc);

/**
 * Get the number of participant in the running conference. The local
 * participant is included in the count only if it is in the conference.
 * @param lc #LinphoneCore
 * @return The number of participant
 */
LINPHONE_PUBLIC int linphone_core_get_conference_size(LinphoneCore *lc);

/**
 * Start recording the running conference
 * @param lc #LinphoneCore
 * @param path Path to the file where the recording will be written
 * @return 0 if succeeded. Negative number if failed
 */
LINPHONE_PUBLIC LinphoneStatus linphone_core_start_conference_recording(LinphoneCore *lc, const char *path);

/**
 * Stop recording the running conference
 * @param lc #LinphoneCore
 * @return 0 if succeeded. Negative number if failed
 */
LINPHONE_PUBLIC LinphoneStatus linphone_core_stop_conference_recording(LinphoneCore *lc);

/**
 * Get a pointer on the internal conference object.
 * @param lc #LinphoneCore
 * @return A pointer on #LinphoneConference or NULL if no conference are going on
 */
LINPHONE_PUBLIC LinphoneConference *linphone_core_get_conference(LinphoneCore *lc);

/**
 * @}
 */

/**
 * Get the maximum number of simultaneous calls Linphone core can manage at a time. All new call above this limit are declined with a busy answer
 * @param lc core
 * @return max number of simultaneous calls
 * @ingroup initializing
 */
LINPHONE_PUBLIC int linphone_core_get_max_calls(LinphoneCore *lc);

/**
 * Set the maximum number of simultaneous calls Linphone core can manage at a time. All new call above this limit are declined with a busy answer
 * @param lc core
 * @param max number of simultaneous calls
 * @ingroup initializing
 */
LINPHONE_PUBLIC void linphone_core_set_max_calls(LinphoneCore *lc, int max);

/**
 * Check if a call will need the sound resources in near future (typically an outgoing call that is awaiting
 * response).
 * In liblinphone, it is not possible to have two independant calls using sound device or camera at the same time.
 * In order to prevent this situation, an application can use linphone_core_sound_resources_locked() to know whether
 * it is possible at a given time to start a new outgoing call.
 * When the function returns TRUE, an application should not allow the user to start an outgoing call.
 * @param[in] lc LinphoneCore object
 * @return A boolean value telling whether a call will need the sound resources in near future
 * @ingroup call_control
**/
LINPHONE_PUBLIC bool_t linphone_core_sound_resources_locked(LinphoneCore *lc);

/**
 * Check if a media encryption type is supported
 * @param lc core
 * @param menc LinphoneMediaEncryption
 * @return whether a media encryption scheme is supported by the LinphoneCore engine
 * @ingroup initializing
**/
LINPHONE_PUBLIC bool_t linphone_core_media_encryption_supported(const LinphoneCore *lc, LinphoneMediaEncryption menc);

/**
 * Choose the media encryption policy to be used for RTP packets.
 * @param[in] lc #LinphoneCore object.
 * @param[in] menc The media encryption policy to be used.
 * @return 0 if successful, any other value otherwise.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_media_encryption(LinphoneCore *lc, LinphoneMediaEncryption menc);

/**
 * Get the media encryption policy being used for RTP packets.
 * @param[in] lc #LinphoneCore object.
 * @return The media encryption policy being used.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC LinphoneMediaEncryption linphone_core_get_media_encryption(LinphoneCore *lc);

/**
 * Get behaviour when encryption parameters negociation fails on outgoing call.
 * @param[in] lc #LinphoneCore object.
 * @return TRUE means the call will fail; FALSE means an INVITE will be resent with encryption disabled.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_is_media_encryption_mandatory(LinphoneCore *lc);

/**
 * Define behaviour when encryption parameters negociation fails on outgoing call.
 * @param[in] lc #LinphoneCore object.
 * @param[in] m If set to TRUE call will fail; if set to FALSE will resend an INVITE with encryption disabled.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_media_encryption_mandatory(LinphoneCore *lc, bool_t m);

/**
 * Init call params using LinphoneCore's current configuration
 */
LINPHONE_PUBLIC void linphone_core_init_default_params(LinphoneCore*lc, LinphoneCallParams *params);

/**
 * True if tunnel support was compiled.
 * @ingroup tunnel
 */
LINPHONE_PUBLIC bool_t linphone_core_tunnel_available(void);

/**
 * get tunnel instance if available
 * @ingroup tunnel
 * @param lc core object
 * @returns LinphoneTunnel or NULL if not available
 */
LINPHONE_PUBLIC LinphoneTunnel *linphone_core_get_tunnel(const LinphoneCore *lc);

/**
 * Set the DSCP field for SIP signaling channel.
 * The DSCP defines the quality of service in IP packets.
 * @param[in] lc LinphoneCore object
 * @param[in] dscp The DSCP value to set
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_sip_dscp(LinphoneCore *lc, int dscp);

/**
 * Get the DSCP field for SIP signaling channel.
 * The DSCP defines the quality of service in IP packets.
 * @param[in] lc LinphoneCore object
 * @return The current DSCP value
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_sip_dscp(const LinphoneCore *lc);

/**
 * Set the DSCP field for outgoing audio streams.
 * The DSCP defines the quality of service in IP packets.
 * @param[in] lc LinphoneCore object
 * @param[in] dscp The DSCP value to set
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_audio_dscp(LinphoneCore *lc, int dscp);

/**
 * Get the DSCP field for outgoing audio streams.
 * The DSCP defines the quality of service in IP packets.
 * @param[in] lc LinphoneCore object
 * @return The current DSCP value
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_audio_dscp(const LinphoneCore *lc);

/**
 * Set the DSCP field for outgoing video streams.
 * The DSCP defines the quality of service in IP packets.
 * @param[in] lc LinphoneCore object
 * @param[in] dscp The DSCP value to set
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_video_dscp(LinphoneCore *lc, int dscp);

/**
 * Get the DSCP field for outgoing video streams.
 * The DSCP defines the quality of service in IP packets.
 * @param[in] lc LinphoneCore object
 * @return The current DSCP value
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_video_dscp(const LinphoneCore *lc);

/**
 * Get the name of the mediastreamer2 filter used for rendering video.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char *linphone_core_get_video_display_filter(LinphoneCore *lc);

/**
 * Set the name of the mediastreamer2 filter to be used for rendering video.
 * This is for advanced users of the library, mainly to workaround hardware/driver bugs.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_video_display_filter(LinphoneCore *lc, const char *filtername);

/**
 * Get the name of the mediastreamer2 filter used for echo cancelling.
 * @param[in] lc LinphoneCore object
 * @return The name of the mediastreamer2 filter used for echo cancelling
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC const char * linphone_core_get_echo_canceller_filter_name(const LinphoneCore *lc);

/**
 * Set the name of the mediastreamer2 filter to be used for echo cancelling.
 * This is for advanced users of the library.
 * @param[in] lc LinphoneCore object
 * @param[in] filtername The name of the mediastreamer2 filter to be used for echo cancelling
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_echo_canceller_filter_name(LinphoneCore *lc, const char *filtername);

/** Contact Providers
  */

typedef void (*ContactSearchCallback)( LinphoneContactSearch* id, bctbx_list_t* friends, void* data );

/**
 * Set URI where to download xml configuration file at startup.
 * This can also be set from configuration file or factory config file, from [misc] section, item "config-uri".
 * Calling this function does not load the configuration. It will write the value into configuration so that configuration
 * from remote URI will take place at next LinphoneCore start.
 * @param lc the linphone core
 * @param uri the http or https uri to use in order to download the configuration. Passing NULL will disable remote provisioning.
 * @return -1 if uri could not be parsed, 0 otherwise. Note that this does not check validity of URI endpoint nor scheme and download may still fail.
 * @ingroup initializing
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_provisioning_uri(LinphoneCore *lc, const char*uri);

/**
 * Get provisioning URI.
 * @param lc the linphone core
 * @return the provisioning URI.
 * @ingroup initializing
**/
LINPHONE_PUBLIC const char* linphone_core_get_provisioning_uri(const LinphoneCore *lc);

/**
 * Gets if the provisioning URI should be removed after it's been applied successfully
 * @param lc the linphone core
 * @return TRUE if the provisioning URI should be removed, FALSE otherwise
 */
LINPHONE_PUBLIC bool_t linphone_core_is_provisioning_transient(LinphoneCore *lc);

/**
 * Migrate configuration so that all SIP transports are enabled.
 * Versions of linphone < 3.7 did not support using multiple SIP transport simultaneously.
 * This function helps application to migrate the configuration so that all transports are enabled.
 * Existing proxy configuration are added a transport parameter so that they continue using the unique transport that was set previously.
 * This function must be used just after creating the core, before any call to linphone_core_iterate()
 * @param lc the linphone core
 * @return 1 if migration was done, 0 if not done because unnecessary or already done, -1 in case of error.
 * @ingroup initializing
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_migrate_to_multi_transport(LinphoneCore *lc);


/**
 * Control when media offer is sent in SIP INVITE.
 * @param lc the linphone core
 * @param enable true if INVITE has to be sent whitout SDP.
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_sdp_200_ack(LinphoneCore *lc, bool_t enable);

/**
 * Media offer control param for SIP INVITE.
 * @return true if INVITE has to be sent whitout SDP.
 * @ingroup network_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_sdp_200_ack_enabled(const LinphoneCore *lc);

/**
 * Assign an audio file to be played locally upon call failure, for a given reason.
 * @param lc the core
 * @param reason the #LinphoneReason representing the failure error code.
 * @param audiofile a wav file to be played when such call failure happens.
 * @ingroup misc
**/
LINPHONE_PUBLIC void linphone_core_set_call_error_tone(LinphoneCore *lc, LinphoneReason reason, const char *audiofile);

/**
 * Assign an audio file to be played as a specific tone id.
 * This function typically allows to customize telephony tones per country.
 * @param lc the core
 * @param id the tone id
 * @param audiofile a wav file to be played.
**/
LINPHONE_PUBLIC void linphone_core_set_tone(LinphoneCore *lc, LinphoneToneID id, const char *audiofile);

/**
 * Globaly set an http file transfer server to be used for content type application/vnd.gsma.rcs-ft-http+xml. This value can also be set for a dedicated account using #linphone_proxy_config_set_file_transfer_server
 * @param[in] core #LinphoneCore to be modified
 * @param[in] server_url URL of the file server like https://file.linphone.org/upload.php
 * @ingroup misc
 * */
LINPHONE_PUBLIC void linphone_core_set_file_transfer_server(LinphoneCore *core, const char * server_url);

/**
 * Get the globaly set http file transfer server to be used for content type application/vnd.gsma.rcs-ft-http+xml.
 * @param[in] core #LinphoneCore from which to get the server_url
 * @return URL of the file server like https://file.linphone.org/upload.php
 * @ingroup misc
 * */
LINPHONE_PUBLIC const char * linphone_core_get_file_transfer_server(LinphoneCore *core);

/**
 * Returns a null terminated table of strings containing the file format extension supported for call recording.
 * @param core the core
 * @return the supported formats, typically 'wav' and 'mkv'
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char ** linphone_core_get_supported_file_formats(LinphoneCore *core);

/**
 * Returns whether a specific file format is supported.
 * @see linphone_core_get_supported_file_formats
 * @param lc A #LinphoneCore object
 * @param fmt The format extension (wav, mkv).
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_file_format_supported(LinphoneCore *lc, const char *fmt);

/**
 * This function controls signaling features supported by the core.
 * They are typically included in a SIP Supported header.
 * @param[in] core LinphoneCore object
 * @param[in] tag The feature tag name
 * @ingroup initializing
**/
LINPHONE_PUBLIC void linphone_core_add_supported_tag(LinphoneCore *core, const char *tag);

/**
 * Remove a supported tag.
 * @param[in] core LinphoneCore object
 * @param[in] tag The tag to remove
 * @ingroup initializing
 * @see linphone_core_add_supported_tag()
**/
LINPHONE_PUBLIC void linphone_core_remove_supported_tag(LinphoneCore *core, const char *tag);

/**
 * Enable RTCP feedback (also known as RTP/AVPF profile).
 * Setting LinphoneAVPFDefault is equivalent to LinphoneAVPFDisabled.
 * This setting can be overriden per LinphoneProxyConfig with linphone_proxy_config_set_avpf_mode().
 * The value set here is used for calls placed or received out of any proxy configured, or if the proxy config is configured with LinphoneAVPFDefault.
 * @param[in] lc LinphoneCore object
 * @param[in] mode The AVPF mode to use.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_avpf_mode(LinphoneCore *lc, LinphoneAVPFMode mode);

/**
 * Return AVPF enablement. See linphone_core_set_avpf_mode() .
 * @param[in] lc LinphoneCore object
 * @return The current AVPF mode
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneAVPFMode linphone_core_get_avpf_mode(const LinphoneCore *lc);

/**
 * Set the avpf report interval in seconds.
 * This value can be overriden by the proxy config using linphone_proxy_config_set_avpf_rr_interval().
 * @param[in] lc LinphoneCore object
 * @param[in] interval The report interval in seconds
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_set_avpf_rr_interval(LinphoneCore *lc, int interval);

/**
 * Return the avpf report interval in seconds.
 * @param[in] lc LinphoneCore object
 * @return The current AVPF report interval in seconds
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_avpf_rr_interval(const LinphoneCore *lc);

/**
 * Use to set multicast address to be used for audio stream.
 * @param core #LinphoneCore
 * @param ip an ipv4/6 multicast address
 * @return 0 in case of success
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_audio_multicast_addr(LinphoneCore *lc, const char *ip);

/**
 * Use to set multicast address to be used for video stream.
 * @param lc #LinphoneCore
 * @param ip an ipv4/6 multicast address
 * @return 0 in case of success
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_video_multicast_addr(LinphoneCore *lc, const char *ip);

/**
 * Use to get multicast address to be used for audio stream.
 * @param core #LinphoneCore
 * @return an ipv4/6 multicast address or default value
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char* linphone_core_get_audio_multicast_addr(const LinphoneCore *core);

/**
 * Use to get multicast address to be used for video stream.
 * @param core #LinphoneCore
 * @return an ipv4/6 multicast address, or default value
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char* linphone_core_get_video_multicast_addr(const LinphoneCore *core);

/**
 * Use to set multicast ttl to be used for audio stream.
 * @param core #LinphoneCore
 * @param ttl value or -1 if not used. [0..255] default value is 1
 * @return 0 in case of success
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_audio_multicast_ttl(LinphoneCore *core, int ttl);

/**
 * Use to set multicast ttl to be used for video stream.
 * @param lc #LinphoneCore
 * @param  ttl value or -1 if not used. [0..255] default value is 1
 * @return 0 in case of success
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_video_multicast_ttl(LinphoneCore *lc, int ttl);

/**
 * Use to get multicast ttl to be used for audio stream.
 * @param core #LinphoneCore
 * @return a time to leave value
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_audio_multicast_ttl(const LinphoneCore *core);

/**
 * Use to get multicast ttl to be used for video stream.
 * @param core #LinphoneCore
 * @return a time to leave value
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC int linphone_core_get_video_multicast_ttl(const LinphoneCore *core);

/**
 * Use to enable multicast rtp for audio stream.
 * If enabled, outgoing calls put a multicast address from #linphone_core_get_video_multicast_addr into audio cline. In case of outgoing call audio stream is sent to this multicast address.
 * For incoming calls behavior is unchanged.
 * @param core #LinphoneCore
 * @param yesno if yes, subsequent calls will propose multicast ip set by #linphone_core_set_audio_multicast_addr
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_audio_multicast(LinphoneCore *core, bool_t yesno);

/**
 * Use to get multicast state of audio stream.
 * @param core #LinphoneCore
 * @return true if  subsequent calls will propose multicast ip set by #linphone_core_set_audio_multicast_addr
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_audio_multicast_enabled(const LinphoneCore *core);

/**
 * Use to enable multicast rtp for video stream.
 * If enabled, outgoing calls put a multicast address from #linphone_core_get_video_multicast_addr into video cline. In case of outgoing call video stream is sent to this  multicast address.
 * For incoming calls behavior is unchanged.
 * @param core #LinphoneCore
 * @param yesno if yes, subsequent outgoing calls will propose multicast ip set by #linphone_core_set_video_multicast_addr
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_video_multicast(LinphoneCore *core, bool_t yesno);

/**
 * Use to get multicast state of video stream.
 * @param core #LinphoneCore
 * @return true if  subsequent calls will propose multicast ip set by #linphone_core_set_video_multicast_addr
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_video_multicast_enabled(const LinphoneCore *core);

/**
 * Set the network simulator parameters.
 * Liblinphone has the capabability of simulating the effects of a network (latency, lost packets, jitter, max bandwidth).
 * Please refer to the oRTP documentation for the meaning of the parameters of the OrtpNetworkSimulatorParams structure.
 * This function has effect for future calls, but not for currently running calls, though this behavior may be changed in future versions.
 * @warning Due to design of network simulation in oRTP, simulation is applied independently for audio and video stream. This means for example that a bandwidth
 * limit of 250kbit/s will have no effect on an audio stream running at 40kbit/s while a videostream targetting 400kbit/s will be highly affected.
 * @param lc the LinphoneCore
 * @param params the parameters used for the network simulation.
 * @return 0 if successful, -1 otherwise.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_set_network_simulator_params(LinphoneCore *lc, const OrtpNetworkSimulatorParams *params);

/**
 * Get the previously set network simulation parameters.
 * @see linphone_core_set_network_simulator_params
 * @return a OrtpNetworkSimulatorParams structure.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const OrtpNetworkSimulatorParams *linphone_core_get_network_simulator_params(const LinphoneCore *lc);

/**
 * Set the video preset to be used for video calls.
 * @param[in] lc LinphoneCore object
 * @param[in] preset The name of the video preset to be used (can be NULL to use the default video preset).
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_video_preset(LinphoneCore *lc, const char *preset);

/**
 * Get the video preset used for video calls.
 * @param[in] lc LinphoneCore object
 * @return The name of the video preset used for video calls (can be NULL if the default video preset is used).
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC const char * linphone_core_get_video_preset(const LinphoneCore *lc);

/**
 * Gets if realtime text is enabled or not
 * @param[in] lc LinphoneCore object
 * @return true if realtime text is enabled, false otherwise
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_realtime_text_enabled(LinphoneCore *lc);

/**
 * Set http proxy address to be used for signaling during next channel connection. Use #linphone_core_set_network_reachable FASLE/TRUE to force channel restart.
 * @param[in] lc LinphoneCore object
 * @param[in] host Hostname of IP adress of the http proxy (can be NULL to disable).
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_http_proxy_host(LinphoneCore *lc, const char *host) ;

/**
 * Set http proxy port to be used for signaling.
 * @param[in] lc LinphoneCore object
 * @param[in] port of the http proxy.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_http_proxy_port(LinphoneCore *lc, int port) ;

/**
 * Get http proxy address to be used for signaling.
 * @param[in] lc LinphoneCore object
 * @return hostname of IP adress of the http proxy (can be NULL to disable).
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC const char *linphone_core_get_http_proxy_host(const LinphoneCore *lc);

/**
 * Get http proxy port to be used for signaling.
 * @param[in] lc LinphoneCore object
 * @return port of the http proxy.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC int linphone_core_get_http_proxy_port(const LinphoneCore *lc);

LINPHONE_PUBLIC LinphoneRingtonePlayer *linphone_core_get_ringtoneplayer(LinphoneCore *lc);

/**
 * Sets a TLS certificate used for TLS authentication
 * The certificate won't be stored, you have to set it after each LinphoneCore startup
 * @param lc LinphoneCore object
 * @param tls_cert the TLS certificate
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_tls_cert(LinphoneCore *lc, const char *tls_cert);

/**
 * Sets a TLS key used for TLS authentication
 * The key won't be stored, you have to set it after each LinphoneCore startup
 * @param lc LinphoneCore object
 * @param tls_key the TLS key
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_tls_key(LinphoneCore *lc, const char *tls_key);

/**
 * Sets a TLS certificate path used for TLS authentication
 * The path will be stored in the rc file and automatically restored on startup
 * @param lc LinphoneCore object
 * @param tls_cert_path path to the TLS certificate
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_tls_cert_path(LinphoneCore *lc, const char *tls_cert_path);

/**
 * Sets a TLS key path used for TLS authentication
 * The path will be stored in the rc file and automatically restored on startup
 * @param lc LinphoneCore object
 * @param tls_key_path path to the TLS key
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_tls_key_path(LinphoneCore *lc, const char *tls_key_path);

/**
 * Gets the TLS certificate
 * @param lc LinphoneCore object
 * @return the TLS certificate or NULL if not set yet
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC const char *linphone_core_get_tls_cert(const LinphoneCore *lc);

/**
 * Gets the TLS key
 * @param lc LinphoneCore object
 * @return the TLS key or NULL if not set yet
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC const char *linphone_core_get_tls_key(const LinphoneCore *lc);

/**
 * Gets the path to the TLS certificate file
 * @param lc LinphoneCore object
 * @return the TLS certificate path or NULL if not set yet
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC const char *linphone_core_get_tls_cert_path(const LinphoneCore *lc);

/**
 * Gets the path to the TLS key file
 * @param lc LinphoneCore object
 * @return the TLS key path or NULL if not set yet
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC const char *linphone_core_get_tls_key_path(const LinphoneCore *lc);

/**
 * Sets an IM Encryption Engine in the core
 * @param lc LinphoneCore object
 * @param imee LinphoneImEncryptionEngine object
 * @ingroup chatroom
 */
LINPHONE_PUBLIC void linphone_core_set_im_encryption_engine(LinphoneCore *lc, LinphoneImEncryptionEngine *imee);

/**
 * Gets the IM Encryption Engine in the core if possible
 * @param lc LinphoneCore object
 * @return the IM Encryption Engine in the core or NULL
 * @ingroup chatroom
 */
LINPHONE_PUBLIC LinphoneImEncryptionEngine * linphone_core_get_im_encryption_engine(const LinphoneCore *lc);

/**
 * Tells whether a content type is supported.
 * @param[in] lc LinphoneCore object
 * @param[in] content_type The content type to check
 * @return A boolean value telling whether the specified content type is supported or not.
 */
LINPHONE_PUBLIC bool_t linphone_core_is_content_type_supported(const LinphoneCore *lc, const char *content_type);

/**
 * Add support for the specified content type.
 * It is the application responsibility to handle it correctly afterwards.
 * @param[in] lc LinphoneCore object
 * @param[in] content_type The content type to add support for
 */
LINPHONE_PUBLIC void linphone_core_add_content_type_support(LinphoneCore *lc, const char *content_type);


/**
 * @addtogroup chatroom
 * @{
 */

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
 * Get the LinphoneImNotifPolicy object controlling the instant messaging notifications.
 * @param[in] lc LinphoneCore object
 * @return A LinphoneImNotifPolicy object.
 */
LINPHONE_PUBLIC LinphoneImNotifPolicy * linphone_core_get_im_notif_policy(const LinphoneCore *lc);

/**
 * @}
 */

/**
 * Create a content with default values from Linphone core.
 * @param[in] lc LinphoneCore object
 * @return LinphoneContent object with default values set
 * @ingroup misc
 */
LINPHONE_PUBLIC LinphoneContent * linphone_core_create_content(LinphoneCore *lc);


/**
 * @addtogroup event_api
 * @{
**/

/**
 * Create an outgoing subscription, specifying the destination resource, the event name, and an optional content body.
 * If accepted, the subscription runs for a finite period, but is automatically renewed if not terminated before.
 * @param lc the #LinphoneCore
 * @param resource the destination resource
 * @param event the event name
 * @param expires the whished duration of the subscription
 * @param body an optional body, may be NULL.
 * @return a LinphoneEvent holding the context of the created subcription.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_subscribe(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires, const LinphoneContent *body);

/**
 * Create an outgoing subscription, specifying the destination resource, the event name, and an optional content body.
 * If accepted, the subscription runs for a finite period, but is automatically renewed if not terminated before.
 * Unlike linphone_core_subscribe() the subscription isn't sent immediately. It will be send when calling linphone_event_send_subscribe().
 * @param lc the #LinphoneCore
 * @param resource the destination resource
 * @param event the event name
 * @param expires the whished duration of the subscription
 * @return a LinphoneEvent holding the context of the created subcription.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_create_subscribe(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires);


/**
 * Create an out-of-dialog notification, specifying the destination resource, the event name.
 * The notification can be send with linphone_event_notify().
 * @param lc the #LinphoneCore
 * @param resource the destination resource
 * @param event the event name
 * @return a LinphoneEvent holding the context of the notification.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_create_notify(LinphoneCore *lc, const LinphoneAddress *resource, const char *event);

/**
 * Publish an event state.
 * This first create a LinphoneEvent with linphone_core_create_publish() and calls linphone_event_send_publish() to actually send it.
 * After expiry, the publication is refreshed unless it is terminated before.
 * @param lc the #LinphoneCore
 * @param resource the resource uri for the event
 * @param event the event name
 * @param expires the lifetime of event being published, -1 if no associated duration, in which case it will not be refreshed.
 * @param body the actual published data
 * @return the LinphoneEvent holding the context of the publish.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_publish(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires, const LinphoneContent *body);

/**
 * Create a publish context for an event state.
 * After being created, the publish must be sent using linphone_event_send_publish().
 * After expiry, the publication is refreshed unless it is terminated before.
 * @param lc the #LinphoneCore
 * @param resource the resource uri for the event
 * @param event the event name
 * @param expires the lifetime of event being published, -1 if no associated duration, in which case it will not be refreshed.
 * @return the LinphoneEvent holding the context of the publish.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_create_publish(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires);

/**
 * Create a publish context for a one-shot publish.
 * After being created, the publish must be sent using linphone_event_send_publish().
 * The LinphoneEvent is automatically terminated when the publish transaction is finished, either with success or failure.
 * The application must not call linphone_event_terminate() for such one-shot publish.
 * @param lc the #LinphoneCore
 * @param resource the resource uri for the event
 * @param event the event name
 * @return the LinphoneEvent holding the context of the publish.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_create_one_shot_publish(LinphoneCore *lc, const LinphoneAddress *resource, const char *event);

/**
 * @}
 */


/**
 * @addtogroup buddy_list
 * @{
 */

/**
 * Create a default LinphoneFriend.
 * @param[in] lc #LinphoneCore object
 * @return The created #LinphoneFriend object
 */
LINPHONE_PUBLIC LinphoneFriend * linphone_core_create_friend(LinphoneCore *lc);

/**
 * Create a LinphoneFriend from the given address.
 * @param[in] lc #LinphoneCore object
 * @param[in] address A string containing the address to create the LinphoneFriend from
 * @return The created #LinphoneFriend object
 */
LINPHONE_PUBLIC LinphoneFriend * linphone_core_create_friend_with_address(LinphoneCore *lc, const char *address);

/**
 * Set my presence status
 * @param[in] lc #LinphoneCore object
 * @param[in] minutes_away how long in away
 * @param[in] alternative_contact sip uri used to redirect call in state #LinphoneStatusMoved
 * @param[in] os #LinphoneOnlineStatus
 * @deprecated Use linphone_core_set_presence_model() instead
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_set_presence_info(LinphoneCore *lc,int minutes_away,const char *alternative_contact,LinphoneOnlineStatus os);

/**
 * Set my presence model
 * @param[in] lc #LinphoneCore object
 * @param[in] presence #LinphonePresenceModel
 */
LINPHONE_PUBLIC void linphone_core_set_presence_model(LinphoneCore *lc, LinphonePresenceModel *presence);

/**
 * Get my presence status
 * @param[in] lc #LinphoneCore object
 * @return #LinphoneOnlineStatus
 * @deprecated Use linphone_core_get_presence_model() instead
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneOnlineStatus linphone_core_get_presence_info(const LinphoneCore *lc);

/**
 * Get my presence model
 * @param[in] lc #LinphoneCore object
 * @return A #LinphonePresenceModel object, or NULL if no presence model has been set.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_core_get_presence_model(const LinphoneCore *lc);

/**
 * Get my consolidated presence
 * @param[in] lc LinphoneCore object
 * @return My consolidated presence
 */
LINPHONE_PUBLIC LinphoneConsolidatedPresence linphone_core_get_consolidated_presence(const LinphoneCore *lc);

/**
 * Set my consolidated presence
 * @param[in] lc LinphoneCore object
 * @param[in] presence LinphoneConsolidatedPresence value
 */
LINPHONE_PUBLIC void linphone_core_set_consolidated_presence(LinphoneCore *lc, LinphoneConsolidatedPresence presence);

/**
 * @deprecated Use linphone_core_interpret_url() instead
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_interpret_friend_uri(LinphoneCore *lc, const char *uri, char **result);

/**
 * Add a friend to the current buddy list, if \link linphone_friend_enable_subscribes() subscription attribute \endlink is set, a SIP SUBSCRIBE message is sent.
 * @param lc #LinphoneCore object
 * @param fr #LinphoneFriend to add
 * @deprecated use linphone_friend_list_add_friend() instead.
 */
LINPHONE_PUBLIC	void linphone_core_add_friend(LinphoneCore *lc, LinphoneFriend *fr);

/**
 * Removes a friend from the buddy list
 * @param lc #LinphoneCore object
 * @param fr #LinphoneFriend to remove
 * @deprecated use linphone_friend_list_remove_friend() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_core_remove_friend(LinphoneCore *lc, LinphoneFriend *fr);

/**
 * Black list a friend. same as linphone_friend_set_inc_subscribe_policy() with #LinphoneSPDeny policy;
 * @param lc #LinphoneCore object
 * @param lf #LinphoneFriend to add
 */
LINPHONE_PUBLIC void linphone_core_reject_subscriber(LinphoneCore *lc, LinphoneFriend *lf);

/**
 * Get Buddy list of LinphoneFriend
 * @param[in] lc #LinphoneCore object
 * @return \bctbx_list{LinphoneFriend}
 * @deprecated use linphone_core_get_friends_lists() or linphone_friend_list_get_friends() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC	LINPHONE_DEPRECATED  const bctbx_list_t * linphone_core_get_friend_list(const LinphoneCore *lc);

/**
 * Notify all friends that have subscribed
 * @param lc #LinphoneCore object
 * @param presence #LinphonePresenceModel to notify
 */
LINPHONE_PUBLIC void linphone_core_notify_all_friends(LinphoneCore *lc, LinphonePresenceModel *presence);

/**
 * Search a LinphoneFriend by its address.
 * @param[in] lc #LinphoneCore object.
 * @param[in] addr The address to use to search the friend.
 * @return The #LinphoneFriend object corresponding to the given address.
 * @deprecated use linphone_core_find_friend() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneFriend *linphone_core_get_friend_by_address(const LinphoneCore *lc, const char *addr);

/**
 * Search a LinphoneFriend by its address.
 * @param[in] lc #LinphoneCore object.
 * @param[in] addr The address to use to search the friend.
 * @return The #LinphoneFriend object corresponding to the given address.
 */
LINPHONE_PUBLIC LinphoneFriend *linphone_core_find_friend(const LinphoneCore *lc, const LinphoneAddress *addr);

/**
 * Search a LinphoneFriend by its reference key.
 * @param[in] lc #LinphoneCore object.
 * @param[in] key The reference key to use to search the friend.
 * @return The #LinphoneFriend object corresponding to the given reference key.
 */
LINPHONE_PUBLIC LinphoneFriend *linphone_core_get_friend_by_ref_key(const LinphoneCore *lc, const char *key);

/**
 * Sets the database filename where friends will be stored.
 * If the file does not exist, it will be created.
 * @ingroup initializing
 * @param lc the linphone core
 * @param path filesystem path
**/
LINPHONE_PUBLIC void linphone_core_set_friends_database_path(LinphoneCore *lc, const char *path);

/**
 * Gets the database filename where friends will be stored.
 * @ingroup initializing
 * @param lc the linphone core
 * @return filesystem path
**/
LINPHONE_PUBLIC const char* linphone_core_get_friends_database_path(LinphoneCore *lc);

/**
 * Migrates the friends from the linphonerc to the database if not done yet
 * @ingroup initializing
 * @param lc the linphone core
**/
LINPHONE_PUBLIC void linphone_core_migrate_friends_from_rc_to_db(LinphoneCore *lc);

/**
 * Create a new empty LinphoneFriendList object.
 * @param[in] lc LinphoneCore object.
 * @return A new LinphoneFriendList object.
**/
LINPHONE_PUBLIC LinphoneFriendList * linphone_core_create_friend_list(LinphoneCore *lc);

/**
 * Add a friend list.
 * @param[in] lc LinphoneCore object
 * @param[in] list LinphoneFriendList object
 */
LINPHONE_PUBLIC void linphone_core_add_friend_list(LinphoneCore *lc, LinphoneFriendList *list);

/**
 * Removes a friend list.
 * @param[in] lc LinphoneCore object
 * @param[in] list LinphoneFriendList object
 */
LINPHONE_PUBLIC void linphone_core_remove_friend_list(LinphoneCore *lc, LinphoneFriendList *list);

/**
 * Retrieves the list of LinphoneFriendList from the core.
 * @param[in] lc LinphoneCore object
 * @return \bctbx_list{LinphoneFriendList} a list of LinphoneFriendList
 */
LINPHONE_PUBLIC const bctbx_list_t * linphone_core_get_friends_lists(const LinphoneCore *lc);

/**
 * Retrieves the first list of LinphoneFriend from the core.
 * @param[in] lc LinphoneCore object
 * @return the first LinphoneFriendList object or NULL
 */
LINPHONE_PUBLIC LinphoneFriendList * linphone_core_get_default_friend_list(const LinphoneCore *lc);

/**
 * Create a LinphonePresenceActivity with the given type and description.
 * @param[in] lc #LinphoneCore object.
 * @param[in] acttype The #LinphonePresenceActivityType to set for the activity.
 * @param[in] description An additional description of the activity to set for the activity. Can be NULL if no additional description is to be added.
 * @return The created #LinphonePresenceActivity object.
 */
LINPHONE_PUBLIC LinphonePresenceActivity * linphone_core_create_presence_activity(LinphoneCore *lc, LinphonePresenceActivityType acttype, const char *description);

/**
 * Create a default LinphonePresenceModel.
 * @param[in] lc #LinphoneCore object.
 * @return The created #LinphonePresenceModel object.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_core_create_presence_model(LinphoneCore *lc);

/**
 * Create a LinphonePresenceModel with the given activity type and activity description.
 * @param[in] lc #LinphoneCore object.
 * @param[in] acttype The #LinphonePresenceActivityType to set for the activity of the created model.
 * @param[in] description An additional description of the activity to set for the activity. Can be NULL if no additional description is to be added.
 * @return The created #LinphonePresenceModel object.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_core_create_presence_model_with_activity(LinphoneCore *lc, LinphonePresenceActivityType acttype, const char *description);

/**
 * Create a LinphonePresenceModel with the given activity type, activity description, note content and note language.
 * @param[in] lc #LinphoneCore object.
 * @param[in] acttype The #LinphonePresenceActivityType to set for the activity of the created model.
 * @param[in] description An additional description of the activity to set for the activity. Can be NULL if no additional description is to be added.
 * @param[in] note The content of the note to be added to the created model.
 * @param[in] lang The language of the note to be added to the created model.
 * @return The created #LinphonePresenceModel object.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_core_create_presence_model_with_activity_and_note(LinphoneCore *lc, LinphonePresenceActivityType acttype, const char *description, const char *note, const char *lang);

/**
 * Create a LinphonePresenceNote with the given content and language.
 * @param[in] lc #LinphoneCore object.
 * @param[in] content The content of the note to be created.
 * @param[in] lang The language of the note to be created.
 * @return The created #LinphonePresenceNote object.
 */
LINPHONE_PUBLIC LinphonePresenceNote * linphone_core_create_presence_note(LinphoneCore *lc, const char *content, const char *lang);

/**
 * Create a LinphonePresencePerson with the given id.
 * @param[in] lc #LinphoneCore object
 * @param[in] id The id of the person to be created.
 * @return The created #LinphonePresencePerson object.
 */
LINPHONE_PUBLIC LinphonePresencePerson * linphone_core_create_presence_person(LinphoneCore *lc, const char *id);

/**
 * Create a LinphonePresenceService with the given id, basic status and contact.
 * @param[in] lc #LinphoneCore object.
 * @param[in] id The id of the service to be created.
 * @param[in] basic_status The basic status of the service to be created.
 * @param[in] contact A string containing a contact information corresponding to the service to be created.
 * @return The created #LinphonePresenceService object.
 */
LINPHONE_PUBLIC LinphonePresenceService * linphone_core_create_presence_service(LinphoneCore *lc, const char *id, LinphonePresenceBasicStatus basic_status, const char *contact);


/**
 * Notifies the upper layer that a presence status has been received by calling the appropriate
 * callback if one has been set.
 * This method is for advanced usage, where customization of the liblinphone's internal behavior is required.
 * @param[in]  lc the #LinphoneCore object.
 * @param[in]  lf the #LinphoneFriend whose presence information has been received.
 */
LINPHONE_PUBLIC void linphone_core_notify_notify_presence_received(LinphoneCore *lc, LinphoneFriend *lf);


/**
 * Notifies the upper layer that a presence model change has been received for the uri or
 * telephone number given as a parameter, by calling the appropriate callback if one has been set.
 * This method is for advanced usage, where customization of the liblinphone's internal behavior is required.
 * @param[in]  lc  the #LinphoneCore object.
 * @param[in]  lf  the #LinphoneFriend whose presence information has been received.
 * @param[in]  uri_or_tel  telephone number or sip uri
 * @param[in]  presence_model the #LinphonePresenceModel that has been modified
 */
LINPHONE_PUBLIC void linphone_core_notify_notify_presence_received_for_uri_or_tel(LinphoneCore *lc, LinphoneFriend *lf, const char *uri_or_tel, const LinphonePresenceModel *presence_model);



/**
 * @}
 */


/**
 * Create a new LinphoneNatPolicy object with every policies being disabled.
 * @param[in] lc LinphoneCore object
 * @return A new LinphoneNatPolicy object.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC LinphoneNatPolicy * linphone_core_create_nat_policy(LinphoneCore *lc);

/**
 * Create a new LinphoneNatPolicy by reading the config of a LinphoneCore according to the passed ref.
 * @param[in] lc LinphoneCore object
 * @param[in] ref The reference of a NAT policy in the config of the LinphoneCore
 * @return A new LinphoneNatPolicy object.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC LinphoneNatPolicy * linphone_core_create_nat_policy_from_config(LinphoneCore *lc, const char *ref);


/**
 * Create a LinphoneAccountCreator and set Linphone Request callbacks.
 * @param[in] core The LinphoneCore used for the XML-RPC communication
 * @param[in] xmlrpc_url The URL to the XML-RPC server. Must be NON NULL.
 * @return The new LinphoneAccountCreator object.
 * @ingroup account_creator
**/
LINPHONE_PUBLIC LinphoneAccountCreator * linphone_core_create_account_creator(LinphoneCore *core, const char *xmlrpc_url);


#ifdef __cplusplus
}
#endif

#endif
