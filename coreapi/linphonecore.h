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
	LinphoneCallMissed /**< The call was missed (unanswered)*/
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
	struct _LinphoneCore *lc;
} LinphoneCallLog;



/*public: */
void linphone_call_log_set_user_pointer(LinphoneCallLog *cl, void *up);
void *linphone_call_log_get_user_pointer(const LinphoneCallLog *cl);
void linphone_call_log_set_ref_key(LinphoneCallLog *cl, const char *refkey);
const char *linphone_call_log_get_ref_key(const LinphoneCallLog *cl);
const rtp_stats_t *linphone_call_log_get_local_stats(const LinphoneCallLog *cl);
const rtp_stats_t *linphone_call_log_get_remote_stats(const LinphoneCallLog *cl);
char * linphone_call_log_to_str(LinphoneCallLog *cl);


/**
 * The LinphoneCall object represents a call issued or received by the LinphoneCore
**/
struct _LinphoneCall;
typedef struct _LinphoneCall LinphoneCall;

typedef enum _LinphoneCallState{
	LinphoneCallInit,
	LinphoneCallPreEstablishing,
	LinphoneCallRinging,
	LinphoneCallAVRunning,
	LinphoneCallPaused,
	LinphoneCallTerminated
}LinphoneCallState;

LinphoneCallState linphone_call_get_state(const LinphoneCall *call);
bool_t linphone_call_asked_to_autoanswer(LinphoneCall *call);
bool_t linphone_call_paused(LinphoneCall *call);
const LinphoneAddress * linphone_core_get_current_call_remote_address(struct _LinphoneCore *lc);
const LinphoneAddress * linphone_call_get_remote_address(const LinphoneCall *call);
char *linphone_call_get_remote_address_as_string(const LinphoneCall *call);
void linphone_call_ref(LinphoneCall *call);
void linphone_call_unref(LinphoneCall *call);
LinphoneCallLog *linphone_call_get_call_log(const LinphoneCall *call);


typedef enum{
	LinphoneSPWait,
	LinphoneSPDeny,
	LinphoneSPAccept
}LinphoneSubscribePolicy;

typedef enum _LinphoneOnlineStatus{
	LINPHONE_STATUS_OFFLINE,
	LINPHONE_STATUS_ONLINE,
	LINPHONE_STATUS_BUSY,
	LINPHONE_STATUS_BERIGHTBACK,
	LINPHONE_STATUS_AWAY,
	LINPHONE_STATUS_ONTHEPHONE,
	LINPHONE_STATUS_OUTTOLUNCH,
	LINPHONE_STATUS_NOT_DISTURB,
	LINPHONE_STATUS_MOVED,
	LINPHONE_STATUS_ALT_SERVICE,
	LINPHONE_STATUS_PENDING,
	LINPHONE_STATUS_END
}LinphoneOnlineStatus;

const char *linphone_online_status_to_string(LinphoneOnlineStatus ss);

struct _LinphoneFriend;

typedef struct _LinphoneFriend LinphoneFriend;

LinphoneFriend * linphone_friend_new();
LinphoneFriend *linphone_friend_new_with_addr(const char *addr);
int linphone_friend_set_sip_addr(LinphoneFriend *fr, const char *uri);
int linphone_friend_set_name(LinphoneFriend *fr, const char *name);
int linphone_friend_send_subscribe(LinphoneFriend *fr, bool_t val);
int linphone_friend_set_inc_subscribe_policy(LinphoneFriend *fr, LinphoneSubscribePolicy pol);
void linphone_friend_edit(LinphoneFriend *fr);
void linphone_friend_done(LinphoneFriend *fr);
void linphone_friend_destroy(LinphoneFriend *lf);
const LinphoneAddress *linphone_friend_get_address(const LinphoneFriend *lf);
bool_t linphone_friend_get_send_subscribe(const LinphoneFriend *lf);
LinphoneSubscribePolicy linphone_friend_get_inc_subscribe_policy(const LinphoneFriend *lf);
LinphoneOnlineStatus linphone_friend_get_status(const LinphoneFriend *lf);
BuddyInfo * linphone_friend_get_info(const LinphoneFriend *lf);
void linphone_friend_set_ref_key(LinphoneFriend *lf, const char *key);
const char *linphone_friend_get_ref_key(const LinphoneFriend *lf);
bool_t linphone_friend_in_list(const LinphoneFriend *lf);

