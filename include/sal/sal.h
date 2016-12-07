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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/**
 This header files defines the Signaling Abstraction Layer.
 The purpose of this layer is too allow experiment different call signaling
 protocols and implementations under linphone, for example SIP, JINGLE...
**/

#ifndef sal_h
#define sal_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mediastreamer2/mediastream.h"
#include "ortp/rtpsession.h"
#include "belle-sip/object.h"
#include "belle-sip/mainloop.h"

#ifndef LINPHONE_PUBLIC
	#define LINPHONE_PUBLIC MS2_PUBLIC
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

struct SalBodyHandler;

typedef struct SalBodyHandler SalBodyHandler;

struct SalCustomHeader;

typedef struct SalCustomHeader SalCustomHeader;

struct SalCustomSdpAttribute;

typedef struct SalCustomSdpAttribute SalCustomSdpAttribute;

struct addrinfo;

typedef enum {
	SalTransportUDP, /*UDP*/
	SalTransportTCP, /*TCP*/
	SalTransportTLS, /*TLS*/
	SalTransportDTLS, /*DTLS*/
}SalTransport;

#define SAL_MEDIA_DESCRIPTION_UNCHANGED						0x00
#define SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED				(1)
#define SAL_MEDIA_DESCRIPTION_CODEC_CHANGED					(1<<1)
#define SAL_MEDIA_DESCRIPTION_CRYPTO_KEYS_CHANGED			(1<<2)
#define SAL_MEDIA_DESCRIPTION_CRYPTO_POLICY_CHANGED			(1<<3)
#define SAL_MEDIA_DESCRIPTION_STREAMS_CHANGED				(1<<4)
#define SAL_MEDIA_DESCRIPTION_NETWORK_XXXCAST_CHANGED		(1<<5) /* use to notify when switching from multicast to unicast*/
#define SAL_MEDIA_DESCRIPTION_FORCE_STREAM_RECONSTRUCTION	(1<<6) /* use force graph reconstruction*/
#define SAL_MEDIA_DESCRIPTION_ICE_RESTART_DETECTED			(1<<7)



const char* sal_transport_to_string(SalTransport transport);
SalTransport sal_transport_parse(const char*);
/* Address manipulation API*/
SalAddress * sal_address_new(const char *uri);
SalAddress * sal_address_clone(const SalAddress *addr);
SalAddress * sal_address_ref(SalAddress *addr);
void sal_address_unref(SalAddress *addr);
const char *sal_address_get_scheme(const SalAddress *addr);
const char *sal_address_get_display_name(const SalAddress* addr);
const char *sal_address_get_display_name_unquoted(const SalAddress *addr);
const char *sal_address_get_username(const SalAddress *addr);
const char *sal_address_get_domain(const SalAddress *addr);
int sal_address_get_port(const SalAddress *addr);
bool_t sal_address_is_secure(const SalAddress *addr);
void sal_address_set_secure(SalAddress *addr, bool_t enabled);

SalTransport sal_address_get_transport(const SalAddress* addr);
const char* sal_address_get_transport_name(const SalAddress* addr);
const char *sal_address_get_method_param(const SalAddress *addr);

void sal_address_set_display_name(SalAddress *addr, const char *display_name);
void sal_address_set_username(SalAddress *addr, const char *username);
void sal_address_set_domain(SalAddress *addr, const char *host);
void sal_address_set_port(SalAddress *uri, int port);
void sal_address_clean(SalAddress *addr);
char *sal_address_as_string(const SalAddress *u);
char *sal_address_as_string_uri_only(const SalAddress *u);
void sal_address_destroy(SalAddress *u);
void sal_address_set_param(SalAddress *u,const char* name,const char* value);
void sal_address_set_transport(SalAddress* addr,SalTransport transport);
void sal_address_set_transport_name(SalAddress* addr,const char* transport);
void sal_address_set_method_param(SalAddress *addr, const char *method);
void sal_address_set_params(SalAddress *addr, const char *params);
bool_t sal_address_has_param(const SalAddress *addr, const char *name);
const char * sal_address_get_param(const SalAddress *addr, const char *name);
void sal_address_set_uri_param(SalAddress *addr, const char *name, const char *value);
void sal_address_set_uri_params(SalAddress *addr, const char *params);
bool_t sal_address_has_uri_param(const SalAddress *addr, const char *name);
const char * sal_address_get_uri_param(const SalAddress *addr, const char *name);
bool_t sal_address_is_ipv6(const SalAddress *addr);
bool_t sal_address_is_sip(const SalAddress *addr);
void sal_address_set_password(SalAddress *addr, const char *passwd);
const char *sal_address_get_password(const SalAddress *addr);
void sal_address_set_header(SalAddress *addr, const char *header_name, const char *header_value);

LINPHONE_PUBLIC Sal * sal_init(MSFactory *factory);
LINPHONE_PUBLIC void sal_uninit(Sal* sal);
void sal_set_user_pointer(Sal *sal, void *user_data);
void *sal_get_user_pointer(const Sal *sal);


typedef enum {
	SalAudio,
	SalVideo,
	SalText,
	SalOther
} SalStreamType;
const char* sal_stream_type_to_string(SalStreamType type);

typedef enum{
	SalProtoRtpAvp,
	SalProtoRtpSavp,
	SalProtoRtpAvpf,
	SalProtoRtpSavpf,
	SalProtoUdpTlsRtpSavp,
	SalProtoUdpTlsRtpSavpf,
	SalProtoOther
}SalMediaProto;
const char* sal_media_proto_to_string(SalMediaProto type);

