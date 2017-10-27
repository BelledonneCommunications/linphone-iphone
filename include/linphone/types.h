/*
types.h
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

#ifndef LINPHONE_TYPES_H_
#define LINPHONE_TYPES_H_


#include "ortp/payloadtype.h"
#include "mediastreamer2/msinterfaces.h"
#include "mediastreamer2/msvideo.h"
#include "linphone/defs.h"


/**
 * The LinphoneAccountCreator object used to configure an account on a server via XML-RPC.
 * @ingroup account_creator
**/
typedef struct _LinphoneAccountCreator LinphoneAccountCreator;

/**
 * An object to define a LinphoneAccountCreator service.
 * @ingroup account_creator
 * @donotwrap
**/
typedef struct _LinphoneAccountCreatorService LinphoneAccountCreatorService;

/**
 * An object to handle the responses callbacks for handling the LinphoneAccountCreator operations.
 * @ingroup account_creator
**/
typedef struct _LinphoneAccountCreatorCbs LinphoneAccountCreatorCbs;

/**
 * Enum describing Phone number checking.
 * @ingroup account_creator
**/
typedef enum _LinphoneAccountCreatorPhoneNumberStatus {
	LinphoneAccountCreatorPhoneNumberStatusOk = 0x1, /**< Phone number ok */
	LinphoneAccountCreatorPhoneNumberStatusTooShort = 0x2, /**< Phone number too short */
	LinphoneAccountCreatorPhoneNumberStatusTooLong = 0x4, /**< Phone number too long */
	LinphoneAccountCreatorPhoneNumberStatusInvalidCountryCode = 0x8, /**< Country code invalid */
	LinphoneAccountCreatorPhoneNumberStatusInvalid = 0x10 /**< Phone number invalid */
} LinphoneAccountCreatorPhoneNumberStatus;

/**
 * A mask of #LinphoneAccountCreatorPhoneNumberStatus values
 * @ingroup account_creator
 */
typedef unsigned int LinphoneAccountCreatorPhoneNumberStatusMask;

/**
 * Enum describing Username checking.
 * @ingroup account_creator
**/
typedef enum _LinphoneAccountCreatorUsernameStatus {
	LinphoneAccountCreatorUsernameStatusOk, /**< Username ok */
	LinphoneAccountCreatorUsernameStatusTooShort, /**< Username too short */
	LinphoneAccountCreatorUsernameStatusTooLong,  /**< Username too long */
	LinphoneAccountCreatorUsernameStatusInvalidCharacters, /**< Contain invalid characters */
	LinphoneAccountCreatorUsernameStatusInvalid /**< Invalid username */
} LinphoneAccountCreatorUsernameStatus;

/**
 * Enum describing Email checking.
 * @ingroup account_creator
**/
typedef enum _LinphoneAccountCreatorEmailStatus {
	LinphoneAccountCreatorEmailStatusOk, /**< Email ok */
	LinphoneAccountCreatorEmailStatusMalformed, /**< Email malformed */
	LinphoneAccountCreatorEmailStatusInvalidCharacters /**< Contain invalid characters */
} LinphoneAccountCreatorEmailStatus;

/**
 * Enum describing Password checking.
 * @ingroup account_creator
**/
typedef enum _LinphoneAccountCreatorPasswordStatus {
	LinphoneAccountCreatorPasswordStatusOk, /**< Password ok */
	LinphoneAccountCreatorPasswordStatusTooShort, /**< Password too short */
	LinphoneAccountCreatorPasswordStatusTooLong,  /**< Password too long */
	LinphoneAccountCreatorPasswordStatusInvalidCharacters, /**< Contain invalid characters */
	LinphoneAccountCreatorPasswordStatusMissingCharacters /**< Missing specific characters */
} LinphoneAccountCreatorPasswordStatus;

/**
 * Enum describing language checking.
 * @ingroup account_creator
**/
typedef enum _LinphoneAccountCreatorLanguageStatus {
	LinphoneAccountCreatorLanguageStatusOk /**< Language ok */
} LinphoneAccountCreatorLanguageStatus;

/**
 * Enum describing Activation code checking.
 * @ingroup account_creator
**/
typedef enum _LinphoneAccountCreatorActivationCodeStatus {
	LinphoneAccountCreatorActivationCodeStatusOk, /**< Activation code ok */
	LinphoneAccountCreatorActivationCodeStatusTooShort, /**< Activation code too short */
	LinphoneAccountCreatorActivationCodeStatusTooLong, /**< Activation code too long */
	LinphoneAccountCreatorActivationCodeStatusInvalidCharacters /**< Contain invalid characters */
} LinphoneAccountCreatorActivationCodeStatus;

/**
 * Enum describing Domain checking
 * @ingroup account_creator
**/
typedef enum _LinphoneAccountCreatorDomainStatus {
	LinphoneAccountCreatorDomainOk, /**< Domain ok */
	LinphoneAccountCreatorDomainInvalid /**< Domain invalid */
} LinphoneAccountCreatorDomainStatus;

/**
 * Enum describing Transport checking
 * @ingroup account_creator
**/
typedef enum _LinphoneAccountCreatorTransportStatus {
	LinphoneAccountCreatorTransportOk, /**< Transport ok */
	LinphoneAccountCreatorTransportUnsupported /**< Transport invalid */
} LinphoneAccountCreatorTransportStatus;

