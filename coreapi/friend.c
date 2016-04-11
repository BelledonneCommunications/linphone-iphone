/***************************************************************************
 *            friend.c
 *
 *  Sat May 15 15:25:16 2004
 *  Copyright  2004-2009  Simon Morlat
 *  Email
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "linphonecore.h"
#include "private.h"
#include "lpconfig.h"

#ifdef FRIENDS_SQL_STORAGE_ENABLED
#ifndef _WIN32
#if !defined(ANDROID) && !defined(__QNXNTO__)
#	include <langinfo.h>
#	include <iconv.h>
#	include <string.h>
#endif
#else
#include <Windows.h>
#endif

#define MAX_PATH_SIZE 1024
#include "sqlite3.h"
#endif

const char *linphone_online_status_to_string(LinphoneOnlineStatus ss){
	const char *str=NULL;
	switch(ss){
		case LinphoneStatusOnline:
		str=_("Online");
		break;
		case LinphoneStatusBusy:
		str=_("Busy");
		break;
		case LinphoneStatusBeRightBack:
		str=_("Be right back");
		break;
		case LinphoneStatusAway:
		str=_("Away");
		break;
		case LinphoneStatusOnThePhone:
		str=_("On the phone");
		break;
		case LinphoneStatusOutToLunch:
		str=_("Out to lunch");
		break;
		case LinphoneStatusDoNotDisturb:
		str=_("Do not disturb");
		break;
		case LinphoneStatusMoved:
		str=_("Moved");
		break;
		case LinphoneStatusAltService:
		str=_("Using another messaging service");
		break;
		case LinphoneStatusOffline:
		str=_("Offline");
		break;
		case LinphoneStatusPending:
		str=_("Pending");
		break;
		case LinphoneStatusVacation:
		str=_("Vacation");
		default:
		str=_("Unknown status");
	}
	return str;
}

static int friend_compare(const void * a, const void * b){
	LinphoneAddress *fa=((LinphoneFriend*)a)->uri;
	LinphoneAddress *fb=((LinphoneFriend*)b)->uri;
	if (linphone_address_weak_equal(fa,fb)) return 0;
	return 1;
}


MSList *linphone_find_friend_by_address(MSList *fl, const LinphoneAddress *addr, LinphoneFriend **lf){
	MSList *res=NULL;
	LinphoneFriend dummy;
	if (lf!=NULL) *lf=NULL;
	dummy.uri=(LinphoneAddress*)addr;
	res=ms_list_find_custom(fl,friend_compare,&dummy);
	if (lf!=NULL && res!=NULL) *lf=(LinphoneFriend*)res->data;
	return res;
}

void __linphone_friend_do_subscribe(LinphoneFriend *fr){
	LinphoneCore *lc=fr->lc;

	if (fr->outsub==NULL){
		/* people for which we don't have yet an answer should appear as offline */
		fr->presence=NULL;
		/*
		if (fr->lc->vtable.notify_recv)
			fr->lc->vtable.notify_recv(fr->lc,(LinphoneFriend*)fr);
		 */
	}else{
		sal_op_release(fr->outsub);
		fr->outsub=NULL;
	}
	fr->outsub=sal_op_new(lc->sal);
	linphone_configure_op(lc,fr->outsub,fr->uri,NULL,TRUE);
	sal_subscribe_presence(fr->outsub,NULL,NULL,lp_config_get_int(lc->config,"sip","subscribe_expires",600));
	fr->subscribe_active=TRUE;
}

LinphoneFriend * linphone_friend_new(void){
	LinphoneFriend *obj = belle_sip_object_new(LinphoneFriend);
	obj->pol = LinphoneSPAccept;
	obj->presence = NULL;
	obj->subscribe = TRUE;
	obj->vcard = NULL;
	obj->storage_id = 0;
	return obj;
}

LinphoneFriend *linphone_friend_new_with_address(const char *addr){
	LinphoneAddress* linphone_address = linphone_address_new(addr);
	LinphoneFriend *fr;

	if (linphone_address == NULL) {
		ms_error("Cannot create friend for address [%s]",addr?addr:"null");
		return NULL;
	}
	fr=linphone_friend_new();
	linphone_friend_set_address(fr,linphone_address);
	linphone_address_unref(linphone_address);
	return fr;
}

void linphone_friend_set_user_data(LinphoneFriend *lf, void *data){
	lf->user_data=data;
}

void* linphone_friend_get_user_data(const LinphoneFriend *lf){
	return lf->user_data;
}

bool_t linphone_friend_in_list(const LinphoneFriend *lf) {
	return lf->friend_list != NULL;
}

void linphone_core_interpret_friend_uri(LinphoneCore *lc, const char *uri, char **result){
	LinphoneAddress *fr=NULL;
	*result=NULL;
	fr=linphone_address_new(uri);
	if (fr==NULL){
		char *tmp=NULL;
		if (strchr(uri,'@')!=NULL){
			LinphoneAddress *u;
			/*try adding sip:*/
			tmp=ms_strdup_printf("sip:%s",uri);
			u=linphone_address_new(tmp);
			if (u!=NULL){
				*result=tmp;
			}
		}else if (lc->default_proxy!=NULL){
			/*try adding domain part from default current proxy*/
			LinphoneAddress * id=linphone_address_new(linphone_core_get_identity(lc));
			if ((id!=NULL) && (uri[0] != '\0')){
				linphone_address_set_display_name(id,NULL);
				linphone_address_set_username(id,uri);
				*result=linphone_address_as_string(id);
				linphone_address_unref(id);
			}
		}
		if (*result){
			/*looks good */
			ms_message("%s interpreted as %s",uri,*result);
		}else{
			ms_warning("Fail to interpret friend uri %s",uri);
		}
	}else {
		*result=linphone_address_as_string(fr);
		linphone_address_unref(fr);
	}
}

const LinphoneAddress *linphone_friend_get_address(const LinphoneFriend *lf){
	return lf->uri;
}

int linphone_friend_set_address(LinphoneFriend *lf, const LinphoneAddress *addr){
	LinphoneAddress *fr = linphone_address_clone(addr);
	LinphoneVcard *vcard = NULL;
	
	linphone_address_clean(fr);
	if (lf->uri != NULL) linphone_address_unref(lf->uri);
	lf->uri = fr;
	
	vcard = linphone_friend_get_vcard(lf);
	if (vcard) {
		linphone_vcard_edit_main_sip_address(vcard, linphone_address_as_string_uri_only(fr));
	}
	
	return 0;
}

