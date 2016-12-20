/*
account_creator.h
Copyright (C) 2010-2015 Belledonne Communications SARL

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

#ifndef LINPHONE_ACCOUNT_CREATOR_H_
#define LINPHONE_ACCOUNT_CREATOR_H_

#include "linphonecore.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup misc
 * @{
 */

/**
* Enum describing the status of a LinphoneAccountCreator operation.
**/
typedef enum _LinphoneAccountCreatorStatus {
	LinphoneAccountCreatorOK,
	LinphoneAccountCreatorReqFailed,

	LinphoneAccountCreatorAccountCreated,
	LinphoneAccountCreatorAccountNotCreated,

	LinphoneAccountCreatorAccountExist,
	LinphoneAccountCreatorAccountExistWithAlias,
	LinphoneAccountCreatorAccountNotExist,

	LinphoneAccountCreatorAccountActivated,
	LinphoneAccountCreatorAccountAlreadyActivated,
	LinphoneAccountCreatorAccountNotActivated,

	LinphoneAccountCreatorAccountLinked,
	LinphoneAccountCreatorAccountNotLinked,

	LinphoneAccountCreatorEmailInvalid,

	LinphoneAccountCreatorUsernameInvalid,
	LinphoneAccountCreatorUsernameTooShort,
	LinphoneAccountCreatorUsernameTooLong,
	LinphoneAccountCreatorUsernameInvalidSize,

	LinphoneAccountCreatorPhoneNumberInvalid,
	LinphoneAccountCreatorPhoneNumberTooShort,
	LinphoneAccountCreatorPhoneNumberTooLong,
	LinphoneAccountCreatorPhoneNumberUsedAccount,
	LinphoneAccountCreatorPhoneNumberUsedAlias,
	LinphoneAccountCreatorPhoneNumberNotUsed,

	LinphoneAccountCreatorPasswordTooShort,
	LinphoneAccountCreatorPasswordTooLong,

	LinphoneAccountCreatorDomainInvalid,
	LinphoneAccountCreatorRouteInvalid,
	LinphoneAccountCreatorDisplayNameInvalid,
	LinphoneAccountCreatorTransportNotSupported,
	LinphoneAccountCreatorCountryCodeInvalid,

	LinphoneAccountCreatorErrorServer,
} LinphoneAccountCreatorStatus;

/**
 * The LinphoneAccountCreator object used to create an account on a server via XML-RPC.
**/
typedef struct _LinphoneAccountCreator LinphoneAccountCreator;

/**
 * An object to handle the callbacks for handling the LinphoneAccountCreator operations.
**/
typedef struct _LinphoneAccountCreatorCbs LinphoneAccountCreatorCbs;

/**
 * Callback to notify a status change of the account creator.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] status The status of the LinphoneAccountCreator test existence operation that has just finished
**/
typedef void (*LinphoneAccountCreatorCbsStatusCb)(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char* resp);

/**
 * Create a LinphoneAccountCreator.
 * @param[in] core The LinphoneCore used for the XML-RPC communication
 * @param[in] xmlrpc_url The URL to the XML-RPC server. Must be NON NULL.
 * @return The new LinphoneAccountCreator object
**/
LINPHONE_PUBLIC LinphoneAccountCreator * linphone_account_creator_new(LinphoneCore *core, const char *xmlrpc_url);

/**
 * Acquire a reference to the LinphoneAccountCreator.
 * @param[in] creator LinphoneAccountCreator object.
 * @return The same LinphoneAccountCreator object.
**/
LINPHONE_PUBLIC LinphoneAccountCreator * linphone_account_creator_ref(LinphoneAccountCreator *creator);

/**
 * Release reference to the LinphoneAccountCreator.
 * @param[in] creator LinphoneAccountCreator object.
**/
LINPHONE_PUBLIC void linphone_account_creator_unref(LinphoneAccountCreator *creator);

/**
 * Retrieve the user pointer associated with the LinphoneAccountCreator.
 * @param[in] creator LinphoneAccountCreator object.
 * @return The user pointer associated with the LinphoneAccountCreator.
**/
LINPHONE_PUBLIC void *linphone_account_creator_get_user_data(const LinphoneAccountCreator *creator);

