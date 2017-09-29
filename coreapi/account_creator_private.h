/*
account_creator.h
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

#ifndef _ACCOUNT_CREATOR_PRIVATE_H_
#define _ACCOUNT_CREATOR_PRIVATE_H_

#include "linphone/defs.h"
#include <belle-sip/object.h>
#include "linphone/account_creator_service.h"
#include "linphone/account_creator.h"

struct _LinphoneAccountCreatorService {
	belle_sip_object_t base;
	void *user_data;

	LinphoneAccountCreatorRequestFunc account_creator_service_constructor_cb; /**< Constructor */
	LinphoneAccountCreatorRequestFunc account_creator_service_destructor_cb; /**< Destructor */

	LinphoneAccountCreatorRequestFunc create_account_request_cb; /**< Request to create account */
	LinphoneAccountCreatorRequestFunc is_account_exist_request_cb; /**< Request to know if account exist */

	LinphoneAccountCreatorRequestFunc activate_account_request_cb; /**< Request to activate account */
	LinphoneAccountCreatorRequestFunc is_account_activated_request_cb; /**< Request to know if account is activated */

	LinphoneAccountCreatorRequestFunc link_account_request_cb; /**< Request to link account with an alias */
	LinphoneAccountCreatorRequestFunc activate_alias_request_cb; /**< Request to activate the link of alias */
	LinphoneAccountCreatorRequestFunc is_alias_used_request_cb; /**< Request to know if alias is used */
	LinphoneAccountCreatorRequestFunc is_account_linked_request_cb; /**< Request to know if account is linked with an alias */

	LinphoneAccountCreatorRequestFunc recover_account_request_cb; /**< Request to recover account */
	LinphoneAccountCreatorRequestFunc update_account_request_cb; /**< Request to update account */
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneAccountCreatorService);

struct _LinphoneAccountCreatorCbs {
	belle_sip_object_t base;
	void *user_data;

	LinphoneAccountCreatorCbsStatusCb create_account_response_cb; /**< Response of create_account request */
	LinphoneAccountCreatorCbsStatusCb is_account_exist_response_cb; /**< Response of is_account_exist request */

	LinphoneAccountCreatorCbsStatusCb activate_account_response_cb; /**< Response of activate_account request */
	LinphoneAccountCreatorCbsStatusCb is_account_activated_response_cb; /**< Response of is_account_activated request */

	LinphoneAccountCreatorCbsStatusCb link_account_response_cb; /**< Response of link_account request */
	LinphoneAccountCreatorCbsStatusCb activate_alias_response_cb; /**< Response of activation alias */
	LinphoneAccountCreatorCbsStatusCb is_alias_used_response_cb; /**< Response of is_alias_used request */
	LinphoneAccountCreatorCbsStatusCb is_account_linked_response_cb; /**< Response of is_account_linked request */

	LinphoneAccountCreatorCbsStatusCb recover_account_response_cb; /**< Response of recover_account request */
	LinphoneAccountCreatorCbsStatusCb update_account_response_cb; /**< Response of update_account request */
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneAccountCreatorCbs);

struct _LinphoneAccountCreator {
	belle_sip_object_t base;
	void *user_data;
	LinphoneCore *core;

	/* AccountCreator */
	LinphoneAccountCreatorService *service; /**< Account creator service */
	LinphoneAccountCreatorCbs *cbs; /**< Account creator cbs */
	LinphoneXmlRpcSession *xmlrpc_session; /**< XML-RPC session */
	LinphoneProxyConfig *proxy_cfg; /**< Default proxy config */

	/* User */
	char *username; /**< Username */
	char *display_name; /**< Display name */
	/* Password */
	char *password; /**< Plain text password */
	char *ha1; /**< Hash password */
	/* Phone Number(Alias) */
	char *phone_number; /**< User phone number*/
	char *phone_country_code; /**< User phone number country code */
	/* Email(Alias) */
	char *email; /**< User email */
	/* Misc */
	char *language; /**< User language */
	char *activation_code; /**< Account validation code */
	char *domain; /**< Domain */
	LinphoneTransportType transport; /**< Transport used */

	/* Deprecated */
	char *route;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneAccountCreator);


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Account creator custom to set Linphone default values
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_constructor_linphone(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to test the existence of a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_account_exist_linphone(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to create a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_create_account_linphone(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to activate a Linphone account with phone number.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_activate_account_linphone(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to activate a Linphone account with email.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_activate_email_account_linphone(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to test the validation of a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_account_activated_linphone(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to test the existence a phone number with a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_phone_number_used_linphone(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to link a phone number with a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_link_phone_number_with_account_linphone(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to activate the link of a phone number with a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_activate_phone_number_link_linphone(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to a Linphone account with the phone number.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_recover_phone_account_linphone(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to ask if an account is linked with a phone number
 * @param[in] creator LinphoneAccountCreator object
 * @return if this account is linked with a phone number
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_account_linked_linphone(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to ask if an account is linked with a phone number
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] new_pwd const char * : new password for the account creator
 * @return LinphoneAccountCreatorStatusRequestOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_update_password_linphone(LinphoneAccountCreator *creator);

#ifdef __cplusplus
}
#endif

#endif // _ACCOUNT_CREATOR_PRIVATE_H_