/**
 * Enum describing the status of server request.
 * @ingroup account_creator_request
**/
typedef enum _LinphoneAccountCreatorStatus {
	/** Request status **/
	LinphoneAccountCreatorStatusRequestOk, /**< Request passed */
	LinphoneAccountCreatorStatusRequestFailed, /**< Request failed */
	LinphoneAccountCreatorStatusMissingArguments, /**< Request failed due to missing argument(s) */
	LinphoneAccountCreatorStatusMissingCallbacks, /**< Request failed due to missing callback(s) */

	/** Account status **/
	/* Creation */
	LinphoneAccountCreatorStatusAccountCreated, /**< Account created */
	LinphoneAccountCreatorStatusAccountNotCreated, /**< Account not created */
	/* Existence */
	LinphoneAccountCreatorStatusAccountExist, /**< Account exist */
	LinphoneAccountCreatorStatusAccountExistWithAlias, /**< Account exist with alias */
	LinphoneAccountCreatorStatusAccountNotExist, /**< Account not exist */
	LinphoneAccountCreatorStatusAliasIsAccount, /**< Account was created with Alias */
	LinphoneAccountCreatorStatusAliasExist, /**< Alias exist */
	LinphoneAccountCreatorStatusAliasNotExist, /**< Alias not exist */
	/* Activation */
	LinphoneAccountCreatorStatusAccountActivated, /**< Account activated */
	LinphoneAccountCreatorStatusAccountAlreadyActivated, /**< Account already activated */
	LinphoneAccountCreatorStatusAccountNotActivated, /**< Account not activated */
	/* Linking */
	LinphoneAccountCreatorStatusAccountLinked, /**< Account linked */
	LinphoneAccountCreatorStatusAccountNotLinked, /**< Account not linked */

	/** Server **/
	LinphoneAccountCreatorStatusServerError /**< Error server */
} LinphoneAccountCreatorStatus;

struct SalAddress;

/**
 * Object that represents a SIP address.
 *
 * The LinphoneAddress is an opaque object to represents SIP addresses, ie
 * the content of SIP's 'from' and 'to' headers.
 * A SIP address is made of display name, username, domain name, port, and various
 * uri headers (such as tags). It looks like 'Alice <sip:alice@example.net>'.
 * The LinphoneAddress has methods to extract and manipulate all parts of the address.
 * When some part of the address (for example the username) is empty, the accessor methods
 * return NULL.
 * @ingroup linphone_address
 */
typedef struct SalAddress LinphoneAddress;

/**
 * Enum describing Ip family.
 * @ingroup initializing
**/
typedef enum _LinphoneAddressFamily {
	LinphoneAddressFamilyInet, /**< IpV4 */
	LinphoneAddressFamilyInet6, /**< IpV6 */
	LinphoneAddressFamilyUnspec, /**< Unknown */
} LinphoneAddressFamily;

/**
 * Enum describing type of audio route.
 * @ingroup call_control
**/
typedef enum _LinphoneAudioRoute {
	LinphoneAudioRouteEarpiece,
	LinphoneAudioRouteSpeaker
} LinphoneAudioRoute;

/**
 * Object holding authentication information.
 *
 * In most case, authentication information consists of a username and password.
 * Sometimes, a userid is required by proxy, and realm can be useful to discriminate
 * different SIP domains.
 *
 * Once created and filled, a LinphoneAuthInfo must be added to the LinphoneCore in
 * order to become known and used automatically when needed.
 * Use linphone_core_add_auth_info() for that purpose.
 *
 * The LinphoneCore object can take the initiative to request authentication information
 * when needed to the application through the auth_info_requested callback of the
 * LinphoneCoreVTable structure.
 *
 * The application can respond to this information request later using
 * linphone_core_add_auth_info(). This will unblock all pending authentication
 * transactions and retry them with authentication headers.
 * 
 * @note The object's fields should not be accessed directly. Prefer using
 * the accessor methods.
 *
 * @ingroup authentication
**/
typedef struct _LinphoneAuthInfo LinphoneAuthInfo;

/**
 * Enum describing the authentication methods
 * @ingroup network_parameters
**/
typedef enum _LinphoneAuthMethod {
	LinphoneAuthHttpDigest, /**< Digest authentication requested */
	LinphoneAuthTls, /**< Client certificate requested */
} LinphoneAuthMethod;

/**
 * Enum describing RTP AVPF activation modes.
 * @ingroup media_parameters
**/
typedef enum _LinphoneAVPFMode {
	LinphoneAVPFDefault = -1, /**< Use default value defined at upper level */
	LinphoneAVPFDisabled, /**< AVPF is disabled */
	LinphoneAVPFEnabled /**< AVPF is enabled */
} LinphoneAVPFMode;

/**
 * The LinphoneContent object representing a data buffer.
 * @ingroup misc
**/
typedef struct _LinphoneBuffer LinphoneBuffer;

/**
 * The LinphoneCall object represents a call issued or received by the LinphoneCore
 * @ingroup call_control
**/
typedef struct _LinphoneCall LinphoneCall;

/**
 * That class holds all the callbacks which are called by LinphoneCall objects.
 *
 * Use linphone_factory_create_call_cbs() to create an instance. Then, call the
 * callback setters on the events you need to monitor and pass the object to
 * a LinphoneCall instance through linphone_call_add_callbacks().
 * @ingroup call_control
 */
typedef struct _LinphoneCallCbs LinphoneCallCbs;

/**
 * Enum representing the direction of a call.
 * @ingroup call_logs
**/
typedef enum _LinphoneCallDir {
	LinphoneCallOutgoing, /**< outgoing calls*/
	LinphoneCallIncoming  /**< incoming calls*/
} LinphoneCallDir;

/**
 * Structure representing a call log.
 * @ingroup call_logs
**/
typedef struct _LinphoneCallLog LinphoneCallLog;

/**
 * The LinphoneCallParams is an object containing various call related parameters.
 * It can be used to retrieve parameters from a currently running call or modify
 * the call's characteristics dynamically.
 * @ingroup call_control
**/
typedef struct _LinphoneCallParams LinphoneCallParams;

/**
 * LinphoneCallState enum represents the different state a call can reach into.
 * The application is notified of state changes through the LinphoneCoreVTable::call_state_changed callback.
 * @ingroup call_control
**/
typedef enum _LinphoneCallState{
	LinphoneCallIdle, /**< Initial call state */
	LinphoneCallIncomingReceived, /**< This is a new incoming call */
	LinphoneCallOutgoingInit, /**< An outgoing call is started */
	LinphoneCallOutgoingProgress, /**< An outgoing call is in progress */
	LinphoneCallOutgoingRinging, /**< An outgoing call is ringing at remote end */
	LinphoneCallOutgoingEarlyMedia, /**< An outgoing call is proposed early media */
	LinphoneCallConnected, /**< Connected, the call is answered */
	LinphoneCallStreamsRunning, /**< The media streams are established and running */
	LinphoneCallPausing, /**< The call is pausing at the initiative of local end */
	LinphoneCallPaused, /**< The call is paused, remote end has accepted the pause */
	LinphoneCallResuming, /**< The call is being resumed by local end */
	LinphoneCallRefered, /**< The call is being transfered to another party, resulting in a new outgoing call to follow immediately */
	LinphoneCallError, /**< The call encountered an error */
	LinphoneCallEnd, /**< The call ended normally */
	LinphoneCallPausedByRemote, /**< The call is paused by remote end */
	LinphoneCallUpdatedByRemote, /**< The call's parameters change is requested by remote end, used for example when video is added by remote */
	LinphoneCallIncomingEarlyMedia, /**< We are proposing early media to an incoming call */
	LinphoneCallUpdating, /**< A call update has been initiated by us */
	LinphoneCallReleased, /**< The call object is no more retained by the core */
	LinphoneCallEarlyUpdatedByRemote, /**< The call is updated by remote while not yet answered (early dialog SIP UPDATE received) */
	LinphoneCallEarlyUpdating /**< We are updating the call while not yet answered (early dialog SIP UPDATE sent) */
} LinphoneCallState;

