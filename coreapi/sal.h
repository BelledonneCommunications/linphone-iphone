/*
linphone
Copyright (C) 2010  Simon MORLAT (simon.morlat@free.fr)

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

/** 
 This header files defines the Signaling Abstraction Layer.
 The purpose of this layer is too allow experiment different call signaling 
 protocols and implementations under linphone, for example SIP, JINGLE...
**/

#ifndef sal_h
#define sal_h

#include "mediastreamer2/mscommon.h"
#include "ortp/ortp_srtp.h"

/*Dirty hack, keep in sync with mediastreamer2/include/mediastream.h */
#ifndef PAYLOAD_TYPE_FLAG_CAN_RECV
#define PAYLOAD_TYPE_FLAG_CAN_RECV	PAYLOAD_TYPE_USER_FLAG_1
#define PAYLOAD_TYPE_FLAG_CAN_SEND	PAYLOAD_TYPE_USER_FLAG_2
#endif
struct Sal;

typedef struct Sal Sal;

struct SalOp;

typedef struct SalOp SalOp;

struct SalAddress;

typedef struct SalAddress SalAddress;

typedef enum {
	SalTransportUDP, /*UDP*/
	SalTransportTCP, /*TCP*/
	SalTransportTLS, /*TLS*/
	SalTransportDTLS /*DTLS*/
}SalTransport;

const char* sal_transport_to_string(SalTransport transport);
SalTransport sal_transport_parse(const char*);
/* Address manipulation API*/
SalAddress * sal_address_new(const char *uri);
SalAddress * sal_address_clone(const SalAddress *addr);
const char *sal_address_get_scheme(const SalAddress *addr);
const char *sal_address_get_display_name(const SalAddress* addr);
char *sal_address_get_display_name_unquoted(const SalAddress *addr);
const char *sal_address_get_username(const SalAddress *addr);
const char *sal_address_get_domain(const SalAddress *addr);
const char * sal_address_get_port(const SalAddress *addr);
int sal_address_get_port_int(const SalAddress *addr);
SalTransport sal_address_get_transport(const SalAddress* addr);

void sal_address_set_display_name(SalAddress *addr, const char *display_name);
void sal_address_set_username(SalAddress *addr, const char *username);
void sal_address_set_domain(SalAddress *addr, const char *host);
void sal_address_set_port(SalAddress *addr, const char *port);
void sal_address_set_port_int(SalAddress *uri, int port);
void sal_address_clean(SalAddress *addr);
char *sal_address_as_string(const SalAddress *u);
char *sal_address_as_string_uri_only(const SalAddress *u);
void sal_address_destroy(SalAddress *u);
void sal_address_set_param(SalAddress *u,const char* name,const char* value);
void sal_address_set_transport(SalAddress* addr,SalTransport transport);


Sal * sal_init();
void sal_uninit(Sal* sal);
void sal_set_user_pointer(Sal *sal, void *user_data);
void *sal_get_user_pointer(const Sal *sal);


typedef enum {
	SalAudio,
	SalVideo,
	SalOther
} SalStreamType;

typedef enum{
	SalProtoUnknown,
	SalProtoRtpAvp,
	SalProtoRtpSavp
}SalMediaProto;

typedef enum{
	SalStreamSendRecv,
	SalStreamSendOnly,
	SalStreamRecvOnly,
	SalStreamInactive
}SalStreamDir;

typedef struct SalEndpointCandidate{
	char addr[64];
	int port;
}SalEndpointCandidate;

#define SAL_ENDPOINT_CANDIDATE_MAX 2

typedef struct SalSrtpCryptoAlgo {
	unsigned int tag;
	enum ortp_srtp_crypto_suite_t algo;
	/* 41= 40 max(key_length for all algo) + '\0' */
	char master_key[41];
} SalSrtpCryptoAlgo;

#define SAL_CRYPTO_ALGO_MAX 4