#define linphone_friend_url(lf) ((lf)->url)


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

LinphoneProxyConfig *linphone_proxy_config_new(void);
int linphone_proxy_config_set_server_addr(LinphoneProxyConfig *obj, const char *server_addr);
int linphone_proxy_config_set_identity(LinphoneProxyConfig *obj, const char *identity);
int linphone_proxy_config_set_route(LinphoneProxyConfig *obj, const char *route);
void linphone_proxy_config_expires(LinphoneProxyConfig *obj, int expires);
void linphone_proxy_config_enable_register(LinphoneProxyConfig *obj, bool_t val);
#define linphone_proxy_config_enableregister linphone_proxy_config_enable_register
void linphone_proxy_config_edit(LinphoneProxyConfig *obj);
int linphone_proxy_config_done(LinphoneProxyConfig *obj);
void linphone_proxy_config_enable_publish(LinphoneProxyConfig *obj, bool_t val);
void linphone_proxy_config_set_dial_escape_plus(LinphoneProxyConfig *cfg, bool_t val);
void linphone_proxy_config_set_dial_prefix(LinphoneProxyConfig *cfg, const char *prefix);

bool_t linphone_proxy_config_is_registered(const LinphoneProxyConfig *obj);
const char *linphone_proxy_config_get_domain(const LinphoneProxyConfig *cfg);

const char *linphone_proxy_config_get_route(const LinphoneProxyConfig *obj);
const char *linphone_proxy_config_get_identity(const LinphoneProxyConfig *obj);
bool_t linphone_proxy_config_publish_enabled(const LinphoneProxyConfig *obj);
const char *linphone_proxy_config_get_addr(const LinphoneProxyConfig *obj);
int linphone_proxy_config_get_expires(const LinphoneProxyConfig *obj);
bool_t linphone_proxy_config_register_enabled(const LinphoneProxyConfig *obj);
struct _LinphoneCore * linphone_proxy_config_get_core(const LinphoneProxyConfig *obj);

bool_t linphone_proxy_config_get_dial_escape_plus(const LinphoneProxyConfig *cfg);
const char * linphone_proxy_config_get_dial_prefix(const LinphoneProxyConfig *cfg);

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
	bool_t succeeded;
}LinphoneAccountCreator;

LinphoneAccountCreator *linphone_account_creator_new(struct _LinphoneCore *core, const char *type);
void linphone_account_creator_set_username(LinphoneAccountCreator *obj, const char *username);
void linphone_account_creator_set_password(LinphoneAccountCreator *obj, const char *password);
void linphone_account_creator_set_domain(LinphoneAccountCreator *obj, const char *domain);
const char * linphone_account_creator_get_username(LinphoneAccountCreator *obj);
const char * linphone_account_creator_get_domain(LinphoneAccountCreator *obj);
int linphone_account_creator_test_existence(LinphoneAccountCreator *obj);
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
typedef struct _LinphoneChatRoom LinphoneChatRoom;

LinphoneChatRoom * linphone_core_create_chat_room(struct _LinphoneCore *lc, const char *to);
void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg);
void linphone_chat_room_destroy(LinphoneChatRoom *cr);
void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void * ud);
void * linphone_chat_room_get_user_data(LinphoneChatRoom *cr);

/* describes the different groups of states */
typedef enum _gstate_group {
  GSTATE_GROUP_POWER,
  GSTATE_GROUP_REG,
  GSTATE_GROUP_CALL
} gstate_group_t;

