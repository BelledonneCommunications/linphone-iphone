/*
linphone
Copyright (C) 2000 - 2010 Simon MORLAT (simon.morlat@linphone.org)

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "linphone/event.h"
#include "linphone/lpconfig.h"
#include "sal/event-op.h"

#include "c-wrapper/c-wrapper.h"

// TODO: From coreapi. Remove me later.
#include "private.h"

using namespace LinphonePrivate;

const char * linphone_subscription_dir_to_string(LinphoneSubscriptionDir dir){
	switch(dir){
		case LinphoneSubscriptionIncoming:
			return "LinphoneSubscriptionIncoming";
		case LinphoneSubscriptionOutgoing:
			return "LinphoneSubscriptionOutgoing";
		case LinphoneSubscriptionInvalidDir:
			return "LinphoneSubscriptionInvalidDir";
	}
	return "INVALID";
}

LinphoneSubscriptionState linphone_subscription_state_from_sal(SalSubscribeStatus ss){
	switch(ss){
		case SalSubscribeNone: return LinphoneSubscriptionNone;
		case SalSubscribePending: return LinphoneSubscriptionPending;
		case SalSubscribeTerminated: return LinphoneSubscriptionTerminated;
		case SalSubscribeActive: return LinphoneSubscriptionActive;
	}
	return LinphoneSubscriptionNone;
}

const char *linphone_subscription_state_to_string(LinphoneSubscriptionState state){
	switch(state){
		case LinphoneSubscriptionNone: return "LinphoneSubscriptionNone";
		case LinphoneSubscriptionIncomingReceived: return "LinphoneSubscriptionIncomingReceived";
		case LinphoneSubscriptionOutgoingProgress: return "LinphoneSubscriptionOutgoingProgress";
		case LinphoneSubscriptionPending: return "LinphoneSubscriptionPending";
		case LinphoneSubscriptionActive: return "LinphoneSubscriptionActive";
		case LinphoneSubscriptionTerminated: return "LinphoneSubscriptionTerminated";
		case LinphoneSubscriptionError: return "LinphoneSubscriptionError";
		case LinphoneSubscriptionExpiring: return "LinphoneSubscriptionExpiring";
	}
	return NULL;
}

LINPHONE_PUBLIC const char *linphone_publish_state_to_string(LinphonePublishState state){
	switch(state){
		case LinphonePublishNone: return "LinphonePublishNone";
		case LinphonePublishProgress: return "LinphonePublishProgress";
		case LinphonePublishOk: return "LinphonePublishOk";
		case LinphonePublishError: return "LinphonePublishError";
		case LinphonePublishCleared: return "LinphonePublishCleared";
		case LinphonePublishExpiring: return "LinphonePublishExpiring";
	}
	return NULL;
}


BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneEventCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneEventCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);

static LinphoneEventCbs *linphone_event_cbs_new(void) {
	return belle_sip_object_new(LinphoneEventCbs);
}

LinphoneEventCbs *linphone_event_cbs_ref(LinphoneEventCbs *cbs) {
	belle_sip_object_ref(cbs);
	return cbs;
}

void linphone_event_cbs_unref(LinphoneEventCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void *linphone_event_cbs_get_user_data(const LinphoneEventCbs *cbs) {
	return cbs->user_data;
}

void linphone_event_cbs_set_user_data(LinphoneEventCbs *cbs, void *ud) {
	cbs->user_data = ud;
}

LinphoneEventCbsNotifyResponseCb linphone_event_cbs_get_notify_response(const LinphoneEventCbs *cbs) {
	return cbs->notify_response_cb;
}

void linphone_event_cbs_set_notify_response(LinphoneEventCbs *cbs, LinphoneEventCbsNotifyResponseCb cb) {
	cbs->notify_response_cb = cb;
}


static void linphone_event_release(LinphoneEvent *lev){
	if (lev->op) {
		/*this will stop the refresher*/
		lev->op->stopRefreshing();
	}
	linphone_event_unref(lev);
}

