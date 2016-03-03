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
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	const MSList *friends = linphone_friend_list_get_friends(lfl);
	char *import_filepath = bc_tester_res("common/vcards.vcf");
	char *export_filepath = create_filepath(bc_tester_get_writable_dir_prefix(), "export_vcards", "vcf");
	int count = 0;
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	count = linphone_friend_list_import_friends_from_vcard4_file(lfl, import_filepath);
	BC_ASSERT_EQUAL(count, 3, int, "%d");
	friends = linphone_friend_list_get_friends(lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends), 3, int, "%d");
	
	linphone_friend_list_export_friends_as_vcard4_file(lfl, export_filepath);
	
	lfl = linphone_core_create_friend_list(manager->lc);
	count = linphone_friend_list_import_friends_from_vcard4_file(lfl, export_filepath);
	BC_ASSERT_EQUAL(count, 3, int, "%d");
	friends = linphone_friend_list_get_friends(lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends), 3, int, "%d");
	linphone_friend_list_unref(lfl);
	
	remove(export_filepath);
	ms_free(import_filepath);
	ms_free(export_filepath);
	linphone_core_manager_destroy(manager);
}

static void linphone_vcard_import_a_lot_of_friends_test(void) {
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	char *import_filepath = bc_tester_res("common/thousand_vcards.vcf");
	clock_t start, end;
	double elapsed = 0;
	const MSList *friends = NULL;
	FILE    *infile;
	char    *buffer;
	long    numbytes;

	start = clock();
	linphone_friend_list_import_friends_from_vcard4_file(lfl, import_filepath);
	end = clock();
	
	friends = linphone_friend_list_get_friends(lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends), 482, int, "%i"); // Thousand vcards contains 482 contacts with a SIP URI
	
	elapsed = (double)(end - start);
	ms_error("Imported a thousand of vCards from file (only %i friends with SIP address found) in %f seconds", ms_list_size(friends), elapsed / CLOCKS_PER_SEC);
#ifndef ANDROID
	BC_ASSERT_TRUE(elapsed < 1500000); // 1.5 seconds
#endif
	
	lfl = linphone_core_create_friend_list(manager->lc);
	infile = fopen(import_filepath, "r");
	fseek(infile, 0L, SEEK_END);
	numbytes = ftell(infile);
	fseek(infile, 0L, SEEK_SET);
	buffer = (char*)ms_malloc(numbytes * sizeof(char));
	numbytes = fread(buffer, sizeof(char), numbytes, infile);
	fclose(infile);
	
	start = clock();
	linphone_friend_list_import_friends_from_vcard4_buffer(lfl, buffer);
	end = clock();
	
	friends = linphone_friend_list_get_friends(lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends), 482, int, "%i"); // Thousand vcards contains 482 contacts with a SIP URI
	
	elapsed = (double)(end - start);
	ms_error("Imported a thousand of vCards from buffer (only %i friends with SIP address found) in %f seconds", ms_list_size(friends), elapsed / CLOCKS_PER_SEC);
#ifndef ANDROID
	BC_ASSERT_TRUE(elapsed < 1500000); // 1.5 seconds
#endif
	
	ms_free(buffer);
	linphone_friend_list_unref(lfl);
	
	ms_free(import_filepath);
	linphone_core_manager_destroy(manager);
}

static void linphone_vcard_update_existing_friends_test(void) {
	LinphoneFriend *lf = linphone_friend_new_with_addr("sip:oldfriend@sip.linphone.org");
	
	BC_ASSERT_PTR_NOT_NULL_FATAL(lf);
	BC_ASSERT_PTR_NULL(linphone_friend_get_vcard(lf));

	linphone_friend_edit(lf);
	linphone_friend_set_name(lf, "Old Friend");
	linphone_friend_done(lf);
	
	BC_ASSERT_PTR_NOT_NULL(linphone_friend_get_vcard(lf));
	BC_ASSERT_STRING_EQUAL(linphone_vcard_get_full_name(linphone_friend_get_vcard(lf)), "Old Friend");
	linphone_friend_unref(lf);
	lf = NULL;
}

