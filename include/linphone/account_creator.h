/*
account_creator.h
Copyright (C) 2010-2017 Belledonne Communications SARL

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

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup account_creator
 * @{
 */

/**
 * Callback to notify a response of server.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] status The status of the LinphoneAccountCreator test existence operation that has just finished
**/
typedef void (*LinphoneAccountCreatorCbsStatusCb)(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char* resp);

/************************** Start Account Creator data **************************/

/**
 * Create a LinphoneAccountCreator and set Linphone Request callbacks.
 * @param[in] core The LinphoneCore used for the XML-RPC communication
 * @param[in] xmlrpc_url The URL to the XML-RPC server. Must be NON NULL.
 * @return The new LinphoneAccountCreator object.
**/
LINPHONE_PUBLIC LinphoneAccountCreator * linphone_account_creator_new(LinphoneCore *core, const char *xmlrpc_url);


/**
 * Reset the account creator entries like username, password, phone number...
 * @param[in] creator LinphoneAccountCreator object
**/
LINPHONE_PUBLIC void linphone_account_creator_reset(LinphoneAccountCreator *creator);

/**
 * Send a request to know the existence of account on server.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_account_exist(LinphoneAccountCreator *creator);

/**
 * Send a request to create an account on server.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_create_account(LinphoneAccountCreator *creator);

/**
 * Send a request to know if an account is activated on server.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_account_activated(LinphoneAccountCreator *creator);

/**
 * Send a request to activate an account on server.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_activate_account(LinphoneAccountCreator *creator);

/**
 * Send a request to link an account to an alias.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_link_account(LinphoneAccountCreator *creator);

/**
 * Send a request to activate an alias.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_activate_alias(LinphoneAccountCreator *creator);

/**
 * Send a request to know if an alias is used.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_alias_used(LinphoneAccountCreator *creator);

/**
 * Send a request to know if an account is linked.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_is_account_linked(LinphoneAccountCreator *creator);

/**
 * Send a request to recover an account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_recover_account(LinphoneAccountCreator *creator);

/**
 * Send a request to update an account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorStatusRequestOk if the request has been sent, LinphoneAccountCreatorStatusRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_update_account(LinphoneAccountCreator *creator);

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
 * @return LinphoneAccountCreatorUsernameStatusOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorUsernameStatus linphone_account_creator_set_username(LinphoneAccountCreator *creator, const char *username);

/**
 * Get the username.
 * @param[in] creator LinphoneAccountCreator object
 * @return The username of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_username(const LinphoneAccountCreator *creator);

/**
 * Set the phone number normalized.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] phone_number The phone number to set
 * @param[in] country_code Country code to associate phone number with
 * @return LinphoneAccountCreatorPhoneNumberStatusOk if everything is OK, or specific(s) error(s) otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorPhoneNumberStatusMask linphone_account_creator_set_phone_number(LinphoneAccountCreator *creator, const char *phone_number, const char *country_code);

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
 * @return LinphoneAccountCreatorPasswordStatusOk if everything is OK, or specific(s) error(s) otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorPasswordStatus linphone_account_creator_set_password(LinphoneAccountCreator *creator, const char *password);

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
 * @return LinphoneAccountCreatorPasswordStatusOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorPasswordStatus linphone_account_creator_set_ha1(LinphoneAccountCreator *creator, const char *ha1);

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
 * @return LinphoneAccountCreatorActivationCodeStatusOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorActivationCodeStatus linphone_account_creator_set_activation_code(LinphoneAccountCreator *creator, const char *activation_code);

/**
 * Get the activation code.
 * @param[in] creator LinphoneAccountCreator object
 * @return The activation code of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_activation_code(const LinphoneAccountCreator *creator);

/**
 * Set the language to use in email or SMS if supported.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] lang The language to use
 * @return LinphoneAccountCreatorLanguageStatusOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorLanguageStatus linphone_account_creator_set_language(LinphoneAccountCreator *creator, const char *lang);

/**
 * Get the language use in email of SMS.
 * @param[in] creator LinphoneAccountCreator object
 * @return The language of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_language(const LinphoneAccountCreator *creator);

/**
 * Set the display name.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] display_name The display name to set
 * @return LinphoneAccountCreatorUsernameStatusOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorUsernameStatus linphone_account_creator_set_display_name(LinphoneAccountCreator *creator, const char *display_name);

/**
 * Get the display name.
 * @param[in] creator LinphoneAccountCreator object
 * @return The display name of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_display_name(const LinphoneAccountCreator *creator);

/**
 * Set the email.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] email The email to set
 * @return LinphoneAccountCreatorEmailStatusOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorEmailStatus linphone_account_creator_set_email(LinphoneAccountCreator *creator, const char *email);

/**
 * Get the email.
 * @param[in] creator LinphoneAccountCreator object
 * @return The email of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_email(const LinphoneAccountCreator *creator);

/**
 * Set the domain.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] domain The domain to set
 * @return LinphoneAccountCreatorDomainOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorDomainStatus linphone_account_creator_set_domain(LinphoneAccountCreator *creator, const char *domain);

/**
 * Get the domain.
 * @param[in] creator LinphoneAccountCreator object
 * @return The domain of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_domain(const LinphoneAccountCreator *creator);

/**
 * Set Transport
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] transport The transport to set
 * @return LinphoneAccountCreatorTransportOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorTransportStatus linphone_account_creator_set_transport(LinphoneAccountCreator *creator, LinphoneTransportType transport);

/**
 * get Transport
 * @param[in] creator LinphoneAccountCreator object
 * @return The transport of LinphoneAccountCreator
**/
LINPHONE_PUBLIC LinphoneTransportType linphone_account_creator_get_transport(const LinphoneAccountCreator *creator);

