/***************************************************************************
 *            info.c
 *
 *  Thu May  16 11:48:01 2013
 *  Copyright  2013  Belledonne Communications SARL
 *  Author Simon Morlat
 *  Email simon dot morlat at linphone dot org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "linphone/core.h"
#include "private.h"
#include "linphone/lpconfig.h"


struct _LinphoneInfoMessage{
	belle_sip_object_t base;
	LinphoneContent *content;
	SalCustomHeader *headers;
};

static void _linphone_info_message_uninit(LinphoneInfoMessage *im);
static void _linphone_info_message_copy(LinphoneInfoMessage *im, const LinphoneInfoMessage *orig);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneInfoMessage);
BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneInfoMessage);
BELLE_SIP_INSTANCIATE_VPTR(LinphoneInfoMessage, belle_sip_object_t,
	_linphone_info_message_uninit, // uninit
	_linphone_info_message_copy, // clone
	NULL, // marshal
	FALSE
);

static void _linphone_info_message_uninit(LinphoneInfoMessage *im) {
	if (im->content) linphone_content_unref(im->content);
	if (im->headers) sal_custom_header_free(im->headers);
}

LinphoneInfoMessage *linphone_info_message_ref(LinphoneInfoMessage *im) {
	return (LinphoneInfoMessage *)belle_sip_object_ref(im);
}

void linphone_info_message_unref(LinphoneInfoMessage *im) {
	belle_sip_object_unref(im);
}

void linphone_info_message_destroy(LinphoneInfoMessage *im){
	linphone_info_message_unref(im);
}

static void _linphone_info_message_copy(LinphoneInfoMessage *im, const LinphoneInfoMessage *orig) {
	if (orig->content) im->content=linphone_content_copy(orig->content);
	if (orig->headers) im->headers=sal_custom_header_clone(orig->headers);
}

LinphoneInfoMessage *linphone_info_message_copy(const LinphoneInfoMessage *orig){
	return (LinphoneInfoMessage *)belle_sip_object_clone((const belle_sip_object_t *)orig);
}

LinphoneInfoMessage *linphone_core_create_info_message(LinphoneCore *lc){
	return belle_sip_object_new(LinphoneInfoMessage);
}

LinphoneStatus linphone_call_send_info_message(LinphoneCall *call, const LinphoneInfoMessage *info) {
	SalBodyHandler *body_handler = sal_body_handler_from_content(info->content);
	sal_op_set_sent_custom_header(call->op, info->headers);
	return sal_send_info(call->op,NULL, NULL, body_handler);
}

void linphone_info_message_add_header(LinphoneInfoMessage *im, const char *name, const char *value){
	im->headers=sal_custom_header_append(im->headers, name, value);
}

const char *linphone_info_message_get_header(const LinphoneInfoMessage *im, const char *name){
	return sal_custom_header_find(im->headers,name);
}

void linphone_info_message_set_content(LinphoneInfoMessage *im,  const LinphoneContent *content){
	im->content=linphone_content_copy(content);
}

const LinphoneContent * linphone_info_message_get_content(const LinphoneInfoMessage *im){
	return (im->content && linphone_content_get_type(im->content)) ? im->content : NULL;
}

void linphone_core_notify_info_message(LinphoneCore* lc,SalOp *op, SalBodyHandler *body_handler){
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	if (call){
		LinphoneInfoMessage *info=linphone_core_create_info_message(lc);
		info->headers=sal_custom_header_clone(sal_op_get_recv_custom_header(op));
		if (body_handler) info->content=linphone_content_from_sal_body_handler(body_handler);
		linphone_call_notify_info_message_received(call, info);
		linphone_info_message_unref(info);
	}
}