static void friends_if_no_db_set(void) {
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriend *lf = linphone_friend_new();
	LinphoneAddress *addr = linphone_address_new("sip:sylvain@sip.linphone.org");
	const MSList *friends = NULL;
	LinphoneFriendList *lfl = linphone_core_create_friend_list(manager->lc);
	
	linphone_friend_set_address(lf, addr);
	linphone_friend_set_name(lf, "Sylvain");
	linphone_friend_list_add_friend(lfl, lf);
	friends = linphone_friend_list_get_friends(lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends), 1, int, "%d");
	
	linphone_friend_list_remove_friend(lfl, lf);
	linphone_friend_unref(lf);
	friends = linphone_friend_list_get_friends(lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	linphone_friend_list_unref(lfl);
	linphone_address_unref(addr);
	linphone_core_manager_destroy(manager);
}

#ifdef FRIENDS_SQL_STORAGE_ENABLED
static void friends_migration(void) {
	LinphoneCoreManager* manager = linphone_core_manager_new2("friends_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	const MSList *friends = linphone_friend_list_get_friends(lfl);
	MSList *friends_from_db = NULL;
	char *friends_db = create_filepath(bc_tester_get_writable_dir_prefix(), "friends", "db");
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	unlink(friends_db);
	linphone_core_set_friends_database_path(manager->lc, friends_db);
	friends = linphone_friend_list_get_friends(lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends), 3, int, "%d");
	friends_from_db = linphone_core_fetch_friends_from_db(manager->lc, lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 3, int, "%d");
	
	friends_from_db = ms_list_free_with_data(friends_from_db, (void (*)(void *))linphone_friend_unref);
	unlink(friends_db);
	ms_free(friends_db);
	linphone_core_manager_destroy(manager);
}

typedef struct _LinphoneFriendListStats {
	int new_list_count;
	int removed_list_count;
} LinphoneFriendListStats;

static void friend_list_created_cb(LinphoneCore *lc, LinphoneFriendList *list) {
	LinphoneFriendListStats *stats = (LinphoneFriendListStats *)linphone_friend_list_get_user_data(list);
	stats->new_list_count++;
}

static void friend_list_removed_cb(LinphoneCore *lc, LinphoneFriendList *list) {
	LinphoneFriendListStats *stats = (LinphoneFriendListStats *)linphone_friend_list_get_user_data(list);
	stats->removed_list_count++;
}