typedef enum _gstate {
  /* states for GSTATE_GROUP_POWER */
  GSTATE_POWER_OFF = 0,        /* initial state */
  GSTATE_POWER_STARTUP,
  GSTATE_POWER_ON,
  GSTATE_POWER_SHUTDOWN,
  /* states for GSTATE_GROUP_REG */
  GSTATE_REG_NONE = 10,       /* initial state */
  GSTATE_REG_OK,
  GSTATE_REG_FAILED,
  GSTATE_REG_PENDING, /* a registration request is ongoing*/
  /* states for GSTATE_GROUP_CALL */
  GSTATE_CALL_IDLE = 20,      /* initial state */
  GSTATE_CALL_OUT_INVITE,
  GSTATE_CALL_OUT_CONNECTED,
  GSTATE_CALL_IN_INVITE,
  GSTATE_CALL_IN_CONNECTED,
  GSTATE_CALL_END,
  GSTATE_CALL_ERROR,
  GSTATE_INVALID,
  GSTATE_CALL_OUT_RINGING /*remote ringing*/
} gstate_t;

struct _LinphoneGeneralState {
  gstate_t old_state;
  gstate_t new_state;
  gstate_group_t group;
  const char *message;
};
typedef struct _LinphoneGeneralState LinphoneGeneralState;


typedef union _LinphoneGeneralStateContext{
	LinphoneCall *call;
	LinphoneProxyConfig *proxy;
}LinphoneGeneralStateContext;

/**
 * @addtogroup initializing
 * @{
**/

/** Callback prototype */
typedef void (*ShowInterfaceCb)(struct _LinphoneCore *lc);
/** Callback prototype */
typedef void (*InviteReceivedCb)(struct _LinphoneCore *lc, LinphoneCall *call);
/** Callback prototype */
typedef void (*ByeReceivedCb)(struct _LinphoneCore *lc, LinphoneCall *call);
/** Callback prototype */
typedef void (*RingingReceivedCb)(struct _LinphoneCore *lc, LinphoneCall *call);
/** Callback prototype */
typedef void (*ConnectedReceivedCb)(struct _LinphoneCore *lc, LinphoneCall *call);
/** Callback prototype */
typedef void (*FailureReceivedCb)(struct _LinphoneCore *lc, LinphoneCall *call, int error_code);
/** Callback prototype */
typedef void (*PausedReceivedCb)(struct _LinphoneCore *lc, LinphoneCall *call);
/** Callback prototype */
typedef void (*ResumedReceivedCb)(struct _LinphoneCore *lc, LinphoneCall *call);
/** Callback prototype */
typedef void (*AckPausedReceivedCb)(struct _LinphoneCore *lc, LinphoneCall *call);
/** Callback prototype */
typedef void (*AckResumedReceivedCb)(struct _LinphoneCore *lc, LinphoneCall *call);
/** Callback prototype */
typedef void (*DisplayStatusCb)(struct _LinphoneCore *lc, const char *message);
/** Callback prototype */
typedef void (*DisplayMessageCb)(struct _LinphoneCore *lc, const char *message);
/** Callback prototype */
typedef void (*DisplayUrlCb)(struct _LinphoneCore *lc, const char *message, const char *url);
/** Callback prototype */
typedef void (*DisplayQuestionCb)(struct _LinphoneCore *lc, const char *message);
/** Callback prototype */
typedef void (*LinphoneCoreCbFunc)(struct _LinphoneCore *lc,void * user_data);
/** Callback prototype */
typedef void (*NotifyReceivedCb)(struct _LinphoneCore *lc, const char *from, const char *msg);
/** Callback prototype */
typedef void (*NotifyPresenceReceivedCb)(struct _LinphoneCore *lc, LinphoneFriend * fid);
/** Callback prototype */
typedef void (*NewUnknownSubscriberCb)(struct _LinphoneCore *lc, LinphoneFriend *lf, const char *url);
/** Callback prototype */
typedef void (*AuthInfoRequested)(struct _LinphoneCore *lc, const char *realm, const char *username);
/** Callback prototype */
typedef void (*CallLogUpdated)(struct _LinphoneCore *lc, struct _LinphoneCallLog *newcl);
/** Callback prototype */
typedef void (*TextMessageReceived)(struct _LinphoneCore *lc, LinphoneChatRoom *room, const char *from, const char *message);
/** Callback prototype */
typedef void (*GeneralStateChange)(struct _LinphoneCore *lc, LinphoneGeneralState *gstate, LinphoneGeneralStateContext ctx);
/** Callback prototype */
typedef void (*DtmfReceived)(struct _LinphoneCore* lc, LinphoneCall *call, int dtmf);
/** Callback prototype */
typedef void (*ReferReceived)(struct _LinphoneCore *lc, LinphoneCall *call, const char *refer_to);
/** Callback prototype */
typedef void (*BuddyInfoUpdated)(struct _LinphoneCore *lc, LinphoneFriend *lf);