typedef enum{
	SalStreamSendRecv,
	SalStreamSendOnly,
	SalStreamRecvOnly,
	SalStreamInactive
}SalStreamDir;
const char* sal_stream_dir_to_string(SalStreamDir type);


#define SAL_ENDPOINT_CANDIDATE_MAX 2

#define SAL_MEDIA_DESCRIPTION_MAX_ICE_ADDR_LEN 64
#define SAL_MEDIA_DESCRIPTION_MAX_ICE_FOUNDATION_LEN 32
#define SAL_MEDIA_DESCRIPTION_MAX_ICE_TYPE_LEN 6

typedef struct SalIceCandidate {
	char addr[SAL_MEDIA_DESCRIPTION_MAX_ICE_ADDR_LEN];
	char raddr[SAL_MEDIA_DESCRIPTION_MAX_ICE_ADDR_LEN];
	char foundation[SAL_MEDIA_DESCRIPTION_MAX_ICE_FOUNDATION_LEN];
	char type[SAL_MEDIA_DESCRIPTION_MAX_ICE_TYPE_LEN];
	unsigned int componentID;
	unsigned int priority;
	int port;
	int rport;
} SalIceCandidate;

#define SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES 20

typedef struct SalIceRemoteCandidate {
	char addr[SAL_MEDIA_DESCRIPTION_MAX_ICE_ADDR_LEN];
	int port;
} SalIceRemoteCandidate;

#define SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES 2

#define SAL_MEDIA_DESCRIPTION_MAX_ICE_UFRAG_LEN 256
#define SAL_MEDIA_DESCRIPTION_MAX_ICE_PWD_LEN 256

/*sufficient for 256bit keys encoded in base 64*/
#define SAL_SRTP_KEY_SIZE 128

typedef struct SalSrtpCryptoAlgo {
	unsigned int tag;
	MSCryptoSuite algo;
	char master_key[SAL_SRTP_KEY_SIZE];
} SalSrtpCryptoAlgo;

#define SAL_CRYPTO_ALGO_MAX 4

typedef enum {
	SalDtlsRoleInvalid,
	SalDtlsRoleIsServer,
	SalDtlsRoleIsClient,
	SalDtlsRoleUnset
} SalDtlsRole;

typedef enum {
	SalMulticastInactive=0,
	SalMulticastSender,
	SalMulticastReceiver,
	SalMulticastSenderReceiver
} SalMulticastRole;

typedef enum {
	SalOpSDPNormal = 0, /** No special handling for SDP */
	SalOpSDPSimulateError, /** Will simulate an SDP parsing error */
	SalOpSDPSimulateRemove /** Will simulate no SDP in the op */
} SalOpSDPHandling;

typedef struct SalStreamDescription{
	char name[16]; /*unique name of stream, in order to ease offer/answer model algorithm*/
	SalMediaProto proto;
	SalStreamType type;
	char typeother[32];
	char proto_other[32];
	char rtp_addr[64];
	char rtcp_addr[64];
	unsigned int rtp_ssrc;
	char rtcp_cname[256];
	int rtp_port;
	int rtcp_port;
	MSList *payloads; /*<list of PayloadType */
	MSList *already_assigned_payloads; /*<list of PayloadType offered in the past, used for correct allocation of payload type numbers*/
	int bandwidth;
	int ptime;
	SalStreamDir dir;
	SalSrtpCryptoAlgo crypto[SAL_CRYPTO_ALGO_MAX];
	unsigned int crypto_local_tag;
	int max_rate;
	bool_t  implicit_rtcp_fb;
	OrtpRtcpFbConfiguration rtcp_fb;
	OrtpRtcpXrConfiguration rtcp_xr;
	SalCustomSdpAttribute *custom_sdp_attributes;
	SalIceCandidate ice_candidates[SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES];
	SalIceRemoteCandidate ice_remote_candidates[SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES];
	char ice_ufrag[SAL_MEDIA_DESCRIPTION_MAX_ICE_UFRAG_LEN];
	char ice_pwd[SAL_MEDIA_DESCRIPTION_MAX_ICE_PWD_LEN];
	bool_t ice_mismatch;
	bool_t set_nortpproxy; /*Formely set by ICE to indicate to the proxy that it has nothing to do*/
	bool_t rtcp_mux;
	bool_t pad[1];
	char dtls_fingerprint[256];
	SalDtlsRole dtls_role;
	uint8_t zrtphash[128];
	uint8_t haveZrtpHash; /**< flag for zrtp hash presence */
	int ttl; /*for multicast -1 to disable*/
	SalMulticastRole multicast_role;
} SalStreamDescription;

const char *sal_multicast_role_to_string(SalMulticastRole role);
const char *sal_stream_description_get_type_as_string(const SalStreamDescription *desc);
const char *sal_stream_description_get_proto_as_string(const SalStreamDescription *desc);

#define SAL_MEDIA_DESCRIPTION_MAX_STREAMS 8

typedef struct SalMediaDescription{
	int refcount;
	char name[64];
	char addr[64];
	char username[64];
	int nb_streams;
	int bandwidth;
	unsigned int session_ver;
	unsigned int session_id;
	SalStreamDir dir;
	SalStreamDescription streams[SAL_MEDIA_DESCRIPTION_MAX_STREAMS];
	SalCustomSdpAttribute *custom_sdp_attributes;
	OrtpRtcpXrConfiguration rtcp_xr;
	char ice_ufrag[SAL_MEDIA_DESCRIPTION_MAX_ICE_UFRAG_LEN];
	char ice_pwd[SAL_MEDIA_DESCRIPTION_MAX_ICE_PWD_LEN];
	bool_t ice_lite;
	bool_t set_nortpproxy;
	bool_t pad[2];
} SalMediaDescription;

