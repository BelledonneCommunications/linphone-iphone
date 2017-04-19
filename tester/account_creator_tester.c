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
#include "private.h"

static const char XMLRPC_URL[] = "https://sip2.linphone.org:446/xmlrpc.php";

static const int TIMEOUT_REQUEST = 10000;

/////////// INIT //////////////

static void init_linphone_account_creator_service(LinphoneCore *lc) {
	LinphoneAccountCreatorService *service = linphone_account_creator_service_new();
	linphone_account_creator_service_set_constructor_cb(service, NULL);
	linphone_account_creator_service_set_destructor_cb(service, NULL);
	linphone_account_creator_service_set_create_account_cb(service, linphone_account_creator_create_account_linphone);
	linphone_account_creator_service_set_is_account_exist_cb(service, linphone_account_creator_is_account_exist_linphone);
	linphone_account_creator_service_set_activate_account_cb(service, linphone_account_creator_activate_account_linphone);
	linphone_account_creator_service_set_is_account_activated_cb(service, linphone_account_creator_is_account_activated_linphone);
	linphone_account_creator_service_set_link_account_cb(service, linphone_account_creator_link_phone_number_with_account_linphone);
	linphone_account_creator_service_set_activate_alias_cb(service, linphone_account_creator_activate_phone_number_link_linphone);
	linphone_account_creator_service_set_is_alias_used_cb(service, linphone_account_creator_is_phone_number_used_linphone);
	linphone_account_creator_service_set_is_account_linked_cb(service, linphone_account_creator_is_account_linked_linphone);
	linphone_account_creator_service_set_recover_account_cb(service, linphone_account_creator_recover_phone_account_linphone);
	linphone_account_creator_service_set_update_account_cb(service, linphone_account_creator_update_password_linphone);
	linphone_core_set_account_creator_service(lc, service);
}

static LinphoneAccountCreator * _linphone_account_creator_new(LinphoneCore *lc, const char * url) {
	init_linphone_account_creator_service(lc);
	LinphoneAccountCreator *creator = linphone_account_creator_new(lc, url);
	return creator;
}

/////////// LOCAL TESTS ///////////

