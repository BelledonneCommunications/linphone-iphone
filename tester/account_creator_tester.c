/*
	liblinphone_tester - liblinphone test suite
	Copyright (C) 2017  Belledonne Communications SARL

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "liblinphone_tester.h"

static const char XMLRPC_URL[] = "https://sip2.linphone.org:446/xmlrpc.php";

static const int TIMEOUT_REQUEST = 10000;

/////////// LOCAL TESTS ///////////

////// USERNAME //////
static void local_username_too_short(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_username(creator, ""),
		LinphoneUsernameTooShort,
		LinphoneUsernameCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_username_too_long(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_username(creator, "usernametoolongforyou"),
		LinphoneUsernameTooLong,
		LinphoneUsernameCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_username_invalid_character(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
			linphone_account_creator_set_username(creator, "use!"),
			LinphoneUsernameInvalidCharacters,
			LinphoneUsernameCheck,
			"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_username_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
			linphone_account_creator_set_username(creator, "user"),
			LinphoneUsernameOk,
			LinphoneUsernameCheck,
			"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

////// PASSWORD //////

static void local_password_too_short(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_password(creator, ""),
		LinphonePasswordTooShort,
		LinphonePasswordCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_password_too_long(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_password(creator, "passwordtoolong"),
		LinphonePasswordTooLong,
		LinphonePasswordCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_password_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_password(creator, "pass"),
		LinphonePasswordOk,
		LinphonePasswordCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

////// EMAIL //////

static void local_email_malformed(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test.linphone.org"),
		LinphoneEmailMalformed,
		LinphoneEmailCheck,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test@linphone"),
		LinphoneEmailMalformed,
		LinphoneEmailCheck,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "@linphone.org"),
		LinphoneEmailMalformed,
		LinphoneEmailCheck,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "linphone@.org"),
		LinphoneEmailMalformed,
		LinphoneEmailCheck,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, ".linphone@.org"),
		LinphoneEmailMalformed,
		LinphoneEmailCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_email_invalid_character(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test@linphone.org$"),
		LinphoneEmailInvalidCharacters,
		LinphoneEmailCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_email_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test@linphone.org"),
		LinphoneEmailOk,
		LinphoneEmailCheck,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test02@linphone5252.org"),
		LinphoneEmailOk,
		LinphoneEmailCheck,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "9053test@50255linphone.org"),
		LinphoneEmailOk,
		LinphoneEmailCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

////// PHONE NUMBER //////

static void local_phone_number_too_short(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, "0123", "33")&LinphonePhoneNumberTooShort,
		LinphonePhoneNumberTooShort,
		LinphonePhoneNumberCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_phone_number_too_long(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, "01234567891011", "33")&LinphonePhoneNumberTooLong,
		LinphonePhoneNumberTooLong,
		LinphonePhoneNumberCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_phone_number_invalid(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, NULL, "33")&LinphonePhoneNumberInvalid,
		LinphonePhoneNumberInvalid,
		LinphonePhoneNumberCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_country_code_invalid(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, "0123", "")&LinphonePhoneNumberCountryCodeInvalid,
		LinphonePhoneNumberCountryCodeInvalid,
		LinphonePhoneNumberCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_phone_number_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, "0123456789", "33")&LinphonePhoneNumberOk,
		LinphonePhoneNumberOk,
		LinphonePhoneNumberCheck,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

/////////// SERVER TESTS ///////////

typedef struct _LinphoneAccountCreatorStats {
	int cb_done;
} LinphoneAccountCreatorStats;

static LinphoneAccountCreatorStats* new_linphone_account_creator_stats(void) {
	LinphoneAccountCreatorStats *stats = (LinphoneAccountCreatorStats*) ms_new0(LinphoneAccountCreatorStats, 1);
	return stats;
}

static void account_creator_set_cb_done(LinphoneAccountCreatorResponseCbs *cbs) {
	LinphoneAccountCreatorStats *stats = (LinphoneAccountCreatorStats*) linphone_account_creator_responses_cbs_get_user_data(cbs);
	stats->cb_done++;
	BC_ASSERT_TRUE(stats->cb_done);
}

/*static void account_creator_reset_cb_done(LinphoneAccountCreatorCbs *cbs) {
	LinphoneAccountCreatorStats *stats = (LinphoneAccountCreatorStats*) linphone_account_creator_cbs_get_user_data(cbs);
	stats->cb_done = 0;
	BC_ASSERT_FALSE(stats->cb_done);
}*/