/**
 * The LinphoneCallStats objects carries various statistic informations regarding quality of audio or video streams.
 *
 * To receive these informations periodically and as soon as they are computed, the application is invited to place a #LinphoneCoreCallStatsUpdatedCb callback in the LinphoneCoreVTable structure
 * it passes for instanciating the LinphoneCore object (see linphone_core_new() ).
 *
 * At any time, the application can access last computed statistics using linphone_call_get_audio_stats() or linphone_call_get_video_stats().
 * @ingroup call_misc
**/
typedef struct _LinphoneCallStats LinphoneCallStats;

/**
 * Enum representing the status of a call
 * @ingroup call_logs
**/
typedef enum _LinphoneCallStatus {
	LinphoneCallSuccess, /**< The call was sucessful */
	LinphoneCallAborted, /**< The call was aborted */
	LinphoneCallMissed, /**< The call was missed (unanswered) */
	LinphoneCallDeclined, /**< The call was declined, either locally or by remote end */
	LinphoneCallEarlyAborted, /**<The call was aborted before being advertised to the application - for protocol reasons*/
	LinphoneCallAcceptedElsewhere, /**<The call was answered on another device*/
	LinphoneCallDeclinedElsewhere /**<The call was declined on another device*/
} LinphoneCallStatus;

/**
 * A chat room message to hold content to be sent.
 * Can be created by linphone_chat_room_create_message().
 * @ingroup chatroom
 */
typedef struct _LinphoneChatMessage LinphoneChatMessage;

/**
 * An object to handle the callbacks for the handling a LinphoneChatMessage objects.
 * @ingroup chatroom
 */
typedef struct _LinphoneChatMessageCbs LinphoneChatMessageCbs;

/**
 * LinphoneChatMessageState is used to notify if messages have been succesfully delivered or not.
 * @ingroup chatroom
 */
typedef enum _LinphoneChatMessageState {
	LinphoneChatMessageStateIdle, /**< Initial state */
	LinphoneChatMessageStateInProgress, /**< Delivery in progress */
	LinphoneChatMessageStateDelivered, /**< Message successfully delivered and acknowledged by server */
	LinphoneChatMessageStateNotDelivered, /**< Message was not delivered */
	LinphoneChatMessageStateFileTransferError, /**< Message was received(and acknowledged) but cannot get file from server */
	LinphoneChatMessageStateFileTransferDone, /**< File transfer has been completed successfully */
	LinphoneChatMessageStateDeliveredToUser, /**< Message successfully delivered and acknowledged to destination */
	LinphoneChatMessageStateDisplayed /**< Message displayed to the remote user */
} LinphoneChatMessageState;

/**
 * A chat room is the place where text messages are exchanged.
 * Can be created by linphone_core_create_chat_room().
 * @ingroup chatroom
 */
typedef struct _LinphoneChatRoom LinphoneChatRoom;

/**
 * LinphoneConference class
 * The _LinphoneConference struct does not exists, it's the Conference C++ class that is used behind
 * @ingroup call_control
 */
typedef struct _LinphoneConference LinphoneConference;

/**
 * Parameters for initialization of conferences
 * The _LinphoneConferenceParams struct does not exists, it's the ConferenceParams C++ class that is used behind
 * @ingroup call_control
 */
typedef struct _LinphoneConferenceParams LinphoneConferenceParams;

/**
 * The LinphoneConfig object is used to manipulate a configuration file.
 *
 * The format of the configuration file is a .ini like format:
 * - sections are defined in []
 * - each section contains a sequence of key=value pairs.
 *
 * Example:
 * @code
 * [sound]
 * echocanceler=1
 * playback_dev=ALSA: Default device
 *
 * [video]
 * enabled=1
 * @endcode
 *
 * @ingroup misc
**/
typedef struct _LpConfig LinphoneConfig;

/**
 * Define old struct name for backward compatibility
 */
#define LpConfig LinphoneConfig

/**
 * LinphoneGlobalState describes the global state of the LinphoneCore object.
 * It is notified via the LinphoneCoreVTable::global_state_changed
 * @ingroup initializing
**/
typedef enum _LinphoneConfiguringState {
	LinphoneConfiguringSuccessful,
	LinphoneConfiguringFailed,
	LinphoneConfiguringSkipped
} LinphoneConfiguringState;

/**
 * Consolidated presence information: 'online' means the user is open for communication,
 * 'busy' means the user is open for communication but involved in an other activity,
 * 'do not disturb' means the user is not open for communication, and 'offline' means
 * that no presence information is available.
 * @ingroup buddy_list
 */
typedef enum _LinphoneConsolidatedPresence {
	LinphoneConsolidatedPresenceOnline,
	LinphoneConsolidatedPresenceBusy,
	LinphoneConsolidatedPresenceDoNotDisturb,
	LinphoneConsolidatedPresenceOffline
} LinphoneConsolidatedPresence;

typedef struct _LinphoneContactProvider LinphoneContactProvider;

typedef struct _LinphoneContactSearch LinphoneContactSearch;

typedef unsigned int LinphoneContactSearchID;

/**
 * Old name of LinphoneContactSearchID
 * @deprecated
 * @donotwrap
 */
LINPHONE_DEPRECATED typedef LinphoneContactSearchID ContactSearchID;