////// USERNAME //////
static void local_username_too_short(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_username(creator, ""),
		LinphoneAccountCreatorUsernameStatusTooShort,
		LinphoneAccountCreatorUsernameStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_username_too_long(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_username(creator, "usernametoolongforyou"),
		LinphoneAccountCreatorUsernameStatusTooLong,
		LinphoneAccountCreatorUsernameStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_username_invalid_character(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
			linphone_account_creator_set_username(creator, "use!"),
			LinphoneAccountCreatorUsernameStatusInvalidCharacters,
			LinphoneAccountCreatorUsernameStatus,
			"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_username_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
			linphone_account_creator_set_username(creator, "XXXTESTuser_1"),
			LinphoneAccountCreatorUsernameStatusOk,
			LinphoneAccountCreatorUsernameStatus,
			"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

////// PASSWORD //////

static void local_password_too_short(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_password(creator, ""),
		LinphoneAccountCreatorPasswordStatusTooShort,
		LinphoneAccountCreatorPasswordStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_password_too_long(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_password(creator, "passwordtoolong"),
		LinphoneAccountCreatorPasswordStatusTooLong,
		LinphoneAccountCreatorPasswordStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_password_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_password(creator, "pass"),
		LinphoneAccountCreatorPasswordStatusOk,
		LinphoneAccountCreatorPasswordStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

////// EMAIL //////

static void local_email_malformed(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test.linphone.org"),
		LinphoneAccountCreatorEmailStatusMalformed,
		LinphoneAccountCreatorEmailStatus,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test@linphone"),
		LinphoneAccountCreatorEmailStatusMalformed,
		LinphoneAccountCreatorEmailStatus,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "@linphone.org"),
		LinphoneAccountCreatorEmailStatusMalformed,
		LinphoneAccountCreatorEmailStatus,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "linphone@.org"),
		LinphoneAccountCreatorEmailStatusMalformed,
		LinphoneAccountCreatorEmailStatus,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, ".linphone@.org"),
		LinphoneAccountCreatorEmailStatusMalformed,
		LinphoneAccountCreatorEmailStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_email_invalid_character(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test@linphone.org$"),
		LinphoneAccountCreatorEmailStatusInvalidCharacters,
		LinphoneAccountCreatorEmailStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_email_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test@linphone.org"),
		LinphoneAccountCreatorEmailStatusOk,
		LinphoneAccountCreatorEmailStatus,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "test02@linphone5252.org"),
		LinphoneAccountCreatorEmailStatusOk,
		LinphoneAccountCreatorEmailStatus,
		"%i");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_email(creator, "9053test@50255linphone.org"),
		LinphoneAccountCreatorEmailStatusOk,
		LinphoneAccountCreatorEmailStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

////// PHONE NUMBER //////

static void local_phone_number_too_short(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, "0123", "33")&LinphoneAccountCreatorPhoneNumberStatusTooShort,
		LinphoneAccountCreatorPhoneNumberStatusTooShort,
		LinphoneAccountCreatorPhoneNumberStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_phone_number_too_long(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, "01234567891011", "33")&LinphoneAccountCreatorPhoneNumberStatusTooLong,
		LinphoneAccountCreatorPhoneNumberStatusTooLong,
		LinphoneAccountCreatorPhoneNumberStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_phone_number_invalid(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, NULL, "33")&LinphoneAccountCreatorPhoneNumberStatusInvalid,
		LinphoneAccountCreatorPhoneNumberStatusInvalid,
		LinphoneAccountCreatorPhoneNumberStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_country_code_invalid(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, "0123", "")&LinphoneAccountCreatorPhoneNumberStatusInvalidCountryCode,
		LinphoneAccountCreatorPhoneNumberStatusInvalidCountryCode,
		LinphoneAccountCreatorPhoneNumberStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void local_phone_number_ok(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, "");

	BC_ASSERT_EQUAL(
		linphone_account_creator_set_phone_number(creator, "000555455", "1")&LinphoneAccountCreatorPhoneNumberStatusOk,
		LinphoneAccountCreatorPhoneNumberStatusOk,
		LinphoneAccountCreatorPhoneNumberStatus,
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

static void account_creator_reset_cb_done(LinphoneAccountCreatorCbs *cbs) {
	LinphoneAccountCreatorStats *stats = (LinphoneAccountCreatorStats*) linphone_account_creator_cbs_get_user_data(cbs);
	stats->cb_done = 0;
	BC_ASSERT_FALSE(stats->cb_done);
}

static void account_creator_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char* resp) {
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStatus expected_status = (LinphoneAccountCreatorStatus)linphone_account_creator_service_get_user_data(
		linphone_account_creator_get_service(creator));
	BC_ASSERT_EQUAL(
		status,
		expected_status,
		LinphoneAccountCreatorStatus,
		"%i");
	account_creator_set_cb_done(cbs);
}

static void _get_activation_code_cb(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
	const char* resp = linphone_xml_rpc_request_get_string_response(request);
	if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
		if (strstr(resp, "ERROR_") == resp) {
			status = LinphoneAccountCreatorStatusRequestFailed;
		} else {
			status = LinphoneAccountCreatorStatusRequestOk;
			set_string(&creator->activation_code, resp, FALSE);
		}
	}
	account_creator_cb(creator, status, resp);
}

LinphoneAccountCreatorStatus get_activation_code_account_cb(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if ((!creator->username && !creator->phone_number) || !creator->password) {
		return LinphoneAccountCreatorStatusMissingArguments;
	}

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "get_confirmation_key",
			LinphoneXmlRpcArgString, creator->username ? creator->username : creator->phone_number,
			LinphoneXmlRpcArgString, creator->password,
			LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
			LinphoneXmlRpcArgNone);

	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _get_activation_code_cb);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);

	return LinphoneAccountCreatorStatusRequestOk;
}

static void _delete_account_cb(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
	const char* resp = linphone_xml_rpc_request_get_string_response(request);
	if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
		// We want to delete account from table but it's not an error if it doesn't exist or password doesn't match
		if (strstr(resp, "ERROR_ACCOUNT_DOESNT_EXIST") == resp
			|| strstr(resp, "ERROR_PASSWORD_DOESNT_MATCH") == resp) {
			status = LinphoneAccountCreatorStatusRequestOk;
		} else if (strstr(resp, "ERROR_") == resp) {
			status = LinphoneAccountCreatorStatusRequestFailed;
		} else {
			status = LinphoneAccountCreatorStatusRequestOk;
		}
	}
	account_creator_cb(creator, status, resp);
}

