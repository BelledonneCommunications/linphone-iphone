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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "sal/sal.h"
#include "bellesip_sal/sal_impl.h"

#include <ctype.h>

const char* sal_transport_to_string(SalTransport transport) {
	switch (transport) {
		case SalTransportUDP:return "udp";
		case SalTransportTCP: return "tcp";
		case SalTransportTLS:return "tls";
		case SalTransportDTLS:return "dtls";
		default: {
			ms_fatal("Unexpected transport [%i]",transport);
			return NULL;
		}
	}
}

SalTransport sal_transport_parse(const char* param) {
	if (!param) return SalTransportUDP;
	if (strcasecmp("udp",param)==0) return SalTransportUDP;
	if (strcasecmp("tcp",param)==0) return SalTransportTCP;
	if (strcasecmp("tls",param)==0) return SalTransportTLS;
	if (strcasecmp("dtls",param)==0) return SalTransportDTLS;
	ms_error("Unknown transport type[%s], returning UDP", param);
	return SalTransportUDP;
}

SalMediaDescription *sal_media_description_new(){
	SalMediaDescription *md=ms_new0(SalMediaDescription,1);
	md->refcount=1;
	return md;
}

static void sal_media_description_destroy(SalMediaDescription *md){
	int i;
	for(i=0;i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS;i++){
		ms_list_for_each(md->streams[i].payloads,(void (*)(void *))payload_type_destroy);
		ms_list_free(md->streams[i].payloads);
		md->streams[i].payloads=NULL;
	}
	ms_free(md);
}

SalMediaDescription * sal_media_description_ref(SalMediaDescription *md){
	md->refcount++;
	return md;
}

void sal_media_description_unref(SalMediaDescription *md){
	md->refcount--;
	if (md->refcount==0){
		sal_media_description_destroy (md);
	}
}

SalStreamDescription *sal_media_description_find_stream(SalMediaDescription *md,
	SalMediaProto proto, SalStreamType type){
	int i;
	for(i=0;i<md->nb_streams;++i){
		SalStreamDescription *ss=&md->streams[i];
		if (!sal_stream_description_active(ss)) continue;
		if (ss->proto==proto && ss->type==type) return ss;
	}
	return NULL;
}

unsigned int sal_media_description_nb_active_streams_of_type(SalMediaDescription *md, SalStreamType type) {
	unsigned int i;
	unsigned int nb = 0;
	for (i = 0; i < md->nb_streams; ++i) {
		if (!sal_stream_description_active(&md->streams[i])) continue;
		if (md->streams[i].type == type) nb++;
	}
	return nb;
}

SalStreamDescription * sal_media_description_get_active_stream_of_type(SalMediaDescription *md, SalStreamType type, unsigned int idx) {
	unsigned int i;
	for (i = 0; i < md->nb_streams; ++i) {
		if (!sal_stream_description_active(&md->streams[i])) continue;
		if (md->streams[i].type == type) {
			if (idx-- == 0) return &md->streams[i];
		}
	}
	return NULL;
}

SalStreamDescription * sal_media_description_find_secure_stream_of_type(SalMediaDescription *md, SalStreamType type) {
	SalStreamDescription *desc = sal_media_description_find_stream(md, SalProtoRtpSavpf, type);
	if (desc == NULL) desc = sal_media_description_find_stream(md, SalProtoRtpSavp, type);
	return desc;
}

SalStreamDescription * sal_media_description_find_best_stream(SalMediaDescription *md, SalStreamType type) {
	SalStreamDescription *desc = sal_media_description_find_stream(md, SalProtoRtpSavpf, type);
	if (desc == NULL) desc = sal_media_description_find_stream(md, SalProtoRtpSavp, type);
	if (desc == NULL) desc = sal_media_description_find_stream(md, SalProtoRtpAvpf, type);
	if (desc == NULL) desc = sal_media_description_find_stream(md, SalProtoRtpAvp, type);
	return desc;
}

