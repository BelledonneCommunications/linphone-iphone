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


int sal_send_info(SalOp *op, const char *from, const char *to, const SalBody *body){
	if (op->dialog){
		belle_sip_request_t *req=belle_sip_dialog_create_queued_request(op->dialog,"INFO");
		sal_op_add_body(op,(belle_sip_message_t*)req,body);
		return sal_op_send_request(op,req);
	}
	return -1;
}

