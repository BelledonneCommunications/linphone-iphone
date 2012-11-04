/*
linphone
Copyright (C) 2012  Belledonne Communications, Grenoble, France

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
#include "sal_impl.h"

static void message_response_event(void *op_base, const belle_sip_response_event_t *event){
	/*nop for futur use*/
}

int sal_text_send(SalOp *op, const char *from, const char *to, const char *text){
	belle_sip_request_t* req;
	size_t content_length = strlen(text);
	if (!op->callbacks.process_response_event)
		op->callbacks.process_response_event=message_response_event;
	if (from)
		sal_op_set_from(op,from);
	if (to)
		sal_op_set_to(op,to);
	req=sal_op_build_request(op,"MESSAGE");
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_content_type_create("text","plain")));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_length)));
	belle_sip_message_set_body(BELLE_SIP_MESSAGE(req),text,content_length);
	return sal_op_send_request(op,req);
}