typedef struct SalMessage{
	const char *from;
	const char *text;
	const char *url;
	const char *message_id;
	const char *content_type;
	time_t time;
}SalMessage;

typedef struct SalIsComposing {
	const char *from;
	const char *text;
} SalIsComposing;

typedef struct SalImdn {
	const char *from;
	const char *content;
} SalImdn;

#define SAL_MEDIA_DESCRIPTION_MAX_MESSAGE_ATTRIBUTES 5

SalMediaDescription *sal_media_description_new(void);
SalMediaDescription * sal_media_description_ref(SalMediaDescription *md);
void sal_media_description_unref(SalMediaDescription *md);
bool_t sal_media_description_empty(const SalMediaDescription *md);
int sal_media_description_equals(const SalMediaDescription *md1, const SalMediaDescription *md2);
char * sal_media_description_print_differences(int result);
bool_t sal_media_description_has_dir(const SalMediaDescription *md, SalStreamDir dir);
LINPHONE_PUBLIC SalStreamDescription *sal_media_description_find_stream(SalMediaDescription *md, SalMediaProto proto, SalStreamType type);
unsigned int sal_media_description_nb_active_streams_of_type(SalMediaDescription *md, SalStreamType type);
SalStreamDescription * sal_media_description_get_active_stream_of_type(SalMediaDescription *md, SalStreamType type, unsigned int idx);
SalStreamDescription * sal_media_description_find_secure_stream_of_type(SalMediaDescription *md, SalStreamType type);
SalStreamDescription * sal_media_description_find_best_stream(SalMediaDescription *md, SalStreamType type);
void sal_media_description_set_dir(SalMediaDescription *md, SalStreamDir stream_dir);
bool_t sal_stream_description_active(const SalStreamDescription *sd);
bool_t sal_stream_description_has_avpf(const SalStreamDescription *sd);
bool_t sal_stream_description_has_implicit_avpf(const SalStreamDescription *sd);
bool_t sal_stream_description_has_srtp(const SalStreamDescription *sd);
bool_t sal_stream_description_has_dtls(const SalStreamDescription *sd);
bool_t sal_stream_description_has_ipv6(const SalStreamDescription *md);
bool_t sal_media_description_has_avpf(const SalMediaDescription *md);
bool_t sal_media_description_has_implicit_avpf(const SalMediaDescription *md);
bool_t sal_media_description_has_srtp(const SalMediaDescription *md);
bool_t sal_media_description_has_dtls(const SalMediaDescription *md);
bool_t sal_media_description_has_zrtp(const SalMediaDescription *md);
bool_t sal_media_description_has_ipv6(const SalMediaDescription *md);
int sal_media_description_get_nb_active_streams(const SalMediaDescription *md);

struct SalOpBase;
typedef void (*SalOpReleaseCb)(struct SalOpBase *op);
	
/*this structure must be at the first byte of the SalOp structure defined by implementors*/
typedef struct SalOpBase{
	Sal *root;
	char *route; /*or request-uri for REGISTER*/
	MSList* route_addresses; /*list of SalAddress* */
	SalAddress* contact_address;
	char *from;
	SalAddress* from_address;
	char *to;
	SalAddress* to_address;
	char *origin;
	SalAddress* origin_address;
	SalAddress* diversion_address;
	char *remote_ua;
	SalAddress* remote_contact_address;
	char *remote_contact;
	SalMediaDescription *local_media;
	SalMediaDescription *remote_media;
	void *user_pointer;
	const char* call_id;
	char* realm;
	SalAddress* service_route; /*as defined by rfc3608, might be a list*/
	SalCustomHeader *sent_custom_headers;
	SalCustomHeader *recv_custom_headers;
	char* entity_tag; /*as defined by rfc3903 (I.E publih)*/
	SalOpReleaseCb release_cb;
} SalOpBase;


typedef enum SalReason{
	SalReasonNone, /*no error, please leave first so that it takes 0 value*/
	SalReasonDeclined,
	SalReasonBusy,
	SalReasonRedirect,
	SalReasonTemporarilyUnavailable,
	SalReasonRequestTimeout,
	SalReasonNotFound,
	SalReasonDoNotDisturb,
	SalReasonUnsupportedContent,
	SalReasonForbidden,
	SalReasonUnknown,
	SalReasonServiceUnavailable,
	SalReasonRequestPending,
	SalReasonUnauthorized,
	SalReasonNotAcceptable,
	SalReasonNoMatch, /*equivalent to 481 Transaction/Call leg does not exist*/
	SalReasonMovedPermanently,
	SalReasonGone,
	SalReasonAddressIncomplete,
	SalReasonNotImplemented,
	SalReasonBadGateway,
	SalReasonServerTimeout,
	SalReasonIOError,
	SalReasonInternalError
}SalReason;

const char* sal_reason_to_string(const SalReason reason);

typedef struct SalErrorInfo{
	SalReason reason;
	char *status_string;
	int protocol_code;
	char *warnings;
	char *full_string; /*concatenation of status_string + warnings*/
}SalErrorInfo;

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
	SalPresenceOnVacation
}SalPresenceStatus;

struct _SalPresenceModel;
typedef struct _SalPresenceModel SalPresenceModel;

const char* sal_presence_status_to_string(const SalPresenceStatus status);

typedef enum SalReferStatus{
	SalReferTrying,
	SalReferSuccess,
	SalReferFailed
}SalReferStatus;

typedef enum SalSubscribeStatus{
	SalSubscribeNone,
	SalSubscribePending,
	SalSubscribeActive,
	SalSubscribeTerminated
}SalSubscribeStatus;

typedef enum SalTextDeliveryStatus{
	SalTextDeliveryInProgress,
	SalTextDeliveryDone,
	SalTextDeliveryFailed
}SalTextDeliveryStatus;