void linphone_friend_add_address(LinphoneFriend *lf, const LinphoneAddress *addr) {
	LinphoneVcard *vcard = NULL;
	if (!lf || !addr) {
		return;
	}
	
	vcard = linphone_friend_get_vcard(lf);
	if (!vcard) {
		return;
	}
	
	linphone_vcard_add_sip_address(vcard, linphone_address_as_string_uri_only(addr));
}

MSList* linphone_friend_get_addresses(LinphoneFriend *lf) {
	LinphoneVcard *vcard = NULL;
	MSList *sipAddresses = NULL;
	MSList *addresses = NULL;
	MSList *iterator = NULL;
	
	if (!lf) {
		return NULL;
	}
	
	vcard = linphone_friend_get_vcard(lf);
	if (!vcard) {
		return NULL;
	}
	
	sipAddresses = linphone_vcard_get_sip_addresses(vcard);
	iterator = sipAddresses;
	while (iterator) {
		const char *sipAddress = (const char *)iterator->data;
		LinphoneAddress *addr = linphone_address_new(sipAddress);
		if (addr) {
			addresses = ms_list_append(addresses, addr);
		}
		iterator = ms_list_next(iterator);
	}
	if (sipAddresses) ms_list_free(sipAddresses);
	return addresses;
}

void linphone_friend_remove_address(LinphoneFriend *lf, const LinphoneAddress *addr) {
	LinphoneVcard *vcard = NULL;
	if (!lf || !addr) {
		return;
	}
	
	vcard = linphone_friend_get_vcard(lf);
	if (!vcard) {
		return;
	}
	
	linphone_vcard_remove_sip_address(vcard, linphone_address_as_string_uri_only(addr));
}

void linphone_friend_add_phone_number(LinphoneFriend *lf, const char *phone) {
	LinphoneVcard *vcard = NULL;
	if (!lf || !phone) {
		return;
	}
	
	vcard = linphone_friend_get_vcard(lf);
	if (!vcard) {
		return;
	}
	
	linphone_vcard_add_phone_number(vcard, phone);
}

MSList* linphone_friend_get_phone_numbers(LinphoneFriend *lf) {
	LinphoneVcard *vcard = NULL;
	
	if (!lf) {
		return NULL;
	}
	
	vcard = linphone_friend_get_vcard(lf);
	if (!vcard) {
		return NULL;
	}
	
	return linphone_vcard_get_phone_numbers(vcard);
}

void linphone_friend_remove_phone_number(LinphoneFriend *lf, const char *phone) {
	LinphoneVcard *vcard = NULL;
	if (!lf || !phone) {
		return;
	}
	
	vcard = linphone_friend_get_vcard(lf);
	if (!vcard) {
		return;
	}
	
	linphone_vcard_remove_phone_number(vcard, phone);
}

int linphone_friend_set_name(LinphoneFriend *lf, const char *name){
	LinphoneAddress *fr = lf->uri;
	LinphoneVcard *vcard = NULL;
	bool_t vcard_created = FALSE;
	
	vcard = linphone_friend_get_vcard(lf);
	if (!vcard) {
		linphone_friend_create_vcard(lf, name);
		vcard = linphone_friend_get_vcard(lf);
		vcard_created = TRUE;
	}
	if (vcard) {
		linphone_vcard_set_full_name(vcard, name);
		if (fr && vcard_created) { // SIP address wasn't set yet, let's do it
			linphone_vcard_edit_main_sip_address(vcard, linphone_address_as_string_uri_only(fr));
		}
	}
	
	if (!fr && !vcard) {
		ms_warning("linphone_friend_set_address() must be called before linphone_friend_set_name() to be able to set display name.");
		return -1;
	} else if (fr) {
		linphone_address_set_display_name(fr, name);
	}
	
	return 0;
}

int linphone_friend_enable_subscribes(LinphoneFriend *fr, bool_t val){
	fr->subscribe=val;
	return 0;
}

int linphone_friend_set_inc_subscribe_policy(LinphoneFriend *fr, LinphoneSubscribePolicy pol) {
	fr->pol=pol;
	return 0;
}

void linphone_friend_notify(LinphoneFriend *lf, LinphonePresenceModel *presence){
	MSList *elem;
	if (lf->insubs){
		char *addr=linphone_address_as_string(linphone_friend_get_address(lf));
		ms_message("Want to notify %s",addr);
		ms_free(addr);
	}
	for(elem=lf->insubs; elem!=NULL; elem=elem->next){
		SalOp *op = (SalOp*)elem->data;
		sal_notify_presence(op,(SalPresenceModel *)presence);
	}
}

void linphone_friend_add_incoming_subscription(LinphoneFriend *lf, SalOp *op){
	/*ownership of the op is transfered from sal to the LinphoneFriend*/
	lf->insubs = ms_list_append(lf->insubs, op);
}

void linphone_friend_remove_incoming_subscription(LinphoneFriend *lf, SalOp *op){
	if (ms_list_find(lf->insubs, op)){
		sal_op_release(op);
		lf->insubs = ms_list_remove(lf->insubs, op);
	}

}

static void linphone_friend_unsubscribe(LinphoneFriend *lf){
	if (lf->outsub!=NULL) {
		sal_unsubscribe(lf->outsub);
		lf->subscribe_active=FALSE;
	}
}

void linphone_friend_invalidate_subscription(LinphoneFriend *lf){
	LinphoneCore *lc=lf->lc;

	if (lf->outsub!=NULL) {
		sal_op_release(lf->outsub);
		lf->outsub=NULL;
		lf->subscribe_active=FALSE;
	}

	/* Notify application that we no longer know the presence activity */
	if (lf->presence != NULL) {
		linphone_presence_model_unref(lf->presence);
		lf->presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityOffline,"unknown activity");
		linphone_core_notify_notify_presence_received(lc,lf);
	}

	lf->initial_subscribes_sent=FALSE;
}

void linphone_friend_close_subscriptions(LinphoneFriend *lf){
	linphone_friend_unsubscribe(lf);
	ms_list_for_each(lf->insubs, (MSIterateFunc) sal_notify_presence_close);
	lf->insubs = ms_list_free_with_data(lf->insubs, (MSIterateFunc)sal_op_release);
}

static void _linphone_friend_release_ops(LinphoneFriend *lf){
	lf->insubs = ms_list_free_with_data(lf->insubs, (MSIterateFunc) sal_op_release);
	if (lf->outsub){
		sal_op_release(lf->outsub);
		lf->outsub=NULL;
	}
}

