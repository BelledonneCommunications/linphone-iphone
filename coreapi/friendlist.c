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

#include <bctoolbox/crypto.h>

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneFriendListCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneFriendListCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // Marshall
	FALSE
);

static LinphoneFriendListCbs * linphone_friend_list_cbs_new(void) {
	return belle_sip_object_new(LinphoneFriendListCbs);
}

LinphoneFriendListCbs * linphone_friend_list_get_callbacks(const LinphoneFriendList *list) {
	return list->cbs;
}

LinphoneFriendListCbs * linphone_friend_list_cbs_ref(LinphoneFriendListCbs *cbs) {
	belle_sip_object_ref(cbs);
	return cbs;
}

void linphone_friend_list_cbs_unref(LinphoneFriendListCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void *linphone_friend_list_cbs_get_user_data(const LinphoneFriendListCbs *cbs) {
	return cbs->user_data;
}

void linphone_friend_list_cbs_set_user_data(LinphoneFriendListCbs *cbs, void *ud) {
	cbs->user_data = ud;
}

LinphoneFriendListCbsContactCreatedCb linphone_friend_list_cbs_get_contact_created(const LinphoneFriendListCbs *cbs) {
	return cbs->contact_created_cb;
}

void linphone_friend_list_cbs_set_contact_created(LinphoneFriendListCbs *cbs, LinphoneFriendListCbsContactCreatedCb cb) {
	cbs->contact_created_cb = cb;
}

LinphoneFriendListCbsContactDeletedCb linphone_friend_list_cbs_get_contact_deleted(const LinphoneFriendListCbs *cbs) {
	return cbs->contact_deleted_cb;
}

void linphone_friend_list_cbs_set_contact_deleted(LinphoneFriendListCbs *cbs, LinphoneFriendListCbsContactDeletedCb cb) {
	cbs->contact_deleted_cb = cb;
}

LinphoneFriendListCbsContactUpdatedCb linphone_friend_list_cbs_get_contact_updated(const LinphoneFriendListCbs *cbs) {
	return cbs->contact_updated_cb;
}

void linphone_friend_list_cbs_set_contact_updated(LinphoneFriendListCbs *cbs, LinphoneFriendListCbsContactUpdatedCb cb) {
	cbs->contact_updated_cb = cb;
}

LinphoneFriendListCbsSyncStateChangedCb linphone_friend_list_cbs_get_sync_status_changed(const LinphoneFriendListCbs *cbs) {
	return cbs->sync_state_changed_cb;
}

void linphone_friend_list_cbs_set_sync_status_changed(LinphoneFriendListCbs *cbs, LinphoneFriendListCbsSyncStateChangedCb cb) {
	cbs->sync_state_changed_cb = cb;
}

static char * create_resource_list_xml(const LinphoneFriendList *list) {
	char *xml_content = NULL;
	MSList *elem;
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;

	if (ms_list_size(list->friends) <= 0) return NULL;

	buf = xmlBufferCreate();
	if (buf == NULL) {
		ms_error("%s: Error creating the XML buffer", __FUNCTION__);
		return NULL;
	}
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == NULL) {
		ms_error("%s: Error creating the XML writer", __FUNCTION__);
		return NULL;
	}

	xmlTextWriterSetIndent(writer,1);
	err = xmlTextWriterStartDocument(writer, "1.0", "UTF-8", NULL);
	if (err >= 0) {
		err = xmlTextWriterStartElementNS(writer, NULL, (const xmlChar *)"resource-lists", (const xmlChar *)"urn:ietf:params:xml:ns:resource-lists");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"xsi",
						    NULL, (const xmlChar *)"http://www.w3.org/2001/XMLSchema-instance");
	}

	if (err>= 0) {
		err = xmlTextWriterStartElement(writer, (const xmlChar *)"list");
	}
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *lf = (LinphoneFriend *)elem->data;
		char *uri = linphone_address_as_string_uri_only(lf->uri);
		if (err >= 0) {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"entry");
		}
		if (err >= 0) {
			err = xmlTextWriterWriteAttribute(writer, (const xmlChar *)"uri", (const xmlChar *)uri);
		}
		if (err >= 0) {
			/* Close the "entry" element. */
			err = xmlTextWriterEndElement(writer);
		}
		if (uri) ms_free(uri);
	}
	if (err >= 0) {
		/* Close the "list" element. */
		err = xmlTextWriterEndElement(writer);
	}

	if (err >= 0) {
		/* Close the "resource-lists" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		err = xmlTextWriterEndDocument(writer);
	}
	if (err > 0) {
		/* xmlTextWriterEndDocument returns the size of the content. */
		xml_content = ms_strdup((char *)buf->content);
	}
	xmlFreeTextWriter(writer);
	xmlBufferFree(buf);

	return xml_content;
}

