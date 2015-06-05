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

enum _LinphoneAccountCreatorStatus {
	LinphoneAccountCreatorOk,
	LinphoneAccountCreatorFailed
};

typedef enum _LinphoneAccountCreatorStatus LinphoneAccountCreatorStatus;

typedef struct _LinphoneAccountCreator LinphoneAccountCreator;

typedef void (*LinphoneAccountCreatorCb)(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, void *user_data);

LINPHONE_PUBLIC LinphoneAccountCreator * linphone_account_creator_new(LinphoneCore *core, const char *xmlrpc_url);
LINPHONE_PUBLIC void linphone_account_creator_destroy(LinphoneAccountCreator *creator);

LINPHONE_PUBLIC void linphone_account_creator_set_username(LinphoneAccountCreator *creator, const char *username);
LINPHONE_PUBLIC const char * linphone_account_creator_get_username(const LinphoneAccountCreator *creator);
LINPHONE_PUBLIC void linphone_account_creator_set_password(LinphoneAccountCreator *creator, const char *password);
LINPHONE_PUBLIC const char * linphone_account_creator_get_password(const LinphoneAccountCreator *creator);
LINPHONE_PUBLIC void linphone_account_creator_set_domain(LinphoneAccountCreator *creator, const char *domain);
LINPHONE_PUBLIC const char * linphone_account_creator_get_domain(const LinphoneAccountCreator *creator);
LINPHONE_PUBLIC void linphone_account_creator_set_route(LinphoneAccountCreator *creator, const char *route);
LINPHONE_PUBLIC const char * linphone_account_creator_get_route(const LinphoneAccountCreator *creator);
LINPHONE_PUBLIC void linphone_account_creator_set_email(LinphoneAccountCreator *creator, const char *email);
LINPHONE_PUBLIC const char * linphone_account_creator_get_email(const LinphoneAccountCreator *creator);
LINPHONE_PUBLIC void linphone_account_creator_set_subscribe(LinphoneAccountCreator *creator, int suscribre);
LINPHONE_PUBLIC int linphone_account_creator_get_subscribe(const LinphoneAccountCreator *creator);

LINPHONE_PUBLIC void linphone_account_creator_set_test_existence_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorCb cb, void *user_data);
LINPHONE_PUBLIC void linphone_account_creator_set_test_validation_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorCb cb, void *user_data);
LINPHONE_PUBLIC void linphone_account_creator_set_validate_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorCb cb, void *user_data);

LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_test_existence(LinphoneAccountCreator *creator);
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_test_validation(LinphoneAccountCreator *creator);
LINPHONE_PUBLIC LinphoneAccountCreatorStatus linphone_account_creator_validate(LinphoneAccountCreator *creator);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_ACCOUNT_CREATOR_H_ */
