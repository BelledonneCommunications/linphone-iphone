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

#include "sal.h"
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

void sal_media_description_ref(SalMediaDescription *md){
	md->refcount++;
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
	for(i=0;i<md->n_active_streams;++i){
		SalStreamDescription *ss=&md->streams[i];
		if (ss->proto==proto && ss->type==type) return ss;
	}
	return NULL;
}

bool_t sal_media_description_empty(const SalMediaDescription *md){
	if (md->n_active_streams > 0) return FALSE;
	return TRUE;
}

void sal_media_description_set_dir(SalMediaDescription *md, SalStreamDir stream_dir){
	int i;
	for(i=0;i<md->n_active_streams;++i){
		SalStreamDescription *ss=&md->streams[i];
		ss->dir=stream_dir;
	}
}


static bool_t is_null_address(const char *addr){
	return strcmp(addr,"0.0.0.0")==0 || strcmp(addr,"::0")==0;
}

/*check for the presence of at least one stream with requested direction */
static bool_t has_dir(const SalMediaDescription *md, SalStreamDir stream_dir){
	int i;

	/* we are looking for at least one stream with requested direction, inactive streams are ignored*/
	for(i=0;i<md->n_active_streams;++i){
		const SalStreamDescription *ss=&md->streams[i];
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
			|| (sd1->crypto[i].algo != sd2->crypto[i].algo)
			|| (strncmp(sd1->crypto[i].master_key, sd2->crypto[i].master_key, sizeof(sd1->crypto[i].master_key) - 1))) {
			result |= SAL_MEDIA_DESCRIPTION_CRYPTO_CHANGED;
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
	if (md1->n_total_streams != md2->n_total_streams) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	if (md1->bandwidth != md2->bandwidth) result |= SAL_MEDIA_DESCRIPTION_CODEC_CHANGED;
	for(i = 0; i < md1->n_total_streams; ++i){
		result |= sal_stream_description_equals(&md1->streams[i], &md2->streams[i]);
	}
	return result;
}

static void assign_string(char **str, const char *arg){
	if (*str){
		ms_free(*str);
		*str=NULL;
	}
	if (arg)
		*str=ms_strdup(arg);
}

void sal_op_set_contact(SalOp *op, const char *contact){
	assign_string(&((SalOpBase*)op)->contact,contact);
}

void sal_op_set_route(SalOp *op, const char *route){
	assign_string(&((SalOpBase*)op)->route,route);
}

void sal_op_set_from(SalOp *op, const char *from){
	assign_string(&((SalOpBase*)op)->from,from);
}

void sal_op_set_to(SalOp *op, const char *to){
	assign_string(&((SalOpBase*)op)->to,to);
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

const char *sal_op_get_to(const SalOp *op){
	return ((SalOpBase*)op)->to;
}

const char *sal_op_get_contact(const SalOp *op){
	return ((SalOpBase*)op)->contact;
}

const char *sal_op_get_remote_contact(const SalOp *op){
	return ((SalOpBase*)op)->remote_contact;
}

const char *sal_op_get_route(const SalOp *op){
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
void __sal_op_init(SalOp *b, Sal *sal){
	memset(b,0,sizeof(SalOpBase));
	((SalOpBase*)b)->root=sal;
}

void __sal_op_set_network_origin(SalOp *op, const char *origin){
	assign_string(&((SalOpBase*)op)->origin,origin);
}

void __sal_op_set_remote_contact(SalOp *op, const char *ct){
	assign_string(&((SalOpBase*)op)->remote_contact,ct);
}

void __sal_op_free(SalOp *op){
	SalOpBase *b=(SalOpBase *)op;
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
	if (b->contact) {
		ms_free(b->contact);
		b->contact=NULL;
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
	if (b->local_media)
		sal_media_description_unref(b->local_media);
	if (b->remote_media)
		sal_media_description_unref(b->remote_media);
	if (b->call_id)
		ms_free(b->call_id);
	if (b->custom_headers)
		sal_custom_header_free(b->custom_headers);
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
	new_auth_info->password=auth_info->password?ms_strdup(auth_info->password):NULL;
	return new_auth_info;
}

void sal_auth_info_delete(const SalAuthInfo* auth_info) {
	if (auth_info->username) ms_free(auth_info->username);
	if (auth_info->userid) ms_free(auth_info->userid);
	if (auth_info->realm) ms_free(auth_info->realm);
	if (auth_info->password) ms_free(auth_info->password);
	ms_free((void*)auth_info);
}

SalCustomHeader *sal_custom_header_append(SalCustomHeader *ch, const char *name, const char *value){
	SalCustomHeader *h=ms_new0(SalCustomHeader,1);
	h->header_name=ms_strdup(name);
	h->header_value=ms_strdup(value);
	h->node.data=h;
	return (SalCustomHeader*)ms_list_append_link((MSList*)ch,(MSList*)h);
}

const char *sal_custom_header_find(const SalCustomHeader *ch, const char *name){
	const MSList *it;
	for (it=(const MSList*)ch;it!=NULL;it=it->next){
		const SalCustomHeader *itch=(const SalCustomHeader *)it;
		if (strcasecmp(itch->header_name,name)==0)
			return itch->header_value;
	}
	return NULL;
}

static void sal_custom_header_uninit(SalCustomHeader *ch){
	ms_free(ch->header_name);
	ms_free(ch->header_value);
}

void sal_custom_header_free(SalCustomHeader *ch){
	ms_list_for_each((MSList*)ch,(void (*)(void*))sal_custom_header_uninit);
	ms_list_free((MSList *)ch);
}

SalCustomHeader *sal_custom_header_clone(const SalCustomHeader *ch){
	const MSList *it;
	SalCustomHeader *ret=NULL;
	for (it=(const MSList*)ch;it!=NULL;it=it->next){
		const SalCustomHeader *itch=(const SalCustomHeader *)it;
		ret=sal_custom_header_append(ret,itch->header_name,itch->header_value);
	}
	return ret;
}

const SalCustomHeader *sal_op_get_custom_header(SalOp *op){
	SalOpBase *b=(SalOpBase *)op;
	return b->custom_headers;
}

/*
 * Warning: this function takes owneship of the custom headers
 */
void sal_op_set_custom_header(SalOp *op, SalCustomHeader* ch){
	SalOpBase *b=(SalOpBase *)op;
	if (b->custom_headers){
		sal_custom_header_free(b->custom_headers);
		b->custom_headers=NULL;
	}
	b->custom_headers=ch;
}