bool_t sal_media_description_empty(const SalMediaDescription *md){
	if (sal_media_description_get_nb_active_streams(md) > 0) return FALSE;
	return TRUE;
}

void sal_media_description_set_dir(SalMediaDescription *md, SalStreamDir stream_dir){
	int i;
	for(i=0;i<md->nb_streams;++i){
		SalStreamDescription *ss=&md->streams[i];
		if (!sal_stream_description_active(ss)) continue;
		ss->dir=stream_dir;
	}
}

int sal_media_description_get_nb_active_streams(const SalMediaDescription *md) {
	int i;
	int nb = 0;
	for (i = 0; i < md->nb_streams; i++) {
		if (sal_stream_description_active(&md->streams[i])) nb++;
	}
	return nb;
}


static bool_t is_null_address(const char *addr){
	return strcmp(addr,"0.0.0.0")==0 || strcmp(addr,"::0")==0;
}

/*check for the presence of at least one stream with requested direction */
static bool_t has_dir(const SalMediaDescription *md, SalStreamDir stream_dir){
	int i;

	/* we are looking for at least one stream with requested direction, inactive streams are ignored*/
	for(i=0;i<md->nb_streams;++i){
		const SalStreamDescription *ss=&md->streams[i];
		if (!sal_stream_description_active(ss)) continue;
		if (ss->dir==stream_dir) return TRUE;
		/*compatibility check for phones that only used the null address and no attributes */
		if (ss->dir==SalStreamSendRecv && stream_dir==SalStreamSendOnly && (is_null_address(md->addr) || is_null_address(ss->rtp_addr)))
			return TRUE;
	}
	return FALSE;
}

bool_t sal_media_description_has_dir(const SalMediaDescription *md, SalStreamDir stream_dir){
	if (stream_dir==SalStreamRecvOnly){
		if (has_dir(md,SalStreamSendOnly) || has_dir(md,SalStreamSendRecv)) return FALSE;
		else return TRUE;
	}else if (stream_dir==SalStreamSendOnly){
		if (has_dir(md,SalStreamRecvOnly) || has_dir(md,SalStreamSendRecv)) return FALSE;
		else return TRUE;
	}else if (stream_dir==SalStreamSendRecv){
		return has_dir(md,SalStreamSendRecv);
	}else{
		/*SalStreamInactive*/
		if (has_dir(md,SalStreamSendOnly) || has_dir(md,SalStreamSendRecv)  || has_dir(md,SalStreamRecvOnly))
			return FALSE;
		else return TRUE;
	}
	return FALSE;
}

bool_t sal_stream_description_active(const SalStreamDescription *sd) {
	return (sd->rtp_port > 0);
}

bool_t sal_stream_description_has_avpf(const SalStreamDescription *sd) {
	return ((sd->proto == SalProtoRtpAvpf) || (sd->proto == SalProtoRtpSavpf));
}

bool_t sal_stream_description_has_srtp(const SalStreamDescription *sd) {
	return ((sd->proto == SalProtoRtpSavp) || (sd->proto == SalProtoRtpSavpf));
}

bool_t sal_media_description_has_avpf(const SalMediaDescription *md) {
	int i;
	if (md->nb_streams == 0) return FALSE;
	for (i = 0; i < md->nb_streams; i++) {
		if (!sal_stream_description_active(&md->streams[i])) continue;
		if (sal_stream_description_has_avpf(&md->streams[i]) != TRUE) return FALSE;
	}
	return TRUE;
}

bool_t sal_media_description_has_srtp(const SalMediaDescription *md) {
	int i;
	if (md->nb_streams == 0) return FALSE;
	for (i = 0; i < md->nb_streams; i++) {
		if (!sal_stream_description_active(&md->streams[i])) continue;
		if (sal_stream_description_has_srtp(&md->streams[i]) != TRUE) return FALSE;
	}
	return TRUE;
}