typedef struct SalStreamDescription{
	SalMediaProto proto;
	SalStreamType type;
	char typeother[32];
	char addr[64];
	int port;
	MSList *payloads; //<list of PayloadType
	int bandwidth;
	int ptime;
	SalEndpointCandidate candidates[SAL_ENDPOINT_CANDIDATE_MAX];
	SalStreamDir dir;
	SalSrtpCryptoAlgo crypto[SAL_CRYPTO_ALGO_MAX];
	unsigned int crypto_local_tag;
	int max_rate;
} SalStreamDescription;

#define SAL_MEDIA_DESCRIPTION_MAX_STREAMS 4

typedef struct SalMediaDescription{
	int refcount;
	char addr[64];
	char username[64];
	int nstreams;
	int bandwidth;
	unsigned int session_ver;
	unsigned int session_id;
	SalStreamDescription streams[SAL_MEDIA_DESCRIPTION_MAX_STREAMS];
} SalMediaDescription;

SalMediaDescription *sal_media_description_new();
void sal_media_description_ref(SalMediaDescription *md);
void sal_media_description_unref(SalMediaDescription *md);
bool_t sal_media_description_empty(const SalMediaDescription *md);
bool_t sal_media_description_equals(const SalMediaDescription *md1, const SalMediaDescription *md2);
bool_t sal_media_description_has_dir(const SalMediaDescription *md, SalStreamDir dir);
SalStreamDescription *sal_media_description_find_stream(SalMediaDescription *md,
    SalMediaProto proto, SalStreamType type);
void sal_media_description_set_dir(SalMediaDescription *md, SalStreamDir stream_dir);

/*this structure must be at the first byte of the SalOp structure defined by implementors*/
typedef struct SalOpBase{
	Sal *root;
	char *route; /*or request-uri for REGISTER*/
	char *contact;
	char *from;
	char *to;
	char *origin;
	char *remote_ua;
	SalMediaDescription *local_media;
	SalMediaDescription *remote_media;
	void *user_pointer;
} SalOpBase;


typedef enum SalError{
	SalErrorNoResponse,
	SalErrorProtocol,
	SalErrorFailure, /* see SalReason for more details */
	SalErrorUnknown
} SalError;

typedef enum SalReason{
	SalReasonDeclined,
	SalReasonBusy,
	SalReasonRedirect,
	SalReasonTemporarilyUnavailable,
	SalReasonNotFound,
	SalReasonDoNotDisturb,
	SalReasonMedia,
	SalReasonForbidden,
	SalReasonUnknown
}SalReason;

typedef enum SalPresenceStatus{
	SalPresenceOffline,
	SalPresenceOnline,
	SalPresenceBusy,
	SalPresenceBerightback,
	SalPresenceAway,
	SalPresenceOnthephone,
	SalPresenceOuttolunch,
	SalPresenceDonotdisturb,
	SalPresenceMoved,
	SalPresenceAltService,
}SalPresenceStatus;

typedef enum SalReferStatus{
	SalReferTrying,
	SalReferSuccess,
	SalReferFailed
}SalReferStatus;

typedef enum SalSubscribeStatus{
	SalSubscribeActive,
	SalSubscribeTerminated
}SalSubscribeStatus;

