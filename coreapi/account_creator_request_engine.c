/*
account_creator_request_engine.c
Copyright (C) 2017  Belledonne Communications SARL

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

#include "linphone/account_creator_request_engine.h"
#include "linphone/core.h"
#include "private.h"

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneAccountCreatorRequestCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneAccountCreatorRequestCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);

/************************** Start Account Creator requests_cbs **************************/
LinphoneAccountCreatorRequestCbs * linphone_account_creator_requests_cbs_new(void) {
	return belle_sip_object_new(LinphoneAccountCreatorRequestCbs);
}

LinphoneAccountCreatorRequestCbs * linphone_account_creator_requests_cbs_ref(LinphoneAccountCreatorRequestCbs *requests_cbs) {
	belle_sip_object_ref(requests_cbs);
	return requests_cbs;
}

void linphone_account_creator_requests_cbs_unref(LinphoneAccountCreatorRequestCbs *requests_cbs) {
	belle_sip_object_unref(requests_cbs);
}

void *linphone_account_creator_requests_cbs_get_user_data(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->user_data;
}

void linphone_account_creator_requests_cbs_set_user_data(LinphoneAccountCreatorRequestCbs *requests_cbs, void *ud) {
	requests_cbs->user_data = ud;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_constructor_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->account_creator_request_constructor_cb;
}

void linphone_account_creator_requests_cbs_set_constructor_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb) {
	requests_cbs->account_creator_request_constructor_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_destructor_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->account_creator_request_destructor_cb;
}

void linphone_account_creator_requests_cbs_set_destructor_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb) {
	requests_cbs->account_creator_request_destructor_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_create_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->create_account_request_cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_account_exist_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->is_account_exist_request_cb;
}

void linphone_account_creator_requests_cbs_set_is_account_exist_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb) {
	requests_cbs->is_account_exist_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_activate_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->activate_account_request_cb;
}

void linphone_account_creator_requests_cbs_set_activate_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb) {
	requests_cbs->activate_account_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_account_activated_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->is_account_activated_request_cb;
}

void linphone_account_creator_requests_cbs_set_is_account_activated_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb) {
	requests_cbs->is_account_activated_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_link_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->link_account_request_cb;
}

void linphone_account_creator_requests_cbs_set_link_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb) {
	requests_cbs->link_account_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_activate_alias_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->activate_alias_request_cb;
}

void linphone_account_creator_requests_cbs_set_activate_alias_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb) {
	requests_cbs->activate_alias_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_alias_used_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->is_alias_used_request_cb;
}

void linphone_account_creator_requests_cbs_set_is_alias_used_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb) {
	requests_cbs->is_alias_used_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_account_linked_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->is_account_linked_request_cb;
}

void linphone_account_creator_requests_cbs_set_is_account_linked_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb) {
	requests_cbs->is_account_linked_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_recover_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->is_account_linked_request_cb;
}

void linphone_account_creator_requests_cbs_set_recover_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb) {
	requests_cbs->recover_account_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_update_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs) {
	return requests_cbs->update_account_request_cb;
}

void linphone_account_creator_requests_cbs_set_update_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb) {
	requests_cbs->update_account_request_cb = cb;
}

/************************** End Account Creator requests_cbs **************************/

void linphone_core_set_account_creator_request_engine_cbs(LinphoneCore *lc, LinphoneAccountCreatorRequestCbs *cbs) {
  if (lc->default_ac_request_cbs)
    linphone_account_creator_requests_cbs_unref(lc->default_ac_request_cbs);
  lc->default_ac_request_cbs = cbs;
}

LinphoneAccountCreatorRequestCbs* linphone_core_get_account_creator_request_engine_cbs(LinphoneCore *lc) {
  return lc->default_ac_request_cbs;
}