static LinphoneEvent * linphone_event_new_base(LinphoneCore *lc, LinphoneSubscriptionDir dir, const char *name, LinphonePrivate::SalEventOp *op){
	LinphoneEvent *lev=belle_sip_object_new(LinphoneEvent);
	lev->callbacks = linphone_event_cbs_new();
	lev->lc=lc;
	lev->dir=dir;
	lev->op=op;
	lev->name=ms_strdup(name);
	if (strcmp(lev->name, "conference") == 0)
		lev->internal = TRUE;
	lev->op->setUserPointer(lev);
	return lev;
}

LinphoneEvent *linphone_event_new(LinphoneCore *lc, LinphoneSubscriptionDir dir, const char *name, int expires){
	LinphoneEvent *lev=linphone_event_new_base(lc, dir, name, new SalSubscribeOp(lc->sal));
	lev->expires=expires;
	return lev;
}

static LinphoneEvent *linphone_event_new_with_op_base(LinphoneCore *lc, SalEventOp *op, LinphoneSubscriptionDir dir, const char *name, bool_t is_out_of_dialog){
	LinphoneEvent *lev=linphone_event_new_base(lc, dir, name, op);
	lev->is_out_of_dialog_op=is_out_of_dialog;
	return lev;
}

LinphoneEvent *linphone_event_new_with_op(LinphoneCore *lc, SalEventOp *op, LinphoneSubscriptionDir dir, const char *name) {
	return linphone_event_new_with_op_base(lc,op,dir,name,FALSE);
}

LinphoneEvent *linphone_event_new_with_out_of_dialog_op(LinphoneCore *lc, SalEventOp *op, LinphoneSubscriptionDir dir, const char *name) {
	return linphone_event_new_with_op_base(lc,op,dir,name,TRUE);
}

void linphone_event_set_internal(LinphoneEvent *lev, bool_t internal) {
	lev->internal = internal;
}

bool_t linphone_event_is_internal(LinphoneEvent *lev) {
	return lev->internal;
}

void linphone_event_set_state(LinphoneEvent *lev, LinphoneSubscriptionState state){
	if (lev->subscription_state!=state){
		ms_message("LinphoneEvent [%p] moving to subscription state %s",lev,linphone_subscription_state_to_string(state));
		lev->subscription_state=state;
		linphone_core_notify_subscription_state_changed(lev->lc,lev,state);
		if (state==LinphoneSubscriptionTerminated || state == LinphoneSubscriptionError){
			linphone_event_release(lev);
		}
	}
}

void linphone_event_set_publish_state(LinphoneEvent *lev, LinphonePublishState state){
	if (lev->publish_state!=state){
		ms_message("LinphoneEvent [%p] moving from [%s] to publish state %s"
				   , lev
				   , linphone_publish_state_to_string(lev->publish_state)
				   , linphone_publish_state_to_string(state));
		lev->publish_state=state;
		linphone_core_notify_publish_state_changed(lev->lc,lev,state);
		switch(state){
			case LinphonePublishNone: /*this state is probably trigered by a network state change to DOWN, we should release the op*/
			case LinphonePublishCleared:
				linphone_event_release(lev);
				break;
			case LinphonePublishOk:
				if (lev->oneshot) linphone_event_release(lev);
				break;
			case LinphonePublishError:
				linphone_event_release(lev);
				break;
			case LinphonePublishProgress:
			case LinphonePublishExpiring:
				/*nothing special to do*/
				break;
		}

	}
}

LinphonePublishState linphone_event_get_publish_state(const LinphoneEvent *lev){
	return lev->publish_state;
}

const LinphoneErrorInfo *linphone_event_get_error_info(const LinphoneEvent *lev){
	if (!lev->ei) ((LinphoneEvent*)lev)->ei = linphone_error_info_new();
	linphone_error_info_from_sal_op(lev->ei, lev->op);
	return lev->ei;
}

