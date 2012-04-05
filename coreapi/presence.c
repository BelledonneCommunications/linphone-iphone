/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)

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

#include "linphonecore.h"
#include "private.h"


extern const char *__policy_enum_to_str(LinphoneSubscribePolicy pol);


void linphone_core_add_subscriber(LinphoneCore *lc, const char *subscriber, SalOp *op){
	LinphoneFriend *fl=linphone_friend_new_with_addr(subscriber);
	if (fl==NULL) return ;
	fl->insub=op;
	linphone_friend_set_inc_subscribe_policy(fl,LinphoneSPAccept);
	fl->inc_subscribe_pending=TRUE;
	lc->subscribers=ms_list_append(lc->subscribers,(void *)fl);
	if (lc->vtable.new_subscription_request!=NULL) {
		char *tmp=linphone_address_as_string(fl->uri);
		lc->vtable.new_subscription_request(lc,fl,tmp);
		ms_free(tmp);
	}
}

void linphone_core_reject_subscriber(LinphoneCore *lc, LinphoneFriend *lf){
	linphone_friend_set_inc_subscribe_policy(lf,LinphoneSPDeny);
}

void linphone_core_notify_all_friends(LinphoneCore *lc, LinphoneOnlineStatus os){
	MSList *elem;
	ms_message("Notifying all friends that we are in status %i",os);
	for(elem=lc->friends;elem!=NULL;elem=elem->next){
		LinphoneFriend *lf=(LinphoneFriend *)elem->data;
		if (lf->insub){
			linphone_friend_notify(lf,os);
		}
	}
}

void linphone_subscription_new(LinphoneCore *lc, SalOp *op, const char *from){
	LinphoneFriend *lf=NULL;
	char *tmp;
	LinphoneAddress *uri;
	LinphoneProxyConfig *cfg;
	const char *fixed_contact;
	
	uri=linphone_address_new(from);
	linphone_address_clean(uri);
	tmp=linphone_address_as_string(uri);
	ms_message("Receiving new subscription from %s.",from);

	cfg=linphone_core_lookup_known_proxy(lc,uri);
	if (cfg!=NULL){
		if (cfg->op){
			fixed_contact=sal_op_get_contact(cfg->op);
			if (fixed_contact) {
				sal_op_set_contact (op,fixed_contact);
				ms_message("Contact for next subscribe answer has been fixed using proxy to %s",fixed_contact);
			}
		}
	}
	/* check if we answer to this subscription */
	if (linphone_find_friend(lc->friends,uri,&lf)!=NULL){
		lf->insub=op;
		lf->inc_subscribe_pending=TRUE;
		sal_subscribe_accept(op);
		linphone_friend_done(lf);	/*this will do all necessary actions */
	}else{
		/* check if this subscriber is in our black list */
		if (linphone_find_friend(lc->subscribers,uri,&lf)){
			if (lf->pol==LinphoneSPDeny){
				ms_message("Rejecting %s because we already rejected it once.",from);
				sal_subscribe_decline(op);
			}
			else {
				/* else it is in wait for approval state, because otherwise it is in the friend list.*/
				ms_message("New subscriber found in friend list, in %s state.",__policy_enum_to_str(lf->pol));
			}
		}else {
			sal_subscribe_accept(op);
			linphone_core_add_subscriber(lc,tmp,op);
		}
	}
	linphone_address_destroy(uri);
	ms_free(tmp);
}

void linphone_notify_recv(LinphoneCore *lc, SalOp *op, SalSubscribeStatus ss, SalPresenceStatus sal_status){
	char *tmp;
	LinphoneFriend *lf;
	LinphoneAddress *friend=NULL;
	LinphoneOnlineStatus estatus=LinphoneStatusOffline;
	
	switch(sal_status){
		case SalPresenceOffline:
			estatus=LinphoneStatusOffline;
		break;
		case SalPresenceOnline:
			estatus=LinphoneStatusOnline;
		break;
		case SalPresenceBusy:
			estatus=LinphoneStatusBusy;
		break;
		case SalPresenceBerightback:
			estatus=LinphoneStatusBeRightBack;
		break;
		case SalPresenceAway:
			estatus=LinphoneStatusAway;
		break;
		case SalPresenceOnthephone:
			estatus=LinphoneStatusOnThePhone;
		break;
		case SalPresenceOuttolunch:
			estatus=LinphoneStatusOutToLunch;
		break;
		case SalPresenceDonotdisturb:
			estatus=LinphoneStatusDoNotDisturb;
		break;
		case SalPresenceMoved:
		case SalPresenceAltService:
			estatus=LinphoneStatusMoved;
		break;
	}
	lf=linphone_find_friend_by_out_subscribe(lc->friends,op);
	if (lf!=NULL){
		friend=lf->uri;
		tmp=linphone_address_as_string(friend);
		lf->status=estatus;
		lf->subscribe_active=TRUE;
		if (lc->vtable.notify_presence_recv)
			lc->vtable.notify_presence_recv(lc,(LinphoneFriend*)lf);
		ms_free(tmp);
	}else{
		ms_message("But this person is not part of our friend list, so we don't care.");
	}
	if (ss==SalSubscribeTerminated){
		sal_op_release(op);
		if (lf){
			lf->outsub=NULL;
			lf->subscribe_active=FALSE;
		}
	}
}

void linphone_subscription_closed(LinphoneCore *lc, SalOp *op){
	LinphoneFriend *lf;
	lf=linphone_find_friend_by_inc_subscribe(lc->friends,op);
	sal_op_release(op);
	if (lf!=NULL){
		lf->insub=NULL;
	}else{
		ms_warning("Receiving unsuscribe for unknown in-subscribtion from %s", sal_op_get_from(op));
	}
}
