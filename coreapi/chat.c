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
 
 LinphoneChatRoom * linphone_core_create_chat_room(LinphoneCore *lc, const char *to){
	LinphoneAddress *parsed_url=NULL;

	if ((parsed_url=linphone_core_interpret_url(lc,to))!=NULL){
		LinphoneChatRoom *cr=ms_new0(LinphoneChatRoom,1);
		cr->lc=lc;
		cr->peer=linphone_address_as_string(parsed_url);
		cr->peer_url=parsed_url;
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
	if (cr->op)
		 sal_op_release(cr->op);
 }


static void _linphone_chat_room_send_message(LinphoneChatRoom *cr, LinphoneChatMessage* msg){
	const char *route=NULL;
	const char *identity=linphone_core_find_best_identity(cr->lc,cr->peer_url,&route);
	SalOp *op=NULL;
	LinphoneCall *call;
	char* content_type;
	if((call = linphone_core_get_call_by_remote_address(cr->lc,cr->peer))!=NULL){
		if (call->state==LinphoneCallConnected ||
		    call->state==LinphoneCallStreamsRunning ||
		    call->state==LinphoneCallPaused ||
		    call->state==LinphoneCallPausing ||
		    call->state==LinphoneCallPausedByRemote){
			ms_message("send SIP message through the existing call.");
			op = call->op;
			call->pending_message=msg;
		}
	}
	if (op==NULL){
		/*sending out of calls*/
		op = sal_op_new(cr->lc->sal);
		sal_op_set_route(op,route);
		if (cr->op!=NULL){
			sal_op_release (cr->op);
			cr->op=NULL;
		}
		cr->op=op;
		sal_op_set_user_pointer(op, msg); /*if out of call, directly store msg*/
	}
	if (msg->external_body_url) {
		content_type=ms_strdup_printf("message/external-body; access-type=URL; URL=\"%s\"",msg->external_body_url);
		sal_message_send(op,identity,cr->peer,content_type,NULL);
		ms_free(content_type);
	} else {
		sal_text_send(op, identity, cr->peer, msg->message);
	}
	
	
}

void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg) {
	_linphone_chat_room_send_message(cr,linphone_chat_room_create_message(cr,msg));
}
bool_t linphone_chat_room_matches(LinphoneChatRoom *cr, const LinphoneAddress *from){
	if (linphone_address_get_username(cr->peer_url) && linphone_address_get_username(from) && 
		strcmp(linphone_address_get_username(cr->peer_url),linphone_address_get_username(from))==0) return TRUE;
	return FALSE;
}

void linphone_chat_room_message_received(LinphoneChatRoom *cr, LinphoneCore *lc, LinphoneChatMessage *msg){
	if (msg->message)
		//legacy API
		if (lc->vtable.text_received!=NULL) lc->vtable.text_received(lc, cr, msg->from, msg->message);
	if (lc->vtable.message_received!=NULL) lc->vtable.message_received(lc, cr,msg);
	
}

void linphone_core_message_received(LinphoneCore *lc, const char *from, const char *raw_msg,const char* external_url){
	MSList *elem;
	LinphoneChatRoom *cr=NULL;
	LinphoneAddress *addr;
	char *cleanfrom;
	LinphoneChatMessage* msg;
	addr=linphone_address_new(from);
	linphone_address_clean(addr);
	for(elem=lc->chatrooms;elem!=NULL;elem=ms_list_next(elem)){
		cr=(LinphoneChatRoom*)elem->data;
		if (linphone_chat_room_matches(cr,addr)){
			break;
		}
		cr=NULL;
	}
	cleanfrom=linphone_address_as_string(addr);
	if (cr==NULL){
		/* create a new chat room */
		cr=linphone_core_create_chat_room(lc,cleanfrom);
	}
	msg = linphone_chat_room_create_message(cr, raw_msg);
	linphone_chat_message_set_from(msg, cr->peer_url);
	if (external_url) {
		linphone_chat_message_set_external_body_url(msg, external_url);
	}
	linphone_address_destroy(addr);
	linphone_chat_room_message_received(cr,lc,msg);
	ms_free(cleanfrom);
}

LinphoneCore* linphone_chat_room_get_lc(LinphoneChatRoom *cr){
	return cr->lc;
}