/**
 * The LinphoneContent object holds data that can be embedded in a signaling message.
 * @ingroup misc
**/
typedef struct _LinphoneContent LinphoneContent;

/**
 * Linphone core main object created by function linphone_core_new() .
 * @ingroup initializing
 */
typedef struct _LinphoneCore LinphoneCore;

/**
 * That class holds all the callbacks which are called by #LinphoneCore.
 *
 * Use linphone_factory_create_core_cbs() to create an instance. Then, call the
 * callback setters on the events you need to monitor and pass the object to
 * a #LinphoneCore instance through linphone_core_add_callbacks().
 *
 * That class is inherited from belle_sip_object_t.
 * @ingroup initializing
 */
typedef struct _LinphoneCoreCbs LinphoneCoreCbs;

typedef struct belle_sip_dict LinphoneDictionary;

/**
 * Enum describing the result of the echo canceller calibration process.
 * @ingroup media_parameters
**/
typedef enum _LinphoneEcCalibratorStatus {
	LinphoneEcCalibratorInProgress, /**< The echo canceller calibration process is on going */
	LinphoneEcCalibratorDone, /**< The echo canceller calibration has been performed and produced an echo delay measure */
	LinphoneEcCalibratorFailed, /**< The echo canceller calibration process has failed */
	LinphoneEcCalibratorDoneNoEcho /**< The echo canceller calibration has been performed and no echo has been detected */
} LinphoneEcCalibratorStatus;

/**
 * Object representing full details about a signaling error or status.
 * All LinphoneErrorInfo object returned by the liblinphone API are readonly and transcients. For safety they must be used immediately
 * after obtaining them. Any other function call to the liblinphone may change their content or invalidate the pointer.
 * @ingroup misc
**/
typedef struct _LinphoneErrorInfo LinphoneErrorInfo;

/**
 * Object representing an event state, which is subcribed or published.
 * @see linphone_core_publish()
 * @see linphone_core_subscribe()
 * @ingroup event_api
**/
typedef struct _LinphoneEvent LinphoneEvent;

/**
 * #LinphoneFactory is a singleton object devoted to the creation of all the object
 * of Liblinphone that cannot be created by #LinphoneCore itself.
 * @ingroup initializing
 */
typedef struct _LinphoneFactory LinphoneFactory;

/**
 * Policy to use to pass through firewalls.
 * @ingroup network_parameters
 * @deprecated Use #LinphoneNatPolicy instead.
 * @donotwrap
**/
typedef enum _LinphoneFirewallPolicy {
	LinphonePolicyNoFirewall, /**< Do not use any mechanism to pass through firewalls */
	LinphonePolicyUseNatAddress, /**< Use the specified public adress */
	LinphonePolicyUseStun, /**< Use a STUN server to get the public address */
	LinphonePolicyUseIce, /**< Use the ICE protocol */
	LinphonePolicyUseUpnp, /**< Use the uPnP protocol */
} LinphoneFirewallPolicy;

/**
 * Represents a buddy, all presence actions like subscription and status change notification are performed on this object
 * @ingroup buddy_list
 */
typedef struct _LinphoneFriend LinphoneFriend;

/**
 * The LinphoneFriendList object representing a list of friends.
 * @ingroup buddy_list
**/
typedef struct _LinphoneFriendList LinphoneFriendList;

/**
 * An object to handle the callbacks for LinphoneFriend synchronization.
 * @ingroup buddy_list
**/
typedef struct _LinphoneFriendListCbs LinphoneFriendListCbs;

/**
* Enum describing the status of a LinphoneFriendList operation.
* @ingroup buddy_list
**/
typedef enum _LinphoneFriendListStatus {
	LinphoneFriendListOK,
	LinphoneFriendListNonExistentFriend,
	LinphoneFriendListInvalidFriend
} LinphoneFriendListStatus;

/**
 * Enum describing the status of a CardDAV synchronization
 * @ingroup buddy_list
 */
typedef enum _LinphoneFriendListSyncStatus {
	LinphoneFriendListSyncStarted,
	LinphoneFriendListSyncSuccessful,
	LinphoneFriendListSyncFailure
} LinphoneFriendListSyncStatus;

/**
 * LinphoneGlobalState describes the global state of the LinphoneCore object.
 * It is notified via the LinphoneCoreVTable::global_state_changed
 * @ingroup initializing
**/
typedef enum _LinphoneGlobalState {
	LinphoneGlobalOff,
	LinphoneGlobalStartup,
	LinphoneGlobalOn,
	LinphoneGlobalShutdown,
	LinphoneGlobalConfiguring
} LinphoneGlobalState;

/**
 * Enum describing ICE states.
 * @ingroup initializing
**/
typedef enum _LinphoneIceState {
	LinphoneIceStateNotActivated, /**< ICE has not been activated for this call or stream*/
	LinphoneIceStateFailed, /**< ICE processing has failed */
	LinphoneIceStateInProgress, /**< ICE process is in progress */
	LinphoneIceStateHostConnection, /**< ICE has established a direct connection to the remote host */
	LinphoneIceStateReflexiveConnection, /**< ICE has established a connection to the remote host through one or several NATs */
	LinphoneIceStateRelayConnection /**< ICE has established a connection through a relay */
} LinphoneIceState;

/**
 * IM encryption engine.
 * @ingroup misc
 */
typedef struct _LinphoneImEncryptionEngine LinphoneImEncryptionEngine;

/**
 * An object to handle the callbacks for the handling a LinphoneImEncryptionEngine object.
 * @ingroup misc
 */
typedef struct _LinphoneImEncryptionEngineCbs LinphoneImEncryptionEngineCbs;

/**
 * Policy to use to send/receive instant messaging composing/delivery/display notifications.
 * The sending of this information is done as in the RFCs 3994 (is_composing) and 5438 (imdn delivered/displayed).
 * @ingroup chatroom
 */
typedef struct _LinphoneImNotifPolicy LinphoneImNotifPolicy;

/**
 * The LinphoneInfoMessage is an object representing an informational message sent or received by the core.
 * @ingroup misc
**/
typedef struct _LinphoneInfoMessage LinphoneInfoMessage;

typedef struct _LinphoneLDAPContactProvider LinphoneLDAPContactProvider;

