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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef LINPHONE_ACCOUNT_CREATOR_H_
#define LINPHONE_ACCOUNT_CREATOR_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "linphonecore.h"

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
	LinphoneAccountCreatorAccountNotExist,

	LinphoneAccountCreatorAccountValidated,
	LinphoneAccountCreatorAccountNotValidated,

	LinphoneAccountCreatorEmailInvalid,
	LinphoneAccountCreatorUsernameInvalid,
	LinphoneAccountCreatorUsernameTooShort,
	LinphoneAccountCreatorUsernameTooLong,
	LinphoneAccountCreatorUsernameInvalidSize,
	LinphoneAccountCreatorPasswordTooShort,
	LinphoneAccountCreatorPasswordTooLong,
	LinphoneAccountCreatorDomainInvalid,
	LinphoneAccountCreatorRouteInvalid,
	LinphoneAccountCreatorDisplayNameInvalid,
	LinphoneAccountCreatorTransportNotSupported,
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
 * Callback used to notify the end of a LinphoneAccountCreator test existence operation.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] status The status of the LinphoneAccountCreator test existence operation that has just finished
**/
typedef void (*LinphoneAccountCreatorCbsExistenceTestedCb)(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status);

/**
 * Callback used to notify the end of a LinphoneAccountCreator test validation operation.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] status The status of the LinphoneAccountCreator test validation operation that has just finished
**/
typedef void (*LinphoneAccountCreatorCbsValidationTestedCb)(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status);

/**
 * Callback used to notify the end of a LinphoneAccountCreator validate operation.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] status The status of the LinphoneAccountCreator validate operation that has just finished
**/
typedef void (*LinphoneAccountCreatorCbsCreateAccountCb)(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status);

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
 * Enable the newsletter subscription.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] subscribe A boolean telling whether to subscribe to the newsletter or not.
**/
LINPHONE_PUBLIC void linphone_account_creator_enable_newsletter_subscription(LinphoneAccountCreator *creator, bool_t subscribe);

/**
 * Tell whether to subscribe to the newsletter or not.
 * @param[in] creator LinphoneAccountCreator object
 * @return A boolean telling whether to subscribe to the newsletter or not.
**/
LINPHONE_PUBLIC bool_t linphone_account_creator_newsletter_subscription_enabled(const LinphoneAccountCreator *creator);

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
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_test_existence(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to test the validation of a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOk if the request has been sent, LinphoneAccountCreatorReqFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_test_validation(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to create a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOk if the request has been sent, LinphoneAccountCreatorReqFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_create_account(LinphoneAccountCreator *creator);

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
 * Get the existence tested callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current existence tested callback.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsExistenceTestedCb linphone_account_creator_cbs_get_existence_tested(const LinphoneAccountCreatorCbs *cbs);

/**
 * Set the existence tested callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The existence tested callback to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_existence_tested(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsExistenceTestedCb cb);

/**
 * Get the validation tested callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current validation tested callback.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsValidationTestedCb linphone_account_creator_cbs_get_validation_tested(const LinphoneAccountCreatorCbs *cbs);

/**
 * Set the validation tested callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The validation tested callback to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_validation_tested(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsValidationTestedCb cb);

/**
 * Get the create account callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @return The current create account callback.
**/
LINPHONE_PUBLIC LinphoneAccountCreatorCbsCreateAccountCb linphone_account_creator_cbs_get_create_account(const LinphoneAccountCreatorCbs *cbs);

/**
 * Set the create account callback.
 * @param[in] cbs LinphoneAccountCreatorCbs object.
 * @param[in] cb The create account callback to be used.
**/
LINPHONE_PUBLIC void linphone_account_creator_cbs_set_create_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsCreateAccountCb cb);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_ACCOUNT_CREATOR_H_ */
