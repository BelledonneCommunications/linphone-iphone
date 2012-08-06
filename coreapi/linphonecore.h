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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef LINPHONECORE_H
#define LINPHONECORE_H

#include "ortp/ortp.h"
#include "ortp/payloadtype.h"
#include "mediastreamer2/mscommon.h"
#include "mediastreamer2/msvideo.h"

#ifdef IN_LINPHONE
#include "sipsetup.h"
#else
#include "linphone/sipsetup.h"
#endif

#define LINPHONE_IPADDR_SIZE 64
#define LINPHONE_HOSTNAME_SIZE 128

#ifdef __cplusplus
extern "C" {
#endif

struct _MSSndCard;
struct _LinphoneCore;
/**
 * Linphone core main object created by function linphone_core_new() .
 * @ingroup initializing
 */
typedef struct _LinphoneCore LinphoneCore;
struct SalOp;

struct _LpConfig;


struct _LCSipTransports{
	int udp_port;
	int tcp_port;
	int dtls_port;
	int tls_port;
};

typedef struct _LCSipTransports LCSipTransports;

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
 *
 * @ingroup linphone_address
 * @var LinphoneAddress
 */
typedef struct SalAddress LinphoneAddress;
#ifdef IN_LINPHONE
#include "linphonefriend.h"
#else
#include "linphone/linphonefriend.h"
#endif

LinphoneAddress * linphone_address_new(const char *uri);
LinphoneAddress * linphone_address_clone(const LinphoneAddress *uri);
const char *linphone_address_get_scheme(const LinphoneAddress *u);
const char *linphone_address_get_display_name(const LinphoneAddress* u);
const char *linphone_address_get_username(const LinphoneAddress *u);
const char *linphone_address_get_domain(const LinphoneAddress *u);
/**
 * Get port number as an integer value.
 *
 */
int linphone_address_get_port_int(const LinphoneAddress *u);
/**
 * Get port number, null if not present.
 */
const char* linphone_address_get_port(const LinphoneAddress *u);
void linphone_address_set_display_name(LinphoneAddress *u, const char *display_name);
void linphone_address_set_username(LinphoneAddress *uri, const char *username);
void linphone_address_set_domain(LinphoneAddress *uri, const char *host);
void linphone_address_set_port(LinphoneAddress *uri, const char *port);
void linphone_address_set_port_int(LinphoneAddress *uri, int port);
/*remove tags, params etc... so that it is displayable to the user*/
void linphone_address_clean(LinphoneAddress *uri);
char *linphone_address_as_string(const LinphoneAddress *u);
char *linphone_address_as_string_uri_only(const LinphoneAddress *u);
bool_t linphone_address_weak_equal(const LinphoneAddress *a1, const LinphoneAddress *a2);
void linphone_address_destroy(LinphoneAddress *u);

struct _SipSetupContext;


/**
 * Enum representing the direction of a call.
 * @ingroup call_logs
**/
enum _LinphoneCallDir {
	LinphoneCallOutgoing, /**< outgoing calls*/
	LinphoneCallIncoming  /**< incoming calls*/
};

/**
 * Typedef for enum
 * @ingroup call_logs
**/
typedef enum _LinphoneCallDir LinphoneCallDir;

/**
 * Enum representing the status of a call
 * @ingroup call_logs
**/
typedef enum _LinphoneCallStatus {
	LinphoneCallSuccess, /**< The call was sucessful*/
	LinphoneCallAborted, /**< The call was aborted */
	LinphoneCallMissed, /**< The call was missed (unanswered)*/
	LinphoneCallDeclined /**< The call was declined, either locally or by remote end*/
} LinphoneCallStatus;

/**
 * Structure representing a call log.
 *
 * @ingroup call_logs
 *
**/
typedef struct _LinphoneCallLog{
	LinphoneCallDir dir; /**< The direction of the call*/
	LinphoneCallStatus status; /**< The status of the call*/
	LinphoneAddress *from; /**<Originator of the call as a LinphoneAddress object*/
	LinphoneAddress *to; /**<Destination of the call as a LinphoneAddress object*/
	char start_date[128]; /**<Human readable string containg the start date*/
	int duration; /**<Duration of the call in seconds*/
	char *refkey;
	void *user_pointer;
	rtp_stats_t local_stats;
	rtp_stats_t remote_stats;
	float quality;
    int video_enabled;
	struct _LinphoneCore *lc;
	time_t start_date_time; /**Start date of the call in seconds as expressed in a time_t */
} LinphoneCallLog;


/**
 * Enum describing type of media encryption types.
**/
enum LinphoneMediaEncryption {
	LinphoneMediaEncryptionNone,
	LinphoneMediaEncryptionSRTP,
	LinphoneMediaEncryptionZRTP
};

/**
 * Enum describing type of media encryption types.
**/
typedef enum LinphoneMediaEncryption LinphoneMediaEncryption;

/*public: */
void linphone_call_log_set_user_pointer(LinphoneCallLog *cl, void *up);
void *linphone_call_log_get_user_pointer(const LinphoneCallLog *cl);
void linphone_call_log_set_ref_key(LinphoneCallLog *cl, const char *refkey);
const char *linphone_call_log_get_ref_key(const LinphoneCallLog *cl);
const rtp_stats_t *linphone_call_log_get_local_stats(const LinphoneCallLog *cl);
const rtp_stats_t *linphone_call_log_get_remote_stats(const LinphoneCallLog *cl);
char * linphone_call_log_to_str(LinphoneCallLog *cl);

struct _LinphoneCallParams;

/**
 * The LinphoneCallParams is an object containing various call related parameters.
 * It can be used to retrieve parameters from a currently running call or modify the call's characteristics
 * dynamically.
**/
typedef struct _LinphoneCallParams LinphoneCallParams;

const PayloadType* linphone_call_params_get_used_audio_codec(const LinphoneCallParams *cp);
const PayloadType* linphone_call_params_get_used_video_codec(const LinphoneCallParams *cp);
LinphoneCallParams * linphone_call_params_copy(const LinphoneCallParams *cp);
void linphone_call_params_enable_video(LinphoneCallParams *cp, bool_t enabled);
bool_t linphone_call_params_video_enabled(const LinphoneCallParams *cp);
LinphoneMediaEncryption linphone_call_params_get_media_encryption(const LinphoneCallParams *cp);
void linphone_call_params_set_media_encryption(LinphoneCallParams *cp, LinphoneMediaEncryption e);
void linphone_call_params_enable_early_media_sending(LinphoneCallParams *cp, bool_t enabled);
bool_t linphone_call_params_early_media_sending_enabled(const LinphoneCallParams *cp);
bool_t linphone_call_params_local_conference_mode(const LinphoneCallParams *cp);
void linphone_call_params_set_audio_bandwidth_limit(LinphoneCallParams *cp, int bw);
void linphone_call_params_destroy(LinphoneCallParams *cp);

/**
 * Enum describing failure reasons.
 * @ingroup initializing
**/
enum _LinphoneReason{
	LinphoneReasonNone,
	LinphoneReasonNoResponse, /**<No response received from remote*/
	LinphoneReasonBadCredentials, /**<Authentication failed due to bad or missing credentials*/
	LinphoneReasonDeclined, /**<The call has been declined*/
	LinphoneReasonNotFound, /**<Destination of the calls was not found.*/
	LinphoneReasonNotAnswered
};

/**
 * Enum describing failure reasons.
 * @ingroup initializing
**/
typedef enum _LinphoneReason LinphoneReason;

const char *linphone_reason_to_string(LinphoneReason err);

/**
 * Structure describing policy regarding video streams establishments.
 * @ingroup media_parameters
**/
struct _LinphoneVideoPolicy{
	bool_t automatically_initiate; /**<Whether video shall be automatically proposed for outgoing calls.*/ 
	bool_t automatically_accept; /**<Whether video shall be automatically accepted for incoming calls*/
	bool_t unused[2];
};

/**
 * Structure describing policy regarding video streams establishments.
 * @ingroup media_parameters
**/
typedef struct _LinphoneVideoPolicy LinphoneVideoPolicy;

/**
 * The LinphoneCall object represents a call issued or received by the LinphoneCore
 * @ingroup call_control
**/
struct _LinphoneCall;
/**
 * The LinphoneCall object represents a call issued or received by the LinphoneCore
 * @ingroup call_control
**/
typedef struct _LinphoneCall LinphoneCall;


/**
 * @addtogroup call_misc
 * @{
**/

#define LINPHONE_CALL_STATS_AUDIO 0
#define LINPHONE_CALL_STATS_VIDEO 1

/**
 * The LinphoneCallStats objects carries various statistic informations regarding quality of audio or video streams.
 *
 * To receive these informations periodically and as soon as they are computed, the application is invited to place a #CallStatsUpdated callback in the LinphoneCoreVTable structure
 * it passes for instanciating the LinphoneCore object (see linphone_core_new() ).
 *
 * At any time, the application can access last computed statistics using linphone_call_get_audio_stats() or linphone_call_get_video_stats().
**/
typedef struct _LinphoneCallStats LinphoneCallStats;

/**
 * The LinphoneCallStats objects carries various statistic informations regarding quality of audio or video streams.
 *
 * To receive these informations periodically and as soon as they are computed, the application is invited to place a #CallStatsUpdated callback in the LinphoneCoreVTable structure
 * it passes for instanciating the LinphoneCore object (see linphone_core_new() ).
 *
 * At any time, the application can access last computed statistics using linphone_call_get_audio_stats() or linphone_call_get_video_stats().
**/
struct _LinphoneCallStats {
	int		type; /**< Can be either LINPHONE_CALL_STATS_AUDIO or LINPHONE_CALL_STATS_VIDEO*/
	jitter_stats_t	jitter_stats; /**<jitter buffer statistics, see oRTP documentation for details */
	mblk_t*		received_rtcp; /**<Last RTCP packet received, as a mblk_t structure. See oRTP documentation for details how to extract information from it*/
	mblk_t*		sent_rtcp;/**<Last RTCP packet sent, as a mblk_t structure. See oRTP documentation for details how to extract information from it*/
	float		round_trip_delay; /**<Round trip propagation time in seconds if known, -1 if unknown.*/
};

/**
 * @}
**/

const LinphoneCallStats *linphone_call_get_audio_stats(const LinphoneCall *call);
const LinphoneCallStats *linphone_call_get_video_stats(const LinphoneCall *call);


/** Callback prototype */
typedef void (*LinphoneCallCbFunc)(struct _LinphoneCall *call,void * user_data);

/**
 * LinphoneCallState enum represents the different state a call can reach into.
 * The application is notified of state changes through the LinphoneCoreVTable::call_state_changed callback.
 * @ingroup call_control
**/
typedef enum _LinphoneCallState{
	LinphoneCallIdle,					/**<Initial call state */
	LinphoneCallIncomingReceived, /**<This is a new incoming call */
	LinphoneCallOutgoingInit, /**<An outgoing call is started */
	LinphoneCallOutgoingProgress, /**<An outgoing call is in progress */
	LinphoneCallOutgoingRinging, /**<An outgoing call is ringing at remote end */
	LinphoneCallOutgoingEarlyMedia, /**<An outgoing call is proposed early media */
	LinphoneCallConnected, /**<Connected, the call is answered */
	LinphoneCallStreamsRunning, /**<The media streams are established and running*/
	LinphoneCallPausing, /**<The call is pausing at the initiative of local end */
	LinphoneCallPaused, /**< The call is paused, remote end has accepted the pause */
	LinphoneCallResuming, /**<The call is being resumed by local end*/
	LinphoneCallRefered, /**<The call is being transfered to another party, resulting in a new outgoing call to follow immediately*/
	LinphoneCallError, /**<The call encountered an error*/
	LinphoneCallEnd, /**<The call ended normally*/
	LinphoneCallPausedByRemote, /**<The call is paused by remote end*/
	LinphoneCallUpdatedByRemote, /**<The call's parameters change is requested by remote end, used for example when video is added by remote */
	LinphoneCallIncomingEarlyMedia, /**<We are proposing early media to an incoming call */
	LinphoneCallUpdated, /**<The remote accepted the call update initiated by us */
	LinphoneCallReleased /**< The call object is no more retained by the core */
} LinphoneCallState;

const char *linphone_call_state_to_string(LinphoneCallState cs);

LinphoneCore *linphone_call_get_core(const LinphoneCall *call);
LinphoneCallState linphone_call_get_state(const LinphoneCall *call);
bool_t linphone_call_asked_to_autoanswer(LinphoneCall *call);
const LinphoneAddress * linphone_core_get_current_call_remote_address(struct _LinphoneCore *lc);
const LinphoneAddress * linphone_call_get_remote_address(const LinphoneCall *call);
char *linphone_call_get_remote_address_as_string(const LinphoneCall *call);
LinphoneCallDir linphone_call_get_dir(const LinphoneCall *call);
LinphoneCall * linphone_call_ref(LinphoneCall *call);
void linphone_call_unref(LinphoneCall *call);
LinphoneCallLog *linphone_call_get_call_log(const LinphoneCall *call);
const char *linphone_call_get_refer_to(const LinphoneCall *call);
bool_t linphone_call_has_transfer_pending(const LinphoneCall *call);
LinphoneCall *linphone_call_get_replaced_call(LinphoneCall *call);
int linphone_call_get_duration(const LinphoneCall *call);
const LinphoneCallParams * linphone_call_get_current_params(const LinphoneCall *call);
const LinphoneCallParams * linphone_call_get_remote_params(LinphoneCall *call);
void linphone_call_enable_camera(LinphoneCall *lc, bool_t enabled);
bool_t linphone_call_camera_enabled(const LinphoneCall *lc);
int linphone_call_take_video_snapshot(LinphoneCall *call, const char *file);
LinphoneReason linphone_call_get_reason(const LinphoneCall *call);
const char *linphone_call_get_remote_user_agent(LinphoneCall *call);
float linphone_call_get_play_volume(LinphoneCall *call);
float linphone_call_get_record_volume(LinphoneCall *call);
float linphone_call_get_current_quality(LinphoneCall *call);
float linphone_call_get_average_quality(LinphoneCall *call);
const char* linphone_call_get_authentication_token(LinphoneCall *call);
bool_t linphone_call_get_authentication_token_verified(LinphoneCall *call);
void linphone_call_set_authentication_token_verified(LinphoneCall *call, bool_t verified);
void linphone_call_send_vfu_request(LinphoneCall *call);
void *linphone_call_get_user_pointer(LinphoneCall *call);
void linphone_call_set_user_pointer(LinphoneCall *call, void *user_pointer);
void linphone_call_set_next_video_frame_decoded_callback(LinphoneCall *call, LinphoneCallCbFunc cb, void* user_data);
LinphoneCallState linphone_call_get_transfer_state(LinphoneCall *call);
    
/**
 * Enables or disable echo cancellation for this call
 * @param call
 * @param val
 *
 * @ingroup media_parameters
**/
void linphone_call_enable_echo_cancellation(LinphoneCall *call, bool_t val) ;
/**
 * Returns TRUE if echo cancellation is enabled.
 *
 * @ingroup media_parameters
**/
bool_t linphone_call_echo_cancellation_enabled(LinphoneCall *lc);
/**
 * Enables or disable echo limiter for this call
 * @param call
 * @param val
 *
 * @ingroup media_parameters
**/
void linphone_call_enable_echo_limiter(LinphoneCall *call, bool_t val);
/**
 * Returns TRUE if echo limiter is enabled.
 *
 * @ingroup media_parameters
**/
bool_t linphone_call_echo_limiter_enabled(const LinphoneCall *call);

/*keep this in sync with mediastreamer2/msvolume.h*/

/**
 * Lowest volume measurement that can be returned by linphone_call_get_play_volume() or linphone_call_get_record_volume(), corresponding to pure silence.
 * @ingroup call_misc
**/
#define LINPHONE_VOLUME_DB_LOWEST (-120)

/**
 * @addtogroup proxies
 * @{
**/
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
**/
typedef struct _LinphoneProxyConfig LinphoneProxyConfig;

/**
 * LinphoneRegistrationState describes proxy registration states.
**/
typedef enum _LinphoneRegistrationState{
	LinphoneRegistrationNone, /**<Initial state for registrations */
	LinphoneRegistrationProgress, /**<Registration is in progress */
	LinphoneRegistrationOk,	/**< Registration is successful */
	LinphoneRegistrationCleared, /**< Unregistration succeeded */
	LinphoneRegistrationFailed	/**<Registration failed */
}LinphoneRegistrationState;

/**
 * Human readable version of the #LinphoneRegistrationState
 * @param cs sate
 */
const char *linphone_registration_state_to_string(LinphoneRegistrationState cs);

LinphoneProxyConfig *linphone_proxy_config_new(void);
int linphone_proxy_config_set_server_addr(LinphoneProxyConfig *obj, const char *server_addr);
int linphone_proxy_config_set_identity(LinphoneProxyConfig *obj, const char *identity);
int linphone_proxy_config_set_route(LinphoneProxyConfig *obj, const char *route);
void linphone_proxy_config_expires(LinphoneProxyConfig *obj, int expires);
/**
 * Indicates  either or not, REGISTRATION must be issued for this #LinphoneProxyConfig .
 * <br> In case this #LinphoneProxyConfig has been added to #LinphoneCore, follows the linphone_proxy_config_edit() rule.
 * @param obj object pointer
 * @param val if true, registration will be engaged
 */
void linphone_proxy_config_enable_register(LinphoneProxyConfig *obj, bool_t val);
#define linphone_proxy_config_enableregister linphone_proxy_config_enable_register
void linphone_proxy_config_edit(LinphoneProxyConfig *obj);
int linphone_proxy_config_done(LinphoneProxyConfig *obj);
/**
 * Indicates  either or not, PUBLISH must be issued for this #LinphoneProxyConfig .
 * <br> In case this #LinphoneProxyConfig has been added to #LinphoneCore, follows the linphone_proxy_config_edit() rule.
 * @param obj object pointer
 * @param val if true, publish will be engaged
 *
 */
void linphone_proxy_config_enable_publish(LinphoneProxyConfig *obj, bool_t val);
void linphone_proxy_config_set_dial_escape_plus(LinphoneProxyConfig *cfg, bool_t val);
void linphone_proxy_config_set_dial_prefix(LinphoneProxyConfig *cfg, const char *prefix);

LinphoneRegistrationState linphone_proxy_config_get_state(const LinphoneProxyConfig *obj);
bool_t linphone_proxy_config_is_registered(const LinphoneProxyConfig *obj);
const char *linphone_proxy_config_get_domain(const LinphoneProxyConfig *cfg);

const char *linphone_proxy_config_get_route(const LinphoneProxyConfig *obj);
const char *linphone_proxy_config_get_identity(const LinphoneProxyConfig *obj);
bool_t linphone_proxy_config_publish_enabled(const LinphoneProxyConfig *obj);
const char *linphone_proxy_config_get_addr(const LinphoneProxyConfig *obj);
int linphone_proxy_config_get_expires(const LinphoneProxyConfig *obj);
bool_t linphone_proxy_config_register_enabled(const LinphoneProxyConfig *obj);
void linphone_proxy_config_refresh_register(LinphoneProxyConfig *obj);
const char *linphone_proxy_config_get_contact_parameters(const LinphoneProxyConfig *obj);
void linphone_proxy_config_set_contact_parameters(LinphoneProxyConfig *obj, const char *contact_params);
struct _LinphoneCore * linphone_proxy_config_get_core(const LinphoneProxyConfig *obj);

bool_t linphone_proxy_config_get_dial_escape_plus(const LinphoneProxyConfig *cfg);
const char * linphone_proxy_config_get_dial_prefix(const LinphoneProxyConfig *cfg);

LinphoneReason linphone_proxy_config_get_error(const LinphoneProxyConfig *cfg);

/* destruction is called automatically when removing the proxy config */
void linphone_proxy_config_destroy(LinphoneProxyConfig *cfg);
void linphone_proxy_config_set_sip_setup(LinphoneProxyConfig *cfg, const char *type);
SipSetupContext *linphone_proxy_config_get_sip_setup_context(LinphoneProxyConfig *cfg);
SipSetup *linphone_proxy_config_get_sip_setup(LinphoneProxyConfig *cfg);
/**
 * normalize a human readable phone number into a basic string. 888-444-222 becomes 888444222
 */
int linphone_proxy_config_normalize_number(LinphoneProxyConfig *proxy, const char *username, char *result, size_t result_len);
/*
 *  attached a user data to a proxy config
 */
void linphone_proxy_config_set_user_data(LinphoneProxyConfig *cr, void * ud);
/*
 *  get user data to a proxy config. return null if any
 */
void * linphone_proxy_config_get_user_data(LinphoneProxyConfig *cr);

/**
 * @}
**/

typedef struct _LinphoneAccountCreator{
	struct _LinphoneCore *lc;
	struct _SipSetupContext *ssctx;
	char *username;
	char *password;
	char *domain;
	char *route;
	char *email;
	int suscribe;
	bool_t succeeded;
}LinphoneAccountCreator;

LinphoneAccountCreator *linphone_account_creator_new(struct _LinphoneCore *core, const char *type);
void linphone_account_creator_set_username(LinphoneAccountCreator *obj, const char *username);
void linphone_account_creator_set_password(LinphoneAccountCreator *obj, const char *password);
void linphone_account_creator_set_domain(LinphoneAccountCreator *obj, const char *domain);
void linphone_account_creator_set_route(LinphoneAccountCreator *obj, const char *route);
void linphone_account_creator_set_email(LinphoneAccountCreator *obj, const char *email);
void linphone_account_creator_set_suscribe(LinphoneAccountCreator *obj, int suscribre);
const char * linphone_account_creator_get_username(LinphoneAccountCreator *obj);
const char * linphone_account_creator_get_domain(LinphoneAccountCreator *obj);
int linphone_account_creator_test_existence(LinphoneAccountCreator *obj);
int linphone_account_creator_test_validation(LinphoneAccountCreator *obj);
LinphoneProxyConfig * linphone_account_creator_validate(LinphoneAccountCreator *obj);
void linphone_account_creator_destroy(LinphoneAccountCreator *obj);

struct _LinphoneAuthInfo;

/**
 * @ingroup authentication
 * Object holding authentication information.
 *
 * @note The object's fields should not be accessed directly. Prefer using
 * the accessor methods.
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
**/
typedef struct _LinphoneAuthInfo LinphoneAuthInfo;

LinphoneAuthInfo *linphone_auth_info_new(const char *username, const char *userid,
		const char *passwd, const char *ha1,const char *realm);
void linphone_auth_info_set_passwd(LinphoneAuthInfo *info, const char *passwd);
void linphone_auth_info_set_username(LinphoneAuthInfo *info, const char *username);
void linphone_auth_info_set_userid(LinphoneAuthInfo *info, const char *userid);

const char *linphone_auth_info_get_username(const LinphoneAuthInfo *i);
const char *linphone_auth_info_get_passwd(const LinphoneAuthInfo *i);
const char *linphone_auth_info_get_userid(const LinphoneAuthInfo *i);

/* you don't need those function*/
void linphone_auth_info_destroy(LinphoneAuthInfo *info);
LinphoneAuthInfo * linphone_auth_info_new_from_config_file(struct _LpConfig *config, int pos);


struct _LinphoneChatRoom;
/**
 * @addtogroup chatroom
 * @{
 */
/**
 * A chat room is the place where text messages are exchanged.
 * <br> Can be created by linphone_core_create_chat_room().
 */
typedef struct _LinphoneChatRoom LinphoneChatRoom;
/**
 * Create a new chat room for messaging from a sip uri like sip:joe@sip.linphone.org
 * @param lc #LinphoneCore object
 * @param to destination address for messages
 * @return #LinphoneChatRoom where messaging can take place.
 */
LinphoneChatRoom * linphone_core_create_chat_room(LinphoneCore *lc, const char *to);
/**
 * Destructor
 * @param cr #LinphoneChatRoom object
 */
void linphone_chat_room_destroy(LinphoneChatRoom *cr);


/**
 * get peer address \link linphone_core_create_chat_room() associated to \endlink this #LinphoneChatRoom
 * @param cr #LinphoneChatRoom object
 * @return #LinphoneAddress peer address
 */
const LinphoneAddress* linphone_chat_room_get_peer_address(LinphoneChatRoom *cr);
/**
 * send a message to peer member of this chat room.
 * @param cr #LinphoneChatRoom object
 * @param msg message to be sent
 */
void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg);

