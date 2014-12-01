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
 
#include "linphonecore.h"
#include "private.h"
#include "lpconfig.h"


struct _LinphoneInfoMessage{
	LinphoneContent *content;
	SalCustomHeader *headers;
};

/**
 * Destroy a LinphoneInfoMessage
**/
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

/**
 * Creates an empty info message.
 * @param lc the LinphoneCore
 * @return a new LinphoneInfoMessage.
 * 
 * The info message can later be filled with information using linphone_info_message_add_header() or linphone_info_message_set_content(),
 * and finally sent with linphone_core_send_info_message().
**/
LinphoneInfoMessage *linphone_core_create_info_message(LinphoneCore *lc){
	LinphoneInfoMessage *im=ms_new0(LinphoneInfoMessage,1);
	return im;
}

/**
 * Send a LinphoneInfoMessage through an established call
 * @param call the call
 * @param info the info message
**/
int linphone_call_send_info_message(LinphoneCall *call, const LinphoneInfoMessage *info){
	SalBody body;
	sal_op_set_sent_custom_header(call->op,info->headers);
	return sal_send_info(call->op,NULL, NULL, sal_body_from_content(&body,info->content));
}

/**
 * Add a header to an info message to be sent.
 * @param im the info message
 * @param name the header'name
 * @param value the header's value
**/
void linphone_info_message_add_header(LinphoneInfoMessage *im, const char *name, const char *value){
	im->headers=sal_custom_header_append(im->headers, name, value);
}

/**
 * Obtain a header value from a received info message.
 * @param im the info message
 * @param name the header'name
 * @return the corresponding header's value, or NULL if not exists.
**/
const char *linphone_info_message_get_header(const LinphoneInfoMessage *im, const char *name){
	return sal_custom_header_find(im->headers,name);
}

/**
 * Assign a content to the info message.
 * @param im the linphone info message
 * @param content the content described as a #LinphoneContent structure.
 * All fields of the LinphoneContent are copied, thus the application can destroy/modify/recycloe the content object freely ater the function returns.
**/
void linphone_info_message_set_content(LinphoneInfoMessage *im,  const LinphoneContent *content){
	im->content=linphone_content_copy(content);
}

/**
 * Returns the info message's content as a #LinphoneContent structure.
**/
const LinphoneContent * linphone_info_message_get_content(const LinphoneInfoMessage *im){
	return (im->content && linphone_content_get_type(im->content)) ? im->content : NULL;
}

void linphone_core_notify_info_message(LinphoneCore* lc,SalOp *op, const SalBody *body){
	LinphoneCall *call=(LinphoneCall*)sal_op_get_user_pointer(op);
	if (call){
		LinphoneInfoMessage *info=ms_new0(LinphoneInfoMessage,1);
		info->headers=sal_custom_header_clone(sal_op_get_recv_custom_header(op));
		if (body) info->content=linphone_content_from_sal_body(body);
		linphone_core_notify_info_received(lc,call,info);
		linphone_info_message_destroy(info);
	}
}
