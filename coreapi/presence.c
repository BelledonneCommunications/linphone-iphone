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
#include <eXosip2/eXosip.h>
#include <osipparser2/osip_message.h>
#include "private.h"


extern const char *__policy_enum_to_str(LinphoneSubscribePolicy pol);


void linphone_core_add_subscriber(LinphoneCore *lc, const char *subscriber, int did, int nid){
	LinphoneFriend *fl=linphone_friend_new_with_addr(subscriber);
	if (fl==NULL) return ;
	fl->in_did=did;
	linphone_friend_set_nid(fl,nid);
	linphone_friend_set_inc_subscribe_policy(fl,LinphoneSPAccept);
	fl->inc_subscribe_pending=TRUE;
	lc->subscribers=ms_list_append(lc->subscribers,(void *)fl);
	if (lc->vtable.new_unknown_subscriber!=NULL) {
		char *subscriber=linphone_address_as_string(fl->uri);
		lc->vtable.new_unknown_subscriber(lc,fl,subscriber);
		ms_free(subscriber);
	}
}

void linphone_core_reject_subscriber(LinphoneCore *lc, LinphoneFriend *lf){
	linphone_friend_set_inc_subscribe_policy(lf,LinphoneSPDeny);
}

static void __do_notify(void * data, void * user_data){
	int *tab=(int*)user_data;
	LinphoneFriend *lf=(LinphoneFriend*)data;
	linphone_friend_notify(lf,tab[0],tab[1]);
}

void __linphone_core_notify_all_friends(LinphoneCore *lc, int ss, int os){
	int tab[2];
	tab[0]=ss;
	tab[1]=os;
	ms_list_for_each2(lc->friends,__do_notify,(void *)tab);
}

void linphone_core_notify_all_friends(LinphoneCore *lc, LinphoneOnlineStatus os){
	ms_message("Notifying all friends that we are in status %i",os);
	__linphone_core_notify_all_friends(lc,EXOSIP_SUBCRSTATE_ACTIVE,os);
}

/* check presence state before answering to call; returns TRUE if we can proceed, else answer the appropriate answer
to close the dialog*/
bool_t linphone_core_check_presence(LinphoneCore *lc){
	return TRUE;
}

void linphone_subscription_new(LinphoneCore *lc, eXosip_event_t *ev){
	LinphoneFriend *lf=NULL;
	osip_from_t *from=ev->request->from;
	char *tmp;
	osip_message_t *msg=NULL;
	LinphoneAddress *uri;
	osip_from_to_str(ev->request->from,&tmp);
	uri=linphone_address_new(tmp);
	ms_message("Receiving new subscription from %s.",tmp);
	/* check if we answer to this subscription */
	if (linphone_find_friend(lc->friends,uri,&lf)!=NULL){
		lf->in_did=ev->did;
		linphone_friend_set_nid(lf,ev->nid);
		eXosip_insubscription_build_answer(ev->tid,202,&msg);
		eXosip_insubscription_send_answer(ev->tid,202,msg);
		__eXosip_wakeup_event();
		linphone_friend_done(lf);	/*this will do all necessary actions */
	}else{
		/* check if this subscriber is in our black list */
		if (linphone_find_friend(lc->subscribers,uri,&lf)){
			if (lf->pol==LinphoneSPDeny){
				ms_message("Rejecting %s because we already rejected it once.",from);
				eXosip_insubscription_send_answer(ev->tid,401,NULL);
			}
			else {
				/* else it is in wait for approval state, because otherwise it is in the friend list.*/
				ms_message("New subscriber found in friend list, in %s state.",__policy_enum_to_str(lf->pol));
			}
		}else {
			eXosip_insubscription_build_answer(ev->tid,202,&msg);
			eXosip_insubscription_send_answer(ev->tid,202,msg);
			linphone_core_add_subscriber(lc,tmp,ev->did,ev->nid);
		}
	}
	osip_free(tmp);
}