/*
static bool_t fmtp_equals(const char *p1, const char *p2){
	if (p1 && p2 && strcmp(p1,p2)==0) return TRUE;
	if (p1==NULL && p2==NULL) return TRUE;
	return FALSE;
}
*/

static bool_t payload_type_equals(const PayloadType *p1, const PayloadType *p2){
	if (p1->type!=p2->type) return FALSE;
	if (strcmp(p1->mime_type,p2->mime_type)!=0) return FALSE;
	if (p1->clock_rate!=p2->clock_rate) return FALSE;
	if (p1->channels!=p2->channels) return FALSE;
	if (payload_type_get_number(p1) != payload_type_get_number(p2)) return FALSE;
	/*
	 Do not compare fmtp right now: they are modified internally when the call is started
	*/
	/*
	if (!fmtp_equals(p1->recv_fmtp,p2->recv_fmtp) ||
		!fmtp_equals(p1->send_fmtp,p2->send_fmtp))
		return FALSE;
	*/
	return TRUE;
}

static bool_t is_recv_only(PayloadType *p){
	return (p->flags & PAYLOAD_TYPE_FLAG_CAN_RECV) && ! (p->flags & PAYLOAD_TYPE_FLAG_CAN_SEND);
}

static bool_t payload_list_equals(const MSList *l1, const MSList *l2){
	const MSList *e1,*e2;
	for(e1=l1,e2=l2;e1!=NULL && e2!=NULL; e1=e1->next,e2=e2->next){
		PayloadType *p1=(PayloadType*)e1->data;
		PayloadType *p2=(PayloadType*)e2->data;
		if (!payload_type_equals(p1,p2))
			return FALSE;
	}
	if (e1!=NULL){
		/*skip possible recv-only payloads*/
		for(;e1!=NULL && is_recv_only((PayloadType*)e1->data);e1=e1->next){
			ms_message("Skipping recv-only payload type...");
		}
	}
	if (e1!=NULL || e2!=NULL){
		/*means one list is longer than the other*/
		return FALSE;
	}
	return TRUE;
}

int sal_stream_description_equals(const SalStreamDescription *sd1, const SalStreamDescription *sd2) {
	int result = SAL_MEDIA_DESCRIPTION_UNCHANGED;
	int i;

	/* A different proto should result in SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED but the encryption change
	   needs a stream restart for now, so use SAL_MEDIA_DESCRIPTION_CODEC_CHANGED */
	if (sd1->proto != sd2->proto) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	for (i = 0; i < SAL_CRYPTO_ALGO_MAX; i++) {
		if ((sd1->crypto[i].tag != sd2->crypto[i].tag)
			|| (sd1->crypto[i].algo != sd2->crypto[i].algo)){
			result|=SAL_MEDIA_DESCRIPTION_CRYPTO_POLICY_CHANGED;
		}
		if ((strncmp(sd1->crypto[i].master_key, sd2->crypto[i].master_key, sizeof(sd1->crypto[i].master_key) - 1))) {
			result |= SAL_MEDIA_DESCRIPTION_CRYPTO_KEYS_CHANGED;
		}
	}

	if (sd1->type != sd2->type) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	if (strcmp(sd1->rtp_addr, sd2->rtp_addr) != 0) result |= SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED;
	if (sd1->rtp_port != sd2->rtp_port) {
		if ((sd1->rtp_port == 0) || (sd2->rtp_port == 0)) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
		else result |= SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED;
	}
	if (strcmp(sd1->rtcp_addr, sd2->rtcp_addr) != 0) result |= SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED;
	if (sd1->rtcp_port != sd2->rtcp_port) result |= SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED;
	if (!payload_list_equals(sd1->payloads, sd2->payloads)) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	if (sd1->bandwidth != sd2->bandwidth) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	if (sd1->ptime != sd2->ptime) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	if (sd1->dir != sd2->dir) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;

	return result;
}