static void friends_sqlite_storage(void) {
	LinphoneCoreVTable *v_table = linphone_core_v_table_new();
	LinphoneCore* lc = NULL;
	LinphoneFriendList *lfl = NULL;
	LinphoneFriend *lf = NULL;
	LinphoneFriend *lf2 = NULL;
	LinphoneVCard *lvc = linphone_vcard_new();
	LinphoneAddress *addr = linphone_address_new("sip:sylvain@sip.linphone.org");
	const MSList *friends = NULL;
	MSList *friends_from_db = NULL;
	MSList *friends_lists_from_db = NULL;
	char *friends_db = create_filepath(bc_tester_get_writable_dir_prefix(), "friends", "db");
	LinphoneFriendListStats *stats = (LinphoneFriendListStats *)ms_new0(LinphoneFriendListStats, 1);
	
	v_table->friend_list_created = friend_list_created_cb;
	v_table->friend_list_removed = friend_list_removed_cb;
	lc = linphone_core_new(v_table, NULL, NULL, NULL);
	friends = linphone_friend_list_get_friends(linphone_core_get_default_friend_list(lc));
	lfl = linphone_core_create_friend_list(lc);
	linphone_friend_list_set_user_data(lfl, stats);
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	unlink(friends_db);
	linphone_core_set_friends_database_path(lc, friends_db);
	friends_from_db = linphone_core_fetch_friends_from_db(lc, linphone_core_get_default_friend_list(lc));
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 0, int, "%d");
	
	linphone_vcard_set_etag(lvc, "\"123-456789\"");
	linphone_vcard_set_url(lvc, "http://dav.somewhere.fr/addressbook/me/someone.vcf");
	lf = linphone_friend_new_from_vcard(lvc);
	linphone_friend_set_address(lf, addr);
	linphone_friend_set_name(lf, "Sylvain");
	
	linphone_core_add_friend_list(lc, lfl);
	wait_for_until(lc, NULL, &stats->new_list_count, 1, 1000);
	BC_ASSERT_EQUAL(stats->new_list_count, 1, int, "%i");
	linphone_friend_list_unref(lfl);
	linphone_friend_list_set_display_name(lfl, "Test");
	BC_ASSERT_EQUAL_FATAL(linphone_friend_list_add_friend(lfl, lf), LinphoneFriendListOK, int, "%i");
	linphone_friend_unref(lf);
	BC_ASSERT_EQUAL(lfl->storage_id, 1, int, "%d");
	BC_ASSERT_EQUAL(lf->storage_id, 1, int, "%d");
	
	friends = linphone_friend_list_get_friends(linphone_core_get_default_friend_list(lc));
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	friends_lists_from_db = linphone_core_fetch_friends_lists_from_db(lc);
	BC_ASSERT_EQUAL(ms_list_size(friends_lists_from_db), 1, int, "%d");
	friends_from_db = ((LinphoneFriendList *)friends_lists_from_db->data)->friends;
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 1, int, "%d");
	lf2 = (LinphoneFriend *)friends_from_db->data;
	BC_ASSERT_PTR_NOT_NULL(lf2->lc);
	BC_ASSERT_PTR_NOT_NULL(lf2->friend_list);
	friends_lists_from_db = ms_list_free_with_data(friends_lists_from_db, (void (*)(void *))linphone_friend_list_unref);
	
	friends_from_db = linphone_core_fetch_friends_from_db(lc, lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 1, int, "%d");
	if (ms_list_size(friends_from_db) < 1) {
		goto end;
	}
	lf2 = (LinphoneFriend *)friends_from_db->data;
	BC_ASSERT_STRING_EQUAL(linphone_friend_get_name(lf2), linphone_friend_get_name(lf));
	BC_ASSERT_EQUAL(lf2->storage_id, lf->storage_id, int, "%i");
	BC_ASSERT_STRING_EQUAL(linphone_vcard_get_etag(linphone_friend_get_vcard(lf2)), linphone_vcard_get_etag(linphone_friend_get_vcard(lf)));
	BC_ASSERT_STRING_EQUAL(linphone_vcard_get_url(linphone_friend_get_vcard(lf2)), linphone_vcard_get_url(linphone_friend_get_vcard(lf)));
	BC_ASSERT_STRING_EQUAL(linphone_address_as_string(linphone_friend_get_address(lf2)), linphone_address_as_string(linphone_friend_get_address(lf)));

	linphone_friend_edit(lf);
	linphone_friend_set_name(lf, "Margaux");
	linphone_friend_done(lf);
	friends_from_db = ms_list_free_with_data(friends_from_db, (void (*)(void *))linphone_friend_unref);
	friends_from_db = linphone_core_fetch_friends_from_db(lc, lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 1, int, "%d");
	if (ms_list_size(friends_from_db) < 1) {
		goto end;
	}
	lf2 = (LinphoneFriend *)friends_from_db->data;
	BC_ASSERT_STRING_EQUAL(linphone_friend_get_name(lf2), "Margaux");
	friends_from_db = ms_list_free_with_data(friends_from_db, (void (*)(void *))linphone_friend_unref);
	
	linphone_friend_list_remove_friend(lfl, lf);
	friends = linphone_friend_list_get_friends(linphone_core_get_default_friend_list(lc));
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	friends_from_db = linphone_core_fetch_friends_from_db(lc, lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 0, int, "%d");
	
	linphone_core_remove_friend_list(lc, lfl);
	wait_for_until(lc, NULL, &stats->removed_list_count, 1, 1000);
	BC_ASSERT_EQUAL(stats->removed_list_count, 1, int, "%i");

end:
	unlink(friends_db);
	ms_free(friends_db);
	linphone_address_unref(addr);
	linphone_core_destroy(lc);
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
}

static void carddav_new_contact(LinphoneCardDavContext *c, LinphoneFriend *lf) {
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)linphone_carddav_get_user_data(c);
	BC_ASSERT_PTR_NOT_NULL_FATAL(lf);
	stats->new_contact_count++;
}