LinphoneAccountCreatorStatus delete_account_cb(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if ((!creator->username && !creator->phone_number) || !creator->password) {
		return LinphoneAccountCreatorStatusMissingArguments;
	}

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "delete_account",
			LinphoneXmlRpcArgString, creator->username ? creator->username : creator->phone_number,
			LinphoneXmlRpcArgString, creator->password,
			LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
			LinphoneXmlRpcArgNone);

	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _delete_account_cb);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);

	return LinphoneAccountCreatorStatusRequestOk;
}

static void get_activation_code(LinphoneAccountCreator *creator, int *cb_done) {
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusRequestOk);

	BC_ASSERT_EQUAL(
		get_activation_code_account_cb(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(creator->core, NULL, cb_done, 1, TIMEOUT_REQUEST);
}

static void server_delete_account_test(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusRequestOk);
	linphone_account_creator_set_email(creator, "user_2@linphone.org");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_phone_number(creator, "000555455","1");

	BC_ASSERT_EQUAL(
		delete_account_cb(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	linphone_account_creator_unref(creator);

	// First attempt with the first password
	creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	cbs = linphone_account_creator_get_callbacks(creator);

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	account_creator_reset_cb_done(cbs);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusRequestOk);
	linphone_account_creator_set_username(creator, "XXXTESTuser_1");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");

	BC_ASSERT_EQUAL(
		delete_account_cb(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	linphone_account_creator_unref(creator);

	// Second attempt with the second password
	creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	cbs = linphone_account_creator_get_callbacks(creator);

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	account_creator_reset_cb_done(cbs);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusRequestOk);
	linphone_account_creator_set_username(creator, "XXXTESTuser_1");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "newpassword");

	BC_ASSERT_EQUAL(
		delete_account_cb(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	linphone_account_creator_unref(creator);

	creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	cbs = linphone_account_creator_get_callbacks(creator);

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	account_creator_reset_cb_done(cbs);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusRequestOk);
	linphone_account_creator_set_username(creator, "XXXTESTuser_3");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");

	BC_ASSERT_EQUAL(
		delete_account_cb(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

/****************** Start Is Account Exist ************************/
static void server_account_doesnt_exist(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountNotExist);
	linphone_account_creator_set_username(creator, "user_not_exist");
	linphone_account_creator_cbs_set_is_account_exist(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_exist(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_account_exist(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountExist);
	linphone_account_creator_set_username(creator, "XXXTESTuser_1");
	linphone_account_creator_cbs_set_is_account_exist(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_exist(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_account_exist_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_exist(creator),
		LinphoneAccountCreatorStatusMissingCallbacks,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_account_exist_arg_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_is_account_exist(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_exist(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Is Account Exist ************************/

/****************** Start Create Account ************************/
static void server_account_created_with_email(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountCreated);
	linphone_account_creator_set_username(creator, "XXXTESTuser_1");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_cbs_set_create_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_already_create_with_email(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountExist);
	linphone_account_creator_set_username(creator, "XXXTESTuser_1");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_cbs_set_create_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_account_created_with_phone_number(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountCreated);
	linphone_account_creator_set_email(creator, "user_2@linphone.org");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_phone_number(creator, "000555455","1");
	linphone_account_creator_cbs_set_create_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_already_create_as_account_with_phone_number(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountExist);
	linphone_account_creator_set_email(creator, "user_2@linphone.org");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_phone_number(creator, "000555455","1");
	linphone_account_creator_cbs_set_create_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_already_create_as_alias_with_phone_number(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountExistWithAlias);
	linphone_account_creator_set_username(creator, "XXXTESTuser_3");
	linphone_account_creator_set_email(creator, "user_2@linphone.org");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_phone_number(creator, "000555456","1");
	linphone_account_creator_cbs_set_create_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneAccountCreatorStatusMissingCallbacks,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_with_email_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_create_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_with_email_arg_email_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_create_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_with_email_arg_password_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_create_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_with_phone_number_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_create_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_create_account_with_phone_number_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_create_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_create_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Create Account ************************/

/****************** Start Is Account Activated ************************/
static void server_account_not_activated(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "XXXTESTuser_1");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountNotActivated);
	linphone_account_creator_cbs_set_is_account_activated(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_activated(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_account_already_activated(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "XXXTESTuser_1");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountActivated);
	linphone_account_creator_cbs_set_is_account_activated(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_activated(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_account_activated_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_activated(creator),
		LinphoneAccountCreatorStatusMissingCallbacks,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_account_activated_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_is_account_activated(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_activated(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Is Account Activated ************************/

/****************** Start Activate Account ************************/
static void server_activate_account_not_activated(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_set_username(creator, "XXXTESTuser_1");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");
	get_activation_code(creator, &stats->cb_done);
	account_creator_reset_cb_done(creator->cbs);

	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountActivated);
	linphone_account_creator_cbs_set_activate_account(cbs, account_creator_cb);

	linphone_account_creator_service_set_activate_account_cb(
		linphone_account_creator_get_service(creator),
		linphone_account_creator_activate_email_account_linphone);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_account_already_activated(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_set_username(creator, "XXXTESTuser_1");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	linphone_account_creator_set_password(creator, "password");
	get_activation_code(creator, &stats->cb_done);
	account_creator_reset_cb_done(creator->cbs);

	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountAlreadyActivated);
	linphone_account_creator_cbs_set_activate_account(cbs, account_creator_cb);
	linphone_account_creator_service_set_activate_account_cb(
		linphone_account_creator_get_service(creator),
		linphone_account_creator_activate_email_account_linphone);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_non_existent_account(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "unknown_user");
	linphone_account_creator_set_activation_code(creator, "58c9");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountNotActivated);
	linphone_account_creator_cbs_set_activate_account(cbs, account_creator_cb);
	linphone_account_creator_service_set_activate_account_cb(
		linphone_account_creator_get_service(creator),
		linphone_account_creator_activate_email_account_linphone);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_account_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneAccountCreatorStatusMissingCallbacks,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_account_with_email_activated_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_activation_code(creator, "123456789");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_activate_account(cbs, account_creator_cb);
	linphone_account_creator_service_set_activate_account_cb(
		linphone_account_creator_get_service(creator),
		linphone_account_creator_activate_email_account_linphone);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_account_with_email_activated_arg_activation_code_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_activate_account(cbs, account_creator_cb);
	linphone_account_creator_service_set_activate_account_cb(
		linphone_account_creator_get_service(creator),
		linphone_account_creator_activate_email_account_linphone);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_account_with_phone_number_activated_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_activation_code(creator, "123456789");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_activate_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_account_with_phone_number_activated_arg_activation_code_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_activate_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Activate Account ************************/

/****************** Start Link Account ************************/
static void server_link_account_with_phone_number(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "XXXTESTuser_1");
	linphone_account_creator_set_phone_number(creator, "000555456", "1");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusRequestOk);
	linphone_account_creator_cbs_set_link_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_link_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_link_non_existent_account_with_phone_number(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "unknown_user");
	linphone_account_creator_set_phone_number(creator, "012345678", "33");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountNotLinked);
	linphone_account_creator_cbs_set_link_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_link_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_link_account_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");

	BC_ASSERT_EQUAL(
		linphone_account_creator_link_account(creator),
		LinphoneAccountCreatorStatusMissingCallbacks,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_link_account_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "0123456", "33");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_link_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_link_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_link_account_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_link_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_link_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Link Account ************************/

/****************** Start Activate Alias ************************/
static void server_activate_phone_number_for_non_existent_account(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "unknown_user");
	linphone_account_creator_set_phone_number(creator, "012345678", "33");
	linphone_account_creator_set_activation_code(creator, "12345679");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountNotActivated);
	linphone_account_creator_cbs_set_activate_alias(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_phone_number_for_account(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_set_username(creator, "XXXTESTuser_1");
	linphone_account_creator_set_phone_number(creator, "000555456", "1");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_email(creator, "user_1@linphone.org");
	get_activation_code(creator, &stats->cb_done);
	account_creator_reset_cb_done(creator->cbs);

	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountActivated);
	linphone_account_creator_cbs_set_activate_alias(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_alias_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "012345678", "33");
	linphone_account_creator_set_activation_code(creator, "12345679");
	linphone_account_creator_set_password(creator, "password");

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneAccountCreatorStatusMissingCallbacks,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_alias_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "012345678", "33");
	linphone_account_creator_set_activation_code(creator, "12345679");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_activate_alias(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_alias_arg_activation_code_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "012345678", "33");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_activate_alias(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_alias_arg_password_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "012345678", "33");
	linphone_account_creator_set_activation_code(creator, "12345679");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_activate_alias(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_activate_alias_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_activation_code(creator, "12345679");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_activate_alias(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_activate_alias(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Activate Alias ************************/

/****************** Start Is Alias Used ************************/
static void server_phone_number_is_used_as_alias(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "000555456", "1");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAliasExist);
	linphone_account_creator_cbs_set_is_alias_used(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_alias_used(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_phone_number_is_used_as_account(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "000555455", "1");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAliasIsAccount);
	linphone_account_creator_cbs_set_is_alias_used(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_alias_used(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_phone_number_not_used(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "012345678", "33");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAliasNotExist);
	linphone_account_creator_cbs_set_is_alias_used(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_alias_used(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_alias_used_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_phone_number(creator, "012345678", "33");

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_alias_used(creator),
		LinphoneAccountCreatorStatusMissingCallbacks,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_alias_used_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_is_alias_used(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_alias_used(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Is Alias Used ************************/

/****************** Start Is Account Linked ************************/
static void server_account_link_with_phone_number(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "XXXTESTuser_1");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountLinked);
	linphone_account_creator_cbs_set_is_account_linked(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_linked(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_account_not_link_with_phone_number(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "XXXTESTuser_1");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountNotLinked);
	linphone_account_creator_cbs_set_is_account_linked(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_linked(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_account_linked_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_linked(creator),
		LinphoneAccountCreatorStatusMissingCallbacks,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_is_account_linked_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_is_account_linked(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_is_account_linked(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Is Account Linked ************************/

/****************** Start Recover Account ************************/
static void server_recover_account_with_phone_number_used(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "000555456", "1");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusRequestOk);
	linphone_account_creator_cbs_set_recover_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_recover_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_recover_account_with_phone_number_not_used(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "012345678", "33");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountNotExist);
	linphone_account_creator_cbs_set_recover_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_recover_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_recover_account_with_phone_number_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_phone_number(creator, "0123456", "33");

	BC_ASSERT_EQUAL(
		linphone_account_creator_recover_account(creator),
		LinphoneAccountCreatorStatusMissingCallbacks,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_recover_account_with_phone_number_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);
	linphone_account_creator_cbs_set_recover_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_recover_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Recover Account ************************/

/****************** Start Update Account ************************/
static void server_update_account_password_with_wrong_password(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "XXXTESTuser_1");
	linphone_account_creator_set_password(creator, "pssword");
	linphone_account_creator_set_user_data(creator, "newpassword");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusAccountNotExist);
	linphone_account_creator_cbs_set_update_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_update_account_password_with_correct_password(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "XXXTESTuser_1");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_user_data(creator, "newpassword");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusRequestOk);
	linphone_account_creator_cbs_set_update_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_update_account_password_for_non_existent_account(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "unknown_user");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_user_data(creator, "newpassword");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusServerError);
	linphone_account_creator_cbs_set_update_account(cbs, account_creator_cb);

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneAccountCreatorStatusRequestOk,
		LinphoneAccountCreatorStatus,
		"%i");

	wait_for_until(marie->lc, NULL, &stats->cb_done, 1, TIMEOUT_REQUEST);

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_update_account_password_cb_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_user_data(creator, "newpassword");

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneAccountCreatorStatusMissingCallbacks,
		LinphoneAccountCreatorStatus,
		"%i");

	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_update_account_password_arg_username_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_user_data(creator, "newpassword");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_cbs_set_update_account(cbs, account_creator_cb);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_update_account_password_arg_phone_number_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_password(creator, "password");
	linphone_account_creator_set_user_data(creator, "newpassword");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_cbs_set_update_account(cbs, account_creator_cb);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_update_account_password_arg_password_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_user_data(creator, "newpassword");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_cbs_set_update_account(cbs, account_creator_cb);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}

static void server_update_account_password_arg_new_password_missing(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new2("account_creator_rc", 0);
	LinphoneAccountCreator *creator = _linphone_account_creator_new(marie->lc, XMLRPC_URL);
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	LinphoneAccountCreatorStats *stats = new_linphone_account_creator_stats();

	linphone_account_creator_set_username(creator, "user_exist");
	linphone_account_creator_set_phone_number(creator, "0123456", "33");
	linphone_account_creator_set_password(creator, "password");

	linphone_account_creator_cbs_set_user_data(cbs, stats);
	linphone_account_creator_cbs_set_update_account(cbs, account_creator_cb);
	linphone_account_creator_service_set_user_data(
		linphone_account_creator_get_service(creator),
		(void*)LinphoneAccountCreatorStatusMissingArguments);

	BC_ASSERT_EQUAL(
		linphone_account_creator_update_account(creator),
		LinphoneAccountCreatorStatusMissingArguments,
		LinphoneAccountCreatorStatus,
		"%i");

	ms_free(stats);
	linphone_account_creator_unref(creator);
	linphone_core_manager_destroy(marie);
}
/****************** End Update Account ************************/

test_t account_creator_tests[] = {
	TEST_ONE_TAG(
		"Local - Username too short",
		local_username_too_short,
		"Local"),
	TEST_ONE_TAG(
		"Local - Username too long",
		local_username_too_long,
		"Local"),
	TEST_ONE_TAG(
		"Local - Username invalid character",
		local_username_invalid_character,
		"Local"),
	TEST_ONE_TAG(
		"Local - Username Ok",
		local_username_ok,
		"Local"),
	TEST_ONE_TAG(
		"Local - Password too short",
		local_password_too_short,
		"Local"),
	TEST_ONE_TAG(
		"Local - Password too long",
		local_password_too_long,
		"Local"),
	TEST_ONE_TAG(
		"Local - Password Ok",
		local_password_ok,
		"Local"),
	TEST_ONE_TAG(
		"Local - Email malformed",
		local_email_malformed,
		"Local"),
	TEST_ONE_TAG(
		"Local - Email invalid character",
		local_email_invalid_character,
		"Local"),
	TEST_ONE_TAG(
		"Local - Email Ok",
		local_email_ok,
		"Local"),
	TEST_ONE_TAG(
		"Local - Phone number too short",
		local_phone_number_too_short,
		"Local"),
	TEST_ONE_TAG(
		"Local - Phone number too long",
		local_phone_number_too_long,
		"Local"),
	TEST_ONE_TAG(
		"Local - Phone number invalid",
		local_phone_number_invalid,
		"Local"),
	TEST_ONE_TAG(
		"Local - Country code invalid",
		local_country_code_invalid,
		"Local"),
	TEST_ONE_TAG(
		"Local - Phone number ok",
		local_phone_number_ok,
		"Local"),
	TEST_ONE_TAG(
		"Local - Is account exist callback missing",
		server_is_account_exist_cb_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Is account exist arguments missing",
		server_is_account_exist_arg_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Create Account callback missing",
		server_create_account_cb_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Create Account with email arguments username missing",
		server_create_account_with_email_arg_username_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Create Account with email arguments email missing",
		server_create_account_with_email_arg_email_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Create Account with email arguments password missing",
		server_create_account_with_email_arg_password_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Create Account with phone number arguments username missing",
		server_create_account_with_phone_number_arg_username_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Create Account with phone number arguments phone number missing",
		server_create_account_with_phone_number_arg_phone_number_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Is account activated callbacks missing",
		server_is_account_activated_cb_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Is account activated arguments username missing",
		server_is_account_activated_arg_username_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Activate account callbacks missing",
		server_activate_account_cb_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Activate account with email arguments username missing",
		server_activate_account_with_email_activated_arg_username_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Activate account with email arguments activation code missing",
		server_activate_account_with_email_activated_arg_activation_code_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Activate account with phone number arguments username missing",
		server_activate_account_with_phone_number_activated_arg_username_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Activate account with phone number arguments activation code missing",
		server_activate_account_with_phone_number_activated_arg_activation_code_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Link account callbacks missing",
		server_link_account_cb_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Link account arguments username missing",
		server_link_account_arg_username_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Link account arguments phone number missing",
		server_link_account_arg_phone_number_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Activate alias callbacks missing",
		server_activate_alias_cb_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Activate alias arguments username missing",
		server_activate_alias_arg_username_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Activate alias arguments phone number missing",
		server_activate_alias_arg_phone_number_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Activate alias arguments activation code missing",
		server_activate_alias_arg_activation_code_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Activate alias arguments password missing",
		server_activate_alias_arg_password_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Is alias used callbacks missing",
		server_is_alias_used_cb_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Is alias used arguments phone number missing",
		server_is_alias_used_arg_phone_number_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Is account link callbacks missing",
		server_is_account_linked_cb_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Is account link arguments username missing",
		server_is_account_linked_arg_username_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Recover account with phone number callbacks missing",
		server_recover_account_with_phone_number_cb_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Recover account with phone number arguments phone number missing",
		server_recover_account_with_phone_number_arg_phone_number_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Update account password callbacks missing",
		server_update_account_password_cb_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Update account password arguments username missing",
		server_update_account_password_arg_username_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Update account password arguments phone number missing",
		server_update_account_password_arg_phone_number_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Update account password arguments password missing",
		server_update_account_password_arg_password_missing,
		"Local"),
	TEST_ONE_TAG(
		"Local - Update account password arguments new password missing",
		server_update_account_password_arg_new_password_missing,
		"Local"),
		/* These tests must be carried in a specific order */
	TEST_ONE_TAG(
		"Server - Delete accounts test",
		server_delete_account_test,
		"Server"),
	TEST_ONE_TAG(
		"Server - Account doesn\'t exist",
		server_account_doesnt_exist,
		"Server"),
	TEST_ONE_TAG(
		"Server - Activate a non existent account",
		server_activate_non_existent_account,
		"Server"),
	TEST_ONE_TAG(
		"Server - Activate phone number for a non existent account",
		server_activate_phone_number_for_non_existent_account,
		"Server"),
	TEST_ONE_TAG(
		"Server - Phone number not used",
		server_phone_number_not_used,
		"Server"),
	TEST_ONE_TAG(
		"Server - Update account password for a non existent account",
		server_update_account_password_for_non_existent_account,
		"Server"),
	TEST_ONE_TAG(
		"Server - Recover account with phone number not used",
		server_recover_account_with_phone_number_not_used,
		"Server"),
	TEST_ONE_TAG(
		"Server - Link a non existent account with phone number",
		server_link_non_existent_account_with_phone_number,
		"Server"),
	TEST_ONE_TAG(
		"Server - Account created with email",
		server_account_created_with_email,
		"Server"),
	TEST_ONE_TAG(
		"Server - Account exist",
		server_account_exist,
		"Server"),
	TEST_ONE_TAG(
		"Server - Account already create with email",
		server_create_account_already_create_with_email,
		"Server"),
	TEST_ONE_TAG(
		"Server - Account created with phone number",
		server_account_created_with_phone_number,
		"Server"),
	TEST_ONE_TAG(
		"Server - Account not activated",
		server_account_not_activated,
		"Server"),
	TEST_ONE_TAG(
		"Server - Account already created with phone number as account",
		server_create_account_already_create_as_account_with_phone_number,
		"Server"),
	TEST_ONE_TAG(
		"Server - Phone number is used as account",
		server_phone_number_is_used_as_account,
		"Server"),
	TEST_ONE_TAG(
		"Server - Account not link with phone number",
		server_account_not_link_with_phone_number,
		"Server"),
	TEST_ONE_TAG(
		"Server - Activate account",
		server_activate_account_not_activated,
		"Server"),
	TEST_ONE_TAG(
		"Server - Account already activated",
		server_account_already_activated,
		"Server"),
	TEST_ONE_TAG(
		"Server - Activate account already activated",
		server_activate_account_already_activated,
		"Server"),
	TEST_ONE_TAG(
		"Server - Link account with phone number",
		server_link_account_with_phone_number,
		"Server"),
	TEST_ONE_TAG(
		"Server - Activate phone number for an account",
		server_activate_phone_number_for_account,
		"Server"),
	TEST_ONE_TAG(
		"Server - Account already created with phone number as alias",
		server_create_account_already_create_as_alias_with_phone_number,
		"Server"),
	TEST_ONE_TAG(
		"Server - Phone number is used as alias",
		server_phone_number_is_used_as_alias,
		"Server"),
	TEST_ONE_TAG(
		"Server - Account link with phone number",
		server_account_link_with_phone_number,
		"Server"),
	TEST_ONE_TAG(
		"Server - Update account password with wrong password",
		server_update_account_password_with_wrong_password,
		"Server"),
	TEST_ONE_TAG(
		"Server - Update account password with correct password",
		server_update_account_password_with_correct_password,
		"Server"),
	TEST_ONE_TAG(
		"Server - Recover account with phone number used",
		server_recover_account_with_phone_number_used,
		"Server"),
};

test_suite_t account_creator_test_suite = {
	"Account creator",
	NULL,
	NULL,
	liblinphone_tester_before_each,
	liblinphone_tester_after_each,
	sizeof(account_creator_tests) / sizeof(account_creator_tests[0]),
	account_creator_tests};
