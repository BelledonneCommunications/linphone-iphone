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
#include "carddav.h"

#include <time.h>

#ifdef VCARD_ENABLED
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
	
	linphone_core_set_friend_list(manager->lc, NULL);
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	count = linphone_core_import_friends_from_vcard4_file(manager->lc, export_filepath);
	BC_ASSERT_EQUAL(count, 3, int, "%d");
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 3, int, "%d");
	
	remove(export_filepath);
	linphone_core_manager_destroy(manager);
}

static void linphone_vcard_import_a_lot_of_friends_test(void) {
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	char *import_filepath = bc_tester_res("common/thousand_vcards.vcf");
	clock_t start, end;
	double elapsed = 0;
	const MSList *friends = NULL;

	start = clock();
	linphone_core_import_friends_from_vcard4_file(manager->lc, import_filepath);
	end = clock();
	
	friends = linphone_core_get_friend_list(manager->lc);
	elapsed = (double)(end - start);
	ms_error("Imported a thousand of vCards (only %i friends with SIP address found) in %f seconds", ms_list_size(friends), elapsed / CLOCKS_PER_SEC);
	BC_ASSERT_TRUE(elapsed < 1500000); // 1.5 seconds
	linphone_core_manager_destroy(manager);
}

static void friends_if_no_db_set(void) {
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriend *lf = linphone_friend_new();
	LinphoneAddress *addr = linphone_address_new("sip:sylvain@sip.linphone.org");
	const MSList *friends = NULL;
	
	linphone_friend_set_address(lf, addr);
	linphone_friend_set_name(lf, "Sylvain");
	linphone_core_add_friend(manager->lc, lf);
	linphone_friend_unref(lf);
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 1, int, "%d");
	
	linphone_core_remove_friend(manager->lc, lf);
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	linphone_address_destroy(addr);
	linphone_core_manager_destroy(manager);
}

#ifdef FRIENDS_SQL_STORAGE_ENABLED
static void friends_migration(void) {
	LinphoneCoreManager* manager = linphone_core_manager_new2("friends_rc", FALSE);
	LinphoneAddress *addr = linphone_address_new("sip:sylvain@sip.linphone.org");
	const MSList *friends = linphone_core_get_friend_list(manager->lc);
	MSList *friends_from_db = NULL;
	char *friends_db = create_filepath(bc_tester_get_writable_dir_prefix(), "friends", "db");
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	unlink(friends_db);
	linphone_core_set_friends_database_path(manager->lc, friends_db);
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 3, int, "%d");
	friends_from_db = linphone_core_fetch_friends_from_db(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 3, int, "%d");
	if (ms_list_size(friends_from_db) < 3) {
		goto end;
	}
	
end:
	unlink(friends_db);
	ms_free(friends_db);
	linphone_address_destroy(addr);
	friends_from_db = ms_list_free_with_data(friends_from_db, (void (*)(void *))linphone_friend_unref);
	linphone_core_manager_destroy(manager);
}

static void friends_sqlite_storage(void) {
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriend *lf = linphone_friend_new();
	LinphoneFriend *lf2 = NULL;
	LinphoneAddress *addr = linphone_address_new("sip:sylvain@sip.linphone.org");
	const MSList *friends = linphone_core_get_friend_list(manager->lc);
	MSList *friends_from_db = NULL;
	char *friends_db = create_filepath(bc_tester_get_writable_dir_prefix(), "friends", "db");
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	unlink(friends_db);
	linphone_core_set_friends_database_path(manager->lc, friends_db);
	friends_from_db = linphone_core_fetch_friends_from_db(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	linphone_friend_set_address(lf, addr);
	linphone_friend_set_name(lf, "Sylvain");
	linphone_core_add_friend(manager->lc, lf);
	linphone_friend_unref(lf);
	BC_ASSERT_EQUAL(lf->storage_id, 1, int, "%d");
	
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 1, int, "%d");
	
	friends_from_db = linphone_core_fetch_friends_from_db(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 1, int, "%d");
	if (ms_list_size(friends_from_db) < 1) {
		goto end;
	}
	lf2 = (LinphoneFriend *)friends_from_db->data;
	BC_ASSERT_STRING_EQUAL(linphone_friend_get_name(lf2), linphone_friend_get_name(lf));
	BC_ASSERT_EQUAL(lf2->storage_id, lf->storage_id, int, "%i");
	BC_ASSERT_STRING_EQUAL(linphone_address_as_string(linphone_friend_get_address(lf2)), linphone_address_as_string(linphone_friend_get_address(lf)));

	linphone_friend_edit(lf);
	linphone_friend_set_name(lf, "Margaux");
	linphone_friend_done(lf);
	friends_from_db = ms_list_free_with_data(friends_from_db, (void (*)(void *))linphone_friend_unref);
	friends_from_db = linphone_core_fetch_friends_from_db(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 1, int, "%d");
	if (ms_list_size(friends_from_db) < 1) {
		goto end;
	}
	lf2 = (LinphoneFriend *)friends_from_db->data;
	BC_ASSERT_STRING_EQUAL(linphone_friend_get_name(lf2), "Margaux");
	friends_from_db = ms_list_free_with_data(friends_from_db, (void (*)(void *))linphone_friend_unref);
	
	linphone_core_remove_friend(manager->lc, lf);
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	friends_from_db = linphone_core_fetch_friends_from_db(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 0, int, "%d");

end:
	unlink(friends_db);
	ms_free(friends_db);
	linphone_address_destroy(addr);
	friends_from_db = ms_list_free_with_data(friends_from_db, (void (*)(void *))linphone_friend_unref);
	linphone_core_manager_destroy(manager);
}
#endif