/**
 * This structure holds all callbacks that the application should implement.
 * 
**/
typedef struct _LinphoneVTable
{
	ShowInterfaceCb show; /**< Notifies the application that it should show up*/
	InviteReceivedCb inv_recv; /**< Notifies incoming calls */
	ByeReceivedCb bye_recv; /**< Notify calls terminated by far end*/
	RingingReceivedCb ringing_recv; /**< Notify that the distant phone is ringing*/
	ConnectedReceivedCb connected_recv; /**< Notify that the distant phone answered the call*/
	FailureReceivedCb failure_recv; /**< Notify that the call failed*/
	PausedReceivedCb paused_recv; /**< Notify that the call has been paused*/
	ResumedReceivedCb resumed_recv; /**< Notify that the call has been resumed*/
	AckPausedReceivedCb ack_paused_recv;/**< Notify that the previous command pause sent to the call has been acknowledge*/
	AckResumedReceivedCb ack_resumed_recv;/**< Notify that the previous command resumed sent to the call has been acknowledge*/	
	NotifyPresenceReceivedCb notify_presence_recv; /**< Notify received presence events*/
	NewUnknownSubscriberCb new_unknown_subscriber; /**< Notify about unknown subscriber */
	AuthInfoRequested auth_info_requested; /**< Ask the application some authentication information */
	DisplayStatusCb display_status; /**< Callback that notifies various events with human readable text.*/
	DisplayMessageCb display_message;/**< Callback to display a message to the user */
	DisplayMessageCb display_warning;/** Callback to display a warning to the user */
	DisplayUrlCb display_url;
	DisplayQuestionCb display_question;
	CallLogUpdated call_log_updated; /**< Notifies that call log list has been updated */
	TextMessageReceived text_received; /**< A text message has been received */
	GeneralStateChange general_state; /**< State notification callback */
	DtmfReceived dtmf_received; /**< A dtmf has been received received */
	ReferReceived refer_received; /**< A refer was received */
	BuddyInfoUpdated buddy_info_updated; /**< a LinphoneFriend's BuddyInfo has changed*/
	NotifyReceivedCb notify_recv; /**< Other notifications*/
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
	LINPHONE_POLICY_NO_FIREWALL,
	LINPHONE_POLICY_USE_NAT_ADDRESS,
	LINPHONE_POLICY_USE_STUN
} LinphoneFirewallPolicy;

typedef enum _LinphoneWaitingState{
	LinphoneWaitingStart,
	LinphoneWaitingProgress,
	LinphoneWaitingFinished
} LinphoneWaitingState;
typedef void * (*LinphoneWaitingCallback)(struct _LinphoneCore *lc, void *context, LinphoneWaitingState ws, const char *purpose, float progress);

typedef struct _LinphoneCore LinphoneCore;

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

int linphone_core_refer(LinphoneCore *lc, LinphoneCall *call, const char *url);

bool_t linphone_core_inc_invite_pending(LinphoneCore*lc);

bool_t linphone_core_in_call(const LinphoneCore *lc);

LinphoneCall *linphone_core_get_current_call(LinphoneCore *lc);

int linphone_core_accept_call(LinphoneCore *lc, LinphoneCall *call);