typedef struct _LinphoneLDAPContactSearch LinphoneLDAPContactSearch;

/**
 * @ingroup network_parameters
 */
typedef enum _LinphoneLimeState {
	LinphoneLimeDisabled, /**< Lime is not used at all */
	LinphoneLimeMandatory, /**< Lime is always used */
	LinphoneLimePreferred, /**< Lime is used only if we already shared a secret with remote */
} LinphoneLimeState;

/**
 * @ingroup initializing
 */
typedef enum _LinphoneLogCollectionState {
	LinphoneLogCollectionDisabled,
	LinphoneLogCollectionEnabled,
	LinphoneLogCollectionEnabledWithoutPreviousLogHandler
} LinphoneLogCollectionState;

/**
 * LinphoneCoreLogCollectionUploadState is used to notify if log collection upload have been succesfully delivered or not.
 * @ingroup initializing
 */
typedef enum _LinphoneCoreLogCollectionUploadState {
	LinphoneCoreLogCollectionUploadStateInProgress, /**< Delivery in progress */
	LinphoneCoreLogCollectionUploadStateDelivered, /**< Log collection upload successfully delivered and acknowledged by remote end point */
	LinphoneCoreLogCollectionUploadStateNotDelivered, /**< Log collection upload was not delivered */
} LinphoneCoreLogCollectionUploadState;

/**
 * Indicates for a given media the stream direction
 * @ingroup call_control
 */
typedef enum _LinphoneMediaDirection {
	LinphoneMediaDirectionInvalid = -1,
	LinphoneMediaDirectionInactive, /** No active media not supported yet*/
	LinphoneMediaDirectionSendOnly, /** Send only mode*/
	LinphoneMediaDirectionRecvOnly, /** recv only mode*/
	LinphoneMediaDirectionSendRecv, /** send receive*/
} LinphoneMediaDirection;

/**
 * Enum describing type of media encryption types.
 * @ingroup media_parameters
**/
typedef enum _LinphoneMediaEncryption {
	LinphoneMediaEncryptionNone, /**< No media encryption is used */
	LinphoneMediaEncryptionSRTP, /**< Use SRTP media encryption */
	LinphoneMediaEncryptionZRTP, /**< Use ZRTP media encryption */
	LinphoneMediaEncryptionDTLS /**< Use DTLS media encryption */
} LinphoneMediaEncryption;

/**
 * Policy to use to pass through NATs/firewalls.
 * @ingroup network_parameters
 */
typedef struct _LinphoneNatPolicy LinphoneNatPolicy;

/**
 * Enum describing remote friend status
 * @deprecated Use #LinphonePresenceModel and #LinphonePresenceActivity instead
 * @donotwrap
 */
typedef enum _LinphoneOnlineStatus{
	LinphoneStatusOffline, /**< Offline */
	LinphoneStatusOnline, /**< Online */
	LinphoneStatusBusy, /**< Busy */
	LinphoneStatusBeRightBack, /**< Be right back */
	LinphoneStatusAway, /**< Away */
	LinphoneStatusOnThePhone, /** On the phone */
	LinphoneStatusOutToLunch, /**< Out to lunch */
	LinphoneStatusDoNotDisturb, /**< Do not disturb */
	LinphoneStatusMoved, /**< Moved in this sate, call can be redirected if an alternate contact address has been set using function linphone_core_set_presence_info() */
	LinphoneStatusAltService, /**< Using another messaging service */
	LinphoneStatusPending, /**< Pending */
	LinphoneStatusVacation, /**< Vacation */

	LinphoneStatusEnd
} LinphoneOnlineStatus;

/**
 * Player interface.
 * @ingroup call_control
**/
typedef struct _LinphonePlayer LinphonePlayer;

/**
 * An object to handle the callbacks for the handling a LinphonePlayer objects.
 * @ingroup call_control
 */
typedef struct _LinphonePlayerCbs LinphonePlayerCbs;

/**
 * The state of a LinphonePlayer.
 * @ingroup call_control
 */
typedef enum LinphonePlayerState {
	LinphonePlayerClosed, /**< No file is opened for playing. */
	LinphonePlayerPaused, /**< The player is paused. */
	LinphonePlayerPlaying /**< The player is playing. */
} LinphonePlayerState;

/**
 * Presence activity type holding information about a presence activity.
 * @ingroup buddy_list
 */
typedef struct _LinphonePresenceActivity LinphonePresenceActivity;

/**
 * Activities as defined in section 3.2 of RFC 4480
 * @ingroup buddy_list
 */
typedef enum LinphonePresenceActivityType {
	/** The person has a calendar appointment, without specifying exactly of what type. This activity is
	 *  indicated if more detailed information is not available or the person chooses not to reveal more
	 * information. */
	LinphonePresenceActivityAppointment,

	/** The person is physically away from all interactive communication devices. */
	LinphonePresenceActivityAway,

	/** The person is eating the first meal of the day, usually eaten in the morning. */
	LinphonePresenceActivityBreakfast,

	/** The person is busy, without further details. */
	LinphonePresenceActivityBusy,

	/** The person is having his or her main meal of the day, eaten in the evening or at midday. */
	LinphonePresenceActivityDinner,

	/**  This is a scheduled national or local holiday. */
	LinphonePresenceActivityHoliday,

	/** The person is riding in a vehicle, such as a car, but not steering. */
	LinphonePresenceActivityInTransit,

	/** The person is looking for (paid) work. */
	LinphonePresenceActivityLookingForWork,

	/** The person is eating his or her midday meal. */
	LinphonePresenceActivityLunch,

	/** The person is scheduled for a meal, without specifying whether it is breakfast, lunch, or dinner,
	 *  or some other meal. */
	LinphonePresenceActivityMeal,

	/** The person is in an assembly or gathering of people, as for a business, social, or religious purpose.
	 *  A meeting is a sub-class of an appointment. */
	LinphonePresenceActivityMeeting,

	/** The person is talking on the telephone. */
	LinphonePresenceActivityOnThePhone,

	/** The person is engaged in an activity with no defined representation. A string describing the activity
	 *  in plain text SHOULD be provided. */
	LinphonePresenceActivityOther,

	/** A performance is a sub-class of an appointment and includes musical, theatrical, and cinematic
	 *  performances as well as lectures. It is distinguished from a meeting by the fact that the person
	 *  may either be lecturing or be in the audience, with a potentially large number of other people,
	 *  making interruptions particularly noticeable. */
	LinphonePresenceActivityPerformance,

	/** The person will not return for the foreseeable future, e.g., because it is no longer working for
	 *  the company. */
	LinphonePresenceActivityPermanentAbsence,

	/** The person is occupying himself or herself in amusement, sport, or other recreation. */
	LinphonePresenceActivityPlaying,

	/** The person is giving a presentation, lecture, or participating in a formal round-table discussion. */
	LinphonePresenceActivityPresentation,

	/** The person is visiting stores in search of goods or services. */
	LinphonePresenceActivityShopping,

	/** The person is sleeping.*/
	LinphonePresenceActivitySleeping,

	/** The person is observing an event, such as a sports event. */
	LinphonePresenceActivitySpectator,

	/** The person is controlling a vehicle, watercraft, or plane. */
	LinphonePresenceActivitySteering,

	/** The person is on a business or personal trip, but not necessarily in-transit. */
	LinphonePresenceActivityTravel,

	/** The person is watching television. */
	LinphonePresenceActivityTV,

	/** The activity of the person is unknown. */
	LinphonePresenceActivityUnknown,

	/** A period of time devoted to pleasure, rest, or relaxation. */
	LinphonePresenceActivityVacation,

	/** The person is engaged in, typically paid, labor, as part of a profession or job. */
	LinphonePresenceActivityWorking,

	/** The person is participating in religious rites. */
	LinphonePresenceActivityWorship
} LinphonePresenceActivityType;