void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void * ud);
void * linphone_chat_room_get_user_data(LinphoneChatRoom *cr);

/**
 * @}
 */


/**
 * @addtogroup initializing
 * @{
**/

/**
 * LinphoneGlobalState describes the global state of the LinphoneCore object.
 * It is notified via the LinphoneCoreVTable::global_state_changed
**/
typedef enum _LinphoneGlobalState{
	LinphoneGlobalOff,
	LinphoneGlobalStartup,
	LinphoneGlobalOn,
	LinphoneGlobalShutdown
}LinphoneGlobalState;

const char *linphone_global_state_to_string(LinphoneGlobalState gs);


/**Call state notification callback prototype*/
typedef void (*LinphoneGlobalStateCb)(struct _LinphoneCore *lc, LinphoneGlobalState gstate, const char *message);
/**Call state notification callback prototype*/
typedef void (*LinphoneCallStateCb)(struct _LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *message);
/**Call encryption changed callback prototype*/
typedef void (*CallEncryptionChangedCb)(struct _LinphoneCore *lc, LinphoneCall *call, bool_t on, const char *authentication_token);

/** @ingroup Proxies
 * Registration state notification callback prototype
 * */
typedef void (*LinphoneRegistrationStateCb)(struct _LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message);
/** Callback prototype */
typedef void (*ShowInterfaceCb)(struct _LinphoneCore *lc);
/** Callback prototype */
typedef void (*DisplayStatusCb)(struct _LinphoneCore *lc, const char *message);
/** Callback prototype */
typedef void (*DisplayMessageCb)(struct _LinphoneCore *lc, const char *message);
/** Callback prototype */
typedef void (*DisplayUrlCb)(struct _LinphoneCore *lc, const char *message, const char *url);
/** Callback prototype */
typedef void (*LinphoneCoreCbFunc)(struct _LinphoneCore *lc,void * user_data);
/** Callback prototype */
typedef void (*NotifyReceivedCb)(struct _LinphoneCore *lc, LinphoneCall *call, const char *from, const char *event);
/**
 * Report status change for a friend previously \link linphone_core_add_friend() added \endlink to #LinphoneCore.
 * @param lc #LinphoneCore object .
 * @param lf Updated #LinphoneFriend .
 */
