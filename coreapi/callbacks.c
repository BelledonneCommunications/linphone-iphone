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


#include "sal.h"

#include "linphonecore.h"
#include "private.h"

static void call_received(SalOp *h){
}

static void call_ringing(SalOp *h){
}

static void call_accepted(SalOp *h){
}

static void call_ack(SalOp *h){
}

static void call_updated(SalOp *){
}

static void call_terminated(SalOp *h){
}

static void call_failure(SalOp *h, SalError error, SalReason reason, const char *details){
}

static void auth_requested(SalOp *h, const char *realm, const char *username){
}

static void auth_success(SalOp *h, const char *realm, const char *username){
}

static void register_success(SalOp *op, bool_t registered){
}

static void register_failure(SalOp *op, SalError error, SalReason reason, const char *details){
}

static void vfu_request(SalOp *op){
}

static void dtmf_received(SalOp *op, char dtmf){
}

static void refer_received(SalOp *op, SalOp *op, const char *referto){
}

static void text_received(Sal *sal, const char *from, const char *msg){
}

static void presence_changed(SalOp *op, SalPresenceStatus status, const char *msg){
}

static void subscribe_received(SalOp *op, const char *from){
}

static void internal_message(SalOp *op, const char *msg){
}



SalCallbacks linphone_sal_callbacks={
	call_received,
	call_ringing,
	call_accepted,
	call_ack,
	call_updated,
	call_terminated,
	call_failure,
	auth_requested,
	auth_success,
	register_success,
	register_failure,
	vfu_request,
	dtmf_received,
	refer_received,
	text_received,
	presence_changed,
	subscribe_received,
	internal_message
};


