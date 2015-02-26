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

LinphoneFriend *linphone_find_friend_by_inc_subscribe(MSList *l, SalOp *op){
	MSList *elem;
	for (elem=l;elem!=NULL;elem=elem->next){
		LinphoneFriend *lf=(LinphoneFriend*)elem->data;
		if (lf->insub==op) return lf;
	}
	return NULL;
}

LinphoneFriend *linphone_find_friend_by_out_subscribe(MSList *l, SalOp *op){
	MSList *elem;
	LinphoneFriend *lf;
	for (elem=l;elem!=NULL;elem=elem->next){
		lf=(LinphoneFriend*)elem->data;
		if (lf->outsub==op) return lf;
	}
	return NULL;
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

LinphoneFriend * linphone_friend_new(){
	LinphoneFriend *obj=ms_new0(LinphoneFriend,1);
	obj->pol=LinphoneSPAccept;
	obj->presence=NULL;
	obj->subscribe=TRUE;
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
	linphone_address_destroy(linphone_address);
	return fr;
}

void linphone_friend_set_user_data(LinphoneFriend *lf, void *data){
	lf->up=data;
}

void* linphone_friend_get_user_data(const LinphoneFriend *lf){
	return lf->up;
}

bool_t linphone_friend_in_list(const LinphoneFriend *lf){
	return lf->lc!=NULL;
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
				linphone_address_destroy(id);
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
		linphone_address_destroy(fr);
	}
}

int linphone_friend_set_address(LinphoneFriend *lf, const LinphoneAddress *addr){
	LinphoneAddress *fr=linphone_address_clone(addr);
	linphone_address_clean(fr);
	if (lf->uri!=NULL) linphone_address_destroy(lf->uri);
	lf->uri=fr;
	return 0;
}

int linphone_friend_set_name(LinphoneFriend *lf, const char *name){
	LinphoneAddress *fr=lf->uri;
	if (fr==NULL){
		ms_error("linphone_friend_set_sip_addr() must be called before linphone_friend_set_name().");
		return -1;
	}
	linphone_address_set_display_name(fr,name);
	return 0;
}

int linphone_friend_enable_subscribes(LinphoneFriend *fr, bool_t val){
	fr->subscribe=val;
	return 0;
}

int linphone_friend_set_inc_subscribe_policy(LinphoneFriend *fr, LinphoneSubscribePolicy pol)
{
	fr->pol=pol;
	return 0;
}

void linphone_friend_notify(LinphoneFriend *lf, LinphonePresenceModel *presence){
	char *addr=linphone_address_as_string(linphone_friend_get_address(lf));
	ms_message("Want to notify %s, insub=%p",addr,lf->insub);
	ms_free(addr);
	if (lf->insub!=NULL){
		sal_notify_presence(lf->insub,(SalPresenceModel *)presence);
	}
}

static void linphone_friend_unsubscribe(LinphoneFriend *lf){
	if (lf->outsub!=NULL) {
		sal_unsubscribe(lf->outsub);
		lf->subscribe_active=FALSE;
	}
}

static void linphone_friend_invalidate_subscription(LinphoneFriend *lf){
	if (lf->outsub!=NULL) {
		LinphoneCore *lc=lf->lc;
		sal_op_release(lf->outsub);
		lf->outsub=NULL;
		lf->subscribe_active=FALSE;
		/*notify application that we no longer know the presence activity */
		if (lf->presence != NULL) {
			linphone_presence_model_unref(lf->presence);
		}
		lf->presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityOffline,"unknown activity");
		linphone_core_notify_notify_presence_received(lc,lf);
	}
	lf->initial_subscribes_sent=FALSE;
}

void linphone_friend_close_subscriptions(LinphoneFriend *lf){
	linphone_friend_unsubscribe(lf);
	if (lf->insub){
		sal_notify_presence_close(lf->insub);

	}
}