/**
 * auth event mode
 * */
typedef enum SalAuthMode { /*this enum must be same as belle_sip_auth_mode_t*/
	SalAuthModeHttpDigest, /** Digest authentication requested*/
	SalAuthModeTls /** Client certificate requested*/
}SalAuthMode;

struct _SalCertificatesChain;
typedef struct _SalCertificatesChain SalCertificatesChain;
struct _SalSigningKey;
typedef struct _SalSigningKey SalSigningKey;

/**
 * Format of certificate buffer
 * */
typedef enum SalCertificateRawFormat {/*this enum must be same as belle_sip_certificate_raw_format_t*/
	SAL_CERTIFICATE_RAW_FORMAT_PEM, /** PEM format*/
	SAL_CERTIFICATE_RAW_FORMAT_DER /** ASN.1 raw format*/
}SalCertificateRawFormat;



typedef struct SalAuthInfo{
	char *username;
	char *userid;
	char *password;
	char *realm;
	char *domain;
	char *ha1;
	SalAuthMode mode;
	SalSigningKey *key;
	SalCertificatesChain *certificates;
}SalAuthInfo;

typedef void (*SalOnCallReceived)(SalOp *op);
typedef void (*SalOnCallRinging)(SalOp *op);
typedef void (*SalOnCallAccepted)(SalOp *op);
typedef void (*SalOnCallAck)(SalOp *op);
typedef void (*SalOnCallUpdating)(SalOp *op, bool_t is_update);/*< Called when a reINVITE/UPDATE is received*/
typedef void (*SalOnCallTerminated)(SalOp *op, const char *from);
typedef void (*SalOnCallFailure)(SalOp *op);
typedef void (*SalOnCallReleased)(SalOp *salop);
typedef void (*SalOnCallCancelDone)(SalOp *salop);
typedef void (*SalOnAuthRequestedLegacy)(SalOp *op, const char *realm, const char *username);
typedef bool_t (*SalOnAuthRequested)(Sal *sal,SalAuthInfo* info);
typedef void (*SalOnAuthFailure)(SalOp *op, SalAuthInfo* info);
typedef void (*SalOnRegisterSuccess)(SalOp *op, bool_t registered);
typedef void (*SalOnRegisterFailure)(SalOp *op);
typedef void (*SalOnVfuRequest)(SalOp *op);
typedef void (*SalOnDtmfReceived)(SalOp *op, char dtmf);
typedef void (*SalOnRefer)(Sal *sal, SalOp *op, const char *referto);
typedef void (*SalOnTextReceived)(SalOp *op, const SalMessage *msg);
typedef void (*SalOnTextDeliveryUpdate)(SalOp *op, SalTextDeliveryStatus);
typedef void (*SalOnIsComposingReceived)(SalOp *op, const SalIsComposing *is_composing);
typedef void (*SalOnImdnReceived)(SalOp *op, const SalImdn *imdn);
typedef void (*SalOnNotifyRefer)(SalOp *op, SalReferStatus state);
typedef void (*SalOnSubscribeResponse)(SalOp *op, SalSubscribeStatus status, int will_retry);
typedef void (*SalOnNotify)(SalOp *op, SalSubscribeStatus status, const char *event, SalBodyHandler *body);
typedef void (*SalOnSubscribeReceived)(SalOp *salop, const char *event, const SalBodyHandler *body);
typedef void (*SalOnIncomingSubscribeClosed)(SalOp *salop);
typedef void (*SalOnParsePresenceRequested)(SalOp *salop, const char *content_type, const char *content_subtype, const char *content, SalPresenceModel **result);
typedef void (*SalOnConvertPresenceToXMLRequested)(SalOp *salop, SalPresenceModel *presence, const char *contact, char **content);
typedef void (*SalOnNotifyPresence)(SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model, const char *msg);
typedef void (*SalOnSubscribePresenceReceived)(SalOp *salop, const char *from);
typedef void (*SalOnSubscribePresenceClosed)(SalOp *salop, const char *from);
typedef void (*SalOnPingReply)(SalOp *salop);
typedef void (*SalOnInfoReceived)(SalOp *salop, SalBodyHandler *body);
typedef void (*SalOnPublishResponse)(SalOp *salop);
typedef void (*SalOnNotifyResponse)(SalOp *salop);
typedef void (*SalOnExpire)(SalOp *salop);
/*allows sal implementation to access auth info if available, return TRUE if found*/



typedef struct SalCallbacks{
	SalOnCallReceived call_received;
	SalOnCallRinging call_ringing;
	SalOnCallAccepted call_accepted;
	SalOnCallAck call_ack;
	SalOnCallUpdating call_updating;
	SalOnCallTerminated call_terminated;
	SalOnCallFailure call_failure;
	SalOnCallReleased call_released;
	SalOnCallCancelDone call_cancel_done;
	SalOnAuthFailure auth_failure;
	SalOnRegisterSuccess register_success;
	SalOnRegisterFailure register_failure;
	SalOnVfuRequest vfu_request;
	SalOnDtmfReceived dtmf_received;
	SalOnRefer refer_received;
	SalOnTextReceived text_received;
	SalOnTextDeliveryUpdate text_delivery_update;
	SalOnIsComposingReceived is_composing_received;
	SalOnImdnReceived imdn_received;
	SalOnNotifyRefer notify_refer;
	SalOnSubscribeReceived subscribe_received;
	SalOnIncomingSubscribeClosed incoming_subscribe_closed;
	SalOnSubscribeResponse subscribe_response;
	SalOnNotify notify;
	SalOnSubscribePresenceReceived subscribe_presence_received;
	SalOnSubscribePresenceClosed subscribe_presence_closed;
	SalOnParsePresenceRequested parse_presence_requested;
	SalOnConvertPresenceToXMLRequested convert_presence_to_xml_requested;
	SalOnNotifyPresence notify_presence;
	SalOnPingReply ping_reply;
	SalOnAuthRequested auth_requested;
	SalOnInfoReceived info_received;
	SalOnPublishResponse on_publish_response;
	SalOnExpire on_expire;
	SalOnNotifyResponse on_notify_response;
}SalCallbacks;



