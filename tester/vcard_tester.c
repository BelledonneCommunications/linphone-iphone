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
	
	BC_ASSERT_PTR_NOT_NULL_FATAL(linphone_core_get_default_friend_list(manager->lc));
	count = linphone_core_import_friends_from_vcard4_file(manager->lc, import_filepath);
	BC_ASSERT_EQUAL(count, 3, int, "%d");
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 3, int, "%d");
	
	linphone_core_export_friends_as_vcard4_file(manager->lc, export_filepath);
	
	linphone_core_remove_friend_list(manager->lc, linphone_core_get_default_friend_list(manager->lc));
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	linphone_core_add_friend_list(manager->lc, linphone_core_create_friend_list(manager->lc));
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
	
	linphone_address_unref(addr);
	linphone_core_manager_destroy(manager);
}

#ifdef FRIENDS_SQL_STORAGE_ENABLED
static void friends_migration(void) {
	LinphoneCoreManager* manager = linphone_core_manager_new2("friends_rc", FALSE);
	const MSList *friends = linphone_core_get_friend_list(manager->lc);
	MSList *friends_from_db = NULL;
	char *friends_db = create_filepath(bc_tester_get_writable_dir_prefix(), "friends", "db");
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	unlink(friends_db);
	linphone_core_set_friends_database_path(manager->lc, friends_db);
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 3, int, "%d");
	friends_from_db = linphone_core_fetch_friends_from_db(manager->lc, linphone_core_get_default_friend_list(manager->lc));
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 3, int, "%d");
	if (ms_list_size(friends_from_db) < 3) {
		goto end;
	}
	
end:
	unlink(friends_db);
	ms_free(friends_db);
	friends_from_db = ms_list_free_with_data(friends_from_db, (void (*)(void *))linphone_friend_unref);
	linphone_core_manager_destroy(manager);
}

static void friends_sqlite_storage(void) {
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_create_friend_list(manager->lc);
	LinphoneVCard *lvc = linphone_vcard_new();
	LinphoneFriend *lf = NULL;
	LinphoneFriend *lf2 = NULL;
	LinphoneAddress *addr = linphone_address_new("sip:sylvain@sip.linphone.org");
	const MSList *friends = linphone_core_get_friend_list(manager->lc);
	MSList *friends_from_db = NULL;
	MSList *friends_lists_from_db = NULL;
	char *friends_db = create_filepath(bc_tester_get_writable_dir_prefix(), "friends", "db");
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	unlink(friends_db);
	linphone_core_set_friends_database_path(manager->lc, friends_db);
	friends_from_db = linphone_core_fetch_friends_from_db(manager->lc, linphone_core_get_default_friend_list(manager->lc));
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 0, int, "%d");
	
	linphone_vcard_set_etag(lvc, "\"123-456789\"");
	linphone_vcard_set_url(lvc, "http://dav.somewhere.fr/addressbook/me/someone.vcf");
	lf = linphone_friend_new_from_vcard(lvc);
	linphone_friend_set_address(lf, addr);
	linphone_friend_set_name(lf, "Sylvain");
	
	linphone_core_add_friend_list(manager->lc, lfl);
	linphone_friend_list_unref(lfl);
	linphone_friend_list_set_display_name(lfl, "Test");
	BC_ASSERT_EQUAL_FATAL(linphone_friend_list_add_friend(lfl, lf), LinphoneFriendListOK, int, "%i");
	linphone_friend_unref(lf);
	BC_ASSERT_EQUAL(lfl->storage_id, 1, int, "%d");
	BC_ASSERT_EQUAL(lf->storage_id, 1, int, "%d");
	
	friends = linphone_core_get_friend_list(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends), 0, int, "%d");
	
	friends_lists_from_db = linphone_core_fetch_friends_lists_from_db(manager->lc);
	BC_ASSERT_EQUAL(ms_list_size(friends_lists_from_db), 1, int, "%d");
	friends_lists_from_db = ms_list_free_with_data(friends_lists_from_db, (void (*)(void *))linphone_friend_list_unref);
	
	friends_from_db = linphone_core_fetch_friends_from_db(manager->lc, lfl);
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
	friends_from_db = linphone_core_fetch_friends_from_db(manager->lc, lfl);
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
	friends_from_db = linphone_core_fetch_friends_from_db(manager->lc, lfl);
	BC_ASSERT_EQUAL(ms_list_size(friends_from_db), 0, int, "%d");