int linphone_core_terminate_call(LinphoneCore *lc, LinphoneCall *call);

int linphone_core_terminate_all_calls(LinphoneCore *lc);

int linphone_core_pause_call(LinphoneCore *lc, LinphoneCall *call);

int linphone_core_resume_call(LinphoneCore *lc, LinphoneCall *call);

LinphoneCall *linphone_core_get_call_by_remote_address(LinphoneCore *lc, const char *remote_address);

void linphone_core_send_dtmf(LinphoneCore *lc,char dtmf);

int linphone_core_set_primary_contact(LinphoneCore *lc, const char *contact);

const char *linphone_core_get_primary_contact(LinphoneCore *lc);

void linphone_core_set_guess_hostname(LinphoneCore *lc, bool_t val);
bool_t linphone_core_get_guess_hostname(LinphoneCore *lc);

bool_t linphone_core_ipv6_enabled(LinphoneCore *lc);
void linphone_core_enable_ipv6(LinphoneCore *lc, bool_t val);

LinphoneAddress *linphone_core_get_primary_contact_parsed(LinphoneCore *lc);

/*0= no bandwidth limit*/
void linphone_core_set_download_bandwidth(LinphoneCore *lc, int bw);
void linphone_core_set_upload_bandwidth(LinphoneCore *lc, int bw);

int linphone_core_get_download_bandwidth(const LinphoneCore *lc);
int linphone_core_get_upload_bandwidth(const LinphoneCore *lc);
/**
 * set audio packetization time linphone expect to received from peer
 * @ingroup media_parameters
 *
 */
void linphone_core_set_download_ptime(LinphoneCore *lc, int ptime);
/**
 * get audio packetization time linphone expect to received from peer, 0 means unspecified
 * @ingroup media_parameters
 */
int  linphone_core_get_download_ptime(LinphoneCore *lc);

#ifdef VINCENT_MAURY_RSVP
/* QoS functions */
int linphone_core_set_rpc_mode(LinphoneCore *lc, int on); /* on = 1 (RPC_ENABLE = 1) */
int linphone_core_set_rsvp_mode(LinphoneCore *lc, int on); /* on = 1 (RSVP_ENABLE = 1) */
int linphone_core_change_qos(LinphoneCore *lc, int answer); /* answer = 1 for yes, 0 for no */
#endif

/* returns a MSList of PayloadType */
const MSList *linphone_core_get_audio_codecs(const LinphoneCore *lc);

int linphone_core_set_audio_codecs(LinphoneCore *lc, MSList *codecs);
/* returns a MSList of PayloadType */
const MSList *linphone_core_get_video_codecs(const LinphoneCore *lc);

int linphone_core_set_video_codecs(LinphoneCore *lc, MSList *codecs);

bool_t linphone_core_payload_type_enabled(LinphoneCore *lc, PayloadType *pt);

int linphone_core_enable_payload_type(LinphoneCore *lc, PayloadType *pt, bool_t enable);

/*
 * get payload type  from mime type an clock rate
 * @ingroup media_parameters
 * iterates both audio an video
 * return NULL if not found
 */
PayloadType* linphone_core_find_payload_type(LinphoneCore* lc, const char* type, int rate) ;

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
void linphone_core_set_ringback(LinphoneCore *lc, const char *path);
const char * linphone_core_get_ringback(const LinphoneCore *lc);
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

void linphone_core_set_presence_info(LinphoneCore *lc,int minutes_away,const char *contact,LinphoneOnlineStatus os);

LinphoneOnlineStatus linphone_core_get_presence_info(const LinphoneCore *lc);

void linphone_core_interpret_friend_uri(LinphoneCore *lc, const char *uri, char **result);
void linphone_core_add_friend(LinphoneCore *lc, LinphoneFriend *fr);
void linphone_core_remove_friend(LinphoneCore *lc, LinphoneFriend *fr);
void linphone_core_reject_subscriber(LinphoneCore *lc, LinphoneFriend *lf);
/* a list of LinphoneFriend */
const MSList * linphone_core_get_friend_list(const LinphoneCore *lc);
/* notify all friends that have subscribed */
void linphone_core_notify_all_friends(LinphoneCore *lc, LinphoneOnlineStatus os);
LinphoneFriend *linphone_core_get_friend_by_address(const LinphoneCore *lc, const char *addr);
LinphoneFriend *linphone_core_get_friend_by_ref_key(const LinphoneCore *lc, const char *key);