static void linphone_friend_list_parse_multipart_related_body(LinphoneFriendList *list, const LinphoneContent *body, const char *first_part_body) {
	xmlparsing_context_t *xml_ctx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
	xml_ctx->doc = xmlReadDoc((const unsigned char*)first_part_body, 0, NULL, 0);
	if (xml_ctx->doc != NULL) {
		char xpath_str[MAX_XPATH_LENGTH];
		LinphoneFriend *lf;
		LinphoneContent *presence_part;
		xmlXPathObjectPtr resource_object;
		const char *version_str = NULL;
		const char *full_state_str = NULL;
		const char *uri = NULL;
		bool_t full_state = FALSE;
		int version;
		int i;

		if (linphone_create_xml_xpath_context(xml_ctx) < 0) goto end;
		xmlXPathRegisterNs(xml_ctx->xpath_ctx, (const xmlChar *)"rlmi", (const xmlChar *)"urn:ietf:params:xml:ns:rlmi");

		version_str = linphone_get_xml_attribute_text_content(xml_ctx, "/rlmi:list", "version");
		if (version_str == NULL) {
			ms_warning("rlmi+xml: No version attribute in list");
			goto end;
		}
		version = atoi(version_str);
		linphone_free_xml_text_content(version_str);
		if (version < list->expected_notification_version) {
			ms_warning("rlmi+xml: Discarding received notification with version %d because %d was expected", version, list->expected_notification_version);
			linphone_friend_list_update_subscriptions(list, NULL, FALSE); /* Refresh subscription to get new full state notify. */
			goto end;
		}

		full_state_str = linphone_get_xml_attribute_text_content(xml_ctx, "/rlmi:list", "fullState");
		if (full_state_str == NULL) {
			ms_warning("rlmi+xml: No fullState attribute in list");
			goto end;
		}
		if ((strcmp(full_state_str, "true") == 0) || (strcmp(full_state_str, "1") == 0)) {
			MSList *l = list->friends;
			for (; l != NULL; l = l->next) {
				lf = (LinphoneFriend *)l->data;
				linphone_friend_set_presence_model(lf, NULL);
			}
			full_state = TRUE;
		}
		linphone_free_xml_text_content(full_state_str);
		if ((list->expected_notification_version == 0) && (full_state == FALSE)) {
			ms_warning("rlmi+xml: Notification with version 0 is not full state, this is not valid");
			goto end;
		}
		list->expected_notification_version = version + 1;

		resource_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, "/rlmi:list/rlmi:resource");
		if ((resource_object != NULL) && (resource_object->nodesetval != NULL)) {
			for (i = 1; i <= resource_object->nodesetval->nodeNr; i++) {
				snprintf(xpath_str, sizeof(xpath_str), "/rlmi:list/rlmi:resource[%i]/@uri", i);
				uri = linphone_get_xml_text_content(xml_ctx, xpath_str);
				if (uri == NULL) continue;
				lf = linphone_friend_list_find_friend_by_uri(list, uri);
				if (lf != NULL) {
					const char *state = NULL;
					snprintf(xpath_str, sizeof(xpath_str),"/rlmi:list/rlmi:resource[%i]/rlmi:instance/@state", i);
					state = linphone_get_xml_text_content(xml_ctx, xpath_str);
					if ((state != NULL) && (strcmp(state, "active") == 0)) {
						const char *cid = NULL;
						snprintf(xpath_str, sizeof(xpath_str),"/rlmi:list/rlmi:resource[%i]/rlmi:instance/@cid", i);
						cid = linphone_get_xml_text_content(xml_ctx, xpath_str);
						if (cid != NULL) {
							presence_part = linphone_content_find_part_by_header(body, "Content-Id", cid);
							if (presence_part == NULL) {
								ms_warning("rlmi+xml: Cannot find part with Content-Id: %s", cid);
							} else {
								SalPresenceModel *presence = NULL;
								linphone_notify_parse_presence(linphone_content_get_type(presence_part), linphone_content_get_subtype(presence_part), linphone_content_get_string_buffer(presence_part), &presence);
								if (presence != NULL) {
									lf->presence_received = TRUE;
									linphone_friend_set_presence_model(lf, (LinphonePresenceModel *)presence);
									if (full_state == FALSE) {
										linphone_core_notify_notify_presence_received(list->lc, lf);
									}
								}
								linphone_content_unref(presence_part);
							}
						}
						if (cid != NULL) linphone_free_xml_text_content(cid);
					}
					if (state != NULL) linphone_free_xml_text_content(state);
					lf->subscribe_active = TRUE;
				}
				linphone_free_xml_text_content(uri);
			}
		}
		if (resource_object != NULL) xmlXPathFreeObject(resource_object);

		if (full_state == TRUE) {
			MSList *l = list->friends;
			for (; l != NULL; l = l->next) {
				lf = (LinphoneFriend *)l->data;
				if (linphone_friend_is_presence_received(lf) == TRUE) {
					linphone_core_notify_notify_presence_received(list->lc, lf);
				}
			}
		}
	} else {
		ms_warning("Wrongly formatted rlmi+xml body: %s", xml_ctx->errorBuffer);
	}