LinphoneChatRoom* linphone_chat_message_get_chat_room(LinphoneChatMessage *msg){
	return msg->chat_room;
}

void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void * ud){
	cr->user_data=ud;
}
void * linphone_chat_room_get_user_data(LinphoneChatRoom *cr){
	return cr->user_data;
}
const LinphoneAddress* linphone_chat_room_get_peer_address(LinphoneChatRoom *cr) {
	return cr->peer_url;
}

LinphoneChatMessage* linphone_chat_room_create_message(const LinphoneChatRoom *cr,const char* message) {
	LinphoneChatMessage* msg = ms_new0(LinphoneChatMessage,1);
	msg->chat_room=(LinphoneChatRoom*)cr;
	msg->message=message?ms_strdup(message):NULL;
	return msg;
}

void linphone_chat_message_destroy(LinphoneChatMessage* msg) {
	if (msg->message) ms_free(msg->message);
	if (msg->external_body_url) ms_free(msg->external_body_url);
	if (msg->from) linphone_address_destroy(msg->from);
	ms_free(msg);
}

void linphone_chat_room_send_message2(LinphoneChatRoom *cr, LinphoneChatMessage* msg,LinphoneChatMessageStateChangeCb status_cb,void* ud) {
	msg->cb=status_cb;
	msg->cb_ud=ud;
	_linphone_chat_room_send_message(cr, msg);
}

const char* linphone_chat_message_state_to_string(const LinphoneChatMessageState state) {
	switch (state) {
		case LinphoneChatMessageStateIdle:return "LinphoneChatMessageStateIdle"; 
		case LinphoneChatMessageStateInProgress:return "LinphoneChatMessageStateInProgress";
		case LinphoneChatMessageStateDelivered:return "LinphoneChatMessageStateDelivered";
		case  LinphoneChatMessageStateNotDelivered:return "LinphoneChatMessageStateNotDelivered";
		default: return "Unknown state";
	}
	
}

char* linphone_chat_message_get_message(LinphoneChatMessage* msg) {
	return msg->message;
}

const LinphoneAddress* linphone_chat_message_get_peer_address(LinphoneChatMessage *msg) {
	return linphone_chat_room_get_peer_address(msg->chat_room);
}

/**
 * user pointer set function
 */
void linphone_chat_message_set_user_data(LinphoneChatMessage* message,void* ud) {
	message->message_userdata=ud;
}
/**
 * user pointer get function
 */
void* linphone_chat_message_get_user_data(const LinphoneChatMessage* message) {
	return message->message_userdata;
}

const char* linphone_chat_message_get_external_body_url(const LinphoneChatMessage* message) {
	return message->external_body_url;
}

void linphone_chat_message_set_external_body_url(LinphoneChatMessage* message,const char* url) {
	if (message->external_body_url) {
		ms_free(message->external_body_url);
	}
	message->external_body_url=url?ms_strdup(url):NULL;
}
void linphone_chat_message_set_from(LinphoneChatMessage* message, const LinphoneAddress* from) {
	if(message->from) linphone_address_destroy(message->from);
	message->from=linphone_address_clone(from);
	
}
LinphoneAddress* linphone_chat_message_get_from(const LinphoneChatMessage* message) {
	return message->from;
}
const char * linphone_chat_message_get_text(const LinphoneChatMessage* message) {
	return message->message;
}
LinphoneChatMessage* linphone_chat_message_clone(const LinphoneChatMessage* msg) {
	/*struct _LinphoneChatMessage {
	 char* message;
	 LinphoneChatRoom* chat_room;
	 LinphoneChatMessageStateChangeCb cb;
	 void* cb_ud;
	 void* message_userdata;
	 char* external_body_url;
	 LinphoneAddress* from;
	 };*/
	LinphoneChatMessage* new_message = linphone_chat_room_create_message(msg->chat_room,msg->message);
	if (msg->external_body_url) new_message->external_body_url=ms_strdup(msg->external_body_url);
	new_message->cb=msg->cb;
	new_message->cb_ud=msg->cb_ud;
	new_message->message_userdata=msg->message_userdata;
	new_message->cb=msg->cb;
	if (msg->from) new_message->from=linphone_address_clone(msg->from);
	return new_message;
}