/**
 * Basic status as defined in section 4.1.4 of RFC 3863
 * @ingroup buddy_list
 */
typedef enum LinphonePresenceBasicStatus {
	/** This value means that the associated contact element, if any, is ready to accept communication. */
	LinphonePresenceBasicStatusOpen,

	/** This value means that the associated contact element, if any, is unable to accept communication. */
	LinphonePresenceBasicStatusClosed
} LinphonePresenceBasicStatus;

/**
 * Presence model type holding information about the presence of a person.
 * @ingroup buddy_list
 */
typedef struct _LinphonePresenceModel LinphonePresenceModel;

/**
 * Presence note type holding information about a presence note.
 * @ingroup buddy_list
 */
typedef struct _LinphonePresenceNote LinphonePresenceNote;

/**
 * Presence person holding information about a presence person.
 * @ingroup buddy_list
 */
typedef struct _LinphonePresencePerson LinphonePresencePerson;

/**
 * Presence service type holding information about a presence service.
 * @ingroup buddy_list
 */
typedef struct _LinphonePresenceService LinphonePresenceService;

/**
 * @ingroup call_control
 * Defines privacy policy to apply as described by rfc3323
**/
typedef enum _LinphonePrivacy {
	/**
	 * Privacy services must not perform any privacy function
	 */
	LinphonePrivacyNone = 0x0,
	/**
	 * Request that privacy services provide a user-level privacy
	 * function.
	 * With this mode, "from" header is hidden, usually replaced by  From: "Anonymous" <sip:anonymous@anonymous.invalid>
	 */
	LinphonePrivacyUser = 0x1,
	/**
	 * Request that privacy services modify headers that cannot
	 * be set arbitrarily by the user (Contact/Via).
	 */
	LinphonePrivacyHeader = 0x2,
	/**
	 *  Request that privacy services provide privacy for session
	 * media
	 */
	LinphonePrivacySession = 0x4,
	/**
	 * rfc3325
	 * The presence of this privacy type in
	 * a Privacy header field indicates that the user would like the Network
	 * Asserted Identity to be kept private with respect to SIP entities
	 * outside the Trust Domain with which the user authenticated.  Note
	 * that a user requesting multiple types of privacy MUST include all of
	 * the requested privacy types in its Privacy header field value
	 *
	 */
	LinphonePrivacyId = 0x8,
	/**
	 * Privacy service must perform the specified services or
	 * fail the request
	 *
	 **/
	LinphonePrivacyCritical = 0x10,

	/**
	 * Special keyword to use privacy as defined either globally or by proxy using linphone_proxy_config_set_privacy()
	 */
	LinphonePrivacyDefault = 0x8000,
} LinphonePrivacy;
/* WARNING This enum MUST be kept in sync with the SalPrivacy enum from sal.h */

/**
 * A mask of #LinphonePrivacy values
 * @ingroup call_control
 */
typedef unsigned int LinphonePrivacyMask;

/**
 * The LinphoneProxyConfig object represents a proxy configuration to be used
 * by the LinphoneCore object.
 * Its fields must not be used directly in favour of the accessors methods.
 * Once created and filled properly the LinphoneProxyConfig can be given to
 * LinphoneCore with linphone_core_add_proxy_config().
 * This will automatically triggers the registration, if enabled.
 *
 * The proxy configuration are persistent to restarts because they are saved
 * in the configuration file. As a consequence, after linphone_core_new() there
 * might already be a list of configured proxy that can be examined with
 * linphone_core_get_proxy_config_list().
 *
 * The default proxy (see linphone_core_set_default_proxy() ) is the one of the list
 * that is used by default for calls.
 * @ingroup proxies
**/
typedef struct _LinphoneProxyConfig LinphoneProxyConfig;

/**
 * Enum for publish states.
 * @ingroup event_api
**/
typedef enum _LinphonePublishState{
	LinphonePublishNone, /**< Initial state, do not use */
	LinphonePublishProgress, /**< An outgoing publish was created and submitted */
	LinphonePublishOk, /**< Publish is accepted */
	LinphonePublishError, /**< Publish encoutered an error, linphone_event_get_reason() gives reason code */
	LinphonePublishExpiring, /**< Publish is about to expire, only sent if [sip]->refresh_generic_publish property is set to 0 */
	LinphonePublishCleared /**< Event has been un published */
} LinphonePublishState;