typedef void (*SalOnCallReceived)(SalOp *op);
typedef void (*SalOnCallRinging)(SalOp *op);
typedef void (*SalOnCallAccepted)(SalOp *op);
typedef void (*SalOnCallAck)(SalOp *op);
typedef void (*SalOnCallUpdating)(SalOp *op);/*< Called when a reINVITE is received*/
typedef void (*SalOnCallTerminated)(SalOp *op, const char *from);
typedef void (*SalOnCallFailure)(SalOp *op, SalError error, SalReason reason, const char *details, int code);
typedef void (*SalOnCallReleased)(SalOp *salop);
typedef void (*SalOnAuthRequested)(SalOp *op, const char *realm, const char *username);
typedef void (*SalOnAuthSuccess)(SalOp *op, const char *realm, const char *username);
typedef void (*SalOnRegisterSuccess)(SalOp *op, bool_t registered);
typedef void (*SalOnRegisterFailure)(SalOp *op, SalError error, SalReason reason, const char *details);
typedef void (*SalOnVfuRequest)(SalOp *op);
typedef void (*SalOnDtmfReceived)(SalOp *op, char dtmf);
typedef void (*SalOnRefer)(Sal *sal, SalOp *op, const char *referto);
typedef void (*SalOnTextReceived)(Sal *sal, const char *from, const char *msg);
typedef void (*SalOnNotify)(SalOp *op, const char *from, const char *event);
typedef void (*SalOnNotifyRefer)(SalOp *op, SalReferStatus state);
typedef void (*SalOnNotifyPresence)(SalOp *op, SalSubscribeStatus ss, SalPresenceStatus status, const char *msg);
typedef void (*SalOnSubscribeReceived)(SalOp *salop, const char *from);
typedef void (*SalOnSubscribeClosed)(SalOp *salop, const char *from);
typedef void (*SalOnPingReply)(SalOp *salop);

typedef struct SalCallbacks{
	SalOnCallReceived call_received;
	SalOnCallRinging call_ringing;
	SalOnCallAccepted call_accepted;
	SalOnCallAck call_ack;
	SalOnCallUpdating call_updating;
	SalOnCallTerminated call_terminated;
	SalOnCallFailure call_failure;
	SalOnCallReleased call_released;
	SalOnAuthRequested auth_requested;
	SalOnAuthSuccess auth_success;
	SalOnRegisterSuccess register_success;
	SalOnRegisterFailure register_failure;
	SalOnVfuRequest vfu_request;
	SalOnDtmfReceived dtmf_received;
	SalOnRefer refer_received;
	SalOnTextReceived text_received;
	SalOnNotify notify;
	SalOnNotifyPresence notify_presence;
	SalOnNotifyRefer notify_refer;
	SalOnSubscribeReceived subscribe_received;
	SalOnSubscribeClosed subscribe_closed;
	SalOnPingReply ping_reply;
}SalCallbacks;

typedef struct SalAuthInfo{
	char *username;
	char *userid;
	char *password;
	char *realm;
}SalAuthInfo;

SalAuthInfo* sal_auth_info_new();
SalAuthInfo* sal_auth_info_clone(const SalAuthInfo* auth_info);
void sal_auth_info_delete(const SalAuthInfo* auth_info);

void sal_set_callbacks(Sal *ctx, const SalCallbacks *cbs);
int sal_listen_port(Sal *ctx, const char *addr, int port, SalTransport tr, int is_secure);
int sal_unlisten_ports(Sal *ctx);
ortp_socket_t sal_get_socket(Sal *ctx);
void sal_set_user_agent(Sal *ctx, const char *user_agent);
/*keepalive period in ms*/
void sal_set_keepalive_period(Sal *ctx,unsigned int value);
/**
 * returns keepalive period in ms
 * 0 desactiaved
 * */
unsigned int sal_get_keepalive_period(Sal *ctx);
void sal_use_session_timers(Sal *ctx, int expires);
void sal_use_double_registrations(Sal *ctx, bool_t enabled);
void sal_expire_old_registration_contacts(Sal *ctx, bool_t enabled);
void sal_reuse_authorization(Sal *ctx, bool_t enabled);
void sal_use_one_matching_codec_policy(Sal *ctx, bool_t one_matching_codec);
void sal_use_rport(Sal *ctx, bool_t use_rports);
void sal_use_101(Sal *ctx, bool_t use_101);
void sal_set_root_ca(Sal* ctx, const char* rootCa);
void sal_verify_server_certificates(Sal *ctx, bool_t verify);

int sal_iterate(Sal *sal);
MSList * sal_get_pending_auths(Sal *sal);

/*create an operation */
SalOp * sal_op_new(Sal *sal);

