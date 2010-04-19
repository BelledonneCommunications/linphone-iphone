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
	MSList *registers;/*MSList of SalOp */
	MSList *out_subscribes;/*MSList of SalOp */
	MSList *in_subscribes;/*MSList of SalOp */
	MSList *pending_auths;/*MSList of SalOp */
	MSList *other_transactions; /*MSList of SalOp */
	int running;
	int session_expires;
	ortp_socket_t sock;
	void *up;
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
	bool_t supports_session_timers;
	bool_t sdp_offering;
	bool_t reinvite;
	bool_t masquerade_via;
	bool_t auto_answer_asked;
};

void sal_remove_out_subscribe(Sal *sal, SalOp *op);
void sal_remove_in_subscribe(Sal *sal, SalOp *op);

void sal_exosip_subscription_recv(Sal *sal, eXosip_event_t *ev);
void sal_exosip_subscription_answered(Sal *sal,eXosip_event_t *ev);
void sal_exosip_notify_recv(Sal *sal,eXosip_event_t *ev);
void sal_exosip_subscription_closed(Sal *sal,eXosip_event_t *ev);

void sal_exosip_in_subscription_closed(Sal *sal, eXosip_event_t *ev);

void sal_exosip_fix_route(SalOp *op);


#endif