end:
	linphone_xmlparsing_context_destroy(xml_ctx);
}

static bool_t linphone_friend_list_has_subscribe_inactive(const LinphoneFriendList *list) {
	MSList *l = list->friends;
	bool_t has_subscribe_inactive = FALSE;
	for (; l != NULL; l = l->next) {
		LinphoneFriend *lf = (LinphoneFriend *)l->data;
		if (lf->subscribe_active != TRUE) {
			has_subscribe_inactive = TRUE;
			break;
		}
	}
	return has_subscribe_inactive;
}

static LinphoneFriendList * linphone_friend_list_new(void) {
	LinphoneFriendList *list = belle_sip_object_new(LinphoneFriendList);
	list->cbs = linphone_friend_list_cbs_new();
	belle_sip_object_ref(list);
	return list;
}

static void linphone_friend_list_destroy(LinphoneFriendList *list) {
	if (list->display_name != NULL) ms_free(list->display_name);
	if (list->rls_uri != NULL) ms_free(list->rls_uri);
	if (list->content_digest != NULL) ms_free(list->content_digest);
	if (list->event != NULL) {
		linphone_event_terminate(list->event);
		linphone_event_unref(list->event);
		list->event = NULL;
	}
	if (list->uri != NULL) ms_free(list->uri);
	if (list->cbs) linphone_friend_list_cbs_unref(list->cbs);
	if (list->dirty_friends_to_update) list->dirty_friends_to_update = ms_list_free_with_data(list->dirty_friends_to_update, (void (*)(void *))linphone_friend_unref);
	if (list->friends) list->friends = ms_list_free_with_data(list->friends, (void (*)(void *))_linphone_friend_release);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneFriendList);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneFriendList, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_friend_list_destroy,
	NULL, // clone
	NULL, // marshal
	TRUE
);


LinphoneFriendList * linphone_core_create_friend_list(LinphoneCore *lc) {
	LinphoneFriendList *list = linphone_friend_list_new();
	list->lc = lc;
	return list;
}

LinphoneFriendList * linphone_friend_list_ref(LinphoneFriendList *list) {
	belle_sip_object_ref(list);
	return list;
}