static void _linphone_friend_destroy(LinphoneFriend *lf){
	_linphone_friend_release_ops(lf);
	if (lf->presence != NULL) linphone_presence_model_unref(lf->presence);
	if (lf->uri!=NULL) linphone_address_unref(lf->uri);
	if (lf->info!=NULL) buddy_info_free(lf->info);
	if (lf->vcard != NULL) linphone_vcard_free(lf->vcard);
}

static belle_sip_error_code _linphone_friend_marshall(belle_sip_object_t *obj, char* buff, size_t buff_size, size_t *offset) {
	LinphoneFriend *lf = (LinphoneFriend*)obj;
	belle_sip_error_code err = BELLE_SIP_OK;
	if (lf->uri){
		char *tmp = linphone_address_as_string(lf->uri);
		err = belle_sip_snprintf(buff, buff_size, offset, "%s", tmp);
		ms_free(tmp);
	}
	return err;
}

const char * linphone_friend_get_name(const LinphoneFriend *lf) {
	if (lf && lf->vcard) {
		return linphone_vcard_get_full_name(lf->vcard);
	} else if (lf && lf->uri) {
		LinphoneAddress *fr = lf->uri;
		return linphone_address_get_display_name(fr);
	}
	return NULL;
}

bool_t linphone_friend_get_send_subscribe(const LinphoneFriend *lf){
	return lf->subscribe;
}

LinphoneSubscribePolicy linphone_friend_get_inc_subscribe_policy(const LinphoneFriend *lf){
	return lf->pol;
}

LinphoneOnlineStatus linphone_friend_get_status(const LinphoneFriend *lf){
	LinphoneOnlineStatus online_status = LinphoneStatusOffline;
	LinphonePresenceBasicStatus basic_status = LinphonePresenceBasicStatusClosed;
	LinphonePresenceActivity *activity = NULL;
	const char *description = NULL;
	unsigned int nb_activities = 0;

	if (lf->presence != NULL) {
		basic_status = linphone_presence_model_get_basic_status(lf->presence);
		nb_activities = linphone_presence_model_get_nb_activities(lf->presence);
		online_status = (basic_status == LinphonePresenceBasicStatusOpen) ? LinphoneStatusOnline : LinphoneStatusOffline;
		if (nb_activities > 1) {
			char *tmp = NULL;
			const LinphoneAddress *addr = linphone_friend_get_address(lf);
			if (addr) tmp = linphone_address_as_string(addr);
			ms_warning("Friend %s has several activities, get status from the first one", tmp ? tmp : "unknown");
			if (tmp) ms_free(tmp);
			nb_activities = 1;
		}
		if (nb_activities == 1) {
			activity = linphone_presence_model_get_activity(lf->presence);
			description = linphone_presence_activity_get_description(activity);
			switch (linphone_presence_activity_get_type(activity)) {
				case LinphonePresenceActivityBreakfast:
				case LinphonePresenceActivityDinner:
				case LinphonePresenceActivityLunch:
				case LinphonePresenceActivityMeal:
					online_status = LinphoneStatusOutToLunch;
					break;
				case LinphonePresenceActivityAppointment:
				case LinphonePresenceActivityMeeting:
				case LinphonePresenceActivityPerformance:
				case LinphonePresenceActivityPresentation:
				case LinphonePresenceActivitySpectator:
				case LinphonePresenceActivityWorking:
				case LinphonePresenceActivityWorship:
					online_status = LinphoneStatusDoNotDisturb;
					break;
				case LinphonePresenceActivityAway:
				case LinphonePresenceActivitySleeping:
					online_status = LinphoneStatusAway;
					break;
				case LinphonePresenceActivityHoliday:
				case LinphonePresenceActivityTravel:
				case LinphonePresenceActivityVacation:
					online_status = LinphoneStatusVacation;
					break;
				case LinphonePresenceActivityBusy:
					if (description && strcmp(description, "Do not disturb") == 0) { // See linphonecore.c linphone_core_set_presence_info() method
						online_status = LinphoneStatusDoNotDisturb;
					} else {
						online_status = LinphoneStatusBusy;
					}
					break;
				case LinphonePresenceActivityLookingForWork:
				case LinphonePresenceActivityPlaying:
				case LinphonePresenceActivityShopping:
				case LinphonePresenceActivityTV:
					online_status = LinphoneStatusBusy;
					break;
				case LinphonePresenceActivityInTransit:
				case LinphonePresenceActivitySteering:
					online_status = LinphoneStatusBeRightBack;
					break;
				case LinphonePresenceActivityOnThePhone:
					online_status = LinphoneStatusOnThePhone;
					break;
				case LinphonePresenceActivityOther:
				case LinphonePresenceActivityPermanentAbsence:
					online_status = LinphoneStatusMoved;
					break;
				case LinphonePresenceActivityUnknown:
					/* Rely on the basic status information. */
					break;
				case LinphonePresenceActivityOnline:
					/* Should not happen! */
					/*ms_warning("LinphonePresenceActivityOnline should not happen here!");*/
					break;
				case LinphonePresenceActivityOffline:
					online_status = LinphoneStatusOffline;
					break;
			}
		}
	}

	return online_status;
}

const LinphonePresenceModel * linphone_friend_get_presence_model(LinphoneFriend *lf) {
	return lf->presence;
}

void linphone_friend_set_presence_model(LinphoneFriend *lf, LinphonePresenceModel *presence) {
	if (lf->presence != NULL) {
		linphone_presence_model_unref(lf->presence);
	}
	lf->presence = presence;
}

bool_t linphone_friend_is_presence_received(const LinphoneFriend *lf) {
	return lf->presence_received;
}

BuddyInfo * linphone_friend_get_info(const LinphoneFriend *lf){
	return lf->info;
}

/*
 * updates the subscriptions.
 * If only_when_registered is TRUE, subscribe will be sent only if the friend's corresponding proxy config is in registered.
 * Otherwise if the proxy config goes to unregistered state, the subscription refresh will be suspended.
 * An optional proxy whose state has changed can be passed to optimize the processing.
**/
void linphone_friend_update_subscribes(LinphoneFriend *fr, LinphoneProxyConfig *proxy, bool_t only_when_registered){
	int can_subscribe=1;

	if (only_when_registered && (fr->subscribe || fr->subscribe_active)){
		LinphoneProxyConfig *cfg=linphone_core_lookup_known_proxy(fr->lc,fr->uri);
		if (proxy && proxy!=cfg) return;
		if (cfg && cfg->state!=LinphoneRegistrationOk){
			char *tmp=linphone_address_as_string(fr->uri);
			ms_message("Friend [%s] belongs to proxy config with identity [%s], but this one isn't registered. Subscription is suspended.",
				   tmp,linphone_proxy_config_get_identity(cfg));
			ms_free(tmp);
			can_subscribe=0;
		}
	}
	if (can_subscribe && fr->subscribe && fr->subscribe_active==FALSE){
		ms_message("Sending a new SUBSCRIBE");
		__linphone_friend_do_subscribe(fr);
	}else if (can_subscribe && fr->subscribe_active && !fr->subscribe){
		linphone_friend_unsubscribe(fr);
	}else if (!can_subscribe && fr->outsub){
		fr->subscribe_active=FALSE;
		sal_op_stop_refreshing(fr->outsub);
	}
}