/**
 * Assign a user pointer to the LinphoneAccountCreator.
 * @param[in] creator LinphoneAccountCreator object.
 * @param[in] ud The user pointer to associate with the LinphoneAccountCreator.
**/
LINPHONE_PUBLIC void linphone_account_creator_set_user_data(LinphoneAccountCreator *creator, void *ud);

/**
 * Set the username.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] username The username to set
 * @return LinphoneAccountCreatorOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_username(LinphoneAccountCreator *creator, const char *username);

/**
 * Get the username.
 * @param[in] creator LinphoneAccountCreator object
 * @return The username of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_username(const LinphoneAccountCreator *creator);

/**
 * Update the password.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] new_pwd const char * : new password for the account creator
 * @return LinphoneAccountCreatorOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_update_password(LinphoneAccountCreator *creator, const char *new_pwd);

/**
 * Set the phone number normalized.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] phone_number The phone number to set
 * @param[in] country_code Country code to associate phone number with
 * @return LinphoneAccountCreatorOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_phone_number(LinphoneAccountCreator *creator, const char *phone_number, const char *country_code);

/**
 * Get the RFC 3966 normalized phone number.
 * @param[in] creator LinphoneAccountCreator object
 * @return The phone number of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_phone_number(const LinphoneAccountCreator *creator);

/**
 * Set the password.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] password The password to set
 * @return LinphoneAccountCreatorOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_password(LinphoneAccountCreator *creator, const char *password);

/**
 * Get the password.
 * @param[in] creator LinphoneAccountCreator object
 * @return The password of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_password(const LinphoneAccountCreator *creator);

/**
 * Set the ha1.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] ha1 The ha1 to set
 * @return LinphoneAccountCreatorOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_ha1(LinphoneAccountCreator *creator, const char *ha1);

/**
 * Get the ha1.
 * @param[in] creator LinphoneAccountCreator object
 * @return The ha1 of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_ha1(const LinphoneAccountCreator *creator);

/**
 * Set the activation code.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] activation_code The activation code to set
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_activation_code(LinphoneAccountCreator *creator, const char *activation_code);

/**
 * Set the language to use in email or SMS if supported.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] lang The language to use
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_language(LinphoneAccountCreator *creator, const char *lang);

/**
 * Set the transport.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] transport The transport to set
 * @return LinphoneAccountCreatorOk if everything is OK, or a specific error if given transport is not supported by linphone core.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_transport(LinphoneAccountCreator *creator, LinphoneTransportType transport);

/**
 * Get the transport.
 * @param[in] creator LinphoneAccountCreator object
 * @return The transport of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC LinphoneTransportType linphone_account_creator_get_transport(const LinphoneAccountCreator *creator);

/**
 * Set the domain.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] domain The domain to set
 * @return LinphoneAccountCreatorOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_domain(LinphoneAccountCreator *creator, const char *domain);

/**
 * Get the domain.
 * @param[in] creator LinphoneAccountCreator object
 * @return The domain of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_domain(const LinphoneAccountCreator *creator);

/**
 * Set the route.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] route The route to set
 * @return LinphoneAccountCreatorOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_route(LinphoneAccountCreator *creator, const char *route);

/**
 * Get the route.
 * @param[in] creator LinphoneAccountCreator object
 * @return The route of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_route(const LinphoneAccountCreator *creator);

/**
 * Set the email.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] display_name The display name to set
 * @return LinphoneAccountCreatorOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_display_name(LinphoneAccountCreator *creator, const char *display_name);

/**
 * Get the email.
 * @param[in] creator LinphoneAccountCreator object
 * @return The display name of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_display_name(const LinphoneAccountCreator *creator);

/**
 * Set the email.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] email The email to set
 * @return LinphoneAccountCreatorOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_email(LinphoneAccountCreator *creator, const char *email);

/**
 * Get the email.
 * @param[in] creator LinphoneAccountCreator object
 * @return The email of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_email(const LinphoneAccountCreator *creator);

/**
 * Get the LinphoneAccountCreatorCbs object associated with a LinphoneAccountCreator.
 * @param[in] creator LinphoneAccountCreator object
 * @return The LinphoneAccountCreatorCbs object associated with the LinphoneAccountCreator.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbs * linphone_account_creator_get_callbacks(const LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to test the existence of a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOk if the request has been sent, LinphoneAccountCreatorReqFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_account_used(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to create a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOk if the request has been sent, LinphoneAccountCreatorReqFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_create_account(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to activate a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOk if the request has been sent, LinphoneAccountCreatorReqFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_activate_account(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to test the validation of a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOk if the request has been sent, LinphoneAccountCreatorReqFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_account_activated(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to test the existence a phone number with a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOk if the request has been sent, LinphoneAccountCreatorReqFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_phone_number_used(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to link a phone number with a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOK if the request has been sent, LinphoneAccountCreatorReqFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_link_phone_number_with_account(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to activate the link of a phone number with a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOK if the request has been sent, LinphoneAccountCreatorReqFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_activate_phone_number_link(LinphoneAccountCreator *creator);

LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_recover_phone_account(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to ask if an account is linked with a phone number
 * @param[in] creator LinphoneAccountCreator object
 * @return if this account is linked with a phone number
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_account_linked(LinphoneAccountCreator *creator);

/**
 * Configure an account (create a proxy config and authentication info for it).
 * @param[in] creator LinphoneAccountCreator object
 * @return A LinphoneProxyConfig object if successful, NULL otherwise
**/
LINPHONE_PUBLIC LinphoneProxyConfig * linphone_account_creator_configure(const LinphoneAccountCreator *creator);

