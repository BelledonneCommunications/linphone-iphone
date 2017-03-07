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
static const char DOMAIN_URL[] = "sip.accounttest.org";
static const char ROUTE_URL[] = "sip2.linphone.org:5072";

static const int TIMEOUT_REQUEST = 100000;

/////////// LOCAL TESTS ///////////

////// USERNAME //////
static void local_username_too_short(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_username(creator, ""),
		LinphoneAccountCreatorUsernameTooShort,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_username_too_long(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_username(creator, "usernametoolong"),
		LinphoneAccountCreatorUsernameTooLong,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_username_invalid(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
			linphone_account_creator_set_username(creator, "use!"),
			LinphoneAccountCreatorUsernameInvalid,
			LinphoneAccountCreatorStatus,
			"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_username_invalid_size(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_username(creator, "sizeinv"),
		LinphoneAccountCreatorUsernameInvalidSize,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_username_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
			linphone_account_creator_set_username(creator, "user"),
			LinphoneAccountCreatorOK,
			LinphoneAccountCreatorStatus,
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
		LinphoneAccountCreatorPasswordTooShort,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_password_too_long(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_password(creator, "passwordtoolong"),
		LinphoneAccountCreatorPasswordTooLong,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_password_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_username(creator, "pass"),
		LinphoneAccountCreatorOK,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

////// EMAIL //////

static void local_email_invalid(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test.linphone.org"),
		LinphoneAccountCreatorEmailInvalid,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_email_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test@linphone.org"),
		LinphoneAccountCreatorOK,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

////// PHONE NUMBER //////

static void local_phone_number_too_short(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, "0123", "33"),
		LinphoneAccountCreatorPhoneNumberTooShort,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_phone_number_too_long(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, "01234567891011", "33"),
		LinphoneAccountCreatorPhoneNumberTooLong,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_phone_number_invalid(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, NULL, "33"),
		LinphoneAccountCreatorPhoneNumberInvalid,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_country_code_invalid(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, "0123", ""),
		LinphoneAccountCreatorCountryCodeInvalid,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

////// TRANSPORT //////

static void local_transport_not_supported(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_transport(creator, (LinphoneTransportType)4),
		LinphoneAccountCreatorTransportNotSupported,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_transport_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_transport(creator, LinphoneTransportTcp),
		LinphoneAccountCreatorOK,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

////// DOMAIN //////

// Not implemented
/*static void local_domain_invalid(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");
	BC_ASSERT_EQUAL(
		linphone_account_creator_set_domain(creator, "sop2.linphone.org"),
		LinphoneAccountCreatorDomainInvalid,
		LinphoneAccountCreatorStatus,
		"%i");
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}*/

static void local_domain_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_domain(creator, "sip2.linphone.org"),
		LinphoneAccountCreatorOK,
		LinphoneAccountCreatorStatus,
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

static void account_creator_set_cb_done(LinphoneAccountCreatorCbs *cbs) {
	LinphoneAccountCreatorStats *stats = (LinphoneAccountCreatorStats*) linphone_account_creator_cbs_get_user_data(cbs);
	stats->cb_done++;
	BC_ASSERT_TRUE(stats->cb_done);
}

/*static void account_creator_reset_cb_done(LinphoneAccountCreatorCbs *cbs) {
	LinphoneAccountCreatorStats *stats = (LinphoneAccountCreatorStats*) linphone_account_creator_cbs_get_user_data(cbs);
	stats->cb_done = 0;
	BC_ASSERT_FALSE(stats->cb_done);
}*/

static void account_existence_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char* resp) {
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStatus expected_status = (LinphoneAccountCreatorStatus)linphone_account_creator_get_user_data(creator);
	BC_ASSERT_EQUAL(
		status,
		expected_status,
		LinphoneAccountCreatorStatus,
		"%i");
	account_creator_set_cb_done(cbs);
}

static void server_account_doesnt_exist(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_set_user_data(creator, (void*)LinphoneAccountCreatorAccountNotExist);
	linphone_account_creator_set_username(creator, "user_not_exist");
	linphone_account_creator_set_route(creator, ROUTE_URL);
	linphone_account_creator_set_domain(creator, DOMAIN_URL);
	linphone_account_creator_cbs_set_is_account_used(cbs, account_existence_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_used(creator),
		LinphoneAccountCreatorOK,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_account_exist(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_set_user_data(creator, (void*)LinphoneAccountCreatorAccountExist);
	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_route(creator, ROUTE_URL);
	linphone_account_creator_set_domain(creator, DOMAIN_URL);
	linphone_account_creator_cbs_set_is_account_used(cbs, account_existence_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_used(creator),
		LinphoneAccountCreatorOK,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

test_t account_creator_tests[] = {
	TEST_NO_TAG("Local - Username too short", local_username_too_short),
	TEST_NO_TAG("Local - Username too long", local_username_too_long),
	TEST_NO_TAG("Local - Username invalid", local_username_invalid),
	TEST_NO_TAG("Local - Username invalid size", local_username_invalid_size),
	TEST_NO_TAG("Local - Username Ok", local_username_ok),
	TEST_NO_TAG("Local - Password too short", local_password_too_short),
	TEST_NO_TAG("Local - Password too long", local_password_too_long),
	TEST_NO_TAG("Local - Password Ok", local_password_ok),
	TEST_NO_TAG("Local - Email invalid", local_email_invalid),
	TEST_NO_TAG("Local - Email Ok", local_email_ok),
	TEST_NO_TAG("Local - Phone number too short", local_phone_number_too_short),
	TEST_NO_TAG("Local - Phone number too long", local_phone_number_too_long),
	TEST_NO_TAG("Local - Phone number invalid", local_phone_number_invalid),
	TEST_NO_TAG("Local - Country code invalid", local_country_code_invalid),
	TEST_NO_TAG("Local - Transport not supported", local_transport_not_supported),
	TEST_NO_TAG("Local - Transport Ok", local_transport_ok),
	//TEST_NO_TAG("Local - Domain invalid", local_domain_invalid), Not implemented
	TEST_NO_TAG("Local - Domain ok", local_domain_ok),
	TEST_NO_TAG("Server - Account doesn\'t exist", server_account_doesnt_exist),
	TEST_NO_TAG("Server - Account exist", server_account_exist),
};

test_suite_t account_creator_test_suite = {"Account creator", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								sizeof(account_creator_tests) / sizeof(account_creator_tests[0]), account_creator_tests};
