/*
	vcard_tester.c
	Copyright (C) 2015  Belledonne Communications SARL

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"

static char *create_filepath(const char *dir, const char *filename, const char *ext) {
	return ms_strdup_printf("%s/%s.%s", dir, filename, ext);
}

static void linphone_vcard_import_export_friends_test(void) {
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	char *import_filepath = bc_tester_res("common/vcards.vcf");
	char *export_filepath = create_filepath(bc_tester_get_writable_dir_prefix(), "export_vcards", "vcf");
	const MSList *friends = linphone_core_get_friend_list(manager->lc);
	int count = 0;
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	count = linphone_core_import_friends_from_vcard4_file(manager->lc, import_filepath);
	BC_ASSERT_EQUAL(count, 3, int, "%d");
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 3, int, "%d");
	
	linphone_core_export_friends_as_vcard4_file(manager->lc, export_filepath);
	
	manager->lc->friends = ms_list_free(manager->lc->friends);
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	count = linphone_core_import_friends_from_vcard4_file(manager->lc, export_filepath);
	BC_ASSERT_EQUAL(count, 3, int, "%d");
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 3, int, "%d");
	
	remove(export_filepath);
	linphone_core_manager_destroy(manager);
}

test_t vcard_tests[] = {
	{ "Import / Export friends from vCards", linphone_vcard_import_export_friends_test },
};

test_suite_t vcard_test_suite = {
	"VCard", NULL, NULL, 
	liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(vcard_tests) / sizeof(vcard_tests[0]), vcard_tests
};
