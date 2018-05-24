/*
 * c-sal.h
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

/**
 This header files defines the Signaling Abstraction Layer.
 The purpose of this layer is too allow experiment different call signaling
 protocols and implementations under linphone, for example SIP, JINGLE...
**/

#ifndef _L_C_SAL_H_
#define _L_C_SAL_H_

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include "mediastreamer2/mediastream.h"
#include "ortp/rtpsession.h"
#include "belle-sip/belle-sip.h"

#ifndef LINPHONE_PUBLIC
#if defined(_MSC_VER)
#ifdef LINPHONE_EXPORTS
#define LINPHONE_PUBLIC	__declspec(dllexport)
#else
#define LINPHONE_PUBLIC	__declspec(dllimport)
#endif
#else
#define LINPHONE_PUBLIC
#endif
#endif

/*Dirty hack, keep in sync with mediastreamer2/include/mediastream.h */
#ifndef PAYLOAD_TYPE_FLAG_CAN_RECV
#define PAYLOAD_TYPE_FLAG_CAN_RECV	PAYLOAD_TYPE_USER_FLAG_1
#define PAYLOAD_TYPE_FLAG_CAN_SEND	PAYLOAD_TYPE_USER_FLAG_2
#endif

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

#ifdef __cplusplus
extern "C" {
#endif

const char* sal_transport_to_string(SalTransport transport);
SalTransport sal_transport_parse(const char*);
/* Address manipulation API*/
LINPHONE_PUBLIC SalAddress * sal_address_new(const char *uri);
SalAddress * sal_address_clone(const SalAddress *addr);
SalAddress * sal_address_ref(SalAddress *addr);
LINPHONE_PUBLIC void sal_address_unref(SalAddress *addr);
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
LINPHONE_PUBLIC void sal_address_set_param(SalAddress *u,const char* name,const char* value);
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
void sal_address_remove_uri_param(const SalAddress *addr, const char *name);
bool_t sal_address_is_ipv6(const SalAddress *addr);
bool_t sal_address_is_sip(const SalAddress *addr);
void sal_address_set_password(SalAddress *addr, const char *passwd);
const char *sal_address_get_password(const SalAddress *addr);
void sal_address_set_header(SalAddress *addr, const char *header_name, const char *header_value);
const char *sal_address_get_header(const SalAddress *addr, const char *name);


void sal_set_log_handler(BctbxLogFunc log_handler);

#ifdef __cplusplus
}
#endif


typedef enum {
	SalAudio,
	SalVideo,
	SalText,
	SalOther
} SalStreamType;


typedef enum{
	SalProtoRtpAvp,
	SalProtoRtpSavp,
	SalProtoRtpAvpf,
	SalProtoRtpSavpf,
	SalProtoUdpTlsRtpSavp,
	SalProtoUdpTlsRtpSavpf,
	SalProtoOther
}SalMediaProto;

typedef enum{
	SalStreamSendRecv,
	SalStreamSendOnly,
	SalStreamRecvOnly,
	SalStreamInactive
}SalStreamDir;

#ifdef __cplusplus
extern "C" {
#endif

const char* sal_stream_type_to_string(SalStreamType type);
const char* sal_media_proto_to_string(SalMediaProto type);
const char* sal_stream_dir_to_string(SalStreamDir type);

#ifdef __cplusplus
}
#endif

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
	MSList *payloads; /**<list of PayloadType */
	MSList *already_assigned_payloads; /**<list of PayloadType offered in the past, used for correct allocation of payload type numbers*/
	int bandwidth;
	int ptime;
	SalStreamDir dir;
	SalSrtpCryptoAlgo crypto[SAL_CRYPTO_ALGO_MAX];
	unsigned int crypto_local_tag;
	int max_rate;
	bool_t implicit_rtcp_fb;
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

