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
  * Function to set custom server request.
  * @param[in] creator LinphoneAccountCreator object
  */
 typedef LinphoneRequestStatus (*LinphoneAccountCreatorRequestFunc)(LinphoneAccountCreator *creator);

/**
 * Callback to notify a response of server.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] status The status of the LinphoneAccountCreator test existence operation that has just finished
**/
typedef void (*LinphoneAccountCreatorResponseFunc)(LinphoneAccountCreator *creator, LinphoneRequestStatus status, const char* resp);

/************************** Start Account Creator data **************************/

/**
 * Create a LinphoneAccountCreator and set Linphone Request callbacks.
 * @param[in] core The LinphoneCore used for the XML-RPC communication
 * @param[in] xmlrpc_url The URL to the XML-RPC server. Must be NON NULL.
 * @return The new LinphoneAccountCreator object.
**/
LINPHONE_PUBLIC LinphoneAccountCreator * linphone_account_creator_new(LinphoneCore *core, const char *xmlrpc_url);

/**
 * Set Linphone functions to LinphoneAccountCreator.
 * @param[in] creator LinphoneAccountCreator object
**/
LINPHONE_PUBLIC void linphone_account_creator_set_linphone_impl(LinphoneAccountCreator *creator);

/**
 * Send a XML-RPC request to know the existence of account on server.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_is_account_exist(LinphoneAccountCreator *creator);

/**
 * Send a XML-RPC request to create an account on server.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_create_account(LinphoneAccountCreator *creator);

/**
 * Send a XML-RPC request to know if an account is activated on server.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_is_account_activated(LinphoneAccountCreator *creator);

/**
 * Send a XML-RPC request to activate an account on server.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_activate_account(LinphoneAccountCreator *creator);

/**
 * Send a XML-RPC request to link an account to an alias.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_link_account(LinphoneAccountCreator *creator);

/**
 * Send a XML-RPC request to activate an alias.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_activate_alias(LinphoneAccountCreator *creator);

/**
 * Send a XML-RPC request to know if an alias is used.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_is_alias_used(LinphoneAccountCreator *creator);

/**
 * Send a XML-RPC request to know if an account is linked.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_is_account_linked(LinphoneAccountCreator *creator);

/**
 * Send a XML-RPC request to recover an account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_recover_account(LinphoneAccountCreator *creator);

/**
 * Send a XML-RPC request to update an account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_update_account(LinphoneAccountCreator *creator);

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
 * @return LinphoneUsernameOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneUsernameCheck linphone_account_creator_set_username(LinphoneAccountCreator *creator, const char *username);

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
 * @return LinphonePhoneNumberOk if everything is OK, or specific(s) error(s) otherwise.
**/
LINPHONE_PUBLIC LinphonePhoneNumberMask linphone_account_creator_set_phone_number(LinphoneAccountCreator *creator, const char *phone_number, const char *country_code);

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
 * @return LinphonePasswordOk if everything is OK, or specific(s) error(s) otherwise.
**/
LINPHONE_PUBLIC LinphonePasswordCheck linphone_account_creator_set_password(LinphoneAccountCreator *creator, const char *password);

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
 * @return LinphonePasswordOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphonePasswordCheck linphone_account_creator_set_ha1(LinphoneAccountCreator *creator, const char *ha1);

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
 * @return LinphoneActivationCodeOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneActivationCodeCheck linphone_account_creator_set_activation_code(LinphoneAccountCreator *creator, const char *activation_code);

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
 * @return LinphoneLanguageOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneLanguageCheck linphone_account_creator_set_language(LinphoneAccountCreator *creator, const char *lang);

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
 * @return LinphoneUsernameOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneUsernameCheck linphone_account_creator_set_display_name(LinphoneAccountCreator *creator, const char *display_name);

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
 * @return LinphoneEmailOk if everything is OK, or a specific error otherwise.
**/
LINPHONE_PUBLIC LinphoneEmailCheck linphone_account_creator_set_email(LinphoneAccountCreator *creator, const char *email);

/**
 * Get the email.
 * @param[in] creator LinphoneAccountCreator object
 * @return The email of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_email(const LinphoneAccountCreator *creator);

/**
 * Get the LinphoneAccountCreatorResponseCbs object associated with a LinphoneAccountCreator.
 * @param[in] creator LinphoneAccountCreator object
 * @return The LinphoneAccountCreatorResponseCbs object associated with the LinphoneAccountCreator.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseCbs * linphone_account_creator_get_responses_cbs(const LinphoneAccountCreator *creator);

/**
 * Get the LinphoneAccountCreatorRequestCbs object associated with a LinphoneAccountCreator.
 * @param[in] creator LinphoneAccountCreator object
 * @return The LinphoneAccountCreatorRequestCbs object associated with the LinphoneAccountCreator.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestCbs * linphone_account_creator_get_requests_cbs(const LinphoneAccountCreator *creator);

/************************** End Account Creator data **************************/

/************************** Start Account Creator Linphone **************************/