void linphone_friend_destroy(LinphoneFriend *lf){
	if (lf->insub) {
		sal_op_release(lf->insub);
		lf->insub=NULL;
	}
	if (lf->outsub){
		sal_op_release(lf->outsub);
		lf->outsub=NULL;
	}
	if (lf->presence != NULL) linphone_presence_model_unref(lf->presence);
	if (lf->uri!=NULL) linphone_address_destroy(lf->uri);
	if (lf->info!=NULL) buddy_info_free(lf->info);
	ms_free(lf);
}

const LinphoneAddress *linphone_friend_get_address(const LinphoneFriend *lf){
	return lf->uri;
}

const char * linphone_friend_get_name(const LinphoneFriend *lf) {
	LinphoneAddress *fr = lf->uri;
	if (fr == NULL) return NULL;
	return linphone_address_get_display_name(fr);
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
					ms_warning("LinphonePresenceActivityOnline should not happen here!");
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

void linphone_friend_apply(LinphoneFriend *fr, LinphoneCore *lc){
	LinphonePresenceModel *model;

	if (fr->uri==NULL) {
		ms_warning("No sip url defined.");
		return;
	}

	linphone_core_write_friends_config(lc);

	if (fr->inc_subscribe_pending){
		switch(fr->pol){
			case LinphoneSPWait:
				model = linphone_presence_model_new_with_activity(LinphonePresenceActivityOther, "Waiting for user acceptance");
				linphone_friend_notify(fr,model);
				linphone_presence_model_unref(model);
				break;
			case LinphoneSPAccept:
				if (fr->lc!=NULL)
					linphone_friend_notify(fr,fr->lc->presence_model);
				break;
			case LinphoneSPDeny:
				linphone_friend_notify(fr,NULL);
				break;
		}
		fr->inc_subscribe_pending=FALSE;
	}
	if (fr->lc)
		linphone_friend_update_subscribes(fr,NULL,linphone_core_should_subscribe_friends_only_when_registered(fr->lc));
	ms_message("linphone_friend_apply() done.");
	lc->bl_refresh=TRUE;
	fr->commit=FALSE;
}

void linphone_friend_edit(LinphoneFriend *fr){
}

void linphone_friend_done(LinphoneFriend *fr){
	ms_return_if_fail(fr!=NULL);
	if (fr->lc==NULL) return;
	linphone_friend_apply(fr,fr->lc);
}

LinphoneFriend * linphone_core_create_friend(LinphoneCore *lc) {
	return linphone_friend_new();
}

LinphoneFriend * linphone_core_create_friend_with_address(LinphoneCore *lc, const char *address) {
	return linphone_friend_new_with_address(address);
}

void linphone_core_add_friend(LinphoneCore *lc, LinphoneFriend *lf)
{
	ms_return_if_fail(lf->lc==NULL);
	ms_return_if_fail(lf->uri!=NULL);
	if (ms_list_find(lc->friends,lf)!=NULL){
		char *tmp=NULL;
		const LinphoneAddress *addr=linphone_friend_get_address(lf);
		if (addr) tmp=linphone_address_as_string(addr);
		ms_warning("Friend %s already in list, ignored.", tmp ? tmp : "unknown");
		if (tmp) ms_free(tmp);
		return ;
	}
	lc->friends=ms_list_append(lc->friends,lf);
	lf->lc=lc;
	if ( linphone_core_ready(lc)) linphone_friend_apply(lf,lc);
	else lf->commit=TRUE;
	return ;
}

void linphone_core_remove_friend(LinphoneCore *lc, LinphoneFriend* fl){
	MSList *el=ms_list_find(lc->friends,fl);
	if (el!=NULL){
		linphone_friend_destroy((LinphoneFriend*)el->data);
		lc->friends=ms_list_remove_link(lc->friends,el);
		linphone_core_write_friends_config(lc);
	}else{
		ms_error("linphone_core_remove_friend(): friend [%p] is not part of core's list.",fl);
	}
}

void linphone_core_update_friends_subscriptions(LinphoneCore *lc, LinphoneProxyConfig *cfg, bool_t only_when_registered){
	const MSList *elem;
	for(elem=lc->friends;elem!=NULL;elem=elem->next){
		LinphoneFriend *f=(LinphoneFriend*)elem->data;
		linphone_friend_update_subscribes(f,cfg,only_when_registered);
	}
}

bool_t linphone_core_should_subscribe_friends_only_when_registered(const LinphoneCore *lc){
	return lp_config_get_int(lc->config,"sip","subscribe_presence_only_when_registered",1);
}

void linphone_core_send_initial_subscribes(LinphoneCore *lc){
	if (lc->initial_subscribes_sent) return;
	lc->initial_subscribes_sent=TRUE;
	linphone_core_update_friends_subscriptions(lc,NULL,linphone_core_should_subscribe_friends_only_when_registered(lc));
}

void linphone_core_invalidate_friend_subscriptions(LinphoneCore *lc){
	const MSList *elem;
	for(elem=lc->friends;elem!=NULL;elem=elem->next){
		LinphoneFriend *f=(LinphoneFriend*)elem->data;
		linphone_friend_invalidate_subscription(f);
	}
	lc->initial_subscribes_sent=FALSE;
}

void linphone_friend_set_ref_key(LinphoneFriend *lf, const char *key){
	if (lf->refkey!=NULL){
		ms_free(lf->refkey);
		lf->refkey=NULL;
	}
	if (key)
		lf->refkey=ms_strdup(key);
	if (lf->lc)
		linphone_core_write_friends_config(lf->lc);
}

const char *linphone_friend_get_ref_key(const LinphoneFriend *lf){
	return lf->refkey;
}

LinphoneFriend *linphone_core_find_friend(const LinphoneCore *lc, const LinphoneAddress *addr){
	LinphoneFriend *lf=NULL;
	MSList *elem;
	for(elem=lc->friends;elem!=NULL;elem=ms_list_next(elem)){
		lf=(LinphoneFriend*)elem->data;
		if (linphone_address_weak_equal(lf->uri,addr))
			break;
		lf=NULL;
	}
	return lf;
}

LinphoneFriend *linphone_core_get_friend_by_address(const LinphoneCore *lc, const char *uri){
	LinphoneAddress *puri=linphone_address_new(uri);
	LinphoneFriend *lf=puri ? linphone_core_find_friend(lc,puri) : NULL;
	if (puri) linphone_address_unref(puri);
	return lf;
}

LinphoneFriend *linphone_core_get_friend_by_ref_key(const LinphoneCore *lc, const char *key){
	const MSList *elem;
	if (key==NULL) return NULL;
	for(elem=linphone_core_get_friend_list(lc);elem!=NULL;elem=elem->next){
		LinphoneFriend *lf=(LinphoneFriend*)elem->data;
		if (lf->refkey!=NULL && strcmp(lf->refkey,key)==0){
			return lf;
		}
	}
	return NULL;
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
	lf=linphone_friend_new_with_address(tmp);
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

	refkey=linphone_friend_get_ref_key(lf);
	if (refkey){
		lp_config_set_string(config,key,"refkey",refkey);
	}
}

void linphone_core_write_friends_config(LinphoneCore* lc)
{
	MSList *elem;
	int i;
	if (! linphone_core_ready(lc)) return; /*dont write config when reading it !*/
	for (elem=lc->friends,i=0; elem!=NULL; elem=ms_list_next(elem),i++){
		linphone_friend_write_to_config_file(lc->config,(LinphoneFriend*)elem->data,i);
	}
	linphone_friend_write_to_config_file(lc->config,NULL,i);	/* set the end */
}

LinphoneCore *linphone_friend_get_core(const LinphoneFriend *fr){
	return fr->lc;
}

