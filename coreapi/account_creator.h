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


/**
 * @addtogroup misc
 * @{
 */

/**
* Enum describing the status of a LinphoneAccountCreator operation.
**/
typedef enum _LinphoneAccountCreatorStatus {
	LinphoneAccountCreatorOk,
	LinphoneAccountCreatorFailed
} LinphoneAccountCreatorStatus;

/**
 * The LinphoneAccountCreator object used to create an account on a server via XML-RPC.
**/
typedef struct _LinphoneAccountCreator LinphoneAccountCreator;

/**
 * Callback used to notify the end of a LinphoneAccountCreator operation.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] status The status of the LinphoneAccountCreator operation that has just finished
 * @param user_data A user data given when setting the callback.
**/
typedef void (*LinphoneAccountCreatorCb)(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, void *user_data);

/**
 * Create a LinphoneAccountCreator.
 * @param[in] core The LinphoneCore used for the XML-RPC communication
 * @param[in] xmlrpc_url The URL to the XML-RPC server
 * @return The new LinphoneAccountCreator object
**/
LINPHONE_PUBLIC LinphoneAccountCreator * linphone_account_creator_new(LinphoneCore *core, const char *xmlrpc_url);

/**
 * Destroy a LinphoneAccountCreator.
 * @param[in] creator LinphoneAccountCreator object
 * @param
**/
LINPHONE_PUBLIC void linphone_account_creator_destroy(LinphoneAccountCreator *creator);

/**
 * Set the username.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] username The username to set
**/
LINPHONE_PUBLIC void linphone_account_creator_set_username(LinphoneAccountCreator *creator, const char *username);

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
**/
LINPHONE_PUBLIC void linphone_account_creator_set_password(LinphoneAccountCreator *creator, const char *password);

/**
 * Get the password.
 * @param[in] creator LinphoneAccountCreator object
 * @return The password of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_password(const LinphoneAccountCreator *creator);

/**
 * Set the domain.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] domain The domain to set
**/
LINPHONE_PUBLIC void linphone_account_creator_set_domain(LinphoneAccountCreator *creator, const char *domain);

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
**/
LINPHONE_PUBLIC void linphone_account_creator_set_route(LinphoneAccountCreator *creator, const char *route);

/**
 * Get the route.
 * @param[in] creator LinphoneAccountCreator object
 * @return The route of the LinphoneAccountCreator
**/
LINPHONE_PUBLIC const char * linphone_account_creator_get_route(const LinphoneAccountCreator *creator);

/**
 * Set the email.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] email The email to set
**/
LINPHONE_PUBLIC void linphone_account_creator_set_email(LinphoneAccountCreator *creator, const char *email);

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
 * Set the callback called when the account existence test is finished.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] cb The callback called when the account existence test is finished
 * @param[in] user_data The user data passed to the callback
**/
LINPHONE_PUBLIC void linphone_account_creator_set_test_existence_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorCb cb, void *user_data);

/**
 * Set the callback called when the account validation test is finished.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] cb The callback called when the account validation test is finished
 * @param[in] user_data The user data passed to the callback
**/
LINPHONE_PUBLIC void linphone_account_creator_set_test_validation_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorCb cb, void *user_data);

/**
 * Set the callback called when the account creation is finished.
 * @param[in] creator LinphoneAccountCreator object
 * @param[in] cb The callback called when the account creation is finished
 * @param[in] user_data The user data passed to the callback
**/
LINPHONE_PUBLIC void linphone_account_creator_set_validate_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorCb cb, void *user_data);

/**
 * Send an XML-RPC request to test the existence of a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOk if the request has been sent, LinphoneAccountCreatorFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_test_existence(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to test the validation of a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOk if the request has been sent, LinphoneAccountCreatorFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_test_validation(LinphoneAccountCreator *creator);

/**
 * Send an XML-RPC request to create a Linphone account.
 * @param[in] creator LinphoneAccountCreator object
 * @return LinphoneAccountCreatorOk if the request has been sent, LinphoneAccountCreatorFailed otherwise
**/
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_validate(LinphoneAccountCreator *creator);

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