#ifdef __cplusplus
extern "C" {
#endif

const char *sal_multicast_role_to_string(SalMulticastRole role);
const char *sal_stream_description_get_type_as_string(const SalStreamDescription *desc);
const char *sal_stream_description_get_proto_as_string(const SalStreamDescription *desc);

#ifdef __cplusplus
}
#endif

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

#define SAL_MEDIA_DESCRIPTION_MAX_MESSAGE_ATTRIBUTES 5

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif


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
	char *protocol;
	char *full_string; /*concatenation of status_string + warnings*/
	struct SalErrorInfo *sub_sei;
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

#ifdef __cplusplus
extern "C" {
#endif

const char* sal_presence_status_to_string(const SalPresenceStatus status);

#ifdef __cplusplus
}
#endif

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

typedef enum SalMessageDeliveryStatus{
	SalMessageDeliveryInProgress,
	SalMessageDeliveryDone,
	SalMessageDeliveryFailed
}SalMessageDeliveryStatus;

/**
 * auth event mode
 * */
typedef enum SalAuthMode { /*this enum must be same as belle_sip_auth_mode_t*/
	SalAuthModeHttpDigest, /** Digest authentication requested*/
	SalAuthModeTls /** Client certificate requested*/
}SalAuthMode;

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
	char *algorithm;
	SalAuthMode mode;
	belle_sip_signing_key_t *key;
	belle_sip_certificates_chain_t *certificates;
}SalAuthInfo;


#ifdef __cplusplus
extern "C" {
#endif

SalAuthInfo* sal_auth_info_new(void);
SalAuthInfo* sal_auth_info_clone(const SalAuthInfo* auth_info);
void sal_auth_info_delete(SalAuthInfo* auth_info);
LINPHONE_PUBLIC int sal_auth_compute_ha1(const char *userid, const char *realm, const char *password, char ha1[33]);
LINPHONE_PUBLIC int sal_auth_compute_ha1_for_algorithm(const char *userid, const char *realm, const char *password, char *ha1, size_t size, const char *algo);
SalAuthMode sal_auth_info_get_mode(const SalAuthInfo* auth_info);
belle_sip_signing_key_t *sal_auth_info_get_signing_key(const SalAuthInfo* auth_info);
belle_sip_certificates_chain_t *sal_auth_info_get_certificates_chain(const SalAuthInfo* auth_info);
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

void sal_certificates_chain_delete(belle_sip_certificates_chain_t *chain);
void sal_signing_key_delete(belle_sip_signing_key_t *key);



const SalErrorInfo *sal_error_info_none(void);
void sal_error_info_reset(SalErrorInfo *ei);
void sal_error_info_init_to_null(SalErrorInfo *sei);
void sal_error_info_set(SalErrorInfo *ei, SalReason reason, const char *protocol, int code, const char *status_string, const char *warning);

/*entity tag used for publish (see RFC 3903)*/

#ifdef __cplusplus
}
#endif

/*Call API*/
#ifdef __cplusplus
extern "C" {
#endif


/*presence publish */
//int sal_publish_presence(SalOp *op, const char *from, const char *to, int expires, SalPresenceModel *presence);
SalBodyHandler *sal_presence_model_create_body_handler(SalPresenceModel *presence);



#ifdef __cplusplus
}
#endif

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
typedef unsigned int SalPrivacyMask;

#ifdef __cplusplus
extern "C" {
#endif

const char* sal_privacy_to_string(SalPrivacy privacy);

#ifdef __cplusplus
}
#endif



#define payload_type_set_number(pt,n) (pt)->user_data=(void*)((intptr_t)n);
#define payload_type_get_number(pt) ((int)(intptr_t)(pt)->user_data)