LinphoneReason linphone_event_get_reason(const LinphoneEvent *lev){
	return linphone_error_info_get_reason(linphone_event_get_error_info(lev));
}

LinphoneEvent *linphone_core_create_subscribe(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires){
	LinphoneEvent *lev=linphone_event_new(lc, LinphoneSubscriptionOutgoing, event, expires);
	linphone_configure_op(lc,lev->op,resource,NULL,TRUE);
	lev->op->setManualRefresherMode(!lp_config_get_int(lc->config,"sip","refresh_generic_subscribe",1));
	return lev;
}

LinphoneEvent *linphone_core_create_notify(LinphoneCore *lc, const LinphoneAddress *resource, const char *event){
	LinphoneEvent *lev=linphone_event_new(lc, LinphoneSubscriptionIncoming, event, -1);
	linphone_configure_op(lc,lev->op,resource,NULL,TRUE);
	lev->subscription_state = LinphoneSubscriptionIncomingReceived;
	lev->op->setEvent(event);
	lev->is_out_of_dialog_op = TRUE;
	return lev;
}

LinphoneEvent *linphone_core_subscribe(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires, const LinphoneContent *body){
	LinphoneEvent *lev=linphone_core_create_subscribe(lc,resource,event,expires);
	linphone_event_send_subscribe(lev,body);
	return lev;
}

LinphoneStatus linphone_event_send_subscribe(LinphoneEvent *lev, const LinphoneContent *body){
	SalBodyHandler *body_handler;
	int err;

	if (lev->dir!=LinphoneSubscriptionOutgoing){
		ms_error("linphone_event_send_subscribe(): cannot send or update something that is not an outgoing subscription.");
		return -1;
	}
	switch (lev->subscription_state){
		case LinphoneSubscriptionIncomingReceived:
		case LinphoneSubscriptionTerminated:
		case LinphoneSubscriptionOutgoingProgress:
			ms_error("linphone_event_send_subscribe(): cannot update subscription while in state [%s]", linphone_subscription_state_to_string(lev->subscription_state));
			return -1;
		break;
		case LinphoneSubscriptionNone:
		case LinphoneSubscriptionActive:
		case LinphoneSubscriptionExpiring:
		case LinphoneSubscriptionError:
		case LinphoneSubscriptionPending:
			/*those states are ok*/
		break;
	}

	if (lev->send_custom_headers){
		lev->op->setSentCustomHeaders(lev->send_custom_headers);
		sal_custom_header_free(lev->send_custom_headers);
		lev->send_custom_headers=NULL;
	}else lev->op->setSentCustomHeaders(NULL);

	body_handler = sal_body_handler_from_content(body);
    auto subscribeOp = dynamic_cast<SalSubscribeOp *>(lev->op);
	err=subscribeOp->subscribe(NULL,NULL,lev->name,lev->expires,body_handler);
	if (err==0){
		if (lev->subscription_state==LinphoneSubscriptionNone)
			linphone_event_set_state(lev,LinphoneSubscriptionOutgoingProgress);
	}
	return err;
}

LinphoneStatus linphone_event_update_subscribe(LinphoneEvent *lev, const LinphoneContent *body){
	return linphone_event_send_subscribe(lev,body);
}

LinphoneStatus linphone_event_refresh_subscribe(LinphoneEvent *lev) {
	return lev->op->refresh();
}

LinphoneStatus linphone_event_accept_subscription(LinphoneEvent *lev){
	int err;
	if (lev->subscription_state!=LinphoneSubscriptionIncomingReceived){
		ms_error("linphone_event_accept_subscription(): cannot accept subscription if subscription wasn't just received.");
		return -1;
	}
	auto subscribeOp = dynamic_cast<SalSubscribeOp *>(lev->op);
	err=subscribeOp->accept();
	if (err==0){
		linphone_event_set_state(lev,LinphoneSubscriptionActive);
	}
	return err;
}