static void carddav_removed_contact(LinphoneCardDavContext *c, LinphoneFriend *lf) {
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)linphone_carddav_get_user_data(c);
	BC_ASSERT_PTR_NOT_NULL_FATAL(lf);
	stats->removed_contact_count++;
}

static void carddav_updated_contact(LinphoneCardDavContext *c, LinphoneFriend *new_lf, LinphoneFriend *old_lf) {
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)linphone_carddav_get_user_data(c);
	BC_ASSERT_PTR_NOT_NULL_FATAL(new_lf);
	BC_ASSERT_PTR_NOT_NULL_FATAL(old_lf);
	stats->updated_contact_count++;
}

static void carddav_sync(void) {
	LinphoneCoreManager *manager = linphone_core_manager_new2("carddav_rc", FALSE);
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)ms_new0(LinphoneCardDAVStats, 1);
	LinphoneFriendList *lfl = linphone_core_create_friend_list(manager->lc);
	LinphoneCardDavContext *c = NULL;
	
	linphone_friend_list_set_uri(lfl, "http://dav.linphone.org/sabredav/addressbookserver.php/addressbooks/sylvain/default");
	linphone_core_add_friend_list(manager->lc, lfl);
	linphone_friend_list_unref(lfl);
	c = linphone_carddav_context_new(lfl);
	BC_ASSERT_PTR_NOT_NULL_FATAL(c);
	
	linphone_carddav_set_user_data(c, stats);
	linphone_carddav_set_synchronization_done_callback(c, carddav_sync_done);
	linphone_carddav_set_new_contact_callback(c, carddav_new_contact);
	linphone_carddav_set_removed_contact_callback(c, carddav_removed_contact);
	linphone_carddav_set_updated_contact_callback(c, carddav_updated_contact);
	linphone_carddav_synchronize(c);
	
	wait_for_until(manager->lc, NULL, &stats->new_contact_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->new_contact_count, 1, int, "%i");
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 1, int, "%i");
	
	ms_free(stats);
	linphone_carddav_context_destroy(c);
	linphone_core_manager_destroy(manager);
}

static void carddav_sync_2(void) {
	LinphoneCoreManager *manager = linphone_core_manager_new2("carddav_rc", FALSE);
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)ms_new0(LinphoneCardDAVStats, 1);
	LinphoneFriend *lf = linphone_friend_new_with_address("\"Sylvain\" <sip:sylvain@sip.linphone.org>");
	char *friends_db = create_filepath(bc_tester_get_writable_dir_prefix(), "friends", "db");
	LinphoneFriendList *lfl = linphone_core_create_friend_list(manager->lc);
	LinphoneCardDavContext *c = NULL;
	
	linphone_friend_list_set_uri(lfl, "http://dav.linphone.org/sabredav/addressbookserver.php/addressbooks/sylvain/default");
	linphone_core_add_friend_list(manager->lc, lfl);
	linphone_friend_list_unref(lfl);
	c = linphone_carddav_context_new(lfl);
	BC_ASSERT_PTR_NOT_NULL_FATAL(c);
	
	unlink(friends_db);
	linphone_core_set_friends_database_path(manager->lc, friends_db);
	BC_ASSERT_EQUAL_FATAL(linphone_friend_list_add_friend(lfl, lf), LinphoneFriendListOK, int, "%d");
	linphone_friend_unref(lf);
	
	linphone_carddav_set_user_data(c, stats);
	linphone_carddav_set_synchronization_done_callback(c, carddav_sync_done);
	linphone_carddav_set_new_contact_callback(c, carddav_new_contact);
	linphone_carddav_set_removed_contact_callback(c, carddav_removed_contact);
	linphone_carddav_set_updated_contact_callback(c, carddav_updated_contact);
	
	linphone_carddav_synchronize(c);
	
	wait_for_until(manager->lc, NULL, &stats->new_contact_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->new_contact_count, 1, int, "%i");
	wait_for_until(manager->lc, NULL, &stats->removed_contact_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->removed_contact_count, 1, int, "%i");
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 1, int, "%i");
	
	ms_free(stats);
	unlink(friends_db);
	ms_free(friends_db);
	linphone_carddav_context_destroy(c);
	linphone_core_manager_destroy(manager);
}

