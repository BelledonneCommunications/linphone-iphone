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
	LinphoneContent content;
	SalOp *op;
	SalCustomHeader *headers;
};

#define SET_STRING(ptr,field,val) \
	if (ptr->field) { \
		ms_free(ptr->field); \
		ptr->field=NULL; \
	} \
	if (val){ \
		ptr->field=ms_strdup(val); \
	}

static void linphone_content_copy(LinphoneContent *obj, const LinphoneContent *ref){
	SET_STRING(obj,type,ref->type);
	SET_STRING(obj,subtype,ref->subtype);
	if (obj->data) {
		ms_free(obj->data);
		obj->data=NULL;
	}
	if (ref->data){
		obj->data=ms_malloc(ref->size+1);
		memcpy(obj->data, ref->data, ref->size);
		((char*)obj->data)[ref->size]='\0';
	}
	obj->size=ref->size;
}

static void linphone_content_uninit(LinphoneContent * obj){
	if (obj->type) ms_free(obj->type);
	if (obj->subtype) ms_free(obj->subtype);
	if (obj->data) ms_free(obj->data);
}

static LinphoneContent *linphone_content_copy_from_sal_body(LinphoneContent *obj, const SalBody *ref){
	SET_STRING(obj,type,ref->type);
	SET_STRING(obj,subtype,ref->subtype);
	if (obj->data) {
		ms_free(obj->data);
		obj->data=NULL;
	}
	if (ref->data){
		obj->data=ms_malloc(ref->size+1);
		memcpy(obj->data, ref->data, ref->size);
		((char*)obj->data)[ref->size]='\0';
	}
	obj->size=ref->size;
	return obj;
}

static SalBody *sal_body_from_content(SalBody *body, const LinphoneContent *lc){
	if (lc->type){
		body->type=lc->type;
		body->subtype=lc->subtype;
		body->data=lc->data;
		body->size=lc->size;
		return body;
	}
	return NULL;
}

/**
 * Destroy a LinphoneInfoMessage
**/
void linphone_info_message_destroy(LinphoneInfoMessage *im){
	/* FIXME: op is leaked. If we release it now, there is a high risk that the request won't be resent with authentication*/
	/*if (im->op) sal_op_release(im->op);*/
	linphone_content_uninit(&im->content);
	sal_custom_header_free(im->headers);
	ms_free(im);
}

/**
 * Creates an empty info message.
 * @param lc the LinphoneCore object.
 * @return a new LinphoneInfoMessage.
 * 
 * The info message can later be filled with information using linphone_info_message_add_header() or linphone_info_message_set_content(),
 * and finally sent with linphone_core_send_info_message().
**/
LinphoneInfoMessage *linphone_core_create_info_message(LinphoneCore *lc){
	LinphoneInfoMessage *im=ms_new0(LinphoneInfoMessage,1);
	im->op=sal_op_new(lc->sal);
	return im;
}

/**
 * Send a LinphoneInfoMessage to a specified address.
 * @param lc the LinphoneCore
 * @param info the info message
 * @param addr the destination address
**/
int linphone_core_send_info_message(LinphoneCore *lc, const LinphoneInfoMessage *info, const LinphoneAddress *addr){
	SalBody body;
	linphone_configure_op(lc,info->op,addr,info->headers,FALSE);
	return sal_send_info(info->op,NULL, NULL, sal_body_from_content(&body,&info->content));
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
	const SalCustomHeader *ch=sal_op_get_recv_custom_header(im->op);
	return sal_custom_header_find(ch,name);
}

/**
 * Returns origin of received LinphoneInfoMessage 
**/
const char *linphone_info_message_get_from(LinphoneInfoMessage *im){
	return sal_op_get_from(im->op);
}

/**
 * Assign a content to the info message.
 * @param im the linphone info message
 * @param content the content described as a #LinphoneContent structure.
 * All fields of the LinphoneContent are copied, thus the application can destroy/modify/recycloe the content object freely ater the function returns.
**/
void linphone_info_message_set_content(LinphoneInfoMessage *im,  const LinphoneContent *content){
	linphone_content_copy(&im->content,content);
}

/**
 * Returns the info message's content as a #LinphoneContent structure.
**/
const LinphoneContent * linphone_info_message_get_content(const LinphoneInfoMessage *im){
	return im->content.type ? &im->content : NULL;
}

void linphone_core_notify_info_message(LinphoneCore* lc,SalOp *op, const SalBody *body){
	LinphoneInfoMessage *info=ms_new0(LinphoneInfoMessage,1);
	info->op=sal_op_ref(op);
	info->headers=sal_custom_header_clone(sal_op_get_recv_custom_header(op));
	if (body) linphone_content_copy_from_sal_body(&info->content,body);
	if (lc->vtable.info_received)
		lc->vtable.info_received(lc,info);
	linphone_info_message_destroy(info);
}