#ifdef __cplusplus
extern "C" {
#endif


SalCustomHeader *sal_custom_header_ref(SalCustomHeader *ch);
void sal_custom_header_unref(SalCustomHeader *ch);
SalCustomHeader *sal_custom_header_append(SalCustomHeader *ch, const char *name, const char *value);
const char *sal_custom_header_find(const SalCustomHeader *ch, const char *name);
SalCustomHeader *sal_custom_header_remove(SalCustomHeader *ch, const char *name);
void sal_custom_header_free(SalCustomHeader *ch);
SalCustomHeader *sal_custom_header_clone(const SalCustomHeader *ch);



SalCustomSdpAttribute * sal_custom_sdp_attribute_append(SalCustomSdpAttribute *csa, const char *name, const char *value);
const char * sal_custom_sdp_attribute_find(const SalCustomSdpAttribute *csa, const char *name);
void sal_custom_sdp_attribute_free(SalCustomSdpAttribute *csa);
SalCustomSdpAttribute * sal_custom_sdp_attribute_clone(const SalCustomSdpAttribute *csa);

/** deprecated. use sal_set_log_level instead **/
void sal_enable_log(void);
/** deprecated. use sal_set_log_level instead **/
void sal_disable_log(void);
void sal_set_log_level(OrtpLogLevel level);

unsigned int sal_get_random(void);
LINPHONE_PUBLIC char *sal_get_random_token(int size);
unsigned char * sal_get_random_bytes(unsigned char *ret, size_t size);
#if 0
belle_sip_source_t * sal_create_timer(Sal *sal, belle_sip_source_func_t func, void *data, unsigned int timeout_value_ms, const char* timer_name);
void sal_cancel_timer(Sal *sal, belle_sip_source_t *timer);
#endif

//SalBodyHandler * sal_body_handler_new(const char *type, const char *subtype, void *data, size_t size, const char *encoding);
SalBodyHandler * sal_body_handler_new(void);
SalBodyHandler * sal_body_handler_ref(SalBodyHandler *body_handler);
void sal_body_handler_unref(SalBodyHandler *body_handler);
const char * sal_body_handler_get_type(const SalBodyHandler *body_handler);
void sal_body_handler_set_type(SalBodyHandler *body_handler, const char *type);
const char * sal_body_handler_get_subtype(const SalBodyHandler *body_handler);
void sal_body_handler_set_subtype(SalBodyHandler *body_handler, const char *subtype);
const belle_sip_list_t * sal_body_handler_get_content_type_parameters_names(const SalBodyHandler *body_handler);
const char * sal_body_handler_get_content_type_parameter(const SalBodyHandler *body_handler, const char *name);
void sal_body_handler_set_content_type_parameter(SalBodyHandler *body_handler, const char *paramName, const char *paramValue);
const char * sal_body_handler_get_encoding(const SalBodyHandler *body_handler);
void sal_body_handler_set_encoding(SalBodyHandler *body_handler, const char *encoding);
void * sal_body_handler_get_data(const SalBodyHandler *body_handler);
void sal_body_handler_set_data(SalBodyHandler *body_handler, void *data);
size_t sal_body_handler_get_size(const SalBodyHandler *body_handler);
void sal_body_handler_set_size(SalBodyHandler *body_handler, size_t size);
bool_t sal_body_handler_is_multipart(const SalBodyHandler *body_handler);
SalBodyHandler * sal_body_handler_get_part(const SalBodyHandler *body_handler, int idx);
const belle_sip_list_t * sal_body_handler_get_parts(const SalBodyHandler *body_handler);
SalBodyHandler * sal_body_handler_find_part_by_header(const SalBodyHandler *body_handler, const char *header_name, const char *header_value);
const char * sal_body_handler_get_header(const SalBodyHandler *body_handler, const char *header_name);
const belle_sip_list_t* sal_body_handler_get_headers(const SalBodyHandler *body_handler);

/*this function parses a document with key=value pairs separated by new lines, and extracts the value for a given key*/
int sal_lines_get_value(const char *data, const char *key, char *value, size_t value_size);


unsigned long sal_begin_background_task(const char *name, void (*max_time_reached)(void *), void *data);
void sal_end_background_task(unsigned long id);

/*Some old equipment may not only rely on attribute sendonly/recvonly/sendrecv/inative*/

#ifdef __cplusplus
}
#endif

#endif