typedef void (*NotifyPresenceReceivedCb)(struct _LinphoneCore *lc, LinphoneFriend * lf);
/**
 *  Reports that a new subscription request has been received and wait for a decision.
 *  <br> Status on this subscription request is notified by \link linphone_friend_set_inc_subscribe_policy() changing policy \endlink for this friend
 *	@param lc #LinphoneCore object
 *	@param lf #LinphoneFriend corresponding to the subscriber
 *	@param url of the subscriber
 *  Callback prototype
 *  */
typedef void (*NewSubscribtionRequestCb)(struct _LinphoneCore *lc, LinphoneFriend *lf, const char *url);
/** Callback prototype */
typedef void (*AuthInfoRequested)(struct _LinphoneCore *lc, const char *realm, const char *username);
/** Callback prototype */
typedef void (*CallLogUpdated)(struct _LinphoneCore *lc, struct _LinphoneCallLog *newcl);
/**
 * Callback prototype
 *
 * @param lc #LinphoneCore object
 * @param room #LinphoneChatRoom involved in this conversation. Can be be created by the framework in case \link #LinphoneAddress the from \endlink is not present in any chat room.
 * @param from #LinphoneAddress from
 * @param message incoming message
 *  */
typedef void (*TextMessageReceived)(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message);
/** Callback prototype */
typedef void (*DtmfReceived)(struct _LinphoneCore* lc, LinphoneCall *call, int dtmf);
/** Callback prototype */
typedef void (*ReferReceived)(struct _LinphoneCore *lc, const char *refer_to);
/** Callback prototype */
typedef void (*BuddyInfoUpdated)(struct _LinphoneCore *lc, LinphoneFriend *lf);
/** Callback prototype for in progress transfers. The new_call_state is the state of the call resulting of the transfer, at the other party. */
typedef void (*LinphoneTransferStateChanged)(struct _LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state);
/** Callback prototype for receiving quality statistics for calls*/
typedef void (*CallStatsUpdated)(struct _LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *stats);

