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
	LinphoneContent *content;
	SalCustomHeader *headers;
};


void linphone_info_message_destroy(LinphoneInfoMessage *im){
	if (im->content) linphone_content_unref(im->content);
	if (im->headers) sal_custom_header_free(im->headers);
	ms_free(im);
}


LinphoneInfoMessage *linphone_info_message_copy(const LinphoneInfoMessage *orig){
	LinphoneInfoMessage *im=ms_new0(LinphoneInfoMessage,1);
	if (orig->content) im->content=linphone_content_copy(orig->content);
	if (orig->headers) im->headers=sal_custom_header_clone(orig->headers);
	return im;
}

LinphoneInfoMessage *linphone_core_create_info_message(LinphoneCore *lc){
	LinphoneInfoMessage *im=ms_new0(LinphoneInfoMessage,1);
	return im;
}

int linphone_call_send_info_message(LinphoneCall *call, const LinphoneInfoMessage *info) {
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
		LinphoneInfoMessage *info=ms_new0(LinphoneInfoMessage,1);
		info->headers=sal_custom_header_clone(sal_op_get_recv_custom_header(op));
		if (body_handler) info->content=linphone_content_from_sal_body_handler(body_handler);
		linphone_core_notify_info_received(lc,call,info);
		linphone_info_message_destroy(info);
	}
}