/**
 * Set the route.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] route The route to set
 * @return LinphoneAccountCreatorStatusRequestOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_set_route(LinphoneAccountCreator *creator, const char *route);

/**
 * Get the route.
 * @param[in] creator LinphoneAccountCreator object
 * @return The route of the LinphoneAccountCreator
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC const char * linphone_account_creator_get_route(const LinphoneAccountCreator *creator);

/**
 * Get the LinphoneAccountCreatorCbs object associated with a LinphoneAccountCreator.
 * @param[in] creator LinphoneAccountCreator object
 * @return The LinphoneAccountCreatorCbs object associated with the LinphoneAccountCreator.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbs * linphone_account_creator_get_callbacks(const LinphoneAccountCreator *creator);

/**
 * Get the LinphoneAccountCreatorService object associated with a LinphoneAccountCreator.
 * @param[in] creator LinphoneAccountCreator object
 * @return The LinphoneAccountCreatorService object associated with the LinphoneAccountCreator.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorService * linphone_account_creator_get_service(const LinphoneAccountCreator *creator);

/************************** End Account Creator data **************************/

/************************** Start Account Creator Cbs **************************/

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
 * Get the create account request.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current create account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_create_account(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The create account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_create_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the is account exist request.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current is account exist request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_account_exist(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The is account exist request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_is_account_exist(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the activate account request.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current activate account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_activate_account(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The activate account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_activate_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the is account activated request.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current is account activated request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_account_activated(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The is account activated request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_is_account_activated(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the link account request.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current link account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_link_account(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The link account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_link_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the activate alias request.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current link account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_activate_alias(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The activate alias request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_activate_alias(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the is alias used request.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current is alias used request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_alias_used(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The is alias used request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_is_alias_used(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the is account linked request.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current is account linked request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_account_linked(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The is account linked request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_is_account_linked(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the recover account request.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current recover account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_recover_account(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The recover account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_recover_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/**
 * Get the update account request.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current update account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_update_account(const LinphoneAccountCreatorCbs *cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorCbs object.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The update account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_update_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb);

/************************** End Account Creator Cbs **************************/

/**
 * Create and configure a proxy config and a authentication info for an account creator
 * @param[in] creator LinphoneAccountCreator object
 * @return A LinphoneProxyConfig object if successful, NULL otherwise
**/
LINPHONE_PUBLIC LinphoneProxyConfig * linphone_account_creator_create_proxy_config(const LinphoneAccountCreator *creator);

/**
 * Configure an account (create a proxy config and authentication info for it).
 * @param[in] creator LinphoneAccountCreator object
 * @return A LinphoneProxyConfig object if successful, NULL otherwise
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC LinphoneProxyConfig * linphone_account_creator_configure(const LinphoneAccountCreator *creator);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_ACCOUNT_CREATOR_H_ */
