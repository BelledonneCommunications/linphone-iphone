/*
account_creator_request_engine.h
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

#ifndef LINPHONE_ACCOUNT_CREATOR_REQUEST_ENGINE_H_
#define LINPHONE_ACCOUNT_CREATOR_REQUEST_ENGINE_H_

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function to set custom server request.
 * @param[in] creator LinphoneAccountCreator object
 */
typedef LinphoneRequestStatus (*LinphoneAccountCreatorRequestFunc)(LinphoneAccountCreator *creator);

/**
 * @addtogroup account_creator_request
 * @{
 */

/************************** Start Account Creator Requests **************************/

/**
 * Create a new LinphoneAccountCreatorRequestCbs object.
 * @return a new LinphoneAccountCreatorRequestCbs object.
 * @donotwrap
**/
LinphoneAccountCreatorRequestCbs * linphone_account_creator_requests_cbs_new(void);

/**
 * Acquire a reference to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The same LinphoneAccountCreatorRequestCbs object.
 * @donotwrap
**/
LinphoneAccountCreatorRequestCbs * linphone_account_creator_requests_cbs_ref(LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Release a reference to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @donotwrap
**/
void linphone_account_creator_requests_cbs_unref(LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Retrieve the user pointer associated with a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The user pointer associated with the LinphoneAccountCreatorRequestCbs object.
 * @donotwrap
**/
LINPHONE_PUBLIC void *linphone_account_creator_requests_cbs_get_user_data(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] ud The user pointer to associate with the LinphoneAccountCreatorRequestCbs object.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_user_data(LinphoneAccountCreatorRequestCbs *requests_cbs, void *ud);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The constructor of account creator requests.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_constructor_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/**
 * Get the constructor of account creator requests.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current constructor of create account request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_constructor_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The destructor.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_destructor_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/**
 * Get the destructor of create account request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current destructor of create account request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_destructor_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Get the create account request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current create account request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_create_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The create account request to be used.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_create_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/**
 * Get the is account exist request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current is account exist request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_account_exist_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The is account exist request to be used.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_is_account_exist_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/**
 * Get the activate account request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current activate account request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_activate_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The activate account request to be used.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_activate_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/**
 * Get the is account activated request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current is account activated request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_account_activated_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The is account activated request to be used.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_is_account_activated_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/**
 * Get the link account request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current link account request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_link_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The link account request to be used.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_link_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/**
 * Get the activate alias request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current link account request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_activate_alias_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The activate alias request to be used.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_activate_alias_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/**
 * Get the is alias used request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current is alias used request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_alias_used_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The is alias used request to be used.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_is_alias_used_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/**
 * Get the is account linked request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current is account linked request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_is_account_linked_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The is account linked request to be used.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_is_account_linked_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/**
 * Get the recover account request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current recover account request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_recover_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The recover account request to be used.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_recover_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/**
 * Get the update account request.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @return The current update account request.
 * @donotwrap
**/
LINPHONE_PUBLIC LinphoneAccountCreatorRequestFunc linphone_account_creator_requests_cbs_get_update_account_cb(const LinphoneAccountCreatorRequestCbs *requests_cbs);

/**
 * Assign a user pointer to a LinphoneAccountCreatorRequestCbs object.
 * @param[in] requests_cbs LinphoneAccountCreatorRequestCbs object.
 * @param[in] cb The update account request to be used.
 * @donotwrap
**/
LINPHONE_PUBLIC void linphone_account_creator_requests_cbs_set_update_account_cb(LinphoneAccountCreatorRequestCbs *requests_cbs, LinphoneAccountCreatorRequestFunc cb);

/************************** End Account Creator Requests **************************/

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* LINPHONE_ACCOUNT_CREATOR_REQUEST_ENGINE_H_ */