int sal_media_description_equals(const SalMediaDescription *md1, const SalMediaDescription *md2) {
	int result = SAL_MEDIA_DESCRIPTION_UNCHANGED;
	int i;

	if (strcmp(md1->addr, md2->addr) != 0) result |= SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED;
	if (md1->nb_streams != md2->nb_streams) result |= SAL_MEDIA_DESCRIPTION_STREAMS_CHANGED;
	if (md1->bandwidth != md2->bandwidth) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	for(i = 0; i < md1->nb_streams; ++i){
		result |= sal_stream_description_equals(&md1->streams[i], &md2->streams[i]);
	}
	return result;
}
static void assign_address(SalAddress** address, const char *value){
	if (*address){
		sal_address_destroy(*address);
		*address=NULL;
	}
	if (value)
		*address=sal_address_new(value);
}

static void assign_string(char **str, const char *arg){
	if (*str){
		ms_free(*str);
		*str=NULL;
	}
	if (arg)
		*str=ms_strdup(arg);
}

void sal_op_set_contact_address(SalOp *op, const SalAddress *address){
	if (((SalOpBase*)op)->contact_address) sal_address_destroy(((SalOpBase*)op)->contact_address);
	((SalOpBase*)op)->contact_address=address?sal_address_clone(address):NULL;
}
const SalAddress* sal_op_get_contact_address(const SalOp *op) {
	return ((SalOpBase*)op)->contact_address;
}

const SalAddress*sal_op_get_remote_contact_address(const SalOp* op)
{
	return ((SalOpBase*)op)->remote_contact_address;
}