static void account_creator_cb(LinphoneAccountCreator *creator, LinphoneRequestStatus status, const char* resp) {
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneRequestStatus expected_status = (LinphoneRequestStatus)linphone_account_creator_requests_cbs_get_user_data(
		linphone_account_creator_get_requests_cbs(creator));
	BC_ASSERT_EQUAL(
		status,
		expected_status,
		LinphoneRequestStatus,
		"%i");
	account_creator_set_cb_done(cbs);
}

/****************** Start Is Account Exist ************************/
static void server_account_doesnt_exist(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestAccountNotExist);
	linphone_account_creator_set_username(creator, "user_not_exist");
	linphone_account_creator_responses_cbs_set_is_account_exist_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_exist(creator),
		LinphoneRequestOk,
		LinphoneRequestStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_account_exist(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestAccountNotExist);
	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_responses_cbs_set_is_account_exist_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_exist(creator),
		LinphoneRequestOk,
		LinphoneRequestStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_account_exist_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_exist(creator),
		LinphoneRequestMissingCallbacks,
		LinphoneRequestStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_account_exist_arg_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_is_account_exist_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_exist(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Is Account Exist ************************/

/****************** Start Create Account ************************/
static void server_account_created_with_email(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestAccountCreated);
	linphone_account_creator_set_username(creator, "user_1");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_responses_cbs_set_create_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneRequestOk,
		LinphoneRequestStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_already_create_with_email(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestAccountNotExist);
	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_email(creator, "test@bc.com");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_responses_cbs_set_create_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneRequestOk,
		LinphoneRequestStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_account_created_with_phone_number(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestAccountNotExist);
	linphone_account_creator_set_username(creator, "user_1");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_responses_cbs_set_create_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneRequestOk,
		LinphoneRequestStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_already_create_as_account_with_phone_number(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestAccountNotExist);
	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_email(creator, "test@bc.com");
	linphone_account_creator_set_phone_number(creator, "01234567","33");
	linphone_account_creator_responses_cbs_set_create_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneRequestOk,
		LinphoneRequestStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_already_create_as_alias_with_phone_number(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestAccountNotExist);
	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_email(creator, "test@bc.com");
	linphone_account_creator_set_phone_number(creator, "01234567","33");
	linphone_account_creator_responses_cbs_set_create_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneRequestOk,
		LinphoneRequestStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneRequestMissingCallbacks,
		LinphoneRequestStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_with_email_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_create_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_with_email_arg_email_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_create_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_with_email_arg_password_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_create_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_with_phone_number_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_create_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_with_phone_number_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_create_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Create Account ************************/

/****************** Start Is Account Activated ************************/
static void server_is_account_activated_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_activation_code(creator, "123456789");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_activated(creator),
		LinphoneRequestMissingCallbacks,
		LinphoneRequestStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_account_activated_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_is_account_activated_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_activated(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Is Account Activated ************************/

/****************** Start Activate Account ************************/
static void server_activate_account_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneRequestMissingCallbacks,
		LinphoneRequestStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_account_with_email_activated_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_activation_code(creator, "123456789");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_activate_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_account_with_email_activated_arg_activation_code_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_activate_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_account_with_phone_number_activated_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_activation_code(creator, "123456789");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_activate_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_account_with_phone_number_activated_arg_activation_code_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_activate_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Activate Account ************************/

/****************** Start Link Account ************************/
static void server_link_account_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");

	BC_ASSERT_EQUAL(
		linphone_account_creator_link_account(creator),
		LinphoneRequestMissingCallbacks,
		LinphoneRequestStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_link_account_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "0123456", "33");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_link_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_link_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_link_account_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_link_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_link_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Link Account ************************/

/****************** Start Activate Alias ************************/
static void server_activate_alias_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_activation_code(creator, "12345679");
	linphone_account_creator_set_password(creator, "password");

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneRequestMissingCallbacks,
		LinphoneRequestStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_alias_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_activation_code(creator, "12345679");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_activate_alias_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_alias_arg_activation_code_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_activate_alias_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_alias_arg_password_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_activation_code(creator, "12345679");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_activate_alias_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_alias_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_activation_code(creator, "12345679");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_activate_alias_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Activate Alias ************************/

/****************** Start Is Alias Used ************************/
static void server_is_alias_used_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_phone_number(creator, "0123456", "33");

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_alias_used(creator),
		LinphoneRequestMissingCallbacks,
		LinphoneRequestStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_alias_used_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_is_alias_used_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_alias_used(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Is Alias Used ************************/

/****************** Start Is Account Linked ************************/
static void server_is_account_linked_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_linked(creator),
		LinphoneRequestMissingCallbacks,
		LinphoneRequestStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_account_linked_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_is_account_linked_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_linked(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Is Account Linked ************************/

/****************** Start Recover Account ************************/
static void server_recover_account_with_phone_number_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_phone_number(creator, "0123456", "33");

	BC_ASSERT_EQUAL(
		linphone_account_creator_recover_account(creator),
		LinphoneRequestMissingCallbacks,
		LinphoneRequestStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_recover_account_with_phone_number_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);
	linphone_account_creator_responses_cbs_set_recover_account_cb(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_recover_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Recover Account ************************/

/****************** Start Update Account ************************/
static void server_update_account_password_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_user_data(creator, "newpassword");

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneRequestMissingCallbacks,
		LinphoneRequestStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_update_account_password_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_user_data(creator, "newpassword");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_responses_cbs_set_update_account_cb(cbs, account_creator_cb);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_update_account_password_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_user_data(creator, "newpassword");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_responses_cbs_set_update_account_cb(cbs, account_creator_cb);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_update_account_password_arg_password_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_user_data(creator, "newpassword");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_responses_cbs_set_update_account_cb(cbs, account_creator_cb);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_update_account_password_arg_new_password_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorResponseCbs *cbs = linphone_account_creator_get_responses_cbs(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_responses_cbs_set_user_data(cbs, stats);
	linphone_account_creator_responses_cbs_set_update_account_cb(cbs, account_creator_cb);
	linphone_account_creator_requests_cbs_set_user_data(
		linphone_account_creator_get_requests_cbs(creator),
		(void*)LinphoneRequestMissingArguments);

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneRequestMissingArguments,
		LinphoneRequestStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Update Account ************************/

test_t account_creator_tests[] = {
	TEST_ONE_TAG("Local - Username too short", local_username_too_short, "Local"),
	TEST_ONE_TAG("Local - Username too long", local_username_too_long, "Local"),
	TEST_ONE_TAG("Local - Username invalid character", local_username_invalid_character, "Local"),
	TEST_ONE_TAG("Local - Username Ok", local_username_ok, "Local"),
	TEST_ONE_TAG("Local - Password too short", local_password_too_short, "Local"),
	TEST_ONE_TAG("Local - Password too long", local_password_too_long, "Local"),
	TEST_ONE_TAG("Local - Password Ok", local_password_ok, "Local"),
	TEST_ONE_TAG("Local - Email malformed", local_email_malformed, "Local"),
	TEST_ONE_TAG("Local - Email invalid character",local_email_invalid_character, "Local"),
	TEST_ONE_TAG("Local - Email Ok", local_email_ok, "Local"),
	TEST_ONE_TAG("Local - Phone number too short", local_phone_number_too_short, "Local"),
	TEST_ONE_TAG("Local - Phone number too long", local_phone_number_too_long, "Local"),
	TEST_ONE_TAG("Local - Phone number invalid", local_phone_number_invalid, "Local"),
	TEST_ONE_TAG("Local - Country code invalid", local_country_code_invalid, "Local"),
	TEST_ONE_TAG("Local - Phone number ok", local_phone_number_ok, "Local"),

	TEST_ONE_TAG("Server - Account doesn\'t exist", server_account_doesnt_exist, "Server"),
	TEST_ONE_TAG("Server - Account exist", server_account_exist, "Server"),
	TEST_ONE_TAG("Local - Is account exist callback missing", server_is_account_exist_cb_missing, "Local"),
	TEST_ONE_TAG("Local - Is account exist arguments missing", server_is_account_exist_arg_missing, "Local"),
	TEST_ONE_TAG("Server - Account created with email", server_account_created_with_email, "Server"),
	TEST_ONE_TAG("Server - Account already create with email", server_create_account_already_create_with_email, "Server"),
	TEST_ONE_TAG("Server - Account created with phone number", server_account_created_with_phone_number, "Server"),
	TEST_ONE_TAG("Server - Account already created with phone number as account", server_create_account_already_create_as_account_with_phone_number, "Server"),
	TEST_ONE_TAG("Server - Account already created with phone number as alias", server_create_account_already_create_as_alias_with_phone_number, "Server"),
	TEST_ONE_TAG("Local - Create Account callback missing", server_create_account_cb_missing, "Local"),
	TEST_ONE_TAG("Local - Create Account with email arguments username missing", server_create_account_with_email_arg_username_missing, "Local"),
	TEST_ONE_TAG("Local - Create Account with email arguments email missing", server_create_account_with_email_arg_email_missing, "Local"),
	TEST_ONE_TAG("Local - Create Account with email arguments password missing", server_create_account_with_email_arg_password_missing, "Local"),
	TEST_ONE_TAG("Local - Create Account with phone number arguments username missing", server_create_account_with_phone_number_arg_username_missing, "Local"),
	TEST_ONE_TAG("Local - Create Account with phone number arguments phone number missing", server_create_account_with_phone_number_arg_phone_number_missing, "Local"),
	TEST_ONE_TAG("Local - Is account activated callbacks missing", server_is_account_activated_cb_missing, "Local"),
	TEST_ONE_TAG("Local - Is account activated arguments username missing", server_is_account_activated_arg_username_missing, "Local"),
	TEST_ONE_TAG("Local - Activate account callbacks missing", server_activate_account_cb_missing, "Local"),
	TEST_ONE_TAG("Local - Activate account with email arguments username missing", server_activate_account_with_email_activated_arg_username_missing, "Local"),
	TEST_ONE_TAG("Local - Activate account with email arguments activation code missing", server_activate_account_with_email_activated_arg_activation_code_missing, "Local"),
	TEST_ONE_TAG("Local - Activate account with phone number arguments username missing", server_activate_account_with_phone_number_activated_arg_username_missing, "Local"),
	TEST_ONE_TAG("Local - Activate account with phone number arguments activation code missing", server_activate_account_with_phone_number_activated_arg_activation_code_missing, "Local"),
	TEST_ONE_TAG("Local - Link account callbacks missing", server_link_account_cb_missing, "Local"),
	TEST_ONE_TAG("Local - Link account arguments username missing", server_link_account_arg_username_missing, "Local"),
	TEST_ONE_TAG("Local - Link account arguments phone number missing", server_link_account_arg_phone_number_missing, "Local"),
	TEST_ONE_TAG("Local - Activate alias callbacks missing", server_activate_alias_cb_missing, "Local"),
	TEST_ONE_TAG("Local - Activate alias arguments username missing", server_activate_alias_arg_username_missing, "Local"),
	TEST_ONE_TAG("Local - Activate alias arguments phone number missing", server_activate_alias_arg_phone_number_missing, "Local"),
	TEST_ONE_TAG("Local - Activate alias arguments activation code missing", server_activate_alias_arg_activation_code_missing, "Local"),
	TEST_ONE_TAG("Local - Activate alias arguments password missing", server_activate_alias_arg_password_missing, "Local"),
	TEST_ONE_TAG("Local - Is alias used callbacks missing", server_is_alias_used_cb_missing, "Local"),
	TEST_ONE_TAG("Local - Is alias used arguments phone number missing", server_is_alias_used_arg_phone_number_missing, "Local"),
	TEST_ONE_TAG("Local - Is account link callbacks missing", server_is_account_linked_cb_missing, "Local"),
	TEST_ONE_TAG("Local - Is account link arguments username missing", server_is_account_linked_arg_username_missing, "Local"),
	TEST_ONE_TAG("Local - Recover account with phone number callbacks missing", server_recover_account_with_phone_number_cb_missing, "Local"),
	TEST_ONE_TAG("Local - Recover account with phone number arguments phone number missing", server_recover_account_with_phone_number_arg_phone_number_missing, "Local"),
	TEST_ONE_TAG("Local - Update account password callbacks missing", server_update_account_password_cb_missing, "Local"),
	TEST_ONE_TAG("Local - Update account password arguments username missing", server_update_account_password_arg_username_missing, "Local"),
	TEST_ONE_TAG("Local - Update account password arguments phone number missing", server_update_account_password_arg_phone_number_missing, "Local"),
	TEST_ONE_TAG("Local - Update account password arguments password missing", server_update_account_password_arg_password_missing, "Local"),
	TEST_ONE_TAG("Local - Update account password arguments new password missing", server_update_account_password_arg_new_password_missing, "Local"),
};

test_suite_t account_creator_test_suite = {"Account creator", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								sizeof(account_creator_tests) / sizeof(account_creator_tests[0]), account_creator_tests};