static void carddav_sync_3(void) {
	LinphoneCoreManager *manager = linphone_core_manager_new2("carddav_rc", FALSE);
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)ms_new0(LinphoneCardDAVStats, 1);
	LinphoneVCard *lvc = linphone_vcard_new_from_vcard4_buffer("BEGIN:VCARD\r\nVERSION:4.0\r\nUID:1f08dd48-29ac-4097-8e48-8596d7776283\r\nFN:Sylvain Berfini\r\nIMPP;TYPE=work:sip:sylvain@sip.linphone.org\r\nEND:VCARD\r\n");
	LinphoneFriend *lf = linphone_friend_new_from_vcard(lvc);
	char *friends_db = create_filepath(bc_tester_get_writable_dir_prefix(), "friends", "db");
	LinphoneFriendList *lfl = linphone_core_create_friend_list(manager->lc);
	LinphoneCardDavContext *c = NULL;
	
	linphone_friend_list_set_uri(lfl, "http://dav.linphone.org/sabredav/addressbookserver.php/addressbooks/sylvain/default");
	linphone_core_add_friend_list(manager->lc, lfl);
	linphone_friend_list_unref(lfl);
	c = linphone_carddav_context_new(lfl);
	BC_ASSERT_PTR_NOT_NULL_FATAL(c);
	
	unlink(friends_db);
	linphone_core_set_friends_database_path(manager->lc, friends_db);
	BC_ASSERT_EQUAL_FATAL(linphone_friend_list_add_friend(lfl, lf), LinphoneFriendListOK, int, "%d");
	linphone_friend_unref(lf);
	
	linphone_carddav_set_user_data(c, stats);
	linphone_carddav_set_synchronization_done_callback(c, carddav_sync_done);
	linphone_carddav_set_new_contact_callback(c, carddav_new_contact);
	linphone_carddav_set_removed_contact_callback(c, carddav_removed_contact);
	linphone_carddav_set_updated_contact_callback(c, carddav_updated_contact);
	
	linphone_carddav_synchronize(c);
	
	wait_for_until(manager->lc, NULL, &stats->updated_contact_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->updated_contact_count, 1, int, "%i");
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 1, int, "%i");

	ms_free(stats);
	unlink(friends_db);
	ms_free(friends_db);
	linphone_carddav_context_destroy(c);
	linphone_core_manager_destroy(manager);
}

static void carddav_sync_4(void) {
	LinphoneCoreManager *manager = linphone_core_manager_new2("carddav_rc", FALSE);
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)ms_new0(LinphoneCardDAVStats, 1);
	LinphoneVCard *lvc = linphone_vcard_new_from_vcard4_buffer("BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Margaux Clerc\r\nIMPP;TYPE=work:sip:margaux@sip.linphone.org\r\nEND:VCARD\r\n");
	LinphoneFriend *lf = linphone_friend_new_from_vcard(lvc);
	LinphoneFriendList *lfl = linphone_core_create_friend_list(manager->lc);
	LinphoneCardDavContext *c = NULL;
	
	linphone_friend_list_set_uri(lfl, "http://dav.linphone.org/sabredav/addressbookserver.php/addressbooks/sylvain/default");
	linphone_core_add_friend_list(manager->lc, lfl);
	linphone_friend_list_unref(lfl);
	c = linphone_carddav_context_new(lfl);
	BC_ASSERT_PTR_NOT_NULL_FATAL(c);
	
	linphone_carddav_set_user_data(c, stats);
	linphone_carddav_set_synchronization_done_callback(c, carddav_sync_done);
	linphone_carddav_set_new_contact_callback(c, carddav_new_contact);
	linphone_carddav_set_removed_contact_callback(c, carddav_removed_contact);
	linphone_carddav_set_updated_contact_callback(c, carddav_updated_contact);
	
	BC_ASSERT_PTR_NULL(linphone_vcard_get_uid(lvc));	
	linphone_carddav_put_vcard(c, lf);
	BC_ASSERT_PTR_NOT_NULL(linphone_vcard_get_uid(lvc));
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 1, int, "%i");
	
	linphone_carddav_delete_vcard(c, lf);
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 2, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 2, int, "%i");

	linphone_friend_unref(lf);
	ms_free(stats);
	linphone_carddav_context_destroy(c);
	linphone_core_manager_destroy(manager);
}