/**
 * This structure holds all callbacks that the application should implement.
 *  None is mandatory.
**/
typedef struct _LinphoneVTable{
	LinphoneGlobalStateCb global_state_changed; /**<Notifies globlal state changes*/
	LinphoneRegistrationStateCb registration_state_changed;/**<Notifies registration state changes*/
	LinphoneCallStateCb call_state_changed;/**<Notifies call state changes*/
	NotifyPresenceReceivedCb notify_presence_recv; /**< Notify received presence events*/
	NewSubscribtionRequestCb new_subscription_request; /**< Notify about pending subscription request */
	AuthInfoRequested auth_info_requested; /**< Ask the application some authentication information */
	CallLogUpdated call_log_updated; /**< Notifies that call log list has been updated */
	TextMessageReceived text_received; /**< A text message has been received */
	DtmfReceived dtmf_received; /**< A dtmf has been received received */
	ReferReceived refer_received; /**< An out of call refer was received */
	CallEncryptionChangedCb call_encryption_changed; /**<Notifies on change in the encryption of call streams */
	LinphoneTransferStateChanged transfer_state_changed; /**<Notifies when a transfer is in progress */
	BuddyInfoUpdated buddy_info_updated; /**< a LinphoneFriend's BuddyInfo has changed*/
	NotifyReceivedCb notify_recv; /**< Other notifications*/
	CallStatsUpdated call_stats_updated; /**<Notifies on refreshing of call's statistics. */
	DisplayStatusCb display_status; /**< DEPRECATED Callback that notifies various events with human readable text.*/
	DisplayMessageCb display_message;/**< DEPRECATED Callback to display a message to the user */
	DisplayMessageCb display_warning;/**< DEPRECATED Callback to display a warning to the user */
	DisplayUrlCb display_url; /**< DEPRECATED */
	ShowInterfaceCb show; /**< DEPRECATED Notifies the application that it should show up*/
} LinphoneCoreVTable;