LinphoneStatus linphone_event_deny_subscription(LinphoneEvent *lev, LinphoneReason reason){
	int err;
	if (lev->subscription_state!=LinphoneSubscriptionIncomingReceived){
		ms_error("linphone_event_deny_subscription(): cannot deny subscription if subscription wasn't just received.");
		return -1;
	}
    auto subscribeOp = dynamic_cast<SalSubscribeOp *>(lev->op);
	err=subscribeOp->decline(linphone_reason_to_sal(reason));
	linphone_event_set_state(lev,LinphoneSubscriptionTerminated);
	return err;
}

LinphoneStatus linphone_event_notify(LinphoneEvent *lev, const LinphoneContent *body){
	SalBodyHandler *body_handler;
	if (lev->subscription_state!=LinphoneSubscriptionActive && lev->subscription_state!=LinphoneSubscriptionIncomingReceived){
		ms_error("linphone_event_notify(): cannot notify if subscription is not active.");
		return -1;
	}
	if (lev->dir!=LinphoneSubscriptionIncoming){
		ms_error("linphone_event_notify(): cannot notify if not an incoming subscription.");
		return -1;
	}
	body_handler = sal_body_handler_from_content(body, false);
    auto subscribeOp = dynamic_cast<SalSubscribeOp *>(lev->op);
	return subscribeOp->notify(body_handler);
}

static LinphoneEvent *_linphone_core_create_publish(LinphoneCore *core, LinphoneProxyConfig *cfg, const LinphoneAddress *resource, const char *event, int expires){
	LinphoneCore *lc = core;
	LinphoneEvent *lev;

	if (!lc && cfg) {
		if (cfg->lc)
			lc = cfg->lc;
		else  {
			ms_error("Cannot create publish from proxy config [%p] not attached to any core",cfg);
			return NULL;
		}
	}
	if (!resource && cfg)
		resource = linphone_proxy_config_get_identity_address(cfg);

	lev = linphone_event_new_with_op(lc, new SalPublishOp(lc->sal), LinphoneSubscriptionInvalidDir, event);
	lev->expires = expires;
	linphone_configure_op_with_proxy(lc,lev->op,resource,NULL, !!lp_config_get_int(lc->config,"sip","publish_msg_with_contact",0),cfg);
	lev->op->setManualRefresherMode(!lp_config_get_int(lc->config,"sip","refresh_generic_publish",1));
	return lev;
}
LinphoneEvent *linphone_core_create_publish(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires){
	return _linphone_core_create_publish(lc, NULL, resource, event, expires);
}
LinphoneEvent *linphone_proxy_config_create_publish(LinphoneProxyConfig *cfg, const char *event, int expires) {

	return _linphone_core_create_publish(NULL, cfg,NULL, event, expires);


}
LinphoneEvent *linphone_core_create_one_shot_publish(LinphoneCore *lc, const LinphoneAddress *resource, const char *event){
	LinphoneEvent *lev = linphone_core_create_publish(lc, resource, event, -1);
	lev->oneshot = TRUE;
	return lev;
}

static int _linphone_event_send_publish(LinphoneEvent *lev, const LinphoneContent *body, bool_t notify_err){
	SalBodyHandler *body_handler;
	int err;

	if (lev->dir!=LinphoneSubscriptionInvalidDir){
		ms_error("linphone_event_update_publish(): this is not a PUBLISH event.");
		return -1;
	}
	if (lev->send_custom_headers){
		lev->op->setSentCustomHeaders(lev->send_custom_headers);
		sal_custom_header_free(lev->send_custom_headers);
		lev->send_custom_headers=NULL;
	}else lev->op->setSentCustomHeaders(NULL);
	body_handler = sal_body_handler_from_content(body);
    auto publishOp = dynamic_cast<SalPublishOp *>(lev->op);
	err=publishOp->publish(NULL,NULL,lev->name,lev->expires,body_handler);
	if (err==0){
		linphone_event_set_publish_state(lev,LinphonePublishProgress);
	}else if (notify_err){
		linphone_event_set_publish_state(lev,LinphonePublishError);
	}
	return err;
}