/**
 * Send an XML-RPC request to test the existence of a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_is_account_exist_custom(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to create a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_create_account_custom(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to activate a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_activate_account_custom(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to test the validation of a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_is_account_activated_custom(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to test the existence a phone number with a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_is_phone_number_used_custom(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to link a phone number with a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_link_phone_number_with_account_custom(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to activate the link of a phone number with a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_activate_phone_number_link_custom(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to a Linphone account with the phone number.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneRequestOk if the request has been sent, LinphoneRequestFailed otherwise
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_recover_phone_account_custom(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to ask if an account is linked with a phone number
 * @param[in] creator LinphoneAccountCreator object
 * @return if this account is linked with a phone number
**/
LINPHONE_PUBLIC LinphoneRequestStatus linphone_account_creator_is_account_linked_custom(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to ask if an account is linked with a phone number
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] new_pwd const char * : new password for the account creator
 * @return LinphoneRequestOk if everything is OK, or a specific error otherwise.
**/
// TODO viré new_pwd qui sera remplacé par le get_user_data de creator
LINPHONE_PUBLIC /*TODO*/ LinphoneRequestStatus linphone_account_creator_update_password_custom(LinphoneAccountCreator *creator);

/************************** End Account Creator Linphone **************************/

/************************** Start Account Creator Requests **************************/

/**
 * Acquire a reference to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The same LinphoneAccountCreatorRequestCbs object.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestCbs * linphone_account_creator_requests_cbs_ref(LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Release a reference to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_unref(LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Retrieve the user pointer associated with a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The user pointer associated with the LinphoneAccountCreatorRequestCbs object.
**/
LINPHONE_PUBLIC void *linphone_account_creator_requests_cbs_get_user_data(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] ud The user pointer to associate with the LinphoneAccountCreatorRequestCbs object.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_user_data(LinphoneAccountCreatorRequestCbs *requests_cbs, void *ud);

/**
 * Get the create account request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current create account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_create_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] func The create account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_create_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc func);

/**
 * Get the is account exist request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current is account exist request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_account_exist_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] func The is account exist request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_is_account_exist_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc func);

/**
 * Get the activate account request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current activate account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_activate_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] func The activate account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_activate_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc func);

/**
 * Get the is account activated request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current is account activated request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_account_activated_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] func The is account activated request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_is_account_activated_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc func);

/**
 * Get the link account request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current link account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_link_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] func The link account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_link_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc func);

/**
 * Get the activate alias request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current link account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_activate_alias_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] func The activate alias request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_activate_alias_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc func);

/**
 * Get the is alias used request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current is alias used request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_alias_used_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] func The is alias used request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_is_alias_used_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc func);

/**
 * Get the is account linked request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current is account linked request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_account_linked_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] func The is account linked request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_is_account_linked_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc func);

/**
 * Get the recover account request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current recover account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_recover_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] func The recover account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_recover_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc func);

/**
 * Get the update account request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current update account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_update_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] func The update account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_update_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc func);

/************************** End Account Creator Requests **************************/

/************************** Start Account Creator Cbs **************************/

/**
 * Acquire a reference to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The same LinphoneAccountCreatorResponseCbs object.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseCbs * linphone_account_creator_responses_cbs_ref(LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Release a reference to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_unref(LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Retrieve the user pointer associated with a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The user pointer associated with the LinphoneAccountCreatorResponseCbs object.
**/
LINPHONE_PUBLIC void *linphone_account_creator_responses_cbs_get_user_data(const LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @param[in] ud The user pointer to associate with the LinphoneAccountCreatorResponseCbs object.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_set_user_data(LinphoneAccountCreatorResponseCbs *responses_cbs, void *ud);

/**
 * Get the create account request.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The current create account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_create_account_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @param[in] cb The create account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_set_create_account_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb);

/**
 * Get the is account exist request.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The current is account exist request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_is_account_exist_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @param[in] cb The is account exist request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_set_is_account_exist_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb);

/**
 * Get the activate account request.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The current activate account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_activate_account_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @param[in] cb The activate account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_set_activate_account_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb);

/**
 * Get the is account activated request.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The current is account activated request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_is_account_activated_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @param[in] cb The is account activated request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_set_is_account_activated_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb);

/**
 * Get the link account request.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The current link account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_link_account_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @param[in] cb The link account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_set_link_account_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb);

/**
 * Get the activate alias request.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The current link account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_activate_alias_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @param[in] cb The activate alias request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_set_activate_alias_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb);

/**
 * Get the is alias used request.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The current is alias used request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_is_alias_used_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @param[in] cb The is alias used request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_set_is_alias_used_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb);

/**
 * Get the is account linked request.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The current is account linked request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_is_account_linked_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @param[in] cb The is account linked request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_set_is_account_linked_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb);

/**
 * Get the recover account request.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The current recover account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_recover_account_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @param[in] cb The recover account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_set_recover_account_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb);

/**
 * Get the update account request.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @return The current update account request.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_update_account_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorResponseCbs object.
 * @param[in] responses_cbs LinphoneAccountCreatorResponseCbs object.
 * @param[in] cb The update account request to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_responses_cbs_set_update_account_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb);

/************************** End Account Creator Cbs **************************/

/**
 * Configure an account (create a proxy config and authentication info for it).
 * @param[in] creator LinphoneAccountCreator object
 * @return A LinphoneProxyConfig object if successful, NULL otherwise
 **/
LINPHONE_PUBLIC LinphoneProxyConfig * linphone_account_creator_configure(const LinphoneAccountCreator *creator);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_ACCOUNT_CREATOR_H_ */