/**
 * @}
**/

typedef struct _LCCallbackObj
{
  LinphoneCoreCbFunc _func;
  void * _user_data;
}LCCallbackObj;



typedef enum _LinphoneFirewallPolicy{
	LinphonePolicyNoFirewall,
	LinphonePolicyUseNatAddress,
	LinphonePolicyUseStun
} LinphoneFirewallPolicy;

typedef enum _LinphoneWaitingState{
	LinphoneWaitingStart,
	LinphoneWaitingProgress,
	LinphoneWaitingFinished
} LinphoneWaitingState;
typedef void * (*LinphoneWaitingCallback)(struct _LinphoneCore *lc, void *context, LinphoneWaitingState ws, const char *purpose, float progress);


/* THE main API */

void linphone_core_enable_logs(FILE *file);
void linphone_core_enable_logs_with_cb(OrtpLogFunc logfunc);
void linphone_core_disable_logs(void);
/*sets the user-agent string in sip messages, must be set before linphone_core_new() or linphone_core_init() */
void linphone_core_set_user_agent(const char *ua_name, const char *version);
const char *linphone_core_get_version(void);

LinphoneCore *linphone_core_new(const LinphoneCoreVTable *vtable,
						const char *config_path, const char *factory_config, void* userdata);

/* function to be periodically called in a main loop */
void linphone_core_iterate(LinphoneCore *lc);

LinphoneAddress * linphone_core_interpret_url(LinphoneCore *lc, const char *url);

LinphoneCall * linphone_core_invite(LinphoneCore *lc, const char *url);

LinphoneCall * linphone_core_invite_address(LinphoneCore *lc, const LinphoneAddress *addr);

LinphoneCall * linphone_core_invite_with_params(LinphoneCore *lc, const char *url, const LinphoneCallParams *params);

LinphoneCall * linphone_core_invite_address_with_params(LinphoneCore *lc, const LinphoneAddress *addr, const LinphoneCallParams *params);

int linphone_core_transfer_call(LinphoneCore *lc, LinphoneCall *call, const char *refer_to);

int linphone_core_transfer_call_to_another(LinphoneCore *lc, LinphoneCall *call, LinphoneCall *dest);

bool_t linphone_core_inc_invite_pending(LinphoneCore*lc);

bool_t linphone_core_in_call(const LinphoneCore *lc);

LinphoneCall *linphone_core_get_current_call(const LinphoneCore *lc);

int linphone_core_accept_call(LinphoneCore *lc, LinphoneCall *call);

