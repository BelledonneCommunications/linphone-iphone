/***************************************************************************
 *            chat.c
 *
 *  Sun Jun  5 19:34:18 2005
 *  Copyright  2005  Simon Morlat
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
 #include <eXosip2/eXosip.h>
 
 LinphoneChatRoom * linphone_core_create_chat_room(LinphoneCore *lc, const char *to){
	LinphoneAddress *parsed_url=NULL;
	char *route;
	if (linphone_core_interpret_url(lc,to,&parsed_url,&route)){
		LinphoneChatRoom *cr=ms_new0(LinphoneChatRoom,1);
		cr->lc=lc;
		cr->peer=linphone_address_as_string(parsed_url);
		cr->peer_url=parsed_url;
		cr->route=route;
		lc->chatrooms=ms_list_append(lc->chatrooms,(void *)cr);
		return cr;
	}
	return NULL;
 }
 
 
 void linphone_chat_room_destroy(LinphoneChatRoom *cr){
	LinphoneCore *lc=cr->lc;
	lc->chatrooms=ms_list_remove(lc->chatrooms,(void *) cr);
	linphone_address_destroy(cr->peer_url);
	ms_free(cr->peer);
	ms_free(cr->route);
 }
 
void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg){
	const char *identity=linphone_core_get_identity(cr->lc);
	osip_message_t *sip=NULL;
	eXosip_message_build_request(&sip,"MESSAGE",cr->peer,identity,cr->route);
	osip_message_set_content_type(sip,"text/plain");
	osip_message_set_body(sip,msg,strlen(msg));
	eXosip_message_send_request(sip);
}

bool_t linphone_chat_room_matches(LinphoneChatRoom *cr, const LinphoneAddress *from){
	if (linphone_address_get_username(cr->peer_url) && linphone_address_get_username(from) && 
		strcmp(linphone_address_get_username(cr->peer_url),linphone_address_get_username(from))==0) return TRUE;
	return FALSE;
}

void linphone_chat_room_text_received(LinphoneChatRoom *cr, LinphoneCore *lc, const char *from, const char *msg){
	if (lc->vtable.text_received!=NULL) lc->vtable.text_received(lc, cr, from, msg);
}

void linphone_core_text_received(LinphoneCore *lc, eXosip_event_t *ev){
	MSList *elem;
	const char *msg;
	LinphoneChatRoom *cr=NULL;
	char *from;
	osip_from_t *from_url=ev->request->from;
	osip_body_t *body=NULL;
	LinphoneAddress *uri;

	osip_message_get_body(ev->request,0,&body);
	if (body==NULL){
		ms_error("Could not get text message from SIP body");
		return;
	}
	msg=body->body;
	osip_from_to_str(from_url,&from);
	uri=linphone_address_new(from);
	osip_free(from);
	linphone_address_clean(uri);
	for(elem=lc->chatrooms;elem!=NULL;elem=ms_list_next(elem)){
		cr=(LinphoneChatRoom*)elem->data;
		if (linphone_chat_room_matches(cr,uri)){
			break;
		}
		cr=NULL;
	}
	from=linphone_address_as_string(uri);
	if (cr==NULL){
		/* create a new chat room */
		cr=linphone_core_create_chat_room(lc,from);
	}
	linphone_address_destroy(uri);
	linphone_chat_room_text_received(cr,lc,from,msg);
	ms_free(from);
}


void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void * ud){
	cr->user_data=ud;
}
void * linphone_chat_room_get_user_data(LinphoneChatRoom *cr){
	return cr->user_data;
}