SalAuthInfo* sal_auth_info_new(void);
SalAuthInfo* sal_auth_info_clone(const SalAuthInfo* auth_info);
void sal_auth_info_delete(SalAuthInfo* auth_info);
LINPHONE_PUBLIC int sal_auth_compute_ha1(const char* userid,const char* realm,const char* password, char ha1[33]);
SalAuthMode sal_auth_info_get_mode(const SalAuthInfo* auth_info);
SalSigningKey *sal_auth_info_get_signing_key(const SalAuthInfo* auth_info);
SalCertificatesChain *sal_auth_info_get_certificates_chain(const SalAuthInfo* auth_info);
void sal_auth_info_set_mode(SalAuthInfo* auth_info, SalAuthMode mode);

/** Parse a file containing either a certificate chain order in PEM format or a single DER cert
 * @param auth_info structure where to store the result of parsing
 * @param path path to certificate chain file
 * @param format either PEM or DER
 */
void sal_certificates_chain_parse_file(SalAuthInfo* auth_info, const char* path, SalCertificateRawFormat format);

/**
 * Parse a file containing either a private or public rsa key
 * @param auth_info structure where to store the result of parsing
 * @param passwd password (optionnal)
 */
void sal_signing_key_parse_file(SalAuthInfo* auth_info, const char* path, const char *passwd);

/** Parse a buffer containing either a certificate chain order in PEM format or a single DER cert
 * @param auth_info structure where to store the result of parsing
 * @param buffer the buffer to parse
 * @param format either PEM or DER
 */
void sal_certificates_chain_parse(SalAuthInfo* auth_info, const char* buffer, SalCertificateRawFormat format);

/**
 * Parse a buffer containing either a private or public rsa key
 * @param auth_info structure where to store the result of parsing
 * @param passwd password (optionnal)
 */
void sal_signing_key_parse(SalAuthInfo* auth_info, const char* buffer, const char *passwd);

/**
 * Parse a directory for files containing certificate with the given subject CNAME
 * @param[out]	certificate_pem				the address of a string to store the certificate in PEM format. To be freed by caller
 * @param[out]	key_pem						the address of a string to store the key in PEM format. To be freed by caller
 * @param[in]	path						directory to parse
 * @param[in]	subject						subject CNAME
 * @param[in]	format 						either PEM or DER
 * @param[in]	generate_certificate		if true, if matching certificate and key can't be found, generate it and store it into the given dir, filename will be subject.pem
 * @param[in]	generate_dtls_fingerprint	if true and we have a certificate, generate the dtls fingerprint as described in rfc4572
 */
void sal_certificates_chain_parse_directory(char **certificate_pem, char **key_pem, char **fingerprint, const char* path, const char *subject, SalCertificateRawFormat format, bool_t generate_certificate, bool_t generate_dtls_fingerprint);

void sal_certificates_chain_delete(SalCertificatesChain *chain);
void sal_signing_key_delete(SalSigningKey *key);



void sal_set_callbacks(Sal *ctx, const SalCallbacks *cbs);
int sal_listen_port(Sal *ctx, const char *addr, int port, SalTransport tr, int is_tunneled);
int sal_get_listening_port(Sal *ctx, SalTransport tr);
int sal_unlisten_ports(Sal *ctx);
LINPHONE_PUBLIC int sal_transport_available(Sal *ctx, SalTransport t);
LINPHONE_PUBLIC bool_t sal_content_encoding_available(Sal *ctx, const char *content_encoding);
void sal_set_dscp(Sal *ctx, int dscp);
void sal_set_supported_tags(Sal *ctx, const char* tags);
void sal_add_supported_tag(Sal *ctx, const char* tag);
void sal_remove_supported_tag(Sal *ctx, const char* tag);
const char *sal_get_supported_tags(Sal *ctx);
int sal_reset_transports(Sal *ctx);
ortp_socket_t sal_get_socket(Sal *ctx);
void sal_set_user_agent(Sal *ctx, const char *user_agent);
const char* sal_get_user_agent(Sal *ctx);
void sal_append_stack_string_to_user_agent(Sal *ctx);
/*keepalive period in ms*/
void sal_set_keepalive_period(Sal *ctx,unsigned int value);
void sal_use_tcp_tls_keepalive(Sal *ctx, bool_t enabled);
int sal_set_tunnel(Sal *ctx, void *tunnelclient);
/*Default value is true*/
void sal_enable_sip_update_method(Sal *ctx,bool_t value);

/**
 * returns keepalive period in ms
 * 0 desactiaved
 * */