int linphone_core_accept_call_with_params(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params);

int linphone_core_terminate_call(LinphoneCore *lc, LinphoneCall *call);

int linphone_core_redirect_call(LinphoneCore *lc, LinphoneCall *call, const char *redirect_uri);

int linphone_core_terminate_all_calls(LinphoneCore *lc);

int linphone_core_pause_call(LinphoneCore *lc, LinphoneCall *call);

int linphone_core_pause_all_calls(LinphoneCore *lc);

int linphone_core_resume_call(LinphoneCore *lc, LinphoneCall *call);

int linphone_core_update_call(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params);

int linphone_core_defer_call_update(LinphoneCore *lc, LinphoneCall *call);

int linphone_core_accept_call_update(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params);

LinphoneCallParams *linphone_core_create_default_call_parameters(LinphoneCore *lc);

LinphoneCall *linphone_core_get_call_by_remote_address(LinphoneCore *lc, const char *remote_address);

void linphone_core_send_dtmf(LinphoneCore *lc,char dtmf);

int linphone_core_set_primary_contact(LinphoneCore *lc, const char *contact);

const char *linphone_core_get_primary_contact(LinphoneCore *lc);

const char * linphone_core_get_identity(LinphoneCore *lc);

void linphone_core_set_guess_hostname(LinphoneCore *lc, bool_t val);
bool_t linphone_core_get_guess_hostname(LinphoneCore *lc);

bool_t linphone_core_ipv6_enabled(LinphoneCore *lc);
void linphone_core_enable_ipv6(LinphoneCore *lc, bool_t val);

LinphoneAddress *linphone_core_get_primary_contact_parsed(LinphoneCore *lc);
const char * linphone_core_get_identity(LinphoneCore *lc);
/*0= no bandwidth limit*/
void linphone_core_set_download_bandwidth(LinphoneCore *lc, int bw);
void linphone_core_set_upload_bandwidth(LinphoneCore *lc, int bw);

int linphone_core_get_download_bandwidth(const LinphoneCore *lc);
int linphone_core_get_upload_bandwidth(const LinphoneCore *lc);

void linphone_core_enable_adaptive_rate_control(LinphoneCore *lc, bool_t enabled);
bool_t linphone_core_adaptive_rate_control_enabled(const LinphoneCore *lc);
/**
 * set audio packetization time linphone expect to receive from peer
 * @ingroup media_parameters
 *
 */
void linphone_core_set_download_ptime(LinphoneCore *lc, int ptime);
/**
 * get audio packetization time linphone expect to receive from peer, 0 means unspecified
 * @ingroup media_parameters
 */
int  linphone_core_get_download_ptime(LinphoneCore *lc);

void linphone_core_set_upload_ptime(LinphoneCore *lc, int ptime);

int linphone_core_get_upload_ptime(LinphoneCore *lc);

/* returns a MSList of PayloadType */
const MSList *linphone_core_get_audio_codecs(const LinphoneCore *lc);

int linphone_core_set_audio_codecs(LinphoneCore *lc, MSList *codecs);
/* returns a MSList of PayloadType */
const MSList *linphone_core_get_video_codecs(const LinphoneCore *lc);

int linphone_core_set_video_codecs(LinphoneCore *lc, MSList *codecs);

bool_t linphone_core_payload_type_enabled(LinphoneCore *lc, const PayloadType *pt);

int linphone_core_enable_payload_type(LinphoneCore *lc, PayloadType *pt, bool_t enable);

PayloadType* linphone_core_find_payload_type(LinphoneCore* lc, const char* type, int rate) ;

int linphone_core_get_payload_type_number(LinphoneCore *lc, const PayloadType *pt);

const char *linphone_core_get_payload_type_description(LinphoneCore *lc, PayloadType *pt);

bool_t linphone_core_check_payload_type_usability(LinphoneCore *lc, PayloadType *pt);

int linphone_core_add_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *config);

void linphone_core_clear_proxy_config(LinphoneCore *lc);

void linphone_core_remove_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *config);

const MSList *linphone_core_get_proxy_config_list(const LinphoneCore *lc);

void linphone_core_set_default_proxy(LinphoneCore *lc, LinphoneProxyConfig *config);

void linphone_core_set_default_proxy_index(LinphoneCore *lc, int index);

int linphone_core_get_default_proxy(LinphoneCore *lc, LinphoneProxyConfig **config);

void linphone_core_add_auth_info(LinphoneCore *lc, const LinphoneAuthInfo *info);

void linphone_core_remove_auth_info(LinphoneCore *lc, const LinphoneAuthInfo *info);

const MSList *linphone_core_get_auth_info_list(const LinphoneCore *lc);

const LinphoneAuthInfo *linphone_core_find_auth_info(LinphoneCore *lc, const char *realm, const char *username);

void linphone_core_abort_authentication(LinphoneCore *lc,  LinphoneAuthInfo *info);

void linphone_core_clear_all_auth_info(LinphoneCore *lc);

int linphone_core_get_audio_jittcomp(LinphoneCore *lc);

void linphone_core_set_audio_jittcomp(LinphoneCore *lc, int value);

int linphone_core_get_audio_port(const LinphoneCore *lc);

int linphone_core_get_video_port(const LinphoneCore *lc);

int linphone_core_get_nortp_timeout(const LinphoneCore *lc);

void linphone_core_set_audio_port(LinphoneCore *lc, int port);

void linphone_core_set_video_port(LinphoneCore *lc, int port);

void linphone_core_set_nortp_timeout(LinphoneCore *lc, int port);

void linphone_core_set_use_info_for_dtmf(LinphoneCore *lc, bool_t use_info);

bool_t linphone_core_get_use_info_for_dtmf(LinphoneCore *lc);

void linphone_core_set_use_rfc2833_for_dtmf(LinphoneCore *lc,bool_t use_rfc2833);

bool_t linphone_core_get_use_rfc2833_for_dtmf(LinphoneCore *lc);

void linphone_core_set_sip_port(LinphoneCore *lc, int port);

int linphone_core_get_sip_port(LinphoneCore *lc);

int linphone_core_set_sip_transports(LinphoneCore *lc, const LCSipTransports *transports);

int linphone_core_get_sip_transports(LinphoneCore *lc, LCSipTransports *transports);
/**
 *
 * Give access to the UDP sip socket. Can be useful to configure this socket as persistent I.E kCFStreamNetworkServiceType set to kCFStreamNetworkServiceTypeVoIP)
 * @param lc #LinphoneCore
 * @return socket file descriptor
 */
ortp_socket_t linphone_core_get_sip_socket(LinphoneCore *lc);

void linphone_core_set_inc_timeout(LinphoneCore *lc, int seconds);