void _linphone_friend_list_release(LinphoneFriendList *list){
	/*drops all references to core and unref*/
	list->lc = NULL;
	if (list->event != NULL) {
		linphone_event_unref(list->event);
		list->event = NULL;
	}
	if (list->cbs) {
		linphone_friend_list_cbs_unref(list->cbs);
		list->cbs = NULL;
	}
	if (list->dirty_friends_to_update) {
		list->dirty_friends_to_update = ms_list_free_with_data(list->dirty_friends_to_update, (void (*)(void *))linphone_friend_unref);
	}
	if (list->friends) {
		list->friends = ms_list_free_with_data(list->friends, (void (*)(void *))_linphone_friend_release);
	}
	linphone_friend_list_unref(list);
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
		linphone_core_store_friends_list_in_db(list->lc, list);
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
		linphone_core_store_friends_list_in_db(list->lc, list);
	}
}

static LinphoneFriendListStatus _linphone_friend_list_add_friend(LinphoneFriendList *list, LinphoneFriend *lf, bool_t synchronize) {
	if (!list || !lf->uri || lf->friend_list) {
		if (!list)
			ms_error("linphone_friend_list_add_friend(): invalid list, null");
		if (!lf->uri)
			ms_error("linphone_friend_list_add_friend(): invalid friend, no sip uri");
		if (lf->friend_list)
			ms_error("linphone_friend_list_add_friend(): invalid friend, already in list");
		return LinphoneFriendListInvalidFriend;
	}
	if (ms_list_find(list->friends, lf) != NULL) {
		char *tmp = NULL;
		const LinphoneAddress *addr = linphone_friend_get_address(lf);
		if (addr) tmp = linphone_address_as_string(addr);
		ms_warning("Friend %s already in list [%s], ignored.", tmp ? tmp : "unknown", list->display_name);
		if (tmp) ms_free(tmp);
	} else {
		return linphone_friend_list_import_friend(list, lf, synchronize);
	}
	return LinphoneFriendListOK;
}

LinphoneFriendListStatus linphone_friend_list_add_friend(LinphoneFriendList *list, LinphoneFriend *lf) {
	return _linphone_friend_list_add_friend(list, lf, TRUE);
}

LinphoneFriendListStatus linphone_friend_list_add_local_friend(LinphoneFriendList *list, LinphoneFriend *lf) {
	return _linphone_friend_list_add_friend(list, lf, FALSE);
}

LinphoneFriendListStatus linphone_friend_list_import_friend(LinphoneFriendList *list, LinphoneFriend *lf, bool_t synchronize) {
	if (!lf->uri || lf->friend_list) {
		if (!lf->uri)
			ms_error("linphone_friend_list_add_friend(): invalid friend, no sip uri");
		if (lf->friend_list)
			ms_error("linphone_friend_list_add_friend(): invalid friend, already in list");
		return LinphoneFriendListInvalidFriend;
	}
	lf->friend_list = list;
	lf->lc = list->lc;
	list->friends = ms_list_append(list->friends, linphone_friend_ref(lf));
	if (synchronize) {
		list->dirty_friends_to_update = ms_list_append(list->dirty_friends_to_update, linphone_friend_ref(lf));
	}
#ifdef FRIENDS_SQL_STORAGE_ENABLED
	linphone_core_store_friend_in_db(lf->lc, lf);
#endif
	return LinphoneFriendListOK;
}

static void carddav_done(LinphoneCardDavContext *cdc, bool_t success, const char *msg) {
	if (cdc && cdc->friend_list->cbs->sync_state_changed_cb) {
		cdc->friend_list->cbs->sync_state_changed_cb(cdc->friend_list, success ? LinphoneFriendListSyncSuccessful : LinphoneFriendListSyncFailure, msg);
	}
	linphone_carddav_context_destroy(cdc);
}