/**
 * Enum describing various failure reasons or contextual information for some events.
 * @see linphone_call_get_reason()
 * @see linphone_proxy_config_get_error()
 * @see linphone_error_info_get_reason()
 * @ingroup misc
**/
typedef enum _LinphoneReason{
	LinphoneReasonNone, /**< No reason has been set by the core */
	LinphoneReasonNoResponse, /**< No response received from remote */
	LinphoneReasonForbidden, /**< Authentication failed due to bad credentials or resource forbidden */
	LinphoneReasonDeclined, /**< The call has been declined */
	LinphoneReasonNotFound, /**< Destination of the call was not found */
	LinphoneReasonNotAnswered, /**< The call was not answered in time (request timeout) */
	LinphoneReasonBusy, /**< Phone line was busy */
	LinphoneReasonUnsupportedContent, /**< Unsupported content */
	LinphoneReasonIOError, /**< Transport error: connection failures, disconnections etc... */
	LinphoneReasonDoNotDisturb, /**< Do not disturb reason */
	LinphoneReasonUnauthorized, /**< Operation is unauthorized because missing credential */
	LinphoneReasonNotAcceptable, /**< Operation is rejected due to incompatible or unsupported media parameters */
	LinphoneReasonNoMatch, /**< Operation could not be executed by server or remote client because it didn't have any context for it */
	LinphoneReasonMovedPermanently, /**< Resource moved permanently */
	LinphoneReasonGone, /**< Resource no longer exists */
	LinphoneReasonTemporarilyUnavailable, /**< Temporarily unavailable */
	LinphoneReasonAddressIncomplete, /**< Address incomplete */
	LinphoneReasonNotImplemented, /**< Not implemented */
	LinphoneReasonBadGateway, /**< Bad gateway */
	LinphoneReasonServerTimeout, /**< Server timeout */
	LinphoneReasonUnknown /**< Unknown reason */
} LinphoneReason;

#define LinphoneReasonBadCredentials LinphoneReasonForbidden

/*for compatibility*/
#define LinphoneReasonMedia LinphoneReasonUnsupportedContent

/**
 * LinphoneRegistrationState describes proxy registration states.
 * @ingroup proxies
**/
typedef enum _LinphoneRegistrationState {
	LinphoneRegistrationNone, /**< Initial state for registrations */
	LinphoneRegistrationProgress, /**< Registration is in progress */
	LinphoneRegistrationOk,	/**< Registration is successful */
	LinphoneRegistrationCleared, /**< Unregistration succeeded */
	LinphoneRegistrationFailed	/**< Registration failed */
} LinphoneRegistrationState;

typedef struct _LinphoneRingtonePlayer LinphoneRingtonePlayer;

/**
 * Linphone core SIP transport ports.
 * Special values #LC_SIP_TRANSPORT_RANDOM, #LC_SIP_TRANSPORT_RANDOM, #LC_SIP_TRANSPORT_DONTBIND can be used.
 * Use with #linphone_core_set_sip_transports
 * @deprecated
 * @donotwrap
 */
typedef struct _LinphoneSipTransports {
	int udp_port; /**< SIP/UDP port */
	int tcp_port; /**< SIP/TCP port */
	int dtls_port; /**< SIP/DTLS port */
	int tls_port; /**< SIP/TLS port */
} LinphoneSipTransports;

/**
 * Linphone core SIP transport ports.
 * Special values #LC_SIP_TRANSPORT_RANDOM, #LC_SIP_TRANSPORT_RANDOM, #LC_SIP_TRANSPORT_DONTBIND can be used.
 * Use with #linphone_core_set_sip_transports
 * @ingroup initializing
 */
typedef struct _LinphoneTransports LinphoneTransports;

/**
 * Old name of LinphoneSipTransports
 * @deprecated
 * @donotwrap
 */
LINPHONE_DEPRECATED typedef struct _LinphoneSipTransports LCSipTransports;

typedef struct _LinphoneSoundDaemon LinphoneSoundDaemon;

/**
 * Enum describing the stream types.
 * @ingroup initializing
**/
typedef enum _LinphoneStreamType {
	LinphoneStreamTypeAudio,
	LinphoneStreamTypeVideo,
	LinphoneStreamTypeText,
	LinphoneStreamTypeUnknown /* WARNING: Make sure this value remains the last one in the list */
} LinphoneStreamType;

/**
 * Enum controlling behavior for incoming subscription request.
 * Use by linphone_friend_set_inc_subscribe_policy()
 * @ingroup buddy_list
 */
typedef enum _LinphoneSubscribePolicy {
	/**
	 * Does not automatically accept an incoming subscription request.
	 * This policy implies that a decision has to be taken for each incoming subscription request notified by callback LinphoneCoreVTable.new_subscription_requested
	 */
	LinphoneSPWait,
	LinphoneSPDeny, /**< Rejects incoming subscription request */
	LinphoneSPAccept /**< Automatically accepts a subscription request */
} LinphoneSubscribePolicy;

/**
 * Enum for subscription direction (incoming or outgoing).
 * @ingroup event_api
**/
typedef enum _LinphoneSubscriptionDir{
	LinphoneSubscriptionIncoming, /**< Incoming subscription. */
	LinphoneSubscriptionOutgoing, /**< Outgoing subscription. */
	LinphoneSubscriptionInvalidDir /**< Invalid subscription direction. */
} LinphoneSubscriptionDir;

/**
 * Enum for subscription states.
 * LinphoneSubscriptionTerminated and LinphoneSubscriptionError are final states.
 * @ingroup event_api
**/
typedef enum _LinphoneSubscriptionState{
	LinphoneSubscriptionNone, /**< Initial state, should not be used */
	LinphoneSubscriptionOutgoingProgress, /**< An outgoing subcription was sent */
	LinphoneSubscriptionIncomingReceived, /**< An incoming subcription is received */
	LinphoneSubscriptionPending, /**< Subscription is pending, waiting for user approval */
	LinphoneSubscriptionActive, /**< Subscription is accepted */
	LinphoneSubscriptionTerminated, /**< Subscription is terminated normally */
	LinphoneSubscriptionError, /**< Subscription was terminated by an error, indicated by linphone_event_get_reason() */
	LinphoneSubscriptionExpiring, /**< Subscription is about to expire, only sent if [sip]->refresh_generic_subscribe property is set to 0 */
} LinphoneSubscriptionState;