static void carddav_contact_created(LinphoneFriendList *list, LinphoneFriend *lf) {
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)linphone_friend_list_cbs_get_user_data(list->cbs);
	stats->new_contact_count++;
}

static void carddav_contact_deleted(LinphoneFriendList *list, LinphoneFriend *lf) {
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)linphone_friend_list_cbs_get_user_data(list->cbs);
	stats->removed_contact_count++;
}

static void carddav_contact_updated(LinphoneFriendList *list, LinphoneFriend *new_friend, LinphoneFriend *old_friend) {
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)linphone_friend_list_cbs_get_user_data(list->cbs);
	BC_ASSERT_STRING_EQUAL(linphone_vcard_get_full_name(linphone_friend_get_vcard(new_friend)), linphone_vcard_get_full_name(linphone_friend_get_vcard(old_friend)));
	stats->updated_contact_count++;
}

static void carddav_sync_status_changed(LinphoneFriendList *list, LinphoneFriendListSyncStatus status, const char *msg) {
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)linphone_friend_list_cbs_get_user_data(list->cbs);
	char *state = status == LinphoneFriendListSyncStarted ? "Sync started" : (status == LinphoneFriendListSyncFailure ? "Sync failure" : "Sync successful");
	ms_message("[CardDAV] %s : %s", state, msg);
	if (status == LinphoneFriendListSyncFailure || status == LinphoneFriendListSyncSuccessful) {
		stats->sync_done_count++;
	}
}

