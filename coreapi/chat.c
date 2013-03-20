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
#include "lpconfig.h"

/**
 * @addtogroup chatroom
 * @{
 */

/**
 * Create a new chat room for messaging from a sip uri like sip:joe@sip.linphone.org
 * @param lc #LinphoneCore object
 * @param to destination address for messages
 * @return #LinphoneChatRoom where messaging can take place.
 */
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
 
/**
 * Destroy a LinphoneChatRoom.
 * @param cr #LinphoneChatRoom object
 */
void linphone_chat_room_destroy(LinphoneChatRoom *cr){
	LinphoneCore *lc=cr->lc;
	lc->chatrooms=ms_list_remove(lc->chatrooms,(void *) cr);
	linphone_address_destroy(cr->peer_url);
	ms_free(cr->peer);
}



static void _linphone_chat_room_send_message(LinphoneChatRoom *cr, LinphoneChatMessage* msg){
	const char *route=NULL;
	const char *identity=linphone_core_find_best_identity(cr->lc,cr->peer_url,&route);
	SalOp *op=NULL;
	LinphoneCall *call;
	char* content_type;
	time_t t=time(NULL);
	
	if (lp_config_get_int(cr->lc->config,"sip","chat_use_call_dialogs",0)){
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
	}
	msg->time=t;
	if (op==NULL){
		/*sending out of calls*/
		op = sal_op_new(cr->lc->sal);
		sal_op_set_route(op,route);
		sal_op_set_user_pointer(op, msg); /*if out of call, directly store msg*/
		if (msg->custom_headers){
			sal_op_set_custom_header(op,msg->custom_headers);
			msg->custom_headers=NULL; /*transfered to the SalOp*/
		}
	}
	if (msg->external_body_url) {
		content_type=ms_strdup_printf("message/external-body; access-type=URL; URL=\"%s\"",msg->external_body_url);
		sal_message_send(op,identity,cr->peer,content_type, NULL);
		ms_free(content_type);
	} else {
		sal_text_send(op, identity, cr->peer,msg->message);
	}
	msg->dir=LinphoneChatMessageOutgoing;
	msg->from=linphone_address_new(identity);
	linphone_chat_message_store(msg);
}

/**
 * Send a message to peer member of this chat room.
 * @deprecated linphone_chat_room_send_message2() gives more control on the message expedition.
 * @param cr #LinphoneChatRoom object
 * @param msg message to be sent
 */
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

/**
 * Retrieve an existing chat room whose peer is the supplied address, if exists.
 * @param lc the linphone core
 * @param add a linphone address.
 * @returns the matching chatroom, or NULL if no such chatroom exists.
**/
LinphoneChatRoom *linphone_core_get_chat_room(LinphoneCore *lc, const LinphoneAddress *addr){
	LinphoneChatRoom *cr=NULL;
	MSList *elem;
	for(elem=lc->chatrooms;elem!=NULL;elem=ms_list_next(elem)){
		cr=(LinphoneChatRoom*)elem->data;
		if (linphone_chat_room_matches(cr,addr)){
			break;
		}
		cr=NULL;
	}
	return cr;
}

void linphone_core_message_received(LinphoneCore *lc, SalOp *op, const SalMessage *sal_msg){
	
	LinphoneChatRoom *cr=NULL;
	LinphoneAddress *addr;
	char *cleanfrom;
	char *from;
	LinphoneChatMessage* msg;
	const SalCustomHeader *ch;
	
	addr=linphone_address_new(sal_msg->from);
	linphone_address_clean(addr);
	cr=linphone_core_get_chat_room(lc,addr);
	cleanfrom=linphone_address_as_string(addr);
	from=linphone_address_as_string_uri_only(addr);
	if (cr==NULL){
		/* create a new chat room */
		cr=linphone_core_create_chat_room(lc,cleanfrom);
	}
	msg = linphone_chat_room_create_message(cr, sal_msg->text);
	linphone_chat_message_set_from(msg, cr->peer_url);
	
	{
		LinphoneAddress *to;
		to=sal_op_get_to(op) ? linphone_address_new(sal_op_get_to(op)) : linphone_address_new(linphone_core_get_identity(lc));
		msg->to=to;
	}
	
	msg->time=sal_msg->time;
	msg->state=LinphoneChatMessageStateDelivered;
	msg->is_read=FALSE;
	ch=sal_op_get_custom_header(op);
	if (ch) msg->custom_headers=sal_custom_header_clone(ch);
	
	if (sal_msg->url) {
		linphone_chat_message_set_external_body_url(msg, sal_msg->url);
	}
	linphone_address_destroy(addr);
	linphone_chat_message_store(msg);
	linphone_chat_room_message_received(cr,lc,msg);
	ms_free(cleanfrom);
	ms_free(from);
}