/**
 * Acquire a reference to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The same LinphoneAccountCreatorCbs object.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbs * linphone_account_creator_cbs_ref(LinphoneAccountCreatorCbs *cbs);

/**
 * Release a reference to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_unref(LinphoneAccountCreatorCbs *cbs);

/**
 * Retrieve the user pointer associated with a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The user pointer associated with the LinphoneAccountCreatorCbs object.
**/
LINPHONE_PUBLIC void *linphone_account_creator_cbs_get_user_data(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] ud The user pointer to associate with the LinphoneAccountCreatorCbs object.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_user_data(LinphoneAccountCreatorCbs *cbs, void *ud);

/**
 * Retrieve the user pointer associated with a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The user pointer associated with the LinphoneAccountCreatorCbs object.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_update_hash(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The update hash callback to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_update_hash(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the current linked tested callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current linked tested callback.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_account_linked(const LinphoneAccountCreatorCbs *cbs);

/**
 * Set the linked tested callback
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The existence tested callback to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_is_account_linked(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the existence tested callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current existence tested callback.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_account_used(const LinphoneAccountCreatorCbs *cbs);

/**
 * Set the existence tested callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The existence tested callback to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_is_account_used(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the create account callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current create account callback.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_create_account(const LinphoneAccountCreatorCbs *cbs);

/**
 * Set the create account callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The create account callback to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_create_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the activate account callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current activate account callback.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_activate_account(const LinphoneAccountCreatorCbs *cbs);

/**
 * Set the activate account callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The activate account callback to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_activate_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the link phone number with account callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current link phone number with account callback.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_link_phone_number_with_account(const LinphoneAccountCreatorCbs *cbs);

/**
 * Set the link phone number with account callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The link phone number with account callback to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_link_phone_number_with_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the activate phone number link callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current activate phone number link callback.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_activate_phone_number_link(const LinphoneAccountCreatorCbs *cbs);

/**
 * Set the activate phone number link callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The activate phone number link callback to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_activate_phone_number_link(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the validation tested callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current validation tested callback.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_account_activated(const LinphoneAccountCreatorCbs *cbs);

/**
 * Set the validation tested callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The validation tested callback to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_is_account_activated(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the is phone number used callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current is phone number used callback
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_phone_number_used(const LinphoneAccountCreatorCbs *cbs);

/**
 * Set the is phone number used callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb is phone number to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_is_phone_number_used(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

LINPHONE_PUBLIC void linphone_account_creator_cbs_set_recover_phone_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_recover_phone_account(const LinphoneAccountCreatorCbs *cbs);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_ACCOUNT_CREATOR_H_ */