int linphone_core_get_inc_timeout(LinphoneCore *lc);

void linphone_core_set_stun_server(LinphoneCore *lc, const char *server);

const char * linphone_core_get_stun_server(const LinphoneCore *lc);

void linphone_core_set_nat_address(LinphoneCore *lc, const char *addr);

const char *linphone_core_get_nat_address(const LinphoneCore *lc);

void linphone_core_set_firewall_policy(LinphoneCore *lc, LinphoneFirewallPolicy pol);

LinphoneFirewallPolicy linphone_core_get_firewall_policy(const LinphoneCore *lc);

const char * linphone_core_get_relay_addr(const LinphoneCore *lc);

int linphone_core_set_relay_addr(LinphoneCore *lc, const char *addr);

/* sound functions */
/* returns a null terminated static array of string describing the sound devices */
const char**  linphone_core_get_sound_devices(LinphoneCore *lc);
void linphone_core_reload_sound_devices(LinphoneCore *lc);
bool_t linphone_core_sound_device_can_capture(LinphoneCore *lc, const char *device);
bool_t linphone_core_sound_device_can_playback(LinphoneCore *lc, const char *device);
int linphone_core_get_ring_level(LinphoneCore *lc);
int linphone_core_get_play_level(LinphoneCore *lc);
int linphone_core_get_rec_level(LinphoneCore *lc);
void linphone_core_set_ring_level(LinphoneCore *lc, int level);
void linphone_core_set_play_level(LinphoneCore *lc, int level);

void linphone_core_set_playback_gain_db(LinphoneCore *lc, float level);

float linphone_core_get_playback_gain_db(LinphoneCore *lc);
void linphone_core_set_rec_level(LinphoneCore *lc, int level);
const char * linphone_core_get_ringer_device(LinphoneCore *lc);
const char * linphone_core_get_playback_device(LinphoneCore *lc);
const char * linphone_core_get_capture_device(LinphoneCore *lc);
int linphone_core_set_ringer_device(LinphoneCore *lc, const char * devid);
int linphone_core_set_playback_device(LinphoneCore *lc, const char * devid);
int linphone_core_set_capture_device(LinphoneCore *lc, const char * devid);
char linphone_core_get_sound_source(LinphoneCore *lc);
void linphone_core_set_sound_source(LinphoneCore *lc, char source);
void linphone_core_set_ring(LinphoneCore *lc, const char *path);
const char *linphone_core_get_ring(const LinphoneCore *lc);
void linphone_core_verify_server_certificates(LinphoneCore *lc, bool_t yesno);
void linphone_core_set_root_ca(LinphoneCore *lc, const char *path);
void linphone_core_set_ringback(LinphoneCore *lc, const char *path);
const char * linphone_core_get_ringback(const LinphoneCore *lc);

void linphone_core_set_remote_ringback_tone(LinphoneCore *lc,const char *);
const char *linphone_core_get_remote_ringback_tone(const LinphoneCore *lc);

int linphone_core_preview_ring(LinphoneCore *lc, const char *ring,LinphoneCoreCbFunc func,void * userdata);
void linphone_core_enable_echo_cancellation(LinphoneCore *lc, bool_t val);
bool_t linphone_core_echo_cancellation_enabled(LinphoneCore *lc);

void linphone_core_enable_echo_limiter(LinphoneCore *lc, bool_t val);
bool_t linphone_core_echo_limiter_enabled(const LinphoneCore *lc);

void linphone_core_enable_agc(LinphoneCore *lc, bool_t val);
bool_t linphone_core_agc_enabled(const LinphoneCore *lc);

void linphone_core_mute_mic(LinphoneCore *lc, bool_t muted);
/**
 * return mic state.
 *
 * @ingroup media_parameters
**/
bool_t linphone_core_is_mic_muted(LinphoneCore *lc);

bool_t linphone_core_is_audio_muted(LinphoneCore *lc);
bool_t linphone_core_is_rtp_muted(LinphoneCore *lc);

bool_t linphone_core_get_rtp_no_xmit_on_audio_mute(const LinphoneCore *lc);
void linphone_core_set_rtp_no_xmit_on_audio_mute(LinphoneCore *lc, bool_t val);


/* returns a list of LinphoneCallLog */
const MSList * linphone_core_get_call_logs(LinphoneCore *lc);
void linphone_core_clear_call_logs(LinphoneCore *lc);
int linphone_core_get_missed_calls_count(LinphoneCore *lc);
void linphone_core_reset_missed_calls_count(LinphoneCore *lc);
void linphone_core_remove_call_log(LinphoneCore *lc, LinphoneCallLog *call_log);

/* video support */
bool_t linphone_core_video_supported(LinphoneCore *lc);
void linphone_core_enable_video(LinphoneCore *lc, bool_t vcap_enabled, bool_t display_enabled);
bool_t linphone_core_video_enabled(LinphoneCore *lc);
void linphone_core_set_video_policy(LinphoneCore *lc, const LinphoneVideoPolicy *policy);
const LinphoneVideoPolicy *linphone_core_get_video_policy(LinphoneCore *lc);

typedef struct MSVideoSizeDef{
	MSVideoSize vsize;
	const char *name;
}MSVideoSizeDef;
/* returns a zero terminated table of MSVideoSizeDef*/
const MSVideoSizeDef *linphone_core_get_supported_video_sizes(LinphoneCore *lc);
void linphone_core_set_preferred_video_size(LinphoneCore *lc, MSVideoSize vsize);
MSVideoSize linphone_core_get_preferred_video_size(LinphoneCore *lc);
void linphone_core_set_preferred_video_size_by_name(LinphoneCore *lc, const char *name);

void linphone_core_enable_video_preview(LinphoneCore *lc, bool_t val);
bool_t linphone_core_video_preview_enabled(const LinphoneCore *lc);

void linphone_core_enable_self_view(LinphoneCore *lc, bool_t val);
bool_t linphone_core_self_view_enabled(const LinphoneCore *lc);


/* returns a null terminated static array of string describing the webcams */
void linphone_core_reload_video_devices(LinphoneCore *lc);
const char**  linphone_core_get_video_devices(const LinphoneCore *lc);
int linphone_core_set_video_device(LinphoneCore *lc, const char *id);
const char *linphone_core_get_video_device(const LinphoneCore *lc);

/* Set static picture to be used when "Static picture" is the video device */
int linphone_core_set_static_picture(LinphoneCore *lc, const char *path);

/* Set and get frame rate for static picture */
int linphone_core_set_static_picture_fps(LinphoneCore *lc, float fps);
float linphone_core_get_static_picture_fps(LinphoneCore *lc);

/*function to be used for eventually setting window decorations (icons, title...)*/
unsigned long linphone_core_get_native_video_window_id(const LinphoneCore *lc);
void linphone_core_set_native_video_window_id(LinphoneCore *lc, unsigned long id);

