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
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "0033123456789"), "0033123456789");

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

	// Phone normalization for mexican dial plans
	linphone_proxy_config_set_dial_prefix(proxy, "52");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+5217227718184"), "+5217227718184"); /*this is a mobile phone number */
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+528127718184"), "+528127718184"); /*this is a landline phone number from Monterrey*/
	
	// Phone normalization for myanmar dial plans
	linphone_proxy_config_set_dial_prefix(proxy, "95");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "9965066691"), "+959965066691");
	
	// Phone normalization for cameroon dial plans
	linphone_proxy_config_set_dial_prefix(proxy, "237");
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "674788175"), "+237674788175");
	
	linphone_proxy_config_unref(proxy);
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

	linphone_proxy_config_set_dial_escape_plus(proxy, FALSE);
	BC_ASSERT_STRING_EQUAL(phone_normalization(proxy, "+34952636505"), "+34952636505");

	linphone_proxy_config_unref(proxy);
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
		linphone_proxy_config_unref(proxy); \
	}


static void sip_uri_normalization(void) {
	char* expected ="sip:%d9%a1@linphone.org";
	BC_ASSERT_PTR_NULL(linphone_proxy_config_normalize_sip_uri(NULL, "test"));
	SIP_URI_CHECK("test@linphone.org", "sip:test@linphone.org");
	SIP_URI_CHECK("test@linphone.org;transport=tls", "sip:test@linphone.org;transport=tls");

	SIP_URI_CHECK("١", expected); //test that no more invalid memory writes are made (valgrind only)
}

static void load_dynamic_proxy_config(void) {
	LinphoneCoreManager *lauriane = linphone_core_manager_new(NULL);
	LinphoneProxyConfig *proxy;
	LinphoneAddress *read, *expected;
	LinphoneNatPolicy *nat_policy;
	const char* config =	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
							"<config xmlns=\"http://www.linphone.org/xsds/lpconfig.xsd\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.linphone.org/xsds/lpconfig.xsd lpconfig.xsd\">\r\n"
							"<section name=\"proxy_default_values\">\r\n"
								"<entry name=\"avpf\" overwrite=\"true\">1</entry>\r\n"
								"<entry name=\"dial_escape_plus\" overwrite=\"true\">0</entry>\r\n"
								"<entry name=\"publish\" overwrite=\"true\">0</entry>\r\n"
								"<entry name=\"quality_reporting_collector\" overwrite=\"true\">sip:voip-metrics@sip.linphone.org;transport=tls</entry>\r\n"
								"<entry name=\"quality_reporting_enabled\" overwrite=\"true\">1</entry>\r\n"
								"<entry name=\"quality_reporting_interval\" overwrite=\"true\">180</entry>\r\n"
								"<entry name=\"reg_expires\" overwrite=\"true\">31536000</entry>\r\n"
								"<entry name=\"reg_identity\" overwrite=\"true\">sip:?@sip.linphone.org</entry>\r\n"
								"<entry name=\"reg_proxy\" overwrite=\"true\">&lt;sip:sip.linphone.org;transport=tls&gt;</entry>\r\n"
								"<entry name=\"reg_sendregister\" overwrite=\"true\">1</entry>\r\n"
								"<entry name=\"nat_policy_ref\" overwrite=\"true\">nat_policy_default_values</entry>\r\n"
								"<entry name=\"realm\" overwrite=\"true\">sip.linphone.org</entry>\r\n"
							"</section>\r\n"
							"<section name=\"nat_policy_default_values\">\r\n"
								"<entry name=\"stun_server\" overwrite=\"true\">stun.linphone.org</entry>\r\n"
								"<entry name=\"protocols\" overwrite=\"true\">stun,ice</entry>\r\n"
							"</section>\r\n"
							"</config>";
	BC_ASSERT_FALSE(linphone_config_load_from_xml_string(linphone_core_get_config(lauriane->lc),config));
	proxy = linphone_core_create_proxy_config(lauriane->lc);
	
	read = linphone_address_new(linphone_proxy_config_get_server_addr(proxy));
	expected = linphone_address_new("sip:sip.linphone.org;transport=tls");
	
	BC_ASSERT_TRUE(linphone_address_equal(read,expected));
	linphone_address_unref(read);
	linphone_address_unref(expected);
	
	nat_policy = linphone_proxy_config_get_nat_policy(proxy);
	
	if (BC_ASSERT_PTR_NOT_NULL(nat_policy)) {
		BC_ASSERT_TRUE(linphone_nat_policy_ice_enabled(nat_policy));
		BC_ASSERT_TRUE(linphone_nat_policy_stun_enabled(nat_policy));
		BC_ASSERT_FALSE(linphone_nat_policy_turn_enabled(nat_policy));
	}
	linphone_proxy_config_unref(proxy);
	linphone_core_manager_destroy(lauriane);
	
	//BC_ASSERT_STRING_EQUAL(linphone_proxy_config_get(proxy), "sip:sip.linphone.org;transport=tls");
	
}

test_t proxy_config_tests[] = {
	TEST_NO_TAG("Phone normalization without proxy", phone_normalization_without_proxy),
	TEST_NO_TAG("Phone normalization with proxy", phone_normalization_with_proxy),
	TEST_NO_TAG("Phone normalization with dial escape plus", phone_normalization_with_dial_escape_plus),
	TEST_NO_TAG("SIP URI normalization", sip_uri_normalization),
	TEST_NO_TAG("Load new default value for proxy config", load_dynamic_proxy_config)
};

test_suite_t proxy_config_test_suite = {"Proxy config", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
										sizeof(proxy_config_tests) / sizeof(proxy_config_tests[0]), proxy_config_tests};