static LinphoneFriendListStatus _linphone_friend_list_remove_friend(LinphoneFriendList *list, LinphoneFriend *lf, bool_t remove_from_server) {
	MSList *elem = ms_list_find(list->friends, lf);
	if (elem == NULL) return LinphoneFriendListNonExistentFriend;

#ifdef FRIENDS_SQL_STORAGE_ENABLED
	if (lf && lf->lc && lf->lc->friends_db) {
		linphone_core_remove_friend_from_db(lf->lc, lf);
	}
#endif
	if (remove_from_server) {
		LinphoneVcard *lvc = linphone_friend_get_vcard(lf);
		if (lvc && linphone_vcard_get_uid(lvc)) {
			LinphoneCardDavContext *cdc = linphone_carddav_context_new(list);
			if (cdc) {
				cdc->sync_done_cb = carddav_done;
				if (cdc->friend_list->cbs->sync_state_changed_cb) {
					cdc->friend_list->cbs->sync_state_changed_cb(cdc->friend_list, LinphoneFriendListSyncStarted, NULL);
				}
				linphone_carddav_delete_vcard(cdc, lf);
			}
		}
	}

	lf->friend_list = NULL;
	linphone_friend_unref(lf);
	list->friends = ms_list_remove_link(list->friends, elem);
	return LinphoneFriendListOK;
}

LinphoneFriendListStatus linphone_friend_list_remove_friend(LinphoneFriendList *list, LinphoneFriend *lf) {
	return _linphone_friend_list_remove_friend(list, lf, TRUE);
}

const MSList * linphone_friend_list_get_friends(const LinphoneFriendList *list) {
	return list->friends;
}

void linphone_friend_list_update_dirty_friends(LinphoneFriendList *list) {
	LinphoneCardDavContext *cdc = linphone_carddav_context_new(list);
	MSList *dirty_friends = list->dirty_friends_to_update;

	if (cdc) {
		cdc->sync_done_cb = carddav_done;
		while (dirty_friends) {
			LinphoneFriend *lf = (LinphoneFriend *)dirty_friends->data;
			if (lf) {
				if (cdc->friend_list->cbs->sync_state_changed_cb) {
					cdc->friend_list->cbs->sync_state_changed_cb(cdc->friend_list, LinphoneFriendListSyncStarted, NULL);
				}
				linphone_carddav_put_vcard(cdc, lf);
			}
			dirty_friends = ms_list_next(dirty_friends);
		}
		list->dirty_friends_to_update = ms_list_free_with_data(list->dirty_friends_to_update, (void (*)(void *))linphone_friend_unref);
	}
}

static void carddav_created(LinphoneCardDavContext *cdc, LinphoneFriend *lf) {
	if (cdc) {
		LinphoneFriendList *lfl = cdc->friend_list;
		linphone_friend_list_import_friend(lfl, lf, FALSE);
		if (cdc->friend_list->cbs->contact_created_cb) {
			cdc->friend_list->cbs->contact_created_cb(lfl, lf);
		}
	}
}

static void carddav_removed(LinphoneCardDavContext *cdc, LinphoneFriend *lf) {
	if (cdc) {
		LinphoneFriendList *lfl = cdc->friend_list;
		_linphone_friend_list_remove_friend(lfl, lf, FALSE);
		if (cdc->friend_list->cbs->contact_deleted_cb) {
			cdc->friend_list->cbs->contact_deleted_cb(lfl, lf);
		}
	}
}

static void carddav_updated(LinphoneCardDavContext *cdc, LinphoneFriend *lf_new, LinphoneFriend *lf_old) {
	if (cdc) {
		LinphoneFriendList *lfl = cdc->friend_list;
		MSList *elem = ms_list_find(lfl->friends, lf_old);
		if (elem) {
			elem->data = linphone_friend_ref(lf_new);
		}
		linphone_core_store_friend_in_db(lf_new->lc, lf_new);

		if (cdc->friend_list->cbs->contact_updated_cb) {
			cdc->friend_list->cbs->contact_updated_cb(lfl, lf_new, lf_old);
		}
		linphone_friend_unref(lf_old);
	}
}

void linphone_friend_list_synchronize_friends_from_server(LinphoneFriendList *list) {
	LinphoneCardDavContext *cdc = linphone_carddav_context_new(list);

	if (cdc) {
		cdc->contact_created_cb = carddav_created;
		cdc->contact_removed_cb = carddav_removed;
		cdc->contact_updated_cb = carddav_updated;
		cdc->sync_done_cb = carddav_done;
		if (cdc && cdc->friend_list->cbs->sync_state_changed_cb) {
			cdc->friend_list->cbs->sync_state_changed_cb(cdc->friend_list, LinphoneFriendListSyncStarted, NULL);
		}
		linphone_carddav_synchronize(cdc);
	}
}