unsigned int sal_get_keepalive_period(Sal *ctx);
void sal_use_session_timers(Sal *ctx, int expires);
void sal_use_dates(Sal *ctx, bool_t enabled);
void sal_use_one_matching_codec_policy(Sal *ctx, bool_t one_matching_codec);
void sal_use_rport(Sal *ctx, bool_t use_rports);
void sal_enable_auto_contacts(Sal *ctx, bool_t enabled);
void sal_set_root_ca(Sal* ctx, const char* rootCa);
void sal_set_root_ca_data(Sal* ctx, const char* data);
const char *sal_get_root_ca(Sal* ctx);
void sal_verify_server_certificates(Sal *ctx, bool_t verify);
void sal_verify_server_cn(Sal *ctx, bool_t verify);
void sal_set_ssl_config(Sal *ctx, void *ssl_config);
void sal_set_uuid(Sal*ctx, const char *uuid);
int sal_create_uuid(Sal*ctx, char *uuid, size_t len);
int sal_generate_uuid(char *uuid, size_t len);
LINPHONE_PUBLIC void sal_enable_test_features(Sal*ctx, bool_t enabled);
void sal_use_no_initial_route(Sal *ctx, bool_t enabled);

int sal_iterate(Sal *sal);
MSList * sal_get_pending_auths(Sal *sal);

/*create an operation */
SalOp * sal_op_new(Sal *sal);

/*generic SalOp API, working for all operations */
Sal *sal_op_get_sal(const SalOp *op);
void sal_op_set_contact_address(SalOp *op, const SalAddress* address);
void sal_op_set_route(SalOp *op, const char *route);
void sal_op_set_route_address(SalOp *op, const SalAddress* address);
void sal_op_add_route_address(SalOp *op, const SalAddress* address);
void sal_op_set_realm(SalOp *op, const char *realm);
void sal_op_set_from(SalOp *op, const char *from);
void sal_op_set_from_address(SalOp *op, const SalAddress *from);
void sal_op_set_to(SalOp *op, const char *to);
void sal_op_set_to_address(SalOp *op, const SalAddress *to);
void sal_op_set_diversion_address(SalOp *op, const SalAddress *diversion);
SalOp *sal_op_ref(SalOp* h);
void sal_op_stop_refreshing(SalOp *op);
int sal_op_refresh(SalOp *op);

void sal_op_kill_dialog(SalOp *op);
void sal_op_release(SalOp *h);
/*same as release, but does not stop refresher if any*/
void* sal_op_unref(SalOp* op);

void sal_op_authenticate(SalOp *h, const SalAuthInfo *info);
void sal_op_cancel_authentication(SalOp *h);
void sal_op_set_user_pointer(SalOp *h, void *up);
SalAuthInfo * sal_op_get_auth_requested(SalOp *h);
const char *sal_op_get_from(const SalOp *op);
const SalAddress *sal_op_get_from_address(const SalOp *op);
const char *sal_op_get_to(const SalOp *op);
const SalAddress *sal_op_get_to_address(const SalOp *op);
const SalAddress *sal_op_get_diversion_address(const SalOp *op);
const SalAddress *sal_op_get_contact_address(const SalOp *op);
const MSList* sal_op_get_route_addresses(const SalOp *op);
const char *sal_op_get_proxy(const SalOp *op);
/*raw contact header value with header params*/
const char *sal_op_get_remote_contact(const SalOp *op);
/*contact header address only (I.E without header params*/
const SalAddress* sal_op_get_remote_contact_address(const SalOp *op);
/*for incoming requests, returns the origin of the packet as a sip uri*/
const char *sal_op_get_network_origin(const SalOp *op);
const SalAddress *sal_op_get_network_origin_address(const SalOp *op);
/*returns far-end "User-Agent" string */
const char *sal_op_get_remote_ua(const SalOp *op);
void *sal_op_get_user_pointer(const SalOp *op);
const char* sal_op_get_call_id(const SalOp *op);
char* sal_op_get_dialog_id(const SalOp *op);
bool_t sal_op_is_forked_of(const SalOp *op1, const SalOp *op2);

const SalAddress* sal_op_get_service_route(const SalOp *op);
void sal_op_set_service_route(SalOp *op,const SalAddress* service_route);

void sal_op_set_manual_refresher_mode(SalOp *op, bool_t enabled);
int sal_op_get_address_family(SalOp *op);
/*returns TRUE if there is no pending request that may block a future one */
bool_t sal_op_is_idle(SalOp *op);

const SalErrorInfo *sal_error_info_none(void);
LINPHONE_PUBLIC const SalErrorInfo *sal_op_get_error_info(const SalOp *op);
void sal_error_info_reset(SalErrorInfo *ei);
void sal_error_info_set(SalErrorInfo *ei, SalReason reason, int code, const char *status_string, const char *warning);

/*entity tag used for publish (see RFC 3903)*/
const char *sal_op_get_entity_tag(const SalOp* op);
void sal_op_set_entity_tag(SalOp *op, const char* entity_tag);
/*set the event header, for used with out of dialog SIP notify*/
void sal_op_set_event(SalOp *op, const char *event);

/*Call API*/
int sal_call_set_local_media_description(SalOp *h, SalMediaDescription *desc);
int sal_call(SalOp *h, const char *from, const char *to);
int sal_call_notify_ringing(SalOp *h, bool_t early_media);
/*accept an incoming call or, during a call accept a reINVITE*/
int sal_call_accept(SalOp*h);
int sal_call_decline(SalOp *h, SalReason reason, const char *redirection /*optional*/);
int sal_call_update(SalOp *h, const char *subject, bool_t no_user_consent);
void sal_call_cancel_invite(SalOp *op);
SalMediaDescription * sal_call_get_remote_media_description(SalOp *h);
LINPHONE_PUBLIC SalMediaDescription * sal_call_get_final_media_description(SalOp *h);
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
bool_t sal_call_compare_op(const SalOp *op1, const SalOp *op2);
LINPHONE_PUBLIC bool_t sal_call_dialog_request_pending(const SalOp *op);
const char * sal_call_get_local_tag(SalOp *op);
const char * sal_call_get_remote_tag(SalOp *op);
void sal_call_set_replaces(SalOp *op, const char *call_id, const char *from_tag, const char *to_tag);
/* Call test API */