/**
 * Enum listing frequent telephony tones.
 * @ingroup misc
**/
typedef enum _LinphoneToneID {
	LinphoneToneUndefined, /**< Not a tone */
	LinphoneToneBusy, /**< Busy tone */
	LinphoneToneCallWaiting, /** Call waiting tone */
	LinphoneToneCallOnHold, /** Call on hold tone */
	LinphoneToneCallLost /** Tone played when call is abruptly disconnected (media lost)*/
} LinphoneToneID;

/**
 * Enum describing transport type for LinphoneAddress.
 * @ingroup linphone_address
**/
typedef enum _LinphoneTransportType {
	LinphoneTransportUdp,
	LinphoneTransportTcp,
	LinphoneTransportTls,
	LinphoneTransportDtls
} LinphoneTransportType;
/* WARNING This enum MUST be kept in sync with the SalTransport enum from sal.h */

/**
 * Linphone tunnel object.
 * @ingroup tunnel
 */
typedef struct _LinphoneTunnel LinphoneTunnel;

/**
 * @brief Class to store tunnel settings.
 * @ingroup tunnel
 */
typedef struct _LinphoneTunnelConfig LinphoneTunnelConfig;

/**
 * Enum describing the tunnel modes.
 * @ingroup tunnel
**/
typedef enum _LinphoneTunnelMode {
	LinphoneTunnelModeDisable, /**< The tunnel is disabled */
	LinphoneTunnelModeEnable, /**< The tunnel is enabled */
	LinphoneTunnelModeAuto /**< The tunnel is enabled automatically if it is required */
} LinphoneTunnelMode;

/**
 * Enum describing uPnP states.
 * @ingroup initializing
**/
typedef enum _LinphoneUpnpState {
	LinphoneUpnpStateIdle, /**< uPnP is not activate */
	LinphoneUpnpStatePending, /**< uPnP process is in progress */
	LinphoneUpnpStateAdding, /**< Internal use: Only used by port binding */
	LinphoneUpnpStateRemoving, /**< Internal use: Only used by port binding */
	LinphoneUpnpStateNotAvailable, /**< uPnP is not available */
	LinphoneUpnpStateOk, /**< uPnP is enabled */
	LinphoneUpnpStateKo, /**< uPnP processing has failed */
	LinphoneUpnpStateBlacklisted, /**< IGD router is blacklisted */
} LinphoneUpnpState;

/**
 * The LinphoneVcard object.
 * @ingroup carddav_vcard
 */
typedef struct _LinphoneVcard LinphoneVcard;

/**
 * Enum describing the result of a version update check.
 * @ingroup misc
 */
typedef enum _LinphoneVersionUpdateCheckResult {
	LinphoneVersionUpdateCheckUpToDate,
	LinphoneVersionUpdateCheckNewVersionAvailable,
	LinphoneVersionUpdateCheckError
} LinphoneVersionUpdateCheckResult;

/**
 * The LinphoneVideoDefinition object represents a video definition, eg. its width and its height.
 * @ingroup media_parameters
 */
typedef struct _LinphoneVideoDefinition LinphoneVideoDefinition;

/**
 * Structure describing policy regarding video streams establishments.
 * @ingroup media_parameters
 * @deprecated
 * @donotwrap
**/
typedef struct _LinphoneVideoPolicy {
	bool_t automatically_initiate; /**<Whether video shall be automatically proposed for outgoing calls.*/
	bool_t automatically_accept; /**<Whether video shall be automatically accepted for incoming calls*/
	bool_t unused[2];
} LinphoneVideoPolicy;

/**
 * Structure describing policy regarding video streams establishments.
 * @ingroup media_parameters
**/
typedef struct _LinphoneVideoActivationPolicy LinphoneVideoActivationPolicy;

typedef struct LinphoneVideoSizeDef {
	MSVideoSize vsize;
	const char *name;
} LinphoneVideoSizeDef;

/**
 * Old name of LinphoneVideoSizeDef
 * @deprecated
 */
typedef LinphoneVideoSizeDef MSVideoSizeDef;

typedef enum _LinphoneWaitingState {
	LinphoneWaitingStart,
	LinphoneWaitingProgress,
	LinphoneWaitingFinished
} LinphoneWaitingState;

/**
* Enum describing the types of argument for LinphoneXmlRpcRequest.
* @ingroup misc
**/
typedef enum _LinphoneXmlRpcArgType {
	LinphoneXmlRpcArgNone,
	LinphoneXmlRpcArgInt,
	LinphoneXmlRpcArgString
} LinphoneXmlRpcArgType;

/**
 * The LinphoneXmlRpcRequest object representing a XML-RPC request to be sent.
 * @ingroup misc
**/
typedef struct _LinphoneXmlRpcRequest LinphoneXmlRpcRequest;

/**
 * An object to handle the callbacks for handling the LinphoneXmlRpcRequest operations.
 * @ingroup misc
**/
typedef struct _LinphoneXmlRpcRequestCbs LinphoneXmlRpcRequestCbs;

/**
 * The LinphoneXmlRpcSession object used to send XML-RPC requests and handle their responses.
 * @ingroup misc
**/
typedef struct _LinphoneXmlRpcSession LinphoneXmlRpcSession;

/**
* Enum describing the status of a LinphoneXmlRpcRequest.
* @ingroup misc
**/
typedef enum _LinphoneXmlRpcStatus {
	LinphoneXmlRpcStatusPending,
	LinphoneXmlRpcStatusOk,
	LinphoneXmlRpcStatusFailed
} LinphoneXmlRpcStatus;

typedef struct _LsdPlayer LsdPlayer;

/**
 * Object representing an RTP payload type.
 * @ingroup media_parameters
 */
typedef struct _LinphonePayloadType LinphonePayloadType;

/**
 * Structure describing a range of integers
 * @ingroup misc
 */
typedef struct _LinphoneRange LinphoneRange;

/**
 * Status code returned by some functions to
 * notify whether the execution has been succesfully
 * done or not.
 * @ingroup misc
 */
typedef int LinphoneStatus;

/**
 * Object representing a chain of protocol headers.
 * It provides read/write access to the headers of the underlying protocol.
 * @ingroup misc
**/
typedef struct _LinphoneHeaders LinphoneHeaders;

#endif /* LINPHONE_TYPES_H_ */
