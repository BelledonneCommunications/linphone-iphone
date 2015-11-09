/*
linphone
Copyright (C) 2010-2015 Belledonne Communications SARL

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "linphonecore.h"
#include "private.h"



static void linphone_friend_list_destroy(LinphoneFriendList *list) {
	if (list->display_name != NULL) ms_free(list->display_name);
	if (list->rls_uri != NULL) ms_free(list->rls_uri);
	list->friends = ms_list_free_with_data(list->friends, (void (*)(void *))linphone_friend_unref);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneFriendList);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneFriendList, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_friend_list_destroy,
	NULL, // clone
	NULL, // marshal
	TRUE
);


LinphoneFriendList * linphone_friend_list_new(void) {
	LinphoneFriendList *list = belle_sip_object_new(LinphoneFriendList);
	belle_sip_object_ref(list);
	return list;
}

LinphoneFriendList * linphone_friend_list_ref(LinphoneFriendList *list) {
	belle_sip_object_ref(list);
	return list;
}

void linphone_friend_list_unref(LinphoneFriendList *list) {
	belle_sip_object_unref(list);
}

void * linphone_friend_list_get_user_data(const LinphoneFriendList *list) {
	return list->user_data;
}

void linphone_friend_list_set_user_data(LinphoneFriendList *list, void *ud) {
	list->user_data = ud;
}

const char * linphone_friend_list_get_display_name(const LinphoneFriendList *list) {
	return list->display_name;
}

void linphone_friend_list_set_display_name(LinphoneFriendList *list, const char *display_name) {
	if (list->display_name != NULL) {
		ms_free(list->display_name);
		list->display_name = NULL;
	}
	if (display_name != NULL) {
		list->display_name = ms_strdup(display_name);
	}
}

const char * linphone_friend_list_get_rls_uri(const LinphoneFriendList *list) {
	return list->rls_uri;
}

void linphone_friend_list_set_rls_uri(LinphoneFriendList *list, const char *rls_uri) {
	if (list->rls_uri != NULL) {
		ms_free(list->rls_uri);
		list->rls_uri = NULL;
	}
	if (rls_uri != NULL) {
		list->rls_uri = ms_strdup(rls_uri);
	}
}

LinphoneFriendListStatus linphone_friend_list_add_friend(LinphoneFriendList *list, LinphoneFriend *friend) {
	if ((friend->lc != NULL) || (friend->uri == NULL)) return LinphoneFriendListInvalidFriend;
	if (ms_list_find(list->friends, friend) != NULL) {
		char *tmp = NULL;
		const LinphoneAddress *addr = linphone_friend_get_address(friend);
		if (addr) tmp = linphone_address_as_string(addr);
		ms_warning("Friend %s already in list [%s], ignored.", tmp ? tmp : "unknown", list->display_name);
		if (tmp) ms_free(tmp);
	} else {
		list->friends = ms_list_append(list->friends, linphone_friend_ref(friend));
	}
	return LinphoneFriendListOK;
}

LinphoneFriendListStatus linphone_friend_list_remove_friend(LinphoneFriendList *list, LinphoneFriend *friend) {
	MSList *elem = ms_list_find(list->friends, friend);
	if (elem == NULL) return LinphoneFriendListNonExistentFriend;
	linphone_friend_unref((LinphoneFriend *)elem->data);
	list->friends = ms_list_remove_link(list->friends, elem);
	return LinphoneFriendListOK;
}

LinphoneFriend * linphone_friend_list_find_friend_by_address(const LinphoneFriendList *list, const LinphoneAddress *address) {
	LinphoneFriend *friend = NULL;
	const MSList *elem;
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		friend = (LinphoneFriend *)elem->data;
		if (linphone_address_weak_equal(friend->uri, address))
			return friend;
	}
	return NULL;
}

LinphoneFriend * linphone_friend_list_find_friend_by_uri(const LinphoneFriendList *list, const char *uri) {
	LinphoneAddress *address = linphone_address_new(uri);
	LinphoneFriend *friend = address ? linphone_friend_list_find_friend_by_address(list, address) : NULL;
	if (address) linphone_address_unref(address);
	return friend;
}

LinphoneFriend * linphone_friend_list_find_friend_by_ref_key(const LinphoneFriendList *list, const char *ref_key) {
	const MSList *elem;
	if (ref_key == NULL) return NULL;
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *friend = (LinphoneFriend *)elem->data;
		if ((friend->refkey != NULL) && (strcmp(friend->refkey, ref_key) == 0)) return friend;
	}
	return NULL;
}

LinphoneFriend * linphone_friend_list_find_friend_by_inc_subscribe(const LinphoneFriendList *list, SalOp *op) {
	const MSList *elem;
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *friend = (LinphoneFriend *)elem->data;
		if (ms_list_find(friend->insubs, op)) return friend;
	}
	return NULL;
}

LinphoneFriend * linphone_friend_list_find_friend_by_out_subscribe(const LinphoneFriendList *list, SalOp *op) {
	const MSList *elem;
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *friend = (LinphoneFriend *)elem->data;
		if (friend->outsub && ((friend->outsub == op) || sal_op_is_forked_of(friend->outsub, op))) return friend;
	}
	return NULL;
}

void linphone_friend_list_close_subscriptions(LinphoneFriendList *list) {
	 /* FIXME we should wait until subscription to complete. */
	if (list->friends)
		ms_list_for_each(list->friends, (void (*)(void *))linphone_friend_close_subscriptions);
}

void linphone_friend_list_update_subscriptions(LinphoneFriendList *list, LinphoneProxyConfig *cfg, bool_t only_when_registered) {
	const MSList *elem;
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *friend = (LinphoneFriend *)elem->data;
		linphone_friend_update_subscribes(friend, cfg, only_when_registered);
	}
}

void linphone_friend_list_invalidate_subscriptions(LinphoneFriendList *list) {
	const MSList *elem;
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *friend = (LinphoneFriend *)elem->data;
		linphone_friend_invalidate_subscription(friend);
	}
}

void linphone_friend_list_notify_presence(LinphoneFriendList *list, LinphonePresenceModel *presence) {
	const MSList *elem;
	for(elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *friend = (LinphoneFriend *)elem->data;
		linphone_friend_notify(friend, presence);
	}
}