LinphoneFriend * linphone_friend_list_find_friend_by_address(const LinphoneFriendList *list, const LinphoneAddress *address) {
	LinphoneFriend *lf = NULL;
	const MSList *elem;
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		lf = (LinphoneFriend *)elem->data;
		if (linphone_address_weak_equal(lf->uri, address))
			return lf;
	}
	return NULL;
}

LinphoneFriend * linphone_friend_list_find_friend_by_uri(const LinphoneFriendList *list, const char *uri) {
	LinphoneAddress *address = linphone_address_new(uri);
	LinphoneFriend *lf = address ? linphone_friend_list_find_friend_by_address(list, address) : NULL;
	if (address) linphone_address_unref(address);
	return lf;
}

LinphoneFriend * linphone_friend_list_find_friend_by_ref_key(const LinphoneFriendList *list, const char *ref_key) {
	const MSList *elem;
	if (ref_key == NULL) return NULL;
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *lf = (LinphoneFriend *)elem->data;
		if ((lf->refkey != NULL) && (strcmp(lf->refkey, ref_key) == 0)) return lf;
	}
	return NULL;
}

LinphoneFriend * linphone_friend_list_find_friend_by_inc_subscribe(const LinphoneFriendList *list, SalOp *op) {
	const MSList *elem;
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *lf = (LinphoneFriend *)elem->data;
		if (ms_list_find(lf->insubs, op)) return lf;
	}
	return NULL;
}

LinphoneFriend * linphone_friend_list_find_friend_by_out_subscribe(const LinphoneFriendList *list, SalOp *op) {
	const MSList *elem;
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *lf = (LinphoneFriend *)elem->data;
		if (lf->outsub && ((lf->outsub == op) || sal_op_is_forked_of(lf->outsub, op))) return lf;
	}
	return NULL;
}

void linphone_friend_list_close_subscriptions(LinphoneFriendList *list) {
	 /* FIXME we should wait until subscription to complete. */
	if (list->event) {
		linphone_event_terminate(list->event);
		linphone_event_unref(list->event);
		list->event = NULL;
	} else if (list->friends)
		ms_list_for_each(list->friends, (void (*)(void *))linphone_friend_close_subscriptions);
}

void linphone_friend_list_update_subscriptions(LinphoneFriendList *list, LinphoneProxyConfig *cfg, bool_t only_when_registered) {
	const MSList *elem;
	if (list->rls_uri != NULL) {
		LinphoneAddress *address = linphone_address_new(list->rls_uri);
		char *xml_content = create_resource_list_xml(list);
		if ((address != NULL) && (xml_content != NULL) && (linphone_friend_list_has_subscribe_inactive(list) == TRUE)) {
			unsigned char digest[16];
			bctoolbox_md5((unsigned char *)xml_content, strlen(xml_content), digest);
			if ((list->event != NULL) && (list->content_digest != NULL) && (memcmp(list->content_digest, digest, sizeof(digest)) == 0)) {
				/* The content has not changed, only refresh the event. */
				linphone_event_refresh_subscribe(list->event);
			} else {
				LinphoneContent *content;
				int expires = lp_config_get_int(list->lc->config, "sip", "rls_presence_expires", 3600);
				list->expected_notification_version = 0;
				if (list->content_digest != NULL) ms_free(list->content_digest);
				list->content_digest = ms_malloc(sizeof(digest));
				memcpy(list->content_digest, digest, sizeof(digest));
				if (list->event != NULL) {
					linphone_event_terminate(list->event);
					linphone_event_unref(list->event);
				}
				list->event = linphone_core_create_subscribe(list->lc, address, "presence", expires);
				linphone_event_ref(list->event);
				linphone_event_set_internal(list->event, TRUE);
				linphone_event_add_custom_header(list->event, "Require", "recipient-list-subscribe");
				linphone_event_add_custom_header(list->event, "Supported", "eventlist");
				linphone_event_add_custom_header(list->event, "Accept", "multipart/related, application/pidf+xml, application/rlmi+xml");
				linphone_event_add_custom_header(list->event, "Content-Disposition", "recipient-list");
				content = linphone_core_create_content(list->lc);
				linphone_content_set_type(content, "application");
				linphone_content_set_subtype(content, "resource-lists+xml");
				linphone_content_set_string_buffer(content, xml_content);
				if (linphone_core_content_encoding_supported(list->lc, "deflate")) {
					linphone_content_set_encoding(content, "deflate");
					linphone_event_add_custom_header(list->event, "Accept-Encoding", "deflate");
				}
				linphone_event_send_subscribe(list->event, content);
				linphone_content_unref(content);
				linphone_event_set_user_data(list->event, list);
			}
		}
		if (address != NULL) linphone_address_unref(address);
		if (xml_content != NULL) ms_free(xml_content);
	} else {
		for (elem = list->friends; elem != NULL; elem = elem->next) {
			LinphoneFriend *lf = (LinphoneFriend *)elem->data;
			linphone_friend_update_subscribes(lf, cfg, only_when_registered);
		}
	}
}