void linphone_friend_save(LinphoneFriend *fr, LinphoneCore *lc) {
#ifdef FRIENDS_SQL_STORAGE_ENABLED
	linphone_core_store_friend_in_db(lc, fr);
#else
	linphone_core_write_friends_config(lc);
#endif
}

void linphone_friend_apply(LinphoneFriend *fr, LinphoneCore *lc) {
	LinphonePresenceModel *model;

	if (!fr->uri) {
		ms_warning("No sip url defined.");
		return;
	}

	if (fr->inc_subscribe_pending) {
		switch(fr->pol) {
			case LinphoneSPWait:
				model = linphone_presence_model_new_with_activity(LinphonePresenceActivityOther, "Waiting for user acceptance");
				linphone_friend_notify(fr, model);
				linphone_presence_model_unref(model);
				break;
			case LinphoneSPAccept:
				if (fr->lc)
					linphone_friend_notify(fr, fr->lc->presence_model);
				break;
			case LinphoneSPDeny:
				linphone_friend_notify(fr, NULL);
				break;
		}
		fr->inc_subscribe_pending = FALSE;
	}
	if (fr->lc) {
		linphone_friend_list_update_subscriptions(fr->friend_list, NULL, linphone_core_should_subscribe_friends_only_when_registered(fr->lc));
	}
	ms_debug("linphone_friend_apply() done.");
	lc->bl_refresh=TRUE;
	fr->commit=FALSE;
}

void linphone_friend_edit(LinphoneFriend *fr) {
	if (fr && fr->vcard) {
		linphone_vcard_compute_md5_hash(fr->vcard);
	}
}

void linphone_friend_done(LinphoneFriend *fr) {
	ms_return_if_fail(fr);
	if (!fr->lc || !fr->friend_list) return;
	linphone_friend_apply(fr, fr->lc);
	linphone_friend_save(fr, fr->lc);
	
	if (fr && fr->vcard) {
		if (linphone_vcard_compare_md5_hash(fr->vcard) != 0) {
			ms_debug("vCard's md5 has changed, mark friend as dirty");
			fr->friend_list->dirty_friends_to_update = ms_list_append(fr->friend_list->dirty_friends_to_update, linphone_friend_ref(fr));
		}
	}
}

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
LinphoneFriend * linphone_core_create_friend(LinphoneCore *lc) {
	LinphoneFriend * lf = linphone_friend_new();
	lf->lc = lc;
	return lf;
}

LinphoneFriend * linphone_core_create_friend_with_address(LinphoneCore *lc, const char *address) {
	LinphoneFriend * lf = linphone_friend_new_with_address(address);
	if (lf)
		lf->lc = lc;
	return lf;
}
#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif

void linphone_core_add_friend(LinphoneCore *lc, LinphoneFriend *lf) {
	if (linphone_friend_list_add_friend(linphone_core_get_default_friend_list(lc), lf) != LinphoneFriendListOK) return;
	if (ms_list_find(lc->subscribers, lf)) {
		/*if this friend was in the pending subscriber list, now remove it from this list*/
		lc->subscribers = ms_list_remove(lc->subscribers, lf);
		linphone_friend_unref(lf);
	}
	if (linphone_core_ready(lc)) linphone_friend_apply(lf, lc);
	else lf->commit = TRUE;
	linphone_friend_save(lf, lc);
}

void linphone_core_remove_friend(LinphoneCore *lc, LinphoneFriend *lf) {
	if (lf && lf->friend_list) {
		if (linphone_friend_list_remove_friend(lf->friend_list, lf) == LinphoneFriendListNonExistentFriend) {
			ms_error("linphone_core_remove_friend(): friend [%p] is not part of core's list.", lf);
		}
	}
}

void linphone_core_update_friends_subscriptions(LinphoneCore *lc, LinphoneProxyConfig *cfg, bool_t only_when_registered) {
	MSList *lists = lc->friends_lists;
	while (lists) {
		LinphoneFriendList *list = (LinphoneFriendList *)lists->data;
		linphone_friend_list_update_subscriptions(list, cfg, only_when_registered);
		lists = ms_list_next(lists);
	}
}

bool_t linphone_core_should_subscribe_friends_only_when_registered(const LinphoneCore *lc){
	return lp_config_get_int(lc->config,"sip","subscribe_presence_only_when_registered",1);
}

void linphone_core_send_initial_subscribes(LinphoneCore *lc) {
	MSList *lists = lc->friends_lists;
	bool_t proxy_config_for_rls_presence_uri_domain = FALSE;
	LinphoneAddress *rls_address = NULL;
	const MSList *elem;

	if (lc->initial_subscribes_sent) return;
	lc->initial_subscribes_sent=TRUE;
	while (lists) {
		LinphoneFriendList *list = (LinphoneFriendList *)lists->data;
		if (list->rls_uri != NULL) {
			rls_address = linphone_core_create_address(lc, list->rls_uri);
			if (rls_address != NULL) {
				const char *rls_domain = linphone_address_get_domain(rls_address);
				if (rls_domain != NULL) {
					for (elem = linphone_core_get_proxy_config_list(lc); elem != NULL; elem = elem->next) {
						LinphoneProxyConfig *cfg = (LinphoneProxyConfig *)elem->data;
						const char *proxy_domain = linphone_proxy_config_get_domain(cfg);
						if (strcmp(rls_domain, proxy_domain) == 0) {
							proxy_config_for_rls_presence_uri_domain = TRUE;
							break;
						}
					}
				}
				linphone_address_unref(rls_address);
			}
		}
		if (proxy_config_for_rls_presence_uri_domain == TRUE) {
			ms_message("Presence list activated so do not send initial subscribes it will be done when registered");
		} else {
			linphone_core_update_friends_subscriptions(lc,NULL,linphone_core_should_subscribe_friends_only_when_registered(lc));
		}
		lists = ms_list_next(lists);
	}
}