/**
 * @brief Invoking this on the SAL will modify every subsequent SalOp to have a special handling for SDP.
 * @details This is especially useful while testing, to simulate some specific behaviors, like missing SDP or an error in parsing.
 *
 * @warning Don't forget to reset the handling method to SalOpSDPNormal afterwards.
 *
 * @param h the Sal instance
 * @param handling_method Could be SalOpSDPNormal, SalOpSDPSimulateError, SalOpSDPSimulateRemoval (\ref SalOpSDPHandling)
 */
LINPHONE_PUBLIC void sal_default_set_sdp_handling(Sal* h, SalOpSDPHandling handling_method) ;
/* Second version: for a specific call*/
LINPHONE_PUBLIC void sal_call_set_sdp_handling(SalOp *h, SalOpSDPHandling handling) ;

/*Registration*/
int sal_register(SalOp *op, const char *proxy, const char *from, int expires,SalAddress* old_contact);
/*refresh a register, -1 mean use the last known value*/
int sal_register_refresh(SalOp *op, int expires);
int sal_unregister(SalOp *h);

/*Messaging */
int sal_text_send(SalOp *op, const char *from, const char *to, const char *text);
int sal_message_send(SalOp *op, const char *from, const char *to, const char* content_type, const char *msg, const char *peer_uri);
int sal_message_reply(SalOp *op, SalReason reason);

/*presence Subscribe/notify*/
int sal_subscribe_presence(SalOp *op, const char *from, const char *to, int expires);
int sal_notify_presence(SalOp *op, SalPresenceModel *presence);
int sal_notify_presence_close(SalOp *op);

/*presence publish */
//int sal_publish_presence(SalOp *op, const char *from, const char *to, int expires, SalPresenceModel *presence);
SalBodyHandler *sal_presence_model_create_body_handler(SalPresenceModel *presence);


/*ping: main purpose is to obtain its own contact address behind firewalls*/
int sal_ping(SalOp *op, const char *from, const char *to);

/*info messages*/
int sal_send_info(SalOp *op, const char *from, const char *to, const SalBodyHandler *body);

/*generic subscribe/notify/publish api*/
int sal_subscribe(SalOp *op, const char *from, const char *to, const char *eventname, int expires, const SalBodyHandler *body);
int sal_unsubscribe(SalOp *op);
int sal_subscribe_accept(SalOp *op);
int sal_subscribe_decline(SalOp *op, SalReason reason);
int sal_notify_pending_state(SalOp *op);
int sal_notify(SalOp *op, const SalBodyHandler *body);
int sal_notify_close(SalOp *op);
int sal_publish(SalOp *op, const char *from, const char *to, const char*event_name, int expires, const SalBodyHandler *body);
int sal_op_unpublish(SalOp *op);

/*privacy, must be in sync with LinphonePrivacyMask*/
typedef enum _SalPrivacy {
	SalPrivacyNone=0x0,
	SalPrivacyUser=0x1,
	SalPrivacyHeader=0x2,
	SalPrivacySession=0x4,
	SalPrivacyId=0x8,
	SalPrivacyCritical=0x10,
	SalPrivacyDefault=0x8000
} SalPrivacy;
typedef  unsigned int SalPrivacyMask;

const char* sal_privacy_to_string(SalPrivacy  privacy);
void sal_op_set_privacy(SalOp* op,SalPrivacy privacy);
SalPrivacy sal_op_get_privacy(const SalOp* op);



#define payload_type_set_number(pt,n)		(pt)->user_data=(void*)((intptr_t)n);
#define payload_type_get_number(pt)		((int)(intptr_t)(pt)->user_data)

/*misc*/
void sal_get_default_local_ip(Sal *sal, int address_family, char *ip, size_t iplen);

typedef void (*SalResolverCallback)(void *data, const char *name, struct addrinfo *ai_list);


typedef struct SalResolverContext SalResolverContext;
#define sal_resolver_context_ref(obj) belle_sip_object_ref(obj)
#define sal_resolver_context_unref(obj) belle_sip_object_unref(obj)
LINPHONE_PUBLIC SalResolverContext * sal_resolve_a(Sal* sal, const char *name, int port, int family, SalResolverCallback cb, void *data);
LINPHONE_PUBLIC SalResolverContext * sal_resolve(Sal *sal, const char *service, const char *transport, const char *name, int port, int family, SalResolverCallback cb, void *data);
void sal_resolve_cancel(SalResolverContext *ctx);

SalCustomHeader *sal_custom_header_append(SalCustomHeader *ch, const char *name, const char *value);
const char *sal_custom_header_find(const SalCustomHeader *ch, const char *name);
SalCustomHeader *sal_custom_header_remove(SalCustomHeader *ch, const char *name);
void sal_custom_header_free(SalCustomHeader *ch);
SalCustomHeader *sal_custom_header_clone(const SalCustomHeader *ch);

const SalCustomHeader *sal_op_get_recv_custom_header(SalOp *op);

void sal_op_set_sent_custom_header(SalOp *op, SalCustomHeader* ch);


SalCustomSdpAttribute * sal_custom_sdp_attribute_append(SalCustomSdpAttribute *csa, const char *name, const char *value);
const char * sal_custom_sdp_attribute_find(const SalCustomSdpAttribute *csa, const char *name);
void sal_custom_sdp_attribute_free(SalCustomSdpAttribute *csa);
SalCustomSdpAttribute * sal_custom_sdp_attribute_clone(const SalCustomSdpAttribute *csa);

/** deprecated. use sal_set_log_level instead **/
void sal_enable_log(void);
/** deprecated. use sal_set_log_level instead **/
void sal_disable_log(void);
void sal_set_log_level(OrtpLogLevel level);