LinphoneEvent *linphone_core_publish(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires, const LinphoneContent *body){
	int err;
	LinphoneEvent *lev=linphone_core_create_publish(lc,resource,event,expires);
	err=_linphone_event_send_publish(lev,body,FALSE);
	if (err==-1){
		linphone_event_unref(lev);
		lev=NULL;
	}
	return lev;
}


LinphoneStatus linphone_event_send_publish(LinphoneEvent *lev, const LinphoneContent *body){
	return _linphone_event_send_publish(lev,body,TRUE);
}

LinphoneStatus linphone_event_update_publish(LinphoneEvent* lev, const LinphoneContent* body ) {
	return linphone_event_send_publish(lev,body);
}

LinphoneStatus linphone_event_refresh_publish(LinphoneEvent *lev) {
	return lev->op->refresh();
}
void linphone_event_pause_publish(LinphoneEvent *lev) {
	if (lev->op) lev->op->stopRefreshing();
}
void linphone_event_unpublish(LinphoneEvent *lev) {
	lev->terminating = TRUE; /* needed to get clear event*/
	if (lev->op) {
        auto op = dynamic_cast<SalPublishOp *>(lev->op);
        op->unpublish();
    }
}
void linphone_event_set_user_data(LinphoneEvent *ev, void *up){
	ev->userdata=up;
}

void *linphone_event_get_user_data(const LinphoneEvent *ev){
	return ev->userdata;
}

void linphone_event_add_custom_header(LinphoneEvent *ev, const char *name, const char *value){
	ev->send_custom_headers=sal_custom_header_append(ev->send_custom_headers, name, value);
}

const char* linphone_event_get_custom_header(LinphoneEvent* ev, const char* name){
	const SalCustomHeader *ch=ev->op->getRecvCustomHeaders();
	return sal_custom_header_find(ch,name);
}


void linphone_event_terminate(LinphoneEvent *lev){
	// if event was already terminated (including on error), we should not terminate it again
	// otherwise it will be unreffed twice.
	if (lev->publish_state == LinphonePublishError || lev->publish_state == LinphonePublishCleared) {
		return;
	}
	if (lev->subscription_state == LinphoneSubscriptionError || lev->subscription_state == LinphoneSubscriptionTerminated) {
		return;
	}

	lev->terminating=TRUE;
	if (lev->dir==LinphoneSubscriptionIncoming){
        auto op = dynamic_cast<SalSubscribeOp *>(lev->op);
		op->closeNotify();
	}else if (lev->dir==LinphoneSubscriptionOutgoing){
        auto op = dynamic_cast<SalSubscribeOp *>(lev->op);
		op->unsubscribe();
	}

	if (lev->publish_state!=LinphonePublishNone){
		if (lev->publish_state==LinphonePublishOk && lev->expires!=-1){
            auto op = dynamic_cast<SalPublishOp *>(lev->op);
			op->unpublish();
		}
		linphone_event_set_publish_state(lev,LinphonePublishCleared);
		return;
	}

	if (lev->subscription_state!=LinphoneSubscriptionNone){
		linphone_event_set_state(lev,LinphoneSubscriptionTerminated);
		return;
	}
}


LinphoneEvent *linphone_event_ref(LinphoneEvent *lev){
	belle_sip_object_ref(lev);
	return lev;
}

static void linphone_event_destroy(LinphoneEvent *lev){
	if (lev->ei) linphone_error_info_unref(lev->ei);
	if (lev->op) lev->op->release();
	if (lev->send_custom_headers) sal_custom_header_free(lev->send_custom_headers);
	if (lev->to_address) linphone_address_unref(lev->to_address);
	if (lev->from_address) linphone_address_unref(lev->from_address);
	if (lev->remote_contact_address) linphone_address_unref(lev->remote_contact_address);
	linphone_event_cbs_unref(lev->callbacks);

	ms_free(lev->name);
}