void linphone_core_invalidate_friend_subscriptions(LinphoneCore *lc) {
	MSList *lists = lc->friends_lists;
	while (lists) {
		LinphoneFriendList *list = (LinphoneFriendList *)lists->data;
		linphone_friend_list_invalidate_subscriptions(list);
		lists = ms_list_next(lists);
	}		
	lc->initial_subscribes_sent=FALSE;
}

void linphone_friend_set_ref_key(LinphoneFriend *lf, const char *key){
	if (lf->refkey != NULL) {
		ms_free(lf->refkey);
		lf->refkey = NULL;
	}
	if (key) {
		lf->refkey = ms_strdup(key);
	}
	if (lf->lc) {
		linphone_friend_save(lf, lf->lc);
	}
}

const char *linphone_friend_get_ref_key(const LinphoneFriend *lf){
	return lf->refkey;
}

LinphoneFriend *linphone_core_find_friend(const LinphoneCore *lc, const LinphoneAddress *addr) {
	MSList *lists = lc->friends_lists;
	LinphoneFriend *lf = NULL;
	while (lists && !lf) {
		LinphoneFriendList *list = (LinphoneFriendList *)lists->data;
		lf = linphone_friend_list_find_friend_by_address(list, addr);
		lists = ms_list_next(lists);
	}
	return lf;
}

LinphoneFriend *linphone_core_get_friend_by_address(const LinphoneCore *lc, const char *uri) {
	MSList *lists = lc->friends_lists;
	LinphoneFriend *lf = NULL;
	while (lists && !lf) {
		LinphoneFriendList *list = (LinphoneFriendList *)lists->data;
		lf = linphone_friend_list_find_friend_by_uri(list, uri);
		lists = ms_list_next(lists);
	}
	return lf;
}

LinphoneFriend *linphone_core_get_friend_by_ref_key(const LinphoneCore *lc, const char *key) {
	MSList *lists = lc->friends_lists;
	LinphoneFriend *lf = NULL;
	while (lists && !lf) {
		LinphoneFriendList *list = (LinphoneFriendList *)lists->data;
		lf = linphone_friend_list_find_friend_by_ref_key(list, key);
		lists = ms_list_next(lists);
	}
	return lf;
}

#define key_compare(s1,s2)	strcmp(s1,s2)

LinphoneSubscribePolicy __policy_str_to_enum(const char* pol){
	if (key_compare("accept",pol)==0){
		return LinphoneSPAccept;
	}
	if (key_compare("deny",pol)==0){
		return LinphoneSPDeny;
	}
	if (key_compare("wait",pol)==0){
		return LinphoneSPWait;
	}
	ms_warning("Unrecognized subscribe policy: %s",pol);
	return LinphoneSPWait;
}

LinphoneProxyConfig *__index_to_proxy(LinphoneCore *lc, int index){
	if (index>=0) return (LinphoneProxyConfig*)ms_list_nth_data(lc->sip_conf.proxies,index);
	else return NULL;
}

LinphoneFriend * linphone_friend_new_from_config_file(LinphoneCore *lc, int index){
	const char *tmp;
	char item[50];
	int a;
	LinphoneFriend *lf;
	LpConfig *config=lc->config;

	sprintf(item,"friend_%i",index);

	if (!lp_config_has_section(config,item)){
		return NULL;
	}

	tmp=lp_config_get_string(config,item,"url",NULL);
	if (tmp==NULL) {
		return NULL;
	}
	lf=linphone_core_create_friend_with_address(lc, tmp);
	if (lf==NULL) {
		return NULL;
	}
	tmp=lp_config_get_string(config,item,"pol",NULL);
	if (tmp==NULL) linphone_friend_set_inc_subscribe_policy(lf,LinphoneSPWait);
	else{
		linphone_friend_set_inc_subscribe_policy(lf,__policy_str_to_enum(tmp));
	}
	a=lp_config_get_int(config,item,"subscribe",0);
	linphone_friend_send_subscribe(lf,a);
	a = lp_config_get_int(config, item, "presence_received", 0);
	lf->presence_received = (bool_t)a;

	linphone_friend_set_ref_key(lf,lp_config_get_string(config,item,"refkey",NULL));
	return lf;
}

const char *__policy_enum_to_str(LinphoneSubscribePolicy pol){
	switch(pol){
		case LinphoneSPAccept:
			return "accept";
			break;
		case LinphoneSPDeny:
			return "deny";
			break;
		case LinphoneSPWait:
			return "wait";
			break;
	}
	ms_warning("Invalid policy enum value.");
	return "wait";
}

void linphone_friend_write_to_config_file(LpConfig *config, LinphoneFriend *lf, int index){
	char key[50];
	char *tmp;
	const char *refkey;

	sprintf(key,"friend_%i",index);

	if (lf==NULL){
		lp_config_clean_section(config,key);
		return;
	}
	if (lf->uri!=NULL){
		tmp=linphone_address_as_string(lf->uri);
		if (tmp==NULL) {
			return;
		}
		lp_config_set_string(config,key,"url",tmp);
		ms_free(tmp);
	}
	lp_config_set_string(config,key,"pol",__policy_enum_to_str(lf->pol));
	lp_config_set_int(config,key,"subscribe",lf->subscribe);
	lp_config_set_int(config, key, "presence_received", lf->presence_received);

	refkey=linphone_friend_get_ref_key(lf);
	if (refkey){
		lp_config_set_string(config,key,"refkey",refkey);
	}
}

void linphone_core_write_friends_config(LinphoneCore* lc) {
	MSList *elem;
	int i;
	int store_friends;
#ifdef FRIENDS_SQL_STORAGE_ENABLED
	return;
#endif
	if (! linphone_core_ready(lc)) return; /*dont write config when reading it !*/
	store_friends = lp_config_get_int(lc->config, "misc", "store_friends", 1);
	if (store_friends) {
		
		for (elem=linphone_core_get_default_friend_list(lc)->friends,i=0; elem!=NULL; elem=ms_list_next(elem),i++){
			linphone_friend_write_to_config_file(lc->config,(LinphoneFriend*)elem->data,i);
		}
		linphone_friend_write_to_config_file(lc->config,NULL,i);	/* set the end */
	}
}

LinphoneCore *linphone_friend_get_core(const LinphoneFriend *fr){
	return fr->lc;
}

LinphoneFriend *linphone_friend_ref(LinphoneFriend *lf) {
	belle_sip_object_ref(lf);
	return lf;
}

void linphone_friend_unref(LinphoneFriend *lf) {
	belle_sip_object_unref(lf);
}