/*internal API */
void __sal_op_init(SalOp *b, Sal *sal);
void __sal_op_set_network_origin(SalOp *op, const char *origin /*a sip uri*/);
void __sal_op_set_network_origin_address(SalOp *op, SalAddress *origin);
void __sal_op_set_remote_contact(SalOp *op, const char *ct);
void __sal_op_free(SalOp *b);

/*test api*/
/*0 for no error*/
LINPHONE_PUBLIC	void sal_set_send_error(Sal *sal,int value);
/*1 for no error*/
LINPHONE_PUBLIC	void sal_set_recv_error(Sal *sal,int value);

/*always answer 480 if value=true*/
LINPHONE_PUBLIC	void sal_enable_unconditional_answer(Sal *sal,int value);

LINPHONE_PUBLIC bool_t sal_pending_trans_checking_enabled(const Sal *sal) ;
LINPHONE_PUBLIC int sal_enable_pending_trans_checking(Sal *sal, bool_t value) ;



/*refresher retry after value in ms*/
LINPHONE_PUBLIC	void sal_set_refresher_retry_after(Sal *sal,int value);
LINPHONE_PUBLIC	int sal_get_refresher_retry_after(const Sal *sal);
/*enable contact fixing*/
void sal_nat_helper_enable(Sal *sal,bool_t enable);
bool_t sal_nat_helper_enabled(Sal *sal);

LINPHONE_PUBLIC	void sal_set_dns_timeout(Sal* sal,int timeout);
LINPHONE_PUBLIC int sal_get_dns_timeout(const Sal* sal);
LINPHONE_PUBLIC	void sal_set_transport_timeout(Sal* sal,int timeout);
LINPHONE_PUBLIC int sal_get_transport_timeout(const Sal* sal);
void sal_set_dns_servers(Sal *sal, const MSList *servers);
LINPHONE_PUBLIC void sal_enable_dns_srv(Sal *sal, bool_t enable);
LINPHONE_PUBLIC bool_t sal_dns_srv_enabled(const Sal *sal);
LINPHONE_PUBLIC void sal_enable_dns_search(Sal *sal, bool_t enable);
LINPHONE_PUBLIC bool_t sal_dns_search_enabled(const Sal *sal);
LINPHONE_PUBLIC void sal_set_dns_user_hosts_file(Sal *sal, const char *hosts_file);
LINPHONE_PUBLIC const char *sal_get_dns_user_hosts_file(const Sal *sal);
unsigned int sal_get_random(void);
LINPHONE_PUBLIC char *sal_get_random_token(int size);
unsigned char * sal_get_random_bytes(unsigned char *ret, size_t size);
belle_sip_source_t * sal_create_timer(Sal *sal, belle_sip_source_func_t func, void *data, unsigned int timeout_value_ms, const char* timer_name);
void sal_cancel_timer(Sal *sal, belle_sip_source_t *timer);

//SalBodyHandler * sal_body_handler_new(const char *type, const char *subtype, void *data, size_t size, const char *encoding);
SalBodyHandler * sal_body_handler_new(void);
SalBodyHandler * sal_body_handler_ref(SalBodyHandler *body_handler);
void sal_body_handler_unref(SalBodyHandler *body_handler);
const char * sal_body_handler_get_type(const SalBodyHandler *body_handler);
void sal_body_handler_set_type(SalBodyHandler *body_handler, const char *type);
const char * sal_body_handler_get_subtype(const SalBodyHandler *body_handler);
void sal_body_handler_set_subtype(SalBodyHandler *body_handler, const char *subtype);
const char * sal_body_handler_get_encoding(const SalBodyHandler *body_handler);
void sal_body_handler_set_encoding(SalBodyHandler *body_handler, const char *encoding);
void * sal_body_handler_get_data(const SalBodyHandler *body_handler);
void sal_body_handler_set_data(SalBodyHandler *body_handler, void *data);
size_t sal_body_handler_get_size(const SalBodyHandler *body_handler);
void sal_body_handler_set_size(SalBodyHandler *body_handler, size_t size);
bool_t sal_body_handler_is_multipart(const SalBodyHandler *body_handler);
SalBodyHandler * sal_body_handler_get_part(const SalBodyHandler *body_handler, int idx);
SalBodyHandler * sal_body_handler_find_part_by_header(const SalBodyHandler *body_handler, const char *header_name, const char *header_value);
const char * sal_body_handler_get_header(const SalBodyHandler *body_handler, const char *header_name);

/*this function parses a document with key=value pairs separated by new lines, and extracts the value for a given key*/
int sal_lines_get_value(const char *data, const char *key, char *value, size_t value_size);

LINPHONE_PUBLIC void *sal_get_stack_impl(Sal *sal);
const char* sal_op_get_public_address(SalOp *sal, int *port);
const char* sal_op_get_local_address(SalOp *sal, int *port);

unsigned long sal_begin_background_task(const char *name, void (*max_time_reached)(void *), void *data);
void sal_end_background_task(unsigned long id);

/*Some old equipment may not only rely on attribute sendonly/recvonly/sendrecv/inative*/
void sal_op_cnx_ip_to_0000_if_sendonly_enable(SalOp *sal,bool_t yesno);
bool_t sal_op_cnx_ip_to_0000_if_sendonly_enabled(SalOp *sal);

void sal_set_http_proxy_host(Sal *sal, const char *host) ;
void sal_set_http_proxy_port(Sal *sal, int port) ;
const char *sal_get_http_proxy_host(const Sal *sal);
int sal_get_http_proxy_port(const Sal *sal);

#ifdef __cplusplus
}
#endif

#endif