/**
 * Returns back pointer to LinphoneCore object.
**/
LinphoneCore* linphone_chat_room_get_lc(LinphoneChatRoom *cr){
	return cr->lc;
}

/**
 * Assign a user pointer to the chat room.
**/
void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void * ud){
	cr->user_data=ud;
}

/**
 * Retrieve the user pointer associated with the chat room.
**/
void * linphone_chat_room_get_user_data(LinphoneChatRoom *cr){
	return cr->user_data;
}

/**
 * get peer address \link linphone_core_create_chat_room() associated to \endlink this #LinphoneChatRoom
 * @param cr #LinphoneChatRoom object
 * @return #LinphoneAddress peer address
 */
const LinphoneAddress* linphone_chat_room_get_peer_address(LinphoneChatRoom *cr) {
	return cr->peer_url;
}

/**
 * Create a message attached to a dedicated chat room;
 * @param cr the chat room.
 * @param message text message, NULL if absent.
 * @return a new #LinphoneChatMessage
 */
LinphoneChatMessage* linphone_chat_room_create_message(LinphoneChatRoom *cr, const char* message) {
	LinphoneChatMessage* msg = ms_new0(LinphoneChatMessage,1);
	msg->chat_room=(LinphoneChatRoom*)cr;
	msg->message=message?ms_strdup(message):NULL;
	msg->is_read=TRUE;
	return msg;
}

/**
 * Send a message to peer member of this chat room.
 * @param cr #LinphoneChatRoom object
 * @param msg #LinphoneChatMessage message to be sent
 * @param status_cb LinphoneChatMessageStateChangeCb status callback invoked when message is delivered or could not be delivered. May be NULL
 * @param ud user data for the status cb.
 * @note The LinphoneChatMessage must not be destroyed until the the callback is called.
 */
void linphone_chat_room_send_message2(LinphoneChatRoom *cr, LinphoneChatMessage* msg,LinphoneChatMessageStateChangeCb status_cb, void* ud) {
	msg->cb=status_cb;
	msg->cb_ud=ud;
	msg->state=LinphoneChatMessageStateInProgress;
	_linphone_chat_room_send_message(cr, msg);
}

/**
 * Returns a #LinphoneChatMessageState as a string.
 */
const char* linphone_chat_message_state_to_string(const LinphoneChatMessageState state) {
	switch (state) {
		case LinphoneChatMessageStateIdle:return "LinphoneChatMessageStateIdle"; 
		case LinphoneChatMessageStateInProgress:return "LinphoneChatMessageStateInProgress";
		case LinphoneChatMessageStateDelivered:return "LinphoneChatMessageStateDelivered";
		case  LinphoneChatMessageStateNotDelivered:return "LinphoneChatMessageStateNotDelivered";
		default: return "Unknown state";
	}
	
}

/**
 * Returns the chatroom this message belongs to.
**/
LinphoneChatRoom* linphone_chat_message_get_chat_room(LinphoneChatMessage *msg){
	return msg->chat_room;
}

/**
 * Returns the peer (remote) address for the message.
**/
const LinphoneAddress* linphone_chat_message_get_peer_address(LinphoneChatMessage *msg) {
	return linphone_chat_room_get_peer_address(msg->chat_room);
}

/**
 *User pointer set function
 */
void linphone_chat_message_set_user_data(LinphoneChatMessage* message,void* ud) {
	message->message_userdata=ud;
}

/**
 * User pointer get function
 */
void* linphone_chat_message_get_user_data(const LinphoneChatMessage* message) {
	return message->message_userdata;
}

/**
 * Linphone message can carry external body as defined by rfc2017
 * @param message #LinphoneChatMessage
 * @return external body url or NULL if not present.
 */
const char* linphone_chat_message_get_external_body_url(const LinphoneChatMessage* message) {
	return message->external_body_url;
}

/**
 * Linphone message can carry external body as defined by rfc2017
 * 
 * @param message a LinphoneChatMessage  
 * @param url ex: access-type=URL; URL="http://www.foo.com/file"
 */
