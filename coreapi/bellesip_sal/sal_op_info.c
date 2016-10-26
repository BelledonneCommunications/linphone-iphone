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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "sal_impl.h"


int sal_send_info(SalOp *op, const char *from, const char *to, const SalBodyHandler *body_handler){
	if (op->dialog){
		belle_sip_request_t *req;
		belle_sip_dialog_enable_pending_trans_checking(op->dialog,op->base.root->pending_trans_checking);
		req=belle_sip_dialog_create_queued_request(op->dialog,"INFO");
		belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req), BELLE_SIP_BODY_HANDLER(body_handler));
		return sal_op_send_request(op,req);
	}
	return -1;
}

