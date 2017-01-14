/*
	liblinphone_tester - liblinphone test suite
	Copyright (C) 2013  Belledonne Communications SARL

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

#include <stdlib.h>

const char* phone_normalization(LinphoneProxyConfig *proxy, const char* in) {
	static char result[255];
	char * output = linphone_proxy_config_normalize_phone_number(proxy, in);
	if (output) {
		memcpy(result, output, strlen(output)+1);
		ms_free(output);
		return result;
	} else {
		return NULL;
	}
}

static void phone_normalization_without_proxy(void) {
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "012 345 6789"), "0123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "+33123456789"), "+33123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "+33012345678"), "+33012345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "+33 0012345678"), "+33012345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "+33012345678"), "+33012345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "+3301234567891"), "+33234567891");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "+33 01234567891"), "+33234567891");
	BC_ASSERT_PTR_NULL(phone_normalization(NULL, "I_AM_NOT_A_NUMBER")); // invalid phone number

	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "0"), "0");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "01"), "01");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "012"), "012");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "0123"), "0123");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "01234"), "01234");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "012345"), "012345");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "0123456"), "0123456");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "01234567"), "01234567");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "012345678"), "012345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "0123456789"), "0123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(NULL, "01234567890"), "01234567890");
}

static void phone_normalization_with_proxy(void) {
	LinphoneProxyConfig *proxy = linphone_proxy_config_new();
	linphone_proxy_config_set_dial_prefix(proxy, "33");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "012 3456 789"), "+33123456789");
	linphone_proxy_config_set_dial_prefix(proxy, NULL);
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+33123456789"), "+33123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+33 0123456789"), "+33123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+330012345678"), "+33012345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+3301 2345678"), "+33012345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+3301234567891"), "+33234567891");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+33 (0) 1 23 45 67 89"), "+33123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+90 (903) 1234567"), "+909031234567");

	linphone_proxy_config_set_dial_prefix(proxy, "33");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "123456789"), "+33123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, " 0123456789"), "+33123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0012345678"), "+12345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01 2345678"), "+33012345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01234567891"), "+33234567891"); // invalid phone number (too long)
	BC_ASSERT_PTR_NULL(phone_normalization(proxy, "I_AM_NOT_A_NUMBER")); // invalid phone number

	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+990012345678"), "+990012345678");

	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0952636505"), "+33952636505");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "09 52 63 65 05"), "+33952636505");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "09-52-63-65-05"), "+33952636505");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+31952636505"), "+31952636505");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0033952636505"), "+33952636505");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0033952636505"), "+33952636505");
	BC_ASSERT_PTR_NULL(phone_normalization(proxy, "toto"));


	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0"), "+330");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01"), "+3301");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "012"), "+33012");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0123"), "+330123");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01234"), "+3301234");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "012345"), "+33012345");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0123456"), "+330123456");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01234567"), "+3301234567");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "012345678"), "+33012345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0123456789"), "+33123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01234567890"), "+33234567890");

	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+330"), "+330");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+3301"), "+3301");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+33012"), "+33012");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+330123"), "+330123");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+3301234"), "+3301234");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+33012345"), "+33012345");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+330123456"), "+330123456");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+3301234567"), "+3301234567");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+33012345678"), "+33012345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+330123456789"), "+33123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+3301234567890"), "+33234567890");

	// invalid prefix - use a generic dialplan with 10 max length
	linphone_proxy_config_set_dial_prefix(proxy, "99");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0012345678"), "+12345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0"), "+990");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01"), "+9901");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "012"), "+99012");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0123"), "+990123");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01234"), "+9901234");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "012345"), "+99012345");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0123456"), "+990123456");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01234567"), "+9901234567");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "012345678"), "+99012345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0123456789"), "+990123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01234567890"), "+991234567890");

	linphone_proxy_config_destroy(proxy);
}

static void phone_normalization_with_dial_escape_plus(void){
	LinphoneProxyConfig *proxy = linphone_proxy_config_new();
	linphone_proxy_config_set_dial_prefix(proxy, "33");
	linphone_proxy_config_set_dial_escape_plus(proxy, TRUE);
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0033952636505"), "0033952636505");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0952636505"), "0033952636505");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+34952636505"), "0034952636505");

	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0"), "00330");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01"), "003301");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "012"), "0033012");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0123"), "00330123");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01234"), "003301234");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "012345"), "0033012345");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0123456"), "00330123456");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01234567"), "003301234567");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "012345678"), "0033012345678");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0123456789"), "0033123456789");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "01234567890"), "0033234567890");


	linphone_proxy_config_destroy(proxy);
}

#define SIP_URI_CHECK(actual, expected) { \
		LinphoneProxyConfig *proxy = linphone_proxy_config_new(); \
		LinphoneAddress* res;\
		char* actual_str;\
		linphone_proxy_config_set_identity(proxy, "sip:username@linphone.org"); \
		res  = linphone_proxy_config_normalize_sip_uri(proxy, actual); \
		actual_str = linphone_address_as_string_uri_only(res); \
		BC_ASSERT_STRING_EQUAL(actual_str, expected); \
		ms_free(actual_str); \
		linphone_address_unref(res); \
		linphone_proxy_config_destroy(proxy); \
	}


static void sip_uri_normalization(void) {
	char* expected ="sip:%d9%a1@linphone.org";
	BC_ASSERT_PTR_NULL(linphone_proxy_config_normalize_sip_uri(NULL, "test"));
	SIP_URI_CHECK("test@linphone.org", "sip:test@linphone.org");
	SIP_URI_CHECK("test@linphone.org;transport=tls", "sip:test@linphone.org;transport=tls");

	SIP_URI_CHECK("١", expected); //test that no more invalid memory writes are made (valgrind only)
}

test_t proxy_config_tests[] = {
	TEST_NO_TAG("Phone normalization without proxy", phone_normalization_without_proxy),
	TEST_NO_TAG("Phone normalization with proxy", phone_normalization_with_proxy),
	TEST_NO_TAG("Phone normalization with dial escape plus", phone_normalization_with_dial_escape_plus),
	TEST_NO_TAG("SIP URI normalization", sip_uri_normalization)
};

test_suite_t proxy_config_test_suite = {"Proxy config", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
										sizeof(proxy_config_tests) / sizeof(proxy_config_tests[0]), proxy_config_tests};