/* DEPRECATED */
void linphone_friend_destroy(LinphoneFriend *lf) {
	linphone_friend_unref(lf);
}

LinphoneVcard* linphone_friend_get_vcard(LinphoneFriend *fr) {
	if (fr) {
		return fr->vcard;
	}
	return NULL;
}

void linphone_friend_set_vcard(LinphoneFriend *fr, LinphoneVcard *vcard) {
	if (!fr) {
		return;
	}
	
	if (fr->vcard) {
		linphone_vcard_free(fr->vcard);
	}
	fr->vcard = vcard;
	linphone_friend_save(fr, fr->lc);
}

bool_t linphone_friend_create_vcard(LinphoneFriend *fr, const char *name) {
	LinphoneVcard *vcard = NULL;
	const char *fullName = NULL;
	LinphoneAddress *addr = NULL;
	
	if (!fr || fr->vcard) {
		ms_error("Friend is either null or already has a vcard");
		return FALSE;
	}
	
	addr = fr->uri;
	if (!addr && !name) {
		ms_error("friend doesn't have an URI and name parameter is null");
		return FALSE;
	}
	
	if (name) {
		fullName = name;
	} else {
		const char *displayName = linphone_address_get_display_name(addr);
		if (!displayName) {
			fullName = linphone_address_get_username(addr);
		} else {
			fullName = displayName;
		}
	}
	
	if (!fullName) {
		ms_error("Couldn't determine the name to use for the vCard");
		return FALSE;
	}
	
	vcard = linphone_vcard_new();
	linphone_vcard_set_full_name(vcard, fullName);
	linphone_friend_set_vcard(fr, vcard);
	return TRUE;
}

LinphoneFriend *linphone_friend_new_from_vcard(LinphoneVcard *vcard) {
	LinphoneAddress* linphone_address = NULL;
	LinphoneFriend *fr;
	const char *name = NULL;
	MSList *sipAddresses = NULL;

	if (vcard == NULL) {
		ms_error("Cannot create friend from null vcard");
		return NULL;
	}
	name = linphone_vcard_get_full_name(vcard);
	sipAddresses = linphone_vcard_get_sip_addresses(vcard);
	
	fr = linphone_friend_new();
	// Currently presence takes too much time when dealing with hundreds of friends, so I disabled it for now
	fr->pol = LinphoneSPDeny;
	fr->subscribe = FALSE;
	
	if (sipAddresses) {
		const char *sipAddress = (const char *)sipAddresses->data;
		linphone_address = linphone_address_new(sipAddress);
		if (linphone_address) {
			linphone_friend_set_address(fr, linphone_address);
			linphone_address_unref(linphone_address);
		}
		ms_free(sipAddresses);
	}
	if (name) {
		linphone_friend_set_name(fr, name);
	}
	fr->vcard = vcard;
	
	return fr;
}

/*drops all references to the core and unref*/
void _linphone_friend_release(LinphoneFriend *lf){
	lf->lc = NULL;
	_linphone_friend_release_ops(lf);
	linphone_friend_unref(lf);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneFriend);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneFriend, belle_sip_object_t,
	(belle_sip_object_destroy_t) _linphone_friend_destroy,
	NULL, // clone
	_linphone_friend_marshall,
	FALSE
);

/*******************************************************************************
 * SQL storage related functions                                               *
 ******************************************************************************/

#ifdef FRIENDS_SQL_STORAGE_ENABLED

static void linphone_create_table(sqlite3* db) {
	char* errmsg = NULL;
	int ret;
	ret = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS friends ("
						"id                INTEGER PRIMARY KEY AUTOINCREMENT,"
						"friend_list_id    INTEGER,"
						"sip_uri           TEXT NOT NULL,"
						"subscribe_policy  INTEGER,"
						"send_subscribe    INTEGER,"
						"ref_key           TEXT,"
						"vCard             TEXT,"
						"vCard_etag        TEXT,"
						"vCard_url         TEXT,"
						"presence_received INTEGER"
						");",
			0, 0, &errmsg);
	if (ret != SQLITE_OK) {
		ms_error("Error in creation: %s.\n", errmsg);
		sqlite3_free(errmsg);
	}
	
	ret = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS friends_lists ("
						"id                INTEGER PRIMARY KEY AUTOINCREMENT,"
						"display_name      TEXT,"
						"rls_uri           TEXT,"
						"uri               TEXT,"
						"revision          INTEGER"
						");",
			0, 0, &errmsg);
	if (ret != SQLITE_OK) {
		ms_error("Error in creation: %s.\n", errmsg);
		sqlite3_free(errmsg);
	}
}

static void linphone_update_table(sqlite3* db) {

}

void linphone_core_friends_storage_init(LinphoneCore *lc) {
	int ret;
	const char *errmsg;
	sqlite3 *db;
	const MSList *friends_lists = NULL;

	linphone_core_friends_storage_close(lc);

	ret = _linphone_sqlite3_open(lc->friends_db_file, &db);
	if (ret != SQLITE_OK) {
		errmsg = sqlite3_errmsg(db);
		ms_error("Error in the opening: %s.\n", errmsg);
		sqlite3_close(db);
		return;
	}

	linphone_create_table(db);
	linphone_update_table(db);
	lc->friends_db = db;
	
	friends_lists = linphone_core_fetch_friends_lists_from_db(lc);
	if (friends_lists) {
		ms_warning("Replacing current default friend list by the one(s) from the database");
		lc->friends_lists = ms_list_free_with_data(lc->friends_lists, (void (*)(void*))linphone_friend_list_unref);
		lc->friends_lists = NULL;
	
		while (friends_lists) {
			LinphoneFriendList *list = (LinphoneFriendList *)friends_lists->data;
			linphone_core_add_friend_list(lc, list);
			friends_lists = ms_list_next(friends_lists);
		}
	}
}

void linphone_core_friends_storage_close(LinphoneCore *lc) {
	if (lc->friends_db) {
		sqlite3_close(lc->friends_db);
		lc->friends_db = NULL;
	}
}

/* DB layout:
 * | 0  | storage_id
 * | 1  | display_name
 * | 2  | rls_uri
 * | 3  | uri
 * | 4  | revision
 */
static int create_friend_list(void *data, int argc, char **argv, char **colName) {
	MSList **list = (MSList **)data;
	unsigned int storage_id = atoi(argv[0]);
	LinphoneFriendList *lfl = linphone_core_create_friend_list(NULL);
	
	lfl->storage_id = storage_id;
	linphone_friend_list_set_display_name(lfl, argv[1]);
	linphone_friend_list_set_rls_uri(lfl, argv[2]);
	linphone_friend_list_set_uri(lfl, argv[3]);
	lfl->revision = atoi(argv[4]);
	
	*list = ms_list_append(*list, linphone_friend_list_ref(lfl));
	linphone_friend_list_unref(lfl);
	return 0;
}

