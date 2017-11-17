/*
 * property-container-tester.cpp
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "object/property-container.h"

#include "liblinphone_tester.h"
#include "tester_utils.h"

// =============================================================================

using namespace std;

using namespace LinphonePrivate;

static void set_int_property () {
	PropertyContainer properties;
	properties.setProperty("integer", 42);
	BC_ASSERT_EQUAL(properties.getProperty("integer").getValue<int>(), 42, int, "%d");
}

static void set_string_property () {
	PropertyContainer properties;
	const string text = "Hey listen!";
	properties.setProperty("string", text);

	{
		string textToCheck = properties.getProperty("string").getValue<string>();
		BC_ASSERT_STRING_EQUAL(textToCheck.c_str(), text.c_str());
	}
}

static void set_generic_property () {
	PropertyContainer properties;
	properties.setProperty("generic", reinterpret_cast<void *>(0x42));
	BC_ASSERT_EQUAL(properties.getProperty("generic").getValue<void *>(), reinterpret_cast<void *>(0x42), void *, "%p");
}

test_t property_container_tests[] = {
	TEST_NO_TAG("Set int property", set_int_property),
	TEST_NO_TAG("Set string property", set_string_property),
	TEST_NO_TAG("Set generic property", set_generic_property)
};

test_suite_t property_container_test_suite = {
	"PropertyContainer", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(property_container_tests) / sizeof(property_container_tests[0]), property_container_tests
};