static void carddav_integration(void) {
	LinphoneCoreManager *manager = linphone_core_manager_new2("carddav_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_create_friend_list(manager->lc);
	LinphoneVCard *lvc = linphone_vcard_new_from_vcard4_buffer("BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Margaux Clerc\r\nIMPP;TYPE=work:sip:margaux@sip.linphone.org\r\nEND:VCARD\r\n");
	LinphoneFriend *lf = linphone_friend_new_from_vcard(lvc);
	LinphoneVCard *lvc2 = NULL;
	LinphoneFriend *lf2 = NULL;
	LinphoneFriendListCbs *cbs = NULL;
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)ms_new0(LinphoneCardDAVStats, 1);
	const char *refkey = "toto";
	
	linphone_friend_list_set_uri(lfl, "http://dav.linphone.org/sabredav/addressbookserver.php/addressbooks/sylvain/default");
	cbs = linphone_friend_list_get_callbacks(lfl);
	linphone_friend_list_cbs_set_user_data(cbs, stats);
	linphone_friend_list_cbs_set_contact_created(cbs, carddav_contact_created);
	linphone_friend_list_cbs_set_contact_deleted(cbs, carddav_contact_deleted);
	linphone_friend_list_cbs_set_contact_updated(cbs, carddav_contact_updated);
	linphone_friend_list_cbs_set_sync_status_changed(cbs, carddav_sync_status_changed);
	linphone_core_add_friend_list(manager->lc, lfl);

	BC_ASSERT_PTR_NULL(linphone_vcard_get_uid(lvc));
	BC_ASSERT_EQUAL(ms_list_size(lfl->dirty_friends_to_update), 0, int, "%d");
	BC_ASSERT_EQUAL_FATAL(linphone_friend_list_add_friend(lfl, lf), LinphoneFriendListOK, int, "%d");
	BC_ASSERT_EQUAL(ms_list_size(lfl->dirty_friends_to_update), 1, int, "%d");
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 1, int, "%i");
	BC_ASSERT_EQUAL(ms_list_size(lfl->dirty_friends_to_update), 0, int, "%d");
	BC_ASSERT_PTR_NOT_NULL(linphone_vcard_get_uid(lvc));
	linphone_friend_list_remove_friend(lfl, lf);
	BC_ASSERT_EQUAL(ms_list_size(lfl->friends), 0, int, "%d");
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 2, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 2, int, "%i");
	linphone_friend_unref(lf);
	lf = NULL;

	lvc = linphone_vcard_new_from_vcard4_buffer("BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Ghislain Mary\r\nIMPP;TYPE=work:sip:ghislain@sip.linphone.org\r\nEND:VCARD\r\n");
	lf = linphone_friend_new_from_vcard(lvc);
	BC_ASSERT_EQUAL_FATAL(linphone_friend_list_add_local_friend(lfl, lf), LinphoneFriendListOK, int, "%d");
	linphone_friend_unref(lf);
	
	lvc2 = linphone_vcard_new_from_vcard4_buffer("BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Sylvain Berfini\r\nIMPP:sip:sberfini@sip.linphone.org\r\nUID:1f08dd48-29ac-4097-8e48-8596d7776283\r\nEND:VCARD\r\n");
	linphone_vcard_set_url(lvc2, "/sabredav/addressbookserver.php/addressbooks/sylvain/default/me.vcf");
	lf2 = linphone_friend_new_from_vcard(lvc2);
	linphone_friend_set_ref_key(lf2, refkey);
	BC_ASSERT_EQUAL_FATAL(linphone_friend_list_add_local_friend(lfl, lf2), LinphoneFriendListOK, int, "%d");
	
	BC_ASSERT_EQUAL(lfl->revision, 0, int, "%i");
	linphone_friend_list_synchronize_friends_from_server(lfl);
	wait_for_until(manager->lc, NULL, &stats->new_contact_count, 0, 2000);
	BC_ASSERT_EQUAL(stats->new_contact_count, 0, int, "%i");
	wait_for_until(manager->lc, NULL, &stats->removed_contact_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->removed_contact_count, 1, int, "%i");
	wait_for_until(manager->lc, NULL, &stats->updated_contact_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->updated_contact_count, 1, int, "%i");
	BC_ASSERT_NOT_EQUAL(lfl->revision, 0, int, "%i");
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 3, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 3, int, "%i");
	
	BC_ASSERT_EQUAL_FATAL(ms_list_size(lfl->friends), 1, int, "%i");
	lf = (LinphoneFriend *)lfl->friends->data;
	BC_ASSERT_STRING_EQUAL(lf->refkey, refkey);
	BC_ASSERT_EQUAL(lf->storage_id, lf2->storage_id, int, "%i");
	linphone_friend_unref(lf2);
	BC_ASSERT_STRING_EQUAL(linphone_address_as_string_uri_only(lf->uri), "sip:sylvain@sip.linphone.org");
	
	linphone_friend_edit(lf);
	linphone_friend_done(lf);
	BC_ASSERT_EQUAL(ms_list_size(lf->friend_list->dirty_friends_to_update), 0, int, "%i");
	
	linphone_core_set_network_reachable(manager->lc, FALSE); //To prevent the CardDAV update
	linphone_friend_edit(lf);
	linphone_friend_set_name(lf, "François Grisez");
	linphone_friend_done(lf);
	BC_ASSERT_EQUAL(ms_list_size(lf->friend_list->dirty_friends_to_update), 1, int, "%i");
	
	ms_free(stats);
	linphone_friend_list_unref(lfl);
	linphone_core_manager_destroy(manager);
}