void linphone_notify_recv(LinphoneCore *lc, eXosip_event_t *ev)
{
	const char *status=_("Gone");
	const char *img="sip-closed.png";
	char *tmp;
	LinphoneFriend *lf;
	LinphoneAddress *friend=NULL;
	osip_from_t *from=NULL;
	osip_body_t *body=NULL;
	LinphoneOnlineStatus estatus=LINPHONE_STATUS_UNKNOWN;
	ms_message("Receiving notify with sid=%i,nid=%i",ev->sid,ev->nid);
	if (ev->request!=NULL){
		from=ev->request->from;
		osip_message_get_body(ev->request,0,&body);
		if (body==NULL){
			ms_error("No body in NOTIFY");
			return;
		}
		if (strstr(body->body,"pending")!=NULL){
			status=_("Waiting for Approval");
			img="sip-wfa.png";
			estatus=LINPHONE_STATUS_PENDING;
		}else if ((strstr(body->body,"online")!=NULL) || (strstr(body->body,"open")!=NULL)) {
			status=_("Online");
			img="sip-online.png";
			estatus=LINPHONE_STATUS_ONLINE;
		}else if (strstr(body->body,"busy")!=NULL){
			status=_("Busy");
			img="sip-busy.png";
			estatus=LINPHONE_STATUS_BUSY;
		}else if (strstr(body->body,"berightback")!=NULL
				|| strstr(body->body,"in-transit")!=NULL ){
			status=_("Be Right Back");
			img="sip-bifm.png";
			estatus=LINPHONE_STATUS_BERIGHTBACK;
		}else if (strstr(body->body,"away")!=NULL){
			status=_("Away");
			img="sip-away.png";
			estatus=LINPHONE_STATUS_AWAY;
		}else if (strstr(body->body,"onthephone")!=NULL
			|| strstr(body->body,"on-the-phone")!=NULL){
			status=_("On The Phone");
			img="sip-otp.png";
			estatus=LINPHONE_STATUS_ONTHEPHONE;
		}else if (strstr(body->body,"outtolunch")!=NULL
				|| strstr(body->body,"meal")!=NULL){
			status=_("Out To Lunch");
			img="sip-otl.png";
			estatus=LINPHONE_STATUS_OUTTOLUNCH;
		}else if (strstr(body->body,"closed")!=NULL){
			status=_("Closed");
			img="sip-away.png";
			estatus=LINPHONE_STATUS_CLOSED;
		}else{
			status=_("Gone");
			img="sip-closed.png";
			estatus=LINPHONE_STATUS_OFFLINE;
		}
		ms_message("We are notified that sip:%s@%s has online status %s",from->url->username,from->url->host,status);
	}
	lf=linphone_find_friend_by_sid(lc->friends,ev->sid);
	if (lf!=NULL){
		friend=lf->uri;
		tmp=linphone_address_as_string(friend);
		lf->status=estatus;
		lc->vtable.notify_recv(lc,(LinphoneFriend*)lf,tmp,status,img);
		ms_free(tmp);
		if (ev->ss_status==EXOSIP_SUBCRSTATE_TERMINATED) {
			lf->sid=-1;
			lf->out_did=-1;
			ms_message("Outgoing subscription terminated by remote.");
		}
	}else{
		ms_message("But this person is not part of our friend list, so we don't care.");
	}
}

void linphone_subscription_answered(LinphoneCore *lc, eXosip_event_t *ev){
	LinphoneFriend *lf;
	osip_from_t *from=ev->response->to;
	char *tmp;
	osip_from_to_str(from,&tmp);
	LinphoneAddress *uri=linphone_address_new(tmp);
	linphone_find_friend(lc->friends,uri,&lf);
	if (lf!=NULL){
		lf->out_did=ev->did;
		linphone_friend_set_sid(lf,ev->sid);
	}else{
		ms_warning("Receiving answer for unknown subscribe sip:%s@%s", from->url->username,from->url->host);
	}
	ms_free(tmp);
}
void linphone_subscription_closed(LinphoneCore *lc,eXosip_event_t *ev){
	LinphoneFriend *lf;
	osip_from_t *from=ev->request->from;
	lf=linphone_find_friend_by_nid(lc->friends,ev->nid);
	if (lf!=NULL){
		lf->in_did=-1;
		linphone_friend_set_nid(lf,-1);
	}else{
		ms_warning("Receiving unsuscribe for unknown in-subscribtion from sip:%s@%s", from->url->username, from->url->host);
	}
}