/* DB layout:
 * | 0  | storage_id
 * | 1  | friend_list_id
 * | 2  | sip_uri
 * | 3  | subscribe_policy
 * | 4  | send_subscribe
 * | 5  | ref_key
 * | 6  | vCard
 * | 7  | vCard eTag
 * | 8  | vCard URL
 * | 9  | presence_received
 */
static int create_friend(void *data, int argc, char **argv, char **colName) {
	MSList **list = (MSList **)data;
	LinphoneFriend *lf = NULL;
	LinphoneVcard *vcard = NULL;
	unsigned int storage_id = atoi(argv[0]);
	
	vcard = linphone_vcard_new_from_vcard4_buffer(argv[6]);
	if (vcard) {
		linphone_vcard_set_etag(vcard, argv[7]);
		linphone_vcard_set_url(vcard, argv[8]);
		lf = linphone_friend_new_from_vcard(vcard);
	}
	if (!lf) {
		LinphoneAddress *addr = linphone_address_new(argv[2]);
		lf = linphone_friend_new();
		linphone_friend_set_address(lf, addr);
		linphone_address_unref(addr);
	}
	linphone_friend_set_inc_subscribe_policy(lf, atoi(argv[3]));
	linphone_friend_send_subscribe(lf, atoi(argv[4]));
	linphone_friend_set_ref_key(lf, ms_strdup(argv[5]));
	lf->presence_received = atoi(argv[9]);
	lf->storage_id = storage_id;
	
	*list = ms_list_append(*list, linphone_friend_ref(lf));
	linphone_friend_unref(lf);
	return 0;
}

static int linphone_sql_request_friend(sqlite3* db, const char *stmt, MSList **list) {
	char* errmsg = NULL;
	int ret;
	ret = sqlite3_exec(db, stmt, create_friend, list, &errmsg);
	if (ret != SQLITE_OK) {
		ms_error("linphone_sql_request: statement %s -> error sqlite3_exec(): %s.", stmt, errmsg);
		sqlite3_free(errmsg);
	}
	return ret;
}

static int linphone_sql_request_friends_list(sqlite3* db, const char *stmt, MSList **list) {
	char* errmsg = NULL;
	int ret;
	ret = sqlite3_exec(db, stmt, create_friend_list, list, &errmsg);
	if (ret != SQLITE_OK) {
		ms_error("linphone_sql_request: statement %s -> error sqlite3_exec(): %s.", stmt, errmsg);
		sqlite3_free(errmsg);
	}
	return ret;
}

static int linphone_sql_request_generic(sqlite3* db, const char *stmt) {
	char* errmsg = NULL;
	int ret;
	ret = sqlite3_exec(db, stmt, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		ms_error("linphone_sql_request: statement %s -> error sqlite3_exec(): %s.", stmt, errmsg);
		sqlite3_free(errmsg);
	}
	return ret;
}

void linphone_core_store_friend_in_db(LinphoneCore *lc, LinphoneFriend *lf) {
	if (lc && lc->friends_db) {
		char *buf;
		int store_friends = lp_config_get_int(lc->config, "misc", "store_friends", 1);
		LinphoneVcard *vcard = linphone_friend_get_vcard(lf);
		
		if (!store_friends) {
			return;
		}
		
		if (!lf || !lf->friend_list) {
			ms_warning("Either the friend or the friend list is null, skipping...");
			return;
		} 
		
		if (lf->friend_list->storage_id == 0) {
			ms_warning("Trying to add a friend in db, but friend list isn't, let's do that first");
			linphone_core_store_friends_list_in_db(lc, lf->friend_list);
		}

		if (lf->storage_id > 0) {
			buf = sqlite3_mprintf("UPDATE friends SET friend_list_id=%i,sip_uri=%Q,subscribe_policy=%i,send_subscribe=%i,ref_key=%Q,vCard=%Q,vCard_etag=%Q,vCard_url=%Q,presence_received=%i WHERE (id = %i);",
				lf->friend_list->storage_id,
				linphone_address_as_string(linphone_friend_get_address(lf)),
				lf->pol,
				lf->subscribe,
				lf->refkey,
				linphone_vcard_as_vcard4_string(vcard),
				linphone_vcard_get_etag(vcard),
				linphone_vcard_get_url(vcard),
				lf->presence_received,
				lf->storage_id
			);
		} else {
			buf = sqlite3_mprintf("INSERT INTO friends VALUES(NULL,%i,%Q,%i,%i,%Q,%Q,%Q,%Q,%i);",
				lf->friend_list->storage_id,
				linphone_address_as_string(linphone_friend_get_address(lf)),
				lf->pol,
				lf->subscribe,
				lf->refkey,
				linphone_vcard_as_vcard4_string(vcard),
				linphone_vcard_get_etag(vcard),
				linphone_vcard_get_url(vcard),
				lf->presence_received
			);
		}
		linphone_sql_request_generic(lc->friends_db, buf);
		sqlite3_free(buf);

		if (lf->storage_id == 0) {
			lf->storage_id = sqlite3_last_insert_rowid(lc->friends_db);
		}
	}
}

void linphone_core_store_friends_list_in_db(LinphoneCore *lc, LinphoneFriendList *list) {
	if (lc && lc->friends_db) {
		char *buf;
		int store_friends = lp_config_get_int(lc->config, "misc", "store_friends", 1);
		
		if (!store_friends) {
			return;
		}

		if (list->storage_id > 0) {
			buf = sqlite3_mprintf("UPDATE friends_lists SET display_name=%Q,rls_uri=%Q,uri=%Q,revision=%i WHERE (id = %i);",
				list->display_name,
				list->rls_uri,
				list->uri,
				list->revision,
				list->storage_id
			);
		} else {
			buf = sqlite3_mprintf("INSERT INTO friends_lists VALUES(NULL,%Q,%Q,%Q,%i);",
				list->display_name,
				list->rls_uri,
				list->uri,
				list->revision
			);
		}
		linphone_sql_request_generic(lc->friends_db, buf);
		sqlite3_free(buf);

		if (list->storage_id == 0) {
			list->storage_id = sqlite3_last_insert_rowid(lc->friends_db);
		}
	}
}