typedef struct _LinphoneCardDAVStats {
	int sync_done_count;
	int new_contact_count;
	int removed_contact_count;
	int updated_contact_count;
} LinphoneCardDAVStats;

static void carddav_sync_done(LinphoneCardDavContext *c, bool_t success, const char *message) {
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)linphone_carddav_get_user_data(c);
	BC_ASSERT_TRUE(success);
	stats->sync_done_count++;
	linphone_carddav_destroy(c);
}

static void carddav_new_contact(LinphoneCardDavContext *c, LinphoneFriend *lf) {
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)linphone_carddav_get_user_data(c);
	BC_ASSERT_PTR_NOT_NULL_FATAL(lf);
	stats->new_contact_count++;
	linphone_friend_unref(lf);
}

static void carddav_removed_contact(LinphoneCardDavContext *c, LinphoneFriend *lf) {
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)linphone_carddav_get_user_data(c);
	BC_ASSERT_PTR_NOT_NULL_FATAL(lf);
	stats->removed_contact_count++;
	linphone_friend_unref(lf);
}

static void carddav_updated_contact(LinphoneCardDavContext *c, LinphoneFriend *lf1, LinphoneFriend *lf2) {
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)linphone_carddav_get_user_data(c);
	BC_ASSERT_PTR_NOT_NULL_FATAL(lf1);
	BC_ASSERT_PTR_NOT_NULL_FATAL(lf2);
	stats->updated_contact_count++;
	linphone_friend_unref(lf1);
	linphone_friend_unref(lf2);
}

static void carddav_sync(void) {
	LinphoneCoreManager *manager = linphone_core_manager_new2("carddav_rc", FALSE);
	LinphoneCardDavContext *c = linphone_core_create_carddav_context(manager->lc);
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)ms_new0(LinphoneCardDAVStats, 1);
	
	BC_ASSERT_PTR_NOT_NULL_FATAL(c);
	BC_ASSERT_PTR_NOT_NULL(c->server_url);
	BC_ASSERT_PTR_NOT_NULL(c->username);
	BC_ASSERT_PTR_NOT_NULL(c->ha1);
	
	linphone_carddav_set_user_data(c, stats);
	linphone_carddav_set_synchronization_done_callback(c, carddav_sync_done);
	linphone_carddav_set_new_contact_callback(c, carddav_new_contact);
	linphone_carddav_set_removed_contact_callback(c, carddav_removed_contact);
	linphone_carddav_set_updated_contact_callback(c, carddav_updated_contact);
	linphone_carddav_synchronize(c);
	
	wait_for_until(manager->lc, NULL, &stats->new_contact_count, 1, 2000);
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 1, 2000);
	linphone_core_manager_destroy(manager);
}
#else
static void dummy_test(void) {
	
}
#endif

test_t vcard_tests[] = {
#ifdef VCARD_ENABLED
	{ "Import / Export friends from vCards", linphone_vcard_import_export_friends_test },
	{ "Import a lot of friends from vCards", linphone_vcard_import_a_lot_of_friends_test },
#ifdef FRIENDS_SQL_STORAGE_ENABLED
	{ "Friends working if no db set", friends_if_no_db_set },
	{ "Friends storage migration from rc to db", friends_migration },
	{ "Friends storage in sqlite database", friends_sqlite_storage },
#endif
	{ "CardDAV synchronization", carddav_sync },
#else
	{ "Dummy test", dummy_test }
#endif
};

test_suite_t vcard_test_suite = {
	"VCard", NULL, NULL, 
	liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(vcard_tests) / sizeof(vcard_tests[0]), vcard_tests
};
