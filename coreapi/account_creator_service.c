/*
account_creator_service.c
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

#include "linphone/account_creator_service.h"
#include "linphone/core.h"
#include "private.h"

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneAccountCreatorService);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneAccountCreatorService, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);

/************************** Start Account Creator service **************************/
LinphoneAccountCreatorService * linphone_account_creator_service_new(void) {
	return belle_sip_object_new(LinphoneAccountCreatorService);
}

LinphoneAccountCreatorService * linphone_account_creator_service_ref(LinphoneAccountCreatorService *service) {
	belle_sip_object_ref(service);
	return service;
}

void linphone_account_creator_service_unref(LinphoneAccountCreatorService *service) {
	belle_sip_object_unref(service);
}

void *linphone_account_creator_service_get_user_data(const LinphoneAccountCreatorService *service) {
	return service->user_data;
}

void linphone_account_creator_service_set_user_data(LinphoneAccountCreatorService *service, void *ud) {
	service->user_data = ud;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_constructor_cb(const LinphoneAccountCreatorService *service) {
	return service->account_creator_service_constructor_cb;
}

void linphone_account_creator_service_set_constructor_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->account_creator_service_constructor_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_destructor_cb(const LinphoneAccountCreatorService *service) {
	return service->account_creator_service_destructor_cb;
}

void linphone_account_creator_service_set_destructor_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->account_creator_service_destructor_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_create_account_cb(const LinphoneAccountCreatorService *service) {
	return service->create_account_request_cb;
}

void linphone_account_creator_service_set_create_account_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->create_account_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_is_account_exist_cb(const LinphoneAccountCreatorService *service) {
	return service->is_account_exist_request_cb;
}

void linphone_account_creator_service_set_is_account_exist_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->is_account_exist_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_activate_account_cb(const LinphoneAccountCreatorService *service) {
	return service->activate_account_request_cb;
}

void linphone_account_creator_service_set_activate_account_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->activate_account_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_is_account_activated_cb(const LinphoneAccountCreatorService *service) {
	return service->is_account_activated_request_cb;
}

void linphone_account_creator_service_set_is_account_activated_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->is_account_activated_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_link_account_cb(const LinphoneAccountCreatorService *service) {
	return service->link_account_request_cb;
}

void linphone_account_creator_service_set_link_account_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->link_account_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_activate_alias_cb(const LinphoneAccountCreatorService *service) {
	return service->activate_alias_request_cb;
}

void linphone_account_creator_service_set_activate_alias_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->activate_alias_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_is_alias_used_cb(const LinphoneAccountCreatorService *service) {
	return service->is_alias_used_request_cb;
}

void linphone_account_creator_service_set_is_alias_used_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->is_alias_used_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_is_account_linked_cb(const LinphoneAccountCreatorService *service) {
	return service->is_account_linked_request_cb;
}

void linphone_account_creator_service_set_is_account_linked_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->is_account_linked_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_recover_account_cb(const LinphoneAccountCreatorService *service) {
	return service->is_account_linked_request_cb;
}

void linphone_account_creator_service_set_recover_account_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->recover_account_request_cb = cb;
}

LinphoneAccountCreatorRequestFunc linphone_account_creator_service_get_update_account_cb(const LinphoneAccountCreatorService *service) {
	return service->update_account_request_cb;
}

void linphone_account_creator_service_set_update_account_cb(LinphoneAccountCreatorService *service, LinphoneAccountCreatorRequestFunc cb) {
	service->update_account_request_cb = cb;
}

/************************** End Account Creator service **************************/

void linphone_core_set_account_creator_service(LinphoneCore *lc, LinphoneAccountCreatorService *service) {
  if (lc->default_ac_service)
    linphone_account_creator_service_unref(lc->default_ac_service);
  lc->default_ac_service = service;
}

LinphoneAccountCreatorService * linphone_core_get_account_creator_service(LinphoneCore *lc) {
  return lc->default_ac_service;
}