end:
	unlink(friends_db);
	ms_free(friends_db);
	linphone_address_unref(addr);
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
	
	linphone_friend_list_set_uri(lfl, "http://192.168.0.230/sabredav/addressbookserver.php/addressbooks/sylvain/default");
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
	
	linphone_friend_list_set_uri(lfl, "http://192.168.0.230/sabredav/addressbookserver.php/addressbooks/sylvain/default");
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
	
	linphone_friend_list_set_uri(lfl, "http://192.168.0.230/sabredav/addressbookserver.php/addressbooks/sylvain/default");
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
	
	linphone_friend_list_set_uri(lfl, "http://192.168.0.230/sabredav/addressbookserver.php/addressbooks/sylvain/default");
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
	BC_ASSERT_TRUE(linphone_vcard_generate_unique_id(lvc));
	BC_ASSERT_PTR_NOT_NULL(linphone_vcard_get_uid(lvc));
	
	linphone_carddav_put_vcard(c, lf);
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

static void carddav_integration(void) {
	LinphoneCoreManager *manager = linphone_core_manager_new2("carddav_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_create_friend_list(manager->lc);
	LinphoneVCard *lvc = linphone_vcard_new_from_vcard4_buffer("BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Margaux Clerc\r\nIMPP;TYPE=work:sip:margaux@sip.linphone.org\r\nEND:VCARD\r\n");
	LinphoneFriend *lf = linphone_friend_new_from_vcard(lvc);
	LinphoneFriendListCbs *cbs = NULL;
	LinphoneCardDAVStats *stats = (LinphoneCardDAVStats *)ms_new0(LinphoneCardDAVStats, 1);
	
	linphone_friend_list_set_uri(lfl, "http://192.168.0.230/sabredav/addressbookserver.php/addressbooks/sylvain/default");
	cbs = linphone_friend_list_get_callbacks(lfl);
	linphone_friend_list_cbs_set_user_data(cbs, stats);
	linphone_friend_list_cbs_set_contact_created(cbs, carddav_contact_created);
	linphone_friend_list_cbs_set_contact_deleted(cbs, carddav_contact_deleted);
	linphone_core_add_friend_list(manager->lc, lfl);

	BC_ASSERT_PTR_NULL(linphone_vcard_get_uid(lvc));
	BC_ASSERT_TRUE(linphone_vcard_generate_unique_id(lvc));
	BC_ASSERT_PTR_NOT_NULL(linphone_vcard_get_uid(lvc));
	BC_ASSERT_EQUAL_FATAL(linphone_friend_list_add_friend(lfl, lf), LinphoneFriendListOK, int, "%d");
	wait_for_until(manager->lc, NULL, NULL, 1, 2000);
	linphone_friend_list_remove_friend(lfl, lf);
	wait_for_until(manager->lc, NULL, NULL, 1, 2000);
	linphone_friend_unref(lf);
	lf = NULL;

	lvc = linphone_vcard_new_from_vcard4_buffer("BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Ghislain Mary\r\nIMPP;TYPE=work:sip:ghislain@sip.linphone.org\r\nEND:VCARD\r\n");
	lf = linphone_friend_new_from_vcard(lvc);
	BC_ASSERT_EQUAL_FATAL(_linphone_friend_list_add_friend(lfl, lf), LinphoneFriendListOK, int, "%d");
	linphone_friend_unref(lf);
	
	BC_ASSERT_EQUAL(lfl->revision, 0, int, "%i");
	linphone_friend_list_synchronize_friends_from_server(lfl);
	wait_for_until(manager->lc, NULL, &stats->new_contact_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->new_contact_count, 1, int, "%i");
	wait_for_until(manager->lc, NULL, &stats->removed_contact_count, 1, 2000);
	BC_ASSERT_EQUAL(stats->removed_contact_count, 1, int, "%i");
	BC_ASSERT_NOT_EQUAL(lfl->revision, 0, int, "%i");
	
	BC_ASSERT_EQUAL_FATAL(ms_list_size(lfl->friends), 1, int, "%i");
	lf = (LinphoneFriend *)lfl->friends->data;
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
	{ "CardDAV synchronization 2", carddav_sync_2 },
	{ "CardDAV synchronization 3", carddav_sync_3 },
	{ "CardDAV synchronization 4", carddav_sync_4 },
	{ "CardDAV integration", carddav_integration },
#else
	{ "Dummy test", dummy_test }
#endif
};

test_suite_t vcard_test_suite = {
	"VCard", NULL, NULL, 
	liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(vcard_tests) / sizeof(vcard_tests[0]), vcard_tests
};