void linphone_core_remove_friend_from_db(LinphoneCore *lc, LinphoneFriend *lf) {
	if (lc && lc->friends_db) {
		char *buf;
		if (lf->storage_id == 0) {
			ms_error("Friend doesn't have a storage_id !");
			return;
		}

		buf = sqlite3_mprintf("DELETE FROM friends WHERE id = %i", lf->storage_id);
		linphone_sql_request_generic(lc->friends_db, buf);
		sqlite3_free(buf);

		lf->storage_id = 0;
	}
}

void linphone_core_remove_friends_list_from_db(LinphoneCore *lc, LinphoneFriendList *list) {
	if (lc && lc->friends_db) {
		char *buf;
		if (list->storage_id == 0) {
			ms_error("Friends list doesn't have a storage_id !");
			return;
		}

		buf = sqlite3_mprintf("DELETE FROM friends_lists WHERE id = %i", list->storage_id);
		linphone_sql_request_generic(lc->friends_db, buf);
		sqlite3_free(buf);

		list->storage_id = 0;
	}
}

MSList* linphone_core_fetch_friends_from_db(LinphoneCore *lc, LinphoneFriendList *list) {
	char *buf;
	uint64_t begin,end;
	MSList *result = NULL;
	MSList *elem = NULL;

	if (!lc || lc->friends_db == NULL || list == NULL) {
		ms_warning("Either lc (or list) is NULL or friends database wasn't initialized with linphone_core_friends_storage_init() yet");
		return NULL;
	}

	buf = sqlite3_mprintf("SELECT * FROM friends WHERE friend_list_id = %i ORDER BY id", list->storage_id);

	begin = ortp_get_cur_time_ms();
	linphone_sql_request_friend(lc->friends_db, buf, &result);
	end = ortp_get_cur_time_ms();
	ms_message("%s(): %i results fetched, completed in %i ms",__FUNCTION__, ms_list_size(result), (int)(end-begin));
	sqlite3_free(buf);
	
	for(elem = result; elem != NULL; elem = elem->next) {
		LinphoneFriend *lf = (LinphoneFriend *)elem->data;
		lf->lc = lc;
		lf->friend_list = list;
	}

	return result;
}

MSList* linphone_core_fetch_friends_lists_from_db(LinphoneCore *lc) {
	char *buf;
	uint64_t begin,end;
	MSList *result = NULL;
	MSList *elem = NULL;

	if (!lc || lc->friends_db == NULL) {
		ms_warning("Either lc is NULL or friends database wasn't initialized with linphone_core_friends_storage_init() yet");
		return NULL;
	}

	buf = sqlite3_mprintf("SELECT * FROM friends_lists ORDER BY id");

	begin = ortp_get_cur_time_ms();
	linphone_sql_request_friends_list(lc->friends_db, buf, &result);
	end = ortp_get_cur_time_ms();
	ms_message("%s(): %i results fetched, completed in %i ms",__FUNCTION__, ms_list_size(result), (int)(end-begin));
	sqlite3_free(buf);
	
	for(elem = result; elem != NULL; elem = elem->next) {
		LinphoneFriendList *lfl = (LinphoneFriendList *)elem->data;
		lfl->lc = lc;
		lfl->friends = linphone_core_fetch_friends_from_db(lc, lfl);
	}

	return result;
}

#else

void linphone_core_friends_storage_init(LinphoneCore *lc) {
}

void linphone_core_friends_storage_close(LinphoneCore *lc) {
}

void linphone_core_store_friend_in_db(LinphoneCore *lc, LinphoneFriend *lf) {
}

void linphone_core_store_friends_list_in_db(LinphoneCore *lc, LinphoneFriendList *list) {
}

void linphone_core_remove_friend_from_db(LinphoneCore *lc, LinphoneFriend *lf) {
}

void linphone_core_remove_friends_list_from_db(LinphoneCore *lc, LinphoneFriendList *list) {
}

MSList* linphone_core_fetch_friends_from_db(LinphoneCore *lc, LinphoneFriendList *list) {
	return NULL;
}

MSList* linphone_core_fetch_friends_lists_from_db(LinphoneCore *lc) {
	return NULL;
}

#endif

void linphone_core_set_friends_database_path(LinphoneCore *lc, const char *path) {
	if (lc->friends_db_file){
		ms_free(lc->friends_db_file);
		lc->friends_db_file = NULL;
	}
	if (path) {
		lc->friends_db_file = ms_strdup(path);
		linphone_core_friends_storage_init(lc);

		linphone_core_migrate_friends_from_rc_to_db(lc);
	}
}

void linphone_core_migrate_friends_from_rc_to_db(LinphoneCore *lc) {
	LpConfig *lpc = NULL;
	LinphoneFriend *lf = NULL;
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(lc);
	int i;
#ifndef FRIENDS_SQL_STORAGE_ENABLED
	ms_warning("linphone has been compiled without sqlite, can't migrate friends");
	return;
#endif
	if (!lc) {
		return;
	}
	
	lpc = linphone_core_get_config(lc);
	if (!lpc) {
		ms_warning("this core has been started without a rc file, nothing to migrate");
		return;
	}
	if (lp_config_get_int(lpc, "misc", "friends_migration_done", 0) == 1) {
		ms_warning("the friends migration has already been done, skipping...");
		return;
	}
	
	if (ms_list_size(linphone_friend_list_get_friends(lfl)) > 0 && lfl->storage_id == 0) {
		linphone_core_remove_friend_list(lc, lfl);
		lfl = linphone_core_create_friend_list(lc);
		linphone_core_add_friend_list(lc, lfl);
		linphone_friend_list_unref(lfl);
	}
	
	for (i = 0; (lf = linphone_friend_new_from_config_file(lc, i)) != NULL; i++) {
		char friend_section[32];
		
		const LinphoneAddress *addr = linphone_friend_get_address(lf);
		if (addr) {
			const char *displayName = linphone_address_get_display_name(addr);
			if (!displayName) {
				displayName = linphone_address_get_username(addr);
			}
			
			if (!linphone_friend_create_vcard(lf, displayName)) {
				ms_warning("Couldn't create vCard for friend %s", linphone_address_as_string(addr));
			} else {
				linphone_vcard_add_sip_address(linphone_friend_get_vcard(lf), linphone_address_as_string_uri_only(addr));
			}
			
			linphone_friend_list_add_friend(lfl, lf);
			linphone_friend_unref(lf);
			
			snprintf(friend_section, sizeof(friend_section), "friend_%i", i);
			lp_config_clean_section(lpc, friend_section);
		}
	}
	
	ms_debug("friends migration successful: %i friends migrated", i);
	lp_config_set_int(lpc, "misc", "friends_migration_done", 1);
}
