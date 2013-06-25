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

#ifndef sal_exosip2_h
#define sal_exosip2_h

#include "sal.h"
#include <eXosip2/eXosip.h>



sdp_message_t *media_description_to_sdp(const SalMediaDescription *sal);
int sdp_to_media_description(sdp_message_t *sdp, SalMediaDescription *desc);

struct Sal{
	SalCallbacks callbacks;
	SalTransport transport;
	MSList *calls; /*MSList of SalOp */
	MSList *registers;/*MSList of SalOp */
	MSList *out_subscribes;/*MSList of SalOp */
	MSList *in_subscribes;/*MSList of SalOp */
	MSList *pending_auths;/*MSList of SalOp */
	MSList *other_transactions; /*MSList of SalOp */
	int running;
	int session_expires;
	int keepalive_period;
	void *up; /*user pointer*/
	char* rootCa; /* File _or_ folder containing root CA */
	int dscp;
	bool_t one_matching_codec;
	bool_t double_reg;
	bool_t use_rports;
	bool_t use_101;
	bool_t reuse_authorization;
	bool_t verify_server_certs;
	bool_t verify_server_cn;
	bool_t expire_old_contact;
	bool_t add_dates;
	bool_t tcp_tls_keepalive;
};

struct SalOp{
	SalOpBase base;
	int cid;
	int did;
	int tid;
	int rid;
	int sid;
	int nid;
	int expires;
	SalMediaDescription *result;
	sdp_message_t *sdp_answer;
	eXosip_event_t *pending_auth;
	osip_call_id_t *call_id; /*used for out of calls transaction in order
	 			to retrieve the operation when receiving a response*/
	char *replaces;
	char *referred_by;
	const SalAuthInfo *auth_info;
	const char *sipfrag_pending;
	bool_t supports_session_timers;
	bool_t sdp_offering;
	bool_t reinvite;
	bool_t masquerade_via;
	bool_t auto_answer_asked;
	bool_t terminated;
};

void sal_remove_out_subscribe(Sal *sal, SalOp *op);
void sal_remove_in_subscribe(Sal *sal, SalOp *op);
void sal_add_other(Sal *sal, SalOp *op,  osip_message_t *request);

void sal_exosip_subscription_recv(Sal *sal, eXosip_event_t *ev);
void sal_exosip_subscription_answered(Sal *sal,eXosip_event_t *ev);
void sal_exosip_notify_recv(Sal *sal,eXosip_event_t *ev);
void sal_exosip_subscription_closed(Sal *sal,eXosip_event_t *ev);

void sal_exosip_in_subscription_closed(Sal *sal, eXosip_event_t *ev);
SalOp * sal_find_out_subscribe(Sal *sal, int sid);
SalOp * sal_find_in_subscribe(Sal *sal, int nid);
void sal_exosip_fix_route(SalOp *op);
void sal_exosip_add_custom_headers(osip_message_t *msg, SalCustomHeader *ch);
SalCustomHeader * sal_exosip_get_custom_headers(osip_message_t *msg);

void _osip_list_set_empty(osip_list_t *l, void (*freefunc)(void*));

void sal_message_add_route(osip_message_t *msg, const char *proxy);

#endif