/* returns a list of LinphoneCallLog */
const MSList * linphone_core_get_call_logs(LinphoneCore *lc);
void linphone_core_clear_call_logs(LinphoneCore *lc);

/* video support */
void linphone_core_enable_video(LinphoneCore *lc, bool_t vcap_enabled, bool_t display_enabled);
bool_t linphone_core_video_enabled(LinphoneCore *lc);

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
const char**  linphone_core_get_video_devices(const LinphoneCore *lc);
int linphone_core_set_video_device(LinphoneCore *lc, const char *id);
const char *linphone_core_get_video_device(const LinphoneCore *lc);

/* Set static picture to be used when "Static picture" is the video device */
int linphone_core_set_static_picture(LinphoneCore *lc, const char *path);

/*function to be used for eventually setting window decorations (icons, title...)*/
unsigned long linphone_core_get_native_video_window_id(const LinphoneCore *lc);


/*play/record support: use files instead of soundcard*/
void linphone_core_use_files(LinphoneCore *lc, bool_t yesno);
void linphone_core_set_play_file(LinphoneCore *lc, const char *file);
void linphone_core_set_record_file(LinphoneCore *lc, const char *file);

gstate_t linphone_core_get_state(const LinphoneCore *lc, gstate_group_t group);
int linphone_core_get_current_call_duration(const LinphoneCore *lc);
const LinphoneAddress *linphone_core_get_remote_address(LinphoneCore *lc);

int linphone_core_get_mtu(const LinphoneCore *lc);
void linphone_core_set_mtu(LinphoneCore *lc, int mtu);

/**
 * This method is called by the application to notify the linphone core library when network is reachable.
 * Calling this method with true trigger linphone to initiate a registration process for all proxy
 * configuration with parameter register set to enable.
 * This method disable the automatic registration mode. It means you must call this method after each network state changes
 *
 */
void linphone_core_set_network_reachable(LinphoneCore* lc,bool_t value);
/**
 * return network state either as positioned by the application or by linphone
 */
bool_t linphone_core_is_network_reachabled(LinphoneCore* lc);


void *linphone_core_get_user_data(LinphoneCore *lc);

/* returns LpConfig object to read/write to the config file: usefull if you wish to extend
the config file with your own sections */
struct _LpConfig *linphone_core_get_config(LinphoneCore *lc);

/* attempts to wake up another linphone engine already running.
The "show" callback is called for the other linphone, causing gui to show up.
call_addr is an optional sip-uri to call immediately after waking up.
The method returns 0 if an already running linphone was found*/

int linphone_core_wake_up_possible_already_running_instance(
    const char * config_file, const char * call_addr);

/*set a callback for some blocking operations, it takes you informed of the progress of the operation*/
void linphone_core_set_waiting_callback(LinphoneCore *lc, LinphoneWaitingCallback cb, void *user_context);

/*returns the list of registered SipSetup (linphonecore plugins) */
const MSList * linphone_core_get_sip_setups(LinphoneCore *lc);

void linphone_core_destroy(LinphoneCore *lc);

/*for advanced users:*/
void linphone_core_set_audio_transports(LinphoneCore *lc, RtpTransport *rtp, RtpTransport *rtcp);

int linphone_core_get_current_call_stats(LinphoneCore *lc, rtp_stats_t *local, rtp_stats_t *remote);

#ifdef __cplusplus
}
#endif
MSList *linphone_core_get_calls(LinphoneCore *lc);
void *linphone_call_get_user_pointer(LinphoneCall *call);
void linphone_call_set_user_pointer(LinphoneCall *call, void *user_pointer);

#endif