void linphone_chat_message_set_external_body_url(LinphoneChatMessage* message,const char* url) {
	if (message->external_body_url) {
		ms_free(message->external_body_url);
	}
	message->external_body_url=url?ms_strdup(url):NULL;
}

/**
 * Set origin of the message
 *@param message #LinphoneChatMessage obj
 *@param from #LinphoneAddress origin of this message (copied)
 */
void linphone_chat_message_set_from(LinphoneChatMessage* message, const LinphoneAddress* from) {
	if(message->from) linphone_address_destroy(message->from);
	message->from=linphone_address_clone(from);
}

/**
 * Get origin of the message 
 *@param message #LinphoneChatMessage obj
 *@return #LinphoneAddress
 */
const LinphoneAddress* linphone_chat_message_get_from(const LinphoneChatMessage* message) {
	return message->from;
}

/**
 * Get destination of the message 
 *@param message #LinphoneChatMessage obj
 *@return #LinphoneAddress
 */
const LinphoneAddress* linphone_chat_message_get_to(const LinphoneChatMessage* message){
	if (message->to) return message->to;
	if (message->dir==LinphoneChatMessageOutgoing){
		return message->chat_room->peer_url;
	}
	return NULL;
}

/**
 * Returns the origin address of a message if it was a outgoing message, or the destination address if it was an incoming message.
 *@param message #LinphoneChatMessage obj
 *@return #LinphoneAddress
 */
LinphoneAddress *linphone_chat_message_get_local_address(const LinphoneChatMessage* message){
	return message->dir==LinphoneChatMessageOutgoing ? message->from : message->to;
}

/**
 * Get the time the message was sent.
 */
time_t linphone_chat_message_get_time(const LinphoneChatMessage* message) {
	return message->time;
}

/**
 * Get the state of the message
 *@param message #LinphoneChatMessage obj
 *@return #LinphoneChatMessageState
 */
LinphoneChatMessageState linphone_chat_message_get_state(const LinphoneChatMessage* message) {
	return message->state;
}

/**
 * Get text part of this message
 * @return text or NULL if no text.
 */
const char * linphone_chat_message_get_text(const LinphoneChatMessage* message) {
	return message->message;
}

/**
 * Add custom headers to the message.
 * @param message the message
 * @param header_name name of the header_name
 * @param header_value header value
**/
void linphone_chat_message_add_custom_header(LinphoneChatMessage* message, const char *header_name, const char *header_value){
	message->custom_headers=sal_custom_header_append(message->custom_headers,header_name,header_value);
}

/**
 * Retrieve a custom header value given its name.
 * @param message the message
 * @param header_name header name searched
**/
const char * linphone_chat_message_get_custom_header(LinphoneChatMessage* message, const char *header_name){
	return sal_custom_header_find(message->custom_headers,header_name);
}

/**
 * Duplicate a LinphoneChatMessage
**/
LinphoneChatMessage* linphone_chat_message_clone(const LinphoneChatMessage* msg) {
	/*struct _LinphoneChatMessage {
	 char* message;
	 LinphoneChatRoom* chat_room;
	 LinphoneChatMessageStateChangeCb cb;
	 void* cb_ud;
	 void* message_userdata;
	 char* external_body_url;
	 LinphoneAddress* from;
	 time_t time;
	 SalCustomHeader *custom_headers;
	 LinphoneChatMessageState state;
	 };*/
	LinphoneChatMessage* new_message = linphone_chat_room_create_message(msg->chat_room,msg->message);
	if (msg->external_body_url) new_message->external_body_url=ms_strdup(msg->external_body_url);
	new_message->cb=msg->cb;
	new_message->cb_ud=msg->cb_ud;
	new_message->message_userdata=msg->message_userdata;
	new_message->cb=msg->cb;
	new_message->time=msg->time;
	new_message->state=msg->state;
	if (msg->from) new_message->from=linphone_address_clone(msg->from);
	return new_message;
}

/**
 * Destroys a LinphoneChatMessage.
**/
void linphone_chat_message_destroy(LinphoneChatMessage* msg) {
	if (msg->message) ms_free(msg->message);
	if (msg->external_body_url) ms_free(msg->external_body_url);
	if (msg->from) linphone_address_destroy(msg->from);
	if (msg->to) linphone_address_destroy(msg->to);
	if (msg->custom_headers) sal_custom_header_free(msg->custom_headers);
	ms_free(msg);
}


/**
 * @}
 */