#define SET_PARAM(op,name) \
		char* name##_string=NULL; \
		assign_address(&((SalOpBase*)op)->name##_address,name); \
		if (((SalOpBase*)op)->name##_address) { \
			name##_string=sal_address_as_string(((SalOpBase*)op)->name##_address); \
		}\
		assign_string(&((SalOpBase*)op)->name,name##_string); \
		if(name##_string) ms_free(name##_string);


void sal_op_set_route(SalOp *op, const char *route){
	char* route_string=(void *)0;
	SalOpBase* op_base = (SalOpBase*)op;
	if (op_base->route_addresses) {
		ms_list_for_each(op_base->route_addresses,(void (*)(void *))sal_address_destroy);
		op_base->route_addresses=ms_list_free(op_base->route_addresses);
	}
	if (route) {
		op_base->route_addresses=ms_list_append(NULL,NULL);
		assign_address((SalAddress**)&(op_base->route_addresses->data),route);
		route_string=sal_address_as_string((SalAddress*)op_base->route_addresses->data); \
	}
	assign_string(&op_base->route,route_string); \
	if(route_string) ms_free(route_string);
}
const MSList* sal_op_get_route_addresses(const SalOp *op) {
	return ((SalOpBase*)op)->route_addresses;
}
void sal_op_set_route_address(SalOp *op, const SalAddress *address){
	char* address_string=sal_address_as_string(address); /*can probably be optimized*/
	sal_op_set_route(op,address_string);
	ms_free(address_string);
}
void sal_op_add_route_address(SalOp *op, const SalAddress *address){
	SalOpBase* op_base = (SalOpBase*)op;
	if (op_base->route_addresses) {
		op_base->route_addresses=ms_list_append(op_base->route_addresses,(void*)sal_address_clone(address));
	} else {
		sal_op_set_route_address(op,address);
	}
}
void sal_op_set_realm(SalOp *op, const char *realm){
	SalOpBase* op_base = (SalOpBase*)op;
	if (op_base->realm != NULL){
		ms_free(op_base->realm);
	}
	op_base->realm = ms_strdup(realm);
}
void sal_op_set_from(SalOp *op, const char *from){
	SET_PARAM(op,from);
}
void sal_op_set_from_address(SalOp *op, const SalAddress *from){
	char* address_string=sal_address_as_string(from); /*can probably be optimized*/
	sal_op_set_from(op,address_string);
	ms_free(address_string);
}
void sal_op_set_to(SalOp *op, const char *to){
	SET_PARAM(op,to);
}
void sal_op_set_to_address(SalOp *op, const SalAddress *to){
	char* address_string=sal_address_as_string(to); /*can probably be optimized*/
	sal_op_set_to(op,address_string);
	ms_free(address_string);
}

void sal_op_set_user_pointer(SalOp *op, void *up){
	((SalOpBase*)op)->user_pointer=up;
}

Sal *sal_op_get_sal(const SalOp *op){
	return ((SalOpBase*)op)->root;
}

const char *sal_op_get_from(const SalOp *op){
	return ((SalOpBase*)op)->from;
}
const SalAddress *sal_op_get_from_address(const SalOp *op){
	return ((SalOpBase*)op)->from_address;
}

const char *sal_op_get_to(const SalOp *op){
	return ((SalOpBase*)op)->to;
}

const SalAddress *sal_op_get_to_address(const SalOp *op){
	return ((SalOpBase*)op)->to_address;
}

const char *sal_op_get_route(const SalOp *op){
#ifdef BELLE_SIP
ms_fatal("sal_op_get_route not supported, use sal_op_get_route_addresses instead");
#endif
	return ((SalOpBase*)op)->route;
}

const char *sal_op_get_remote_ua(const SalOp *op){
	return ((SalOpBase*)op)->remote_ua;
}

void *sal_op_get_user_pointer(const SalOp *op){
	return ((SalOpBase*)op)->user_pointer;
}

const char *sal_op_get_proxy(const SalOp *op){
	return ((SalOpBase*)op)->route;
}

const char *sal_op_get_network_origin(const SalOp *op){
	return ((SalOpBase*)op)->origin;
}
const char* sal_op_get_call_id(const SalOp *op) {
	return  ((SalOpBase*)op)->call_id;
}
char* sal_op_get_dialog_id(const SalOp *op) {
	if (op->dialog != NULL) {
		return ms_strdup_printf("%s;to-tag=%s;from-tag=%s", ((SalOpBase*)op)->call_id,
			belle_sip_dialog_get_remote_tag(op->dialog), belle_sip_dialog_get_local_tag(op->dialog));
	}
	return NULL;

}
void __sal_op_init(SalOp *b, Sal *sal){
	memset(b,0,sizeof(SalOpBase));
	((SalOpBase*)b)->root=sal;
}

void __sal_op_set_network_origin(SalOp *op, const char *origin){
	SET_PARAM(op,origin);
}

void __sal_op_set_remote_contact(SalOp *op, const char* remote_contact){
	SET_PARAM(op,remote_contact);
}

void __sal_op_set_network_origin_address(SalOp *op, SalAddress *origin){
	char* address_string=sal_address_as_string(origin); /*can probably be optimized*/
	__sal_op_set_network_origin(op,address_string);
	ms_free(address_string);
}

void __sal_op_free(SalOp *op){
	SalOpBase *b=(SalOpBase *)op;
	if (b->from_address){
		sal_address_destroy(b->from_address);
		b->from_address=NULL;
	}
	if (b->to_address){
		sal_address_destroy(b->to_address);
		b->to_address=NULL;
	}

	if (b->service_route){
		sal_address_destroy(b->service_route);
		b->service_route=NULL;
	}

	if (b->origin_address){
		sal_address_destroy(b->origin_address);
		b->origin_address=NULL;
	}

	if (b->from) {
		ms_free(b->from);
		b->from=NULL;
	}
	if (b->to) {
		ms_free(b->to);
		b->to=NULL;
	}
	if (b->route) {
		ms_free(b->route);
		b->route=NULL;
	}
	if (b->realm) {
		ms_free(b->realm);
		b->realm=NULL;
	}
	if (b->contact_address) {
		sal_address_destroy(b->contact_address);
	}
	if (b->origin){
		ms_free(b->origin);
		b->origin=NULL;
	}
	if (b->remote_ua){
		ms_free(b->remote_ua);
		b->remote_ua=NULL;
	}
	if (b->remote_contact){
		ms_free(b->remote_contact);
		b->remote_contact=NULL;
	}
	if (b->remote_contact_address){
		sal_address_destroy(b->remote_contact_address);
	}
	if (b->local_media)
		sal_media_description_unref(b->local_media);
	if (b->remote_media)
		sal_media_description_unref(b->remote_media);
	if (b->call_id)
		ms_free((void*)b->call_id);
	if (b->service_route) {
		sal_address_destroy(b->service_route);
	}
	if (b->route_addresses){
		ms_list_for_each(b->route_addresses,(void (*)(void*)) sal_address_destroy);
		b->route_addresses=ms_list_free(b->route_addresses);
	}
	if (b->recv_custom_headers)
		sal_custom_header_free(b->recv_custom_headers);
	if (b->sent_custom_headers)
		sal_custom_header_free(b->sent_custom_headers);
	ms_free(op);
}

SalAuthInfo* sal_auth_info_new() {
	return ms_new0(SalAuthInfo,1);
}

SalAuthInfo* sal_auth_info_clone(const SalAuthInfo* auth_info) {
	SalAuthInfo* new_auth_info=sal_auth_info_new();
	new_auth_info->username=auth_info->username?ms_strdup(auth_info->username):NULL;
	new_auth_info->userid=auth_info->userid?ms_strdup(auth_info->userid):NULL;
	new_auth_info->realm=auth_info->realm?ms_strdup(auth_info->realm):NULL;
	new_auth_info->domain=auth_info->realm?ms_strdup(auth_info->domain):NULL;
	new_auth_info->password=auth_info->password?ms_strdup(auth_info->password):NULL;
	return new_auth_info;
}

void sal_auth_info_delete(SalAuthInfo* auth_info) {
	if (auth_info->username) ms_free(auth_info->username);
	if (auth_info->userid) ms_free(auth_info->userid);
	if (auth_info->realm) ms_free(auth_info->realm);
	if (auth_info->domain) ms_free(auth_info->domain);
	if (auth_info->password) ms_free(auth_info->password);
	if (auth_info->ha1) ms_free(auth_info->ha1);
	if (auth_info->certificates) sal_certificates_chain_delete(auth_info->certificates);
	if (auth_info->key) sal_signing_key_delete(auth_info->key);
	ms_free(auth_info);
}



const char* sal_stream_type_to_string(SalStreamType type) {
	switch (type) {
	case SalAudio:return "audio";
	case SalVideo:return "video";
	default: return "other";
	}
}

const char *sal_stream_description_get_type_as_string(const SalStreamDescription *desc){
	if (desc->type==SalOther) return desc->typeother;
	else return sal_stream_type_to_string(desc->type);
}

const char* sal_media_proto_to_string(SalMediaProto type) {
	switch (type) {
	case SalProtoRtpAvp:return "RTP/AVP";
	case SalProtoRtpSavp:return "RTP/SAVP";
	case SalProtoRtpAvpf:return "RTP/AVPF";
	case SalProtoRtpSavpf:return "RTP/SAVPF";
	default: return "unknown";
	}
}

const char *sal_stream_description_get_proto_as_string(const SalStreamDescription *desc){
	if (desc->proto==SalProtoOther) return desc->proto_other;
	else return sal_media_proto_to_string(desc->proto);
}


const char* sal_stream_dir_to_string(SalStreamDir type) {
	switch (type) {
	case SalStreamSendRecv:return "sendrecv";
	case SalStreamSendOnly:return "sendonly";
	case SalStreamRecvOnly:return "recvonly";
	case SalStreamInactive:return "inative";
	default: return "unknown";
	}

}

const char* sal_reason_to_string(const SalReason reason) {
	switch (reason) {
	case SalReasonDeclined : return "SalReasonDeclined";
	case SalReasonBusy: return "SalReasonBusy";
	case SalReasonRedirect: return "SalReasonRedirect";
	case SalReasonTemporarilyUnavailable: return "SalReasonTemporarilyUnavailable";
	case SalReasonNotFound: return "SalReasonNotFound";
	case SalReasonDoNotDisturb: return "SalReasonDoNotDisturb";
	case SalReasonUnsupportedContent: return "SalReasonUnsupportedContent";
	case SalReasonForbidden: return "SalReasonForbidden";
	case SalReasonUnknown: return "SalReasonUnknown";
	case SalReasonServiceUnavailable: return "SalReasonServiceUnavailable";
	case SalReasonNotAcceptable: return "SalReasonNotAcceptable";
	default: return "Unkown reason";
	}
}
const SalAddress* sal_op_get_service_route(const SalOp *op) {
	return ((SalOpBase*)op)->service_route;
}
void sal_op_set_service_route(SalOp *op,const SalAddress* service_route) {
	if (((SalOpBase*)op)->service_route)
		sal_address_destroy(((SalOpBase*)op)->service_route);

	((SalOpBase*)op)->service_route=service_route?sal_address_clone(service_route):NULL;
}

const char* sal_presence_status_to_string(const SalPresenceStatus status) {
	switch (status) {
	case SalPresenceOffline: return "SalPresenceOffline";
	case SalPresenceOnline: return "SalPresenceOnline";
	case SalPresenceBusy: return "SalPresenceBusy";
	case SalPresenceBerightback: return "SalPresenceBerightback";
	case SalPresenceAway: return "SalPresenceAway";
	case SalPresenceOnthephone: return "SalPresenceOnthephone";
	case SalPresenceOuttolunch: return "SalPresenceOuttolunch";
	case SalPresenceDonotdisturb: return "SalPresenceDonotdisturb";
	case SalPresenceMoved: return "SalPresenceMoved";
	case SalPresenceAltService: return "SalPresenceAltService";
	default : return "unknown";
	}

}
const char* sal_privacy_to_string(SalPrivacy privacy) {
	switch(privacy) {
	case SalPrivacyUser: return "user";
	case SalPrivacyHeader: return "header";
	case SalPrivacySession: return "session";
	case SalPrivacyId: return "id";
	case SalPrivacyNone: return "none";
	case SalPrivacyCritical: return "critical";
	default: return NULL;
	}
}

static void remove_trailing_spaces(char *line){
	int i;
	for(i=strlen(line)-1;i>=0;--i){
		if (isspace(line[i])) line[i]='\0';
		else break;
	}
}

static int line_get_value(const char *input, const char *key, char *value, size_t value_size, int *read){
	const char *end=strchr(input,'\n');
	char line[256]={0};
	char key_candidate[256];
	char *equal;
	size_t len;
	if (!end) len=strlen(input);
	else len=end +1 -input;
	*read=len;
	strncpy(line,input,MIN(len,sizeof(line)));
	equal=strchr(line,'=');
	if (!equal) return FALSE;
	*equal='\0';
	if (sscanf(line,"%s",key_candidate)!=1) return FALSE;
	if (strcasecmp(key,key_candidate)==0){
		equal++;
		remove_trailing_spaces(equal);
		strncpy(value,equal,value_size-1);
		value[value_size-1]='\0';
		return TRUE;
	}
	return FALSE;
}

int sal_lines_get_value(const char *data, const char *key, char *value, size_t value_size){
	int read=0;

	do{
		if (line_get_value(data,key,value,value_size,&read))
			return TRUE;
		data+=read;
	}while(read!=0);
	return FALSE;
}

int sal_body_has_type(const SalBody *body, const char *type, const char *subtype){
	return body->type && body->subtype
		&& strcmp(body->type,type)==0
		&& strcmp(body->subtype,subtype)==0;
}

belle_sip_stack_t *sal_get_belle_sip_stack(Sal *sal) {
	return sal->stack;
}

char* sal_op_get_public_uri(SalOp *op) {
	if (op && op->refresher) {
		return belle_sip_refresher_get_public_uri(op->refresher);
	}
	return NULL;
}