unsigned long linphone_core_get_native_preview_window_id(const LinphoneCore *lc);
void linphone_core_set_native_preview_window_id(LinphoneCore *lc, unsigned long id);

void linphone_core_use_preview_window(LinphoneCore *lc, bool_t yesno);
int linphone_core_get_device_rotation(LinphoneCore *lc );
void linphone_core_set_device_rotation(LinphoneCore *lc, int rotation);

/* start or stop streaming video in case of embedded window */
void linphone_core_show_video(LinphoneCore *lc, bool_t show);

/*play/record support: use files instead of soundcard*/
void linphone_core_use_files(LinphoneCore *lc, bool_t yesno);
void linphone_core_set_play_file(LinphoneCore *lc, const char *file);
void linphone_core_set_record_file(LinphoneCore *lc, const char *file);

void linphone_core_play_dtmf(LinphoneCore *lc, char dtmf, int duration_ms);
void linphone_core_stop_dtmf(LinphoneCore *lc);

int linphone_core_get_current_call_duration(const LinphoneCore *lc);


int linphone_core_get_mtu(const LinphoneCore *lc);
void linphone_core_set_mtu(LinphoneCore *lc, int mtu);

/**
 * @ingroup network_parameters
 * This method is called by the application to notify the linphone core library when network is reachable.
 * Calling this method with true trigger linphone to initiate a registration process for all proxy
 * configuration with parameter register set to enable.
 * This method disable the automatic registration mode. It means you must call this method after each network state changes
 *
 */
void linphone_core_set_network_reachable(LinphoneCore* lc,bool_t value);
/**
 * @ingroup network_parameters
 * return network state either as positioned by the application or by linphone
 */
bool_t linphone_core_is_network_reachabled(LinphoneCore* lc);

/**
 *  @ingroup network_parameters
 *  enable signaling keep alive. small udp packet sent periodically to keep udp NAT association
 */
void linphone_core_enable_keep_alive(LinphoneCore* lc,bool_t enable);
/**
 *  @ingroup network_parameters
 * Is signaling keep alive
 */
bool_t linphone_core_keep_alive_enabled(LinphoneCore* lc);

void *linphone_core_get_user_data(LinphoneCore *lc);
void linphone_core_set_user_data(LinphoneCore *lc, void *userdata);

/* returns LpConfig object to read/write to the config file: usefull if you wish to extend
the config file with your own sections */
struct _LpConfig *linphone_core_get_config(LinphoneCore *lc);

/*set a callback for some blocking operations, it takes you informed of the progress of the operation*/
void linphone_core_set_waiting_callback(LinphoneCore *lc, LinphoneWaitingCallback cb, void *user_context);

/*returns the list of registered SipSetup (linphonecore plugins) */
const MSList * linphone_core_get_sip_setups(LinphoneCore *lc);

void linphone_core_destroy(LinphoneCore *lc);

/*for advanced users:*/
typedef RtpTransport * (*LinphoneRtpTransportFactoryFunc)(void *data, int port);
struct _LinphoneRtpTransportFactories{
	LinphoneRtpTransportFactoryFunc audio_rtp_func;
	void *audio_rtp_func_data;
	LinphoneRtpTransportFactoryFunc audio_rtcp_func;
	void *audio_rtcp_func_data;
	LinphoneRtpTransportFactoryFunc video_rtp_func;
	void *video_rtp_func_data;
	LinphoneRtpTransportFactoryFunc video_rtcp_func;
	void *video_rtcp_func_data;
};
typedef struct _LinphoneRtpTransportFactories LinphoneRtpTransportFactories;

void linphone_core_set_rtp_transport_factories(LinphoneCore* lc, LinphoneRtpTransportFactories *factories);

int linphone_core_get_current_call_stats(LinphoneCore *lc, rtp_stats_t *local, rtp_stats_t *remote);

const MSList *linphone_core_get_calls(LinphoneCore *lc);

LinphoneGlobalState linphone_core_get_global_state(const LinphoneCore *lc);
/**
 * force registration refresh to be initiated upon next iterate
 * @ingroup proxies
 */
void linphone_core_refresh_registers(LinphoneCore* lc);

/* Path to the file storing secrets cache */
void linphone_core_set_zrtp_secrets_file(LinphoneCore *lc, const char* file);


const LinphoneCall* linphone_core_find_call_from_uri(LinphoneCore *lc, const char *uri);

int linphone_core_add_to_conference(LinphoneCore *lc, LinphoneCall *call);
int linphone_core_add_all_to_conference(LinphoneCore *lc);
int linphone_core_remove_from_conference(LinphoneCore *lc, LinphoneCall *call);
bool_t linphone_core_is_in_conference(const LinphoneCore *lc);
int linphone_core_enter_conference(LinphoneCore *lc);
int linphone_core_leave_conference(LinphoneCore *lc);
float linphone_core_get_conference_local_input_volume(LinphoneCore *lc);

int linphone_core_terminate_conference(LinphoneCore *lc);
int linphone_core_get_conference_size(LinphoneCore *lc);

int linphone_core_get_max_calls(LinphoneCore *lc);
void linphone_core_set_max_calls(LinphoneCore *lc, int max);

bool_t linphone_core_sound_resources_locked(LinphoneCore *lc);

bool_t linphone_core_media_encryption_supported(const LinphoneCore *lc, LinphoneMediaEncryption menc);

/**
 * Choose media encryption policy to be used for RTP packets
 */
int linphone_core_set_media_encryption(LinphoneCore *lc, enum LinphoneMediaEncryption menc);
LinphoneMediaEncryption linphone_core_get_media_encryption(LinphoneCore *lc);

bool_t linphone_core_is_media_encryption_mandatory(LinphoneCore *lc);
/**
 * Defines Linphone behaviour when encryption parameters negociation fails on outoing call.
 * If set to TRUE call will fail; if set to FALSE will resend an INVITE with encryption disabled
 */
void linphone_core_set_media_encryption_mandatory(LinphoneCore *lc, bool_t m);

/**
 * Init call params using LinphoneCore's current configuration
 */
void linphone_core_init_default_params(LinphoneCore*lc, LinphoneCallParams *params);

/**
 * True if tunnel support was compiled.
 */
bool_t linphone_core_tunnel_available(void);

typedef struct LinphoneTunnel LinphoneTunnel;
/**
* get tunnel instance if available
*/
LinphoneTunnel *linphone_core_get_tunnel(LinphoneCore *lc);

void linphone_call_zoom_video(LinphoneCall* call, float zoom_factor, float* cx, float* cy);

#ifdef __cplusplus
}
#endif


#endif