void linphone_friend_list_invalidate_subscriptions(LinphoneFriendList *list) {
	const MSList *elem;
	for (elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *lf = (LinphoneFriend *)elem->data;
		linphone_friend_invalidate_subscription(lf);
	}
}

void linphone_friend_list_notify_presence(LinphoneFriendList *list, LinphonePresenceModel *presence) {
	const MSList *elem;
	for(elem = list->friends; elem != NULL; elem = elem->next) {
		LinphoneFriend *lf = (LinphoneFriend *)elem->data;
		linphone_friend_notify(lf, presence);
	}
}

void linphone_friend_list_notify_presence_received(LinphoneFriendList *list, LinphoneEvent *lev, const LinphoneContent *body) {
	if (linphone_content_is_multipart(body)) {
		LinphoneContent *first_part;
		const char *type = linphone_content_get_type(body);
		const char *subtype = linphone_content_get_subtype(body);

		if ((strcmp(type, "multipart") != 0) || (strcmp(subtype, "related") != 0)) {
			ms_warning("multipart presence notified but it is not 'multipart/related'");
			return;
		}

		first_part = linphone_content_get_part(body, 0);
		if (first_part == NULL) {
			ms_warning("'multipart/related' presence notified but it doesn't contain any part");
			return;
		}

		type = linphone_content_get_type(first_part);
		subtype = linphone_content_get_subtype(first_part);
		if ((strcmp(type, "application") != 0) || (strcmp(subtype, "rlmi+xml") != 0)) {
			ms_warning("multipart presence notified but first part is not 'application/rlmi+xml'");
			linphone_content_unref(first_part);
			return;
		}

		linphone_friend_list_parse_multipart_related_body(list, body, linphone_content_get_string_buffer(first_part));
		linphone_content_unref(first_part);
	}
}

const char * linphone_friend_list_get_uri(const LinphoneFriendList *list) {
	return list->uri;
}

void linphone_friend_list_set_uri(LinphoneFriendList *list, const char *uri) {
	if (list->uri != NULL) {
		ms_free(list->uri);
		list->uri = NULL;
	}
	if (uri != NULL) {
		list->uri = ms_strdup(uri);
		linphone_core_store_friends_list_in_db(list->lc, list);
	}
}

void linphone_friend_list_update_revision(LinphoneFriendList *list, int rev) {
	list->revision = rev;
	linphone_core_store_friends_list_in_db(list->lc, list);
}

void linphone_friend_list_subscription_state_changed(LinphoneCore *lc, LinphoneEvent *lev, LinphoneSubscriptionState state) {
	LinphoneFriendList *list = (LinphoneFriendList *)linphone_event_get_user_data(lev);
	if (!list) {
		ms_warning("core [%p] Receiving unexpected state [%s] for event [%p], no associated friend list",lc
					, linphone_subscription_state_to_string(state)
				   , lev);
	} else {
		ms_message("Receiving new state [%s] for event [%p] for friend list [%p]"
				   , linphone_subscription_state_to_string(state)
				   , lev
				   , list);

		if (state == LinphoneSubscriptionOutgoingProgress && linphone_event_get_reason(lev) == LinphoneReasonNoMatch) {
			ms_message("Resseting version count for friend list [%p]",list);
			list->expected_notification_version = 0;
		}
	}
}