static void carddav_clean(void) {  // This is to ensure the content of the test addressbook is in the correct state for the following tests
	LinphoneCoreManager *manager = linphone_core_manager_new2("carddav_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_create_friend_list(manager->lc);
	LinphoneFriendListCbs *cbs = linphone_friend_list_get_callbacks(lfl);
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)ms_new0(LinphoneCardDAVStats, 1);
	MSList *friends = NULL;
	LinphoneFriend *lf = NULL;
	LinphoneVCard *lvc = NULL;
	
	linphone_friend_list_cbs_set_user_data(cbs, stats);
	linphone_friend_list_cbs_set_contact_created(cbs, carddav_contact_created);
	linphone_friend_list_cbs_set_contact_deleted(cbs, carddav_contact_deleted);
	linphone_friend_list_cbs_set_contact_updated(cbs, carddav_contact_updated);
	linphone_friend_list_cbs_set_sync_status_changed(cbs, carddav_sync_status_changed);
	linphone_core_add_friend_list(manager->lc, lfl);
	linphone_friend_list_set_uri(lfl, "http://dav.linphone.org/sabredav/addressbookserver.php/addressbooks/sylvain/default");
	
	linphone_friend_list_synchronize_friends_from_server(lfl);
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 1, int, "%i");
	stats->sync_done_count = 0;
	
	friends = ms_list_copy(lfl->friends);
	while (friends) {
		LinphoneFriend *lf = (LinphoneFriend *)friends->data;
		linphone_friend_list_remove_friend(lfl, lf);
		wait_for_until(manager->lc, NULL, &stats->sync_done_count, 1, 2000);
		BC_ASSERT_EQUAL(stats->sync_done_count, 1, int, "%i");
		stats->sync_done_count = 0;
		stats->removed_contact_count = 0;
		friends = ms_list_next(friends);
	}
	
	lvc = linphone_vcard_new_from_vcard4_buffer("BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Sylvain Berfini\r\nIMPP:sip:sylvain@sip.linphone.org\r\nUID:1f08dd48-29ac-4097-8e48-8596d7776283\r\nEND:VCARD\r\n");
	linphone_vcard_set_url(lvc, "http://dav.linphone.org/sabredav/addressbookserver.php/addressbooks/sylvain/default/me.vcf");
	lf = linphone_friend_new_from_vcard(lvc);
	linphone_friend_list_add_friend(lfl, lf);
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 1, int, "%i");
	
	ms_free(stats);
	linphone_friend_unref(lf);
	linphone_friend_list_unref(lfl);
	linphone_core_manager_destroy(manager);
}

static void carddav_multiple_sync(void) {
	LinphoneCoreManager *manager = linphone_core_manager_new2("carddav_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_create_friend_list(manager->lc);
	LinphoneFriendListCbs *cbs = linphone_friend_list_get_callbacks(lfl);
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)ms_new0(LinphoneCardDAVStats, 1);
	
	linphone_friend_list_cbs_set_user_data(cbs, stats);
	linphone_friend_list_cbs_set_contact_created(cbs, carddav_contact_created);
	linphone_friend_list_cbs_set_contact_deleted(cbs, carddav_contact_deleted);
	linphone_friend_list_cbs_set_contact_updated(cbs, carddav_contact_updated);
	linphone_friend_list_cbs_set_sync_status_changed(cbs, carddav_sync_status_changed);
	linphone_core_add_friend_list(manager->lc, lfl);
	linphone_friend_list_set_uri(lfl, "http://dav.linphone.org/sabredav/addressbookserver.php/addressbooks/sylvain/default");
	
	linphone_friend_list_synchronize_friends_from_server(lfl);
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 1, int, "%i");
	linphone_friend_list_synchronize_friends_from_server(lfl);
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 2, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 2, int, "%i");
	linphone_friend_list_synchronize_friends_from_server(lfl);
	wait_for_until(manager->lc, NULL, &stats->sync_done_count, 3, 2000);
	BC_ASSERT_EQUAL(stats->sync_done_count, 3, int, "%i");
	BC_ASSERT_EQUAL(stats->removed_contact_count, 0, int, "%i");
	
	linphone_friend_list_unref(lfl);
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
	{ "vCard creation for existing friends", linphone_vcard_update_existing_friends_test },
#ifdef FRIENDS_SQL_STORAGE_ENABLED
	{ "Friends working if no db set", friends_if_no_db_set },
	{ "Friends storage migration from rc to db", friends_migration },
	{ "Friends storage in sqlite database", friends_sqlite_storage },
#endif
	{ "CardDAV clean", carddav_clean }, // This is to ensure the content of the test addressbook is in the correct state for the following tests
	{ "CardDAV synchronization", carddav_sync },
	{ "CardDAV synchronization 2", carddav_sync_2 },
	{ "CardDAV synchronization 3", carddav_sync_3 },
	{ "CardDAV synchronization 4", carddav_sync_4 },
	{ "CardDAV integration", carddav_integration },
	{ "CardDAV multiple synchronizations", carddav_multiple_sync },
#else
	{ "Dummy test", dummy_test }
#endif
};

test_suite_t vcard_test_suite = {
	"VCard", NULL, NULL, 
	liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(vcard_tests) / sizeof(vcard_tests[0]), vcard_tests
};