/*generic SalOp API, working for all operations */
Sal *sal_op_get_sal(const SalOp *op);
void sal_op_set_contact(SalOp *op, const char *contact);
void sal_op_set_route(SalOp *op, const char *route);
void sal_op_set_from(SalOp *op, const char *from);
void sal_op_set_to(SalOp *op, const char *to);
void sal_op_release(SalOp *h);
void sal_op_authenticate(SalOp *h, const SalAuthInfo *info);
void sal_op_cancel_authentication(SalOp *h);
void sal_op_set_user_pointer(SalOp *h, void *up);
int sal_op_get_auth_requested(SalOp *h, const char **realm, const char **username);
const char *sal_op_get_from(const SalOp *op);
const char *sal_op_get_to(const SalOp *op);
const char *sal_op_get_contact(const SalOp *op);
const char *sal_op_get_route(const SalOp *op);
const char *sal_op_get_proxy(const SalOp *op);
/*for incoming requests, returns the origin of the packet as a sip uri*/
const char *sal_op_get_network_origin(const SalOp *op);
/*returns far-end "User-Agent" string */
const char *sal_op_get_remote_ua(const SalOp *op);
void *sal_op_get_user_pointer(const SalOp *op);

/*Call API*/
int sal_call_set_local_media_description(SalOp *h, SalMediaDescription *desc);
int sal_call(SalOp *h, const char *from, const char *to);
int sal_call_notify_ringing(SalOp *h, bool_t early_media);
/*accept an incoming call or, during a call accept a reINVITE*/
int sal_call_accept(SalOp*h);
int sal_call_decline(SalOp *h, SalReason reason, const char *redirection /*optional*/);
int sal_call_update(SalOp *h, const char *subject);
SalMediaDescription * sal_call_get_remote_media_description(SalOp *h);
SalMediaDescription * sal_call_get_final_media_description(SalOp *h);
int sal_call_refer(SalOp *h, const char *refer_to);
int sal_call_refer_with_replaces(SalOp *h, SalOp *other_call_h);
int sal_call_accept_refer(SalOp *h);
/*informs this call is consecutive to an incoming refer */
int sal_call_set_referer(SalOp *h, SalOp *refered_call);
/* returns the SalOp of a call that should be replaced by h, if any */
SalOp *sal_call_get_replaces(SalOp *h);
int sal_call_send_dtmf(SalOp *h, char dtmf);
int sal_call_terminate(SalOp *h);
bool_t sal_call_autoanswer_asked(SalOp *op);
void sal_call_send_vfu_request(SalOp *h);
int sal_call_is_offerer(const SalOp *h);
int sal_call_notify_refer_state(SalOp *h, SalOp *newcall);

/*Registration*/
int sal_register(SalOp *op, const char *proxy, const char *from, int expires);
int sal_register_refresh(SalOp *op, int expires);
int sal_unregister(SalOp *h);

/*Messaging */
int sal_text_send(SalOp *op, const char *from, const char *to, const char *text);

/*presence Subscribe/notify*/
int sal_subscribe_presence(SalOp *op, const char *from, const char *to);
int sal_unsubscribe(SalOp *op);
int sal_subscribe_accept(SalOp *op);
int sal_subscribe_decline(SalOp *op);
int sal_notify_presence(SalOp *op, SalPresenceStatus status, const char *status_message);
int sal_notify_close(SalOp *op);

/*presence publish */
int sal_publish(SalOp *op, const char *from, const char *to, SalPresenceStatus status);


/*ping: main purpose is to obtain its own contact address behind firewalls*/
int sal_ping(SalOp *op, const char *from, const char *to);



#define payload_type_set_number(pt,n)	(pt)->user_data=(void*)((long)n);
#define payload_type_get_number(pt)		((int)(long)(pt)->user_data)

/*misc*/
void sal_get_default_local_ip(Sal *sal, int address_family, char *ip, size_t iplen);


/*internal API */
void __sal_op_init(SalOp *b, Sal *sal);
void __sal_op_set_network_origin(SalOp *op, const char *origin /*a sip uri*/);
void __sal_op_free(SalOp *b);

#endif
