/*
 * clonable-object-tester.cpp
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

#include "object/clonable-object-p.h"
#include "object/clonable-object.h"

#include "liblinphone_tester.h"
#include "tester_utils.h"

// =============================================================================

using namespace std;

using namespace LinphonePrivate;

// -----------------------------------------------------------------------------

class TestObjectPrivate : public ClonableObjectPrivate {
public:
	TestObjectPrivate () = default;

	TestObjectPrivate (const TestObjectPrivate &) : TestObjectPrivate() {}
};

class TestObject : public ClonableObject {
public:
	TestObject () : ClonableObject(*new TestObjectPrivate) {}

	TestObject (const TestObject &src) : ClonableObject(*new TestObjectPrivate(*src.getPrivate())) {}

private:
	L_DECLARE_PRIVATE(TestObject);
};

// -----------------------------------------------------------------------------

static void check_clonable_object_creation () {
	TestObject *object = new TestObject();
	TestObject *object2 = new TestObject(*object);

	delete object;
	delete object2;
}

test_t clonable_object_tests[] = {
	TEST_NO_TAG("Check clonable object creation", check_clonable_object_creation)
};

test_suite_t clonable_object_test_suite = {
	"ClonableObject", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(clonable_object_tests) / sizeof(clonable_object_tests[0]), clonable_object_tests
};
