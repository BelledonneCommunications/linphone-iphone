/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)
Copyright (C) 2010  Belledonne Communications SARL

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

#include "private.h"


LinphoneCoreVTable *linphone_core_v_table_new() {
	return ms_new0(LinphoneCoreVTable,1);
}

void linphone_core_v_table_set_user_data(LinphoneCoreVTable *table, void *data) {
	table->user_data = data;
}

void* linphone_core_v_table_get_user_data(LinphoneCoreVTable *table) {
	return table->user_data;
}

void linphone_core_v_table_destroy(LinphoneCoreVTable* table) {
	ms_free(table);
}

LinphoneCoreVTable *linphone_core_get_current_vtable(LinphoneCore *lc) {
	return lc->current_vtable;
}

static void cleanup_dead_vtable_refs(LinphoneCore *lc){
	MSList *it,*next_it;
	for(it=lc->vtable_refs; it!=NULL; ){
		VTableReference *ref=(VTableReference*)it->data;
		next_it=it->next;
		if (ref->valid==0){
			ref->valid=0;
			lc->vtable_refs=ms_list_remove_link(lc->vtable_refs, it);
			ms_free(ref);
		}
		it=next_it;
	}
}

#define NOTIFY_IF_EXIST(function_name, ...) \
	MSList* iterator; \
	VTableReference *ref; \
	bool_t has_cb = FALSE; \
	for (iterator=lc->vtable_refs; iterator!=NULL; iterator=iterator->next)\
		if ((ref=(VTableReference*)iterator->data)->valid && (lc->current_vtable=ref->vtable)->function_name) {\
			lc->current_vtable->function_name(__VA_ARGS__);\
			has_cb = TRUE;\
		}\
	if (has_cb) ms_message("Linphone core [%p] notifying [%s]",lc,#function_name)

#define NOTIFY_IF_EXIST_INTERNAL(function_name, internal_val, ...) \
	MSList* iterator; \
	VTableReference *ref; \
	bool_t has_cb = FALSE; \
	for (iterator=lc->vtable_refs; iterator!=NULL; iterator=iterator->next)\
		if ((ref=(VTableReference*)iterator->data)->valid && (lc->current_vtable=ref->vtable)->function_name && (ref->internal == internal_val)) {\
			lc->current_vtable->function_name(__VA_ARGS__);\
			has_cb = TRUE;\
		}\
	if (has_cb) ms_message("Linphone core [%p] notifying [%s]",lc,#function_name)

void linphone_core_notify_global_state_changed(LinphoneCore *lc, LinphoneGlobalState gstate, const char *message) {
	NOTIFY_IF_EXIST(global_state_changed,lc,gstate,message);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *message){
	NOTIFY_IF_EXIST(call_state_changed, lc,call,cstate,message);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_call_encryption_changed(LinphoneCore *lc, LinphoneCall *call, bool_t on, const char *authentication_token) {
	NOTIFY_IF_EXIST(call_encryption_changed, lc,call,on,authentication_token);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message){
	NOTIFY_IF_EXIST(registration_state_changed, lc,cfg,cstate,message);
	cleanup_dead_vtable_refs(lc);
}
#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void linphone_core_notify_show_interface(LinphoneCore *lc){
	NOTIFY_IF_EXIST(show, lc);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_display_status(LinphoneCore *lc, const char *message) {
	NOTIFY_IF_EXIST(display_status, lc,message);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_display_message(LinphoneCore *lc, const char *message){
	NOTIFY_IF_EXIST(display_message, lc,message);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_display_warning(LinphoneCore *lc, const char *message){
	NOTIFY_IF_EXIST(display_warning, lc,message);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_display_url(LinphoneCore *lc, const char *message, const char *url){
	NOTIFY_IF_EXIST(display_url, lc,message,url);
	cleanup_dead_vtable_refs(lc);
}
#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif
void linphone_core_notify_notify_presence_received(LinphoneCore *lc, LinphoneFriend * lf){
	NOTIFY_IF_EXIST(notify_presence_received, lc,lf);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_new_subscription_requested(LinphoneCore *lc, LinphoneFriend *lf, const char *url){
	NOTIFY_IF_EXIST(new_subscription_requested, lc,lf,url);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain){
	NOTIFY_IF_EXIST(auth_info_requested, lc,realm,username,domain);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_call_log_updated(LinphoneCore *lc, LinphoneCallLog *newcl){
	NOTIFY_IF_EXIST(call_log_updated, lc,newcl);
	cleanup_dead_vtable_refs(lc);
}
#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

void linphone_core_notify_text_message_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message){
	NOTIFY_IF_EXIST(text_received, lc,room,from,message);
	cleanup_dead_vtable_refs(lc);
}
#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif

void linphone_core_notify_message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *message){
	NOTIFY_IF_EXIST(message_received, lc,room,message);
	cleanup_dead_vtable_refs(lc);
}
#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void linphone_core_notify_file_transfer_recv(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, const char* buff, size_t size) {
	NOTIFY_IF_EXIST(file_transfer_recv, lc,message,content,buff,size);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_file_transfer_send(LinphoneCore *lc, LinphoneChatMessage *message,  const LinphoneContent* content, char* buff, size_t* size) {
	NOTIFY_IF_EXIST(file_transfer_send, lc,message,content,buff,size);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_file_transfer_progress_indication(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t total) {
	NOTIFY_IF_EXIST(file_transfer_progress_indication, lc,message,content,offset,total);
	cleanup_dead_vtable_refs(lc);
}
#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif
void linphone_core_notify_is_composing_received(LinphoneCore *lc, LinphoneChatRoom *room) {
	NOTIFY_IF_EXIST(is_composing_received, lc,room);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_dtmf_received(LinphoneCore* lc, LinphoneCall *call, int dtmf) {
	NOTIFY_IF_EXIST(dtmf_received, lc,call,dtmf);
	cleanup_dead_vtable_refs(lc);
}

bool_t linphone_core_dtmf_received_has_listener(const LinphoneCore* lc) {
	MSList* iterator;
	for (iterator=lc->vtable_refs; iterator!=NULL; iterator=iterator->next){
		VTableReference *ref=(VTableReference*)iterator->data;
		if (ref->valid && ref->vtable->dtmf_received)
			return TRUE;
	}
	return FALSE;
}

void linphone_core_notify_refer_received(LinphoneCore *lc, const char *refer_to) {
	NOTIFY_IF_EXIST(refer_received, lc,refer_to);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_buddy_info_updated(LinphoneCore *lc, LinphoneFriend *lf) {
	NOTIFY_IF_EXIST(buddy_info_updated, lc,lf);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_transfer_state_changed(LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state) {
	NOTIFY_IF_EXIST(transfer_state_changed, lc,transfered,new_call_state);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_call_stats_updated(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *stats) {
	NOTIFY_IF_EXIST(call_stats_updated, lc,call,stats);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_info_received(LinphoneCore *lc, LinphoneCall *call, const LinphoneInfoMessage *msg) {
	NOTIFY_IF_EXIST(info_received, lc,call,msg);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_configuring_status(LinphoneCore *lc, LinphoneConfiguringState status, const char *message) {
	NOTIFY_IF_EXIST(configuring_status, lc,status,message);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_network_reachable(LinphoneCore *lc, bool_t reachable) {
	NOTIFY_IF_EXIST(network_reachable, lc,reachable);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_notify_received(LinphoneCore *lc, LinphoneEvent *lev, const char *notified_event, const LinphoneContent *body) {
	NOTIFY_IF_EXIST_INTERNAL(notify_received, linphone_event_is_internal(lev), lc, lev, notified_event, body);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_subscription_state_changed(LinphoneCore *lc, LinphoneEvent *lev, LinphoneSubscriptionState state) {
	NOTIFY_IF_EXIST_INTERNAL(subscription_state_changed,linphone_event_is_internal(lev), lc,lev,state);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_publish_state_changed(LinphoneCore *lc, LinphoneEvent *lev, LinphonePublishState state) {
	NOTIFY_IF_EXIST_INTERNAL(publish_state_changed, linphone_event_is_internal(lev), lc, lev, state);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_log_collection_upload_state_changed(LinphoneCore *lc, LinphoneCoreLogCollectionUploadState state, const char *info) {
	NOTIFY_IF_EXIST(log_collection_upload_state_changed, lc, state, info);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_log_collection_upload_progress_indication(LinphoneCore *lc, size_t offset, size_t total) {
	NOTIFY_IF_EXIST(log_collection_upload_progress_indication, lc, offset, total);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_friend_list_created(LinphoneCore *lc, LinphoneFriendList *list) {
	NOTIFY_IF_EXIST(friend_list_created, lc, list);
	cleanup_dead_vtable_refs(lc);
}

void linphone_core_notify_friend_list_removed(LinphoneCore *lc, LinphoneFriendList *list) {
	NOTIFY_IF_EXIST(friend_list_removed, lc, list);
	cleanup_dead_vtable_refs(lc);
}

static VTableReference * v_table_reference_new(LinphoneCoreVTable *vtable, bool_t autorelease, bool_t internal){
	VTableReference *ref=ms_new0(VTableReference,1);
	ref->valid=1;
	ref->autorelease=autorelease;
	ref->internal = internal;
	ref->vtable=vtable;
	return ref;
}

void v_table_reference_destroy(VTableReference *ref){
	if (ref->autorelease) linphone_core_v_table_destroy(ref->vtable);
	ms_free(ref);
}

void _linphone_core_add_listener(LinphoneCore *lc, LinphoneCoreVTable *vtable, bool_t autorelease, bool_t internal) {
	ms_message("Vtable [%p] registered on core [%p]",lc,vtable);
	lc->vtable_refs=ms_list_append(lc->vtable_refs,v_table_reference_new(vtable, autorelease, internal));
}

void linphone_core_add_listener(LinphoneCore *lc, LinphoneCoreVTable *vtable){
	_linphone_core_add_listener(lc, vtable, FALSE, FALSE);
}

void linphone_core_remove_listener(LinphoneCore *lc, const LinphoneCoreVTable *vtable) {
	MSList *it;
	ms_message("Vtable [%p] unregistered on core [%p]",lc,vtable);
	for(it=lc->vtable_refs; it!=NULL; it=it->next){
		VTableReference *ref=(VTableReference*)it->data;
		if (ref->vtable==vtable)
			ref->valid=0;
	}
}