LinphoneCore* linphone_friend_list_get_core(LinphoneFriendList *list) {
	return list->lc;
}

int linphone_friend_list_import_friends_from_vcard4_file(LinphoneFriendList *list, const char *vcard_file) {
	MSList *vcards = linphone_vcard_list_from_vcard4_file(vcard_file);
	int count = 0;

#ifndef VCARD_ENABLED
	ms_error("vCard support wasn't enabled at compilation time");
	return -1;
#endif
	if (!vcards) {
		ms_error("Failed to parse the file %s", vcard_file);
		return -1;
	}
	if (!list) {
		ms_error("Can't import into a NULL list");
		return -1;
	}

	while (vcards != NULL && vcards->data != NULL) {
		LinphoneVcard *vcard = (LinphoneVcard *)vcards->data;
		LinphoneFriend *lf = linphone_friend_new_from_vcard(vcard);
		if (lf) {
			if (LinphoneFriendListOK == linphone_friend_list_import_friend(list, lf, TRUE)) {
				count++;
			}
			linphone_friend_unref(lf);
		} else {
			linphone_vcard_free(vcard);
		}
		vcards = ms_list_next(vcards);
	}
#ifndef FRIENDS_SQL_STORAGE_ENABLED
	linphone_core_write_friends_config(list->lc);
#endif
	return count;
}

int linphone_friend_list_import_friends_from_vcard4_buffer(LinphoneFriendList *list, const char *vcard_buffer) {
	MSList *vcards = linphone_vcard_list_from_vcard4_buffer(vcard_buffer);
	int count = 0;

#ifndef VCARD_ENABLED
	ms_error("vCard support wasn't enabled at compilation time");
	return -1;
#endif
	if (!vcards) {
		ms_error("Failed to parse the buffer");
		return -1;
	}
	if (!list) {
		ms_error("Can't import into a NULL list");
		return -1;
	}

	while (vcards != NULL && vcards->data != NULL) {
		LinphoneVcard *vcard = (LinphoneVcard *)vcards->data;
		LinphoneFriend *lf = linphone_friend_new_from_vcard(vcard);
		if (lf) {
			if (LinphoneFriendListOK == linphone_friend_list_import_friend(list, lf, TRUE)) {
				count++;
			}
			linphone_friend_unref(lf);
		} else {
			linphone_vcard_free(vcard);
		}
		vcards = ms_list_next(vcards);
	}
#ifndef FRIENDS_SQL_STORAGE_ENABLED
	linphone_core_write_friends_config(list->lc);
#endif
	return count;
}

void linphone_friend_list_export_friends_as_vcard4_file(LinphoneFriendList *list, const char *vcard_file) {
	FILE *file = NULL;
	const MSList *friends = linphone_friend_list_get_friends(list);

	file = fopen(vcard_file, "wb");
	if (file == NULL) {
		ms_warning("Could not write %s ! Maybe it is read-only. Contacts will not be saved.", vcard_file);
		return;
	}

#ifndef VCARD_ENABLED
	ms_error("vCard support wasn't enabled at compilation time");
#endif
	while (friends != NULL && friends->data != NULL) {
		LinphoneFriend *lf = (LinphoneFriend *)friends->data;
		LinphoneVcard *vcard = linphone_friend_get_vcard(lf);
		if (vcard) {
			const char *vcard_text = linphone_vcard_as_vcard4_string(vcard);
			fprintf(file, "%s", vcard_text);
		} else {
			ms_warning("Couldn't export friend %s because it doesn't have a vCard attached", linphone_address_as_string(linphone_friend_get_address(lf)));
		}
		friends = ms_list_next(friends);
	}

	fclose(file);
}