void linphone_event_unref(LinphoneEvent *lev){
	belle_sip_object_unref(lev);
}

LinphoneSubscriptionDir linphone_event_get_subscription_dir(LinphoneEvent *lev){
	return lev->dir;
}

LinphoneSubscriptionState linphone_event_get_subscription_state(const LinphoneEvent *lev){
	return lev->subscription_state;
}

const char *linphone_event_get_name(const LinphoneEvent *lev){
	return lev->name;
}

static const LinphoneAddress *_linphone_event_cache_to (const LinphoneEvent *lev) {
	if (lev->to_address)
		linphone_address_unref(lev->to_address);
	char *buf = sal_address_as_string(lev->op->getToAddress());
	((LinphoneEvent *)lev)->to_address = linphone_address_new(buf);
	ms_free(buf);
	return lev->to_address;
}

static const LinphoneAddress *_linphone_event_cache_from (const LinphoneEvent *lev) {
	if (lev->from_address)
		linphone_address_unref(lev->from_address);
	char *buf = sal_address_as_string(lev->op->getFromAddress());
	((LinphoneEvent *)lev)->from_address = linphone_address_new(buf);
	ms_free(buf);
	return lev->from_address;
}

static const LinphoneAddress *_linphone_event_cache_remote_contact (const LinphoneEvent *lev) {
	if (lev->remote_contact_address)
		linphone_address_unref(lev->remote_contact_address);
	char *buf = sal_address_as_string(lev->op->getRemoteContactAddress());
	((LinphoneEvent *)lev)->remote_contact_address = linphone_address_new(buf);
	ms_free(buf);
	return lev->remote_contact_address;
}

const LinphoneAddress *linphone_event_get_from (const LinphoneEvent *lev) {
	if (lev->is_out_of_dialog_op && lev->dir == LinphoneSubscriptionOutgoing)
		return _linphone_event_cache_to(lev);
	return _linphone_event_cache_from(lev);
}

const LinphoneAddress *linphone_event_get_resource(const LinphoneEvent *lev){
	if (lev->is_out_of_dialog_op && lev->dir == LinphoneSubscriptionOutgoing)
		return _linphone_event_cache_from(lev);
	return _linphone_event_cache_to(lev);
}

const LinphoneAddress *linphone_event_get_remote_contact (const LinphoneEvent *lev) {
	return _linphone_event_cache_remote_contact(lev);
}

LinphoneCore *linphone_event_get_core(const LinphoneEvent *lev){
	return lev->lc;
}

static belle_sip_error_code _linphone_event_marshall(belle_sip_object_t *obj, char* buff, size_t buff_size, size_t *offset) {
	LinphoneEvent *ev = (LinphoneEvent*)obj;
	belle_sip_error_code err = BELLE_SIP_OK;

	err = belle_sip_snprintf(buff, buff_size, offset, "%s of %s", ev->dir == LinphoneSubscriptionIncoming ?
		"Incoming Subscribe" : (ev->dir == LinphoneSubscriptionOutgoing ? "Outgoing subscribe" : "Publish"), ev->name);

	return err;
}

void _linphone_event_notify_notify_response(const LinphoneEvent *lev) {
	LinphoneEventCbsNotifyResponseCb cb = linphone_event_cbs_get_notify_response(lev->callbacks);
	if (cb)
		cb(lev);
}

LinphoneEventCbs *linphone_event_get_callbacks(const LinphoneEvent *ev) {
	return ev->callbacks;
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneEvent);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneEvent, belle_sip_object_t,
	(belle_sip_object_destroy_t) linphone_event_destroy,
	NULL, // clone
	_linphone_event_marshall,
	FALSE
);
