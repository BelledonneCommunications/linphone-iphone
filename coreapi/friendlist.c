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
	belle_sip_object_ref(list);
	return list;
}

static void linphone_friend_list_destroy(LinphoneFriendList *list) {
	if (list->display_name != NULL) ms_free(list->display_name);
	if (list->rls_uri != NULL) ms_free(list->rls_uri);
	if (list->event != NULL) linphone_event_unref(list->event);
	list->friends = ms_list_free_with_data(list->friends, (void (*)(void *))linphone_friend_unref);
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
	list->friends = ms_list_free_with_data(list->friends, (void (*)(void *))_linphone_friend_release);
	belle_sip_object_unref(list);
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

LinphoneFriendListStatus linphone_friend_list_add_friend(LinphoneFriendList *list, LinphoneFriend *lf) {
	if (lf->uri == NULL || lf->in_list) {
		ms_error("linphone_friend_list_add_friend(): invalid friend");
		return LinphoneFriendListInvalidFriend;
	}
	if (ms_list_find(list->friends, lf) != NULL) {
		char *tmp = NULL;
		const LinphoneAddress *addr = linphone_friend_get_address(lf);
		if (addr) tmp = linphone_address_as_string(addr);
		ms_warning("Friend %s already in list [%s], ignored.", tmp ? tmp : "unknown", list->display_name);
		if (tmp) ms_free(tmp);
	} else {
		return linphone_friend_list_import_friend(list, lf);
	}
	return LinphoneFriendListOK;
}

LinphoneFriendListStatus linphone_friend_list_import_friend(LinphoneFriendList *list, LinphoneFriend *lf) {
	if ((lf->lc != NULL) || (lf->uri == NULL)) return LinphoneFriendListInvalidFriend;
	list->friends = ms_list_append(list->friends, linphone_friend_ref(lf));
	lf->in_list = TRUE;
	return LinphoneFriendListOK;
}

LinphoneFriendListStatus linphone_friend_list_remove_friend(LinphoneFriendList *list, LinphoneFriend *lf) {
	MSList *elem = ms_list_find(list->friends, lf);
	if (elem == NULL) return LinphoneFriendListNonExistentFriend;

#ifdef FRIENDS_SQL_STORAGE_ENABLED
	linphone_core_remove_friend_from_db(lf->lc, lf);
#endif
	
	lf->in_list = FALSE;
	linphone_friend_unref(lf);
	list->friends = ms_list_remove_link(list->friends, elem);
	return LinphoneFriendListOK;
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
	if (list->friends)
		ms_list_for_each(list->friends, (void (*)(void *))linphone_friend_close_subscriptions);
}

void linphone_friend_list_update_subscriptions(LinphoneFriendList *list, LinphoneProxyConfig *cfg, bool_t only_when_registered) {
	const MSList *elem;
	if (list->event != NULL) {
		linphone_event_refresh_subscribe(list->event);
	} else if (list->rls_uri != NULL) {
		LinphoneAddress *address = linphone_address_new(list->rls_uri);
		char *xml_content = create_resource_list_xml(list);
		if ((address != NULL) && (xml_content != NULL) && (linphone_friend_list_has_subscribe_inactive(list) == TRUE)) {
			LinphoneContent *content;
			int expires = lp_config_get_int(list->lc->config, "sip", "rls_presence_expires", 3600);
			list->expected_notification_version = 0;
			list->event = linphone_core_create_subscribe(list->lc, address, "presence", expires);
			linphone_event_set_internal(list->event, TRUE);
			linphone_event_add_custom_header(list->event, "Require", "recipient-list-subscribe");
			linphone_event_add_custom_header(list->event, "Supported", "eventlist");
			linphone_event_add_custom_header(list->event, "Accept", "multipart/related, application/pidf+xml, application/rlmi+xml");
			linphone_event_add_custom_header(list->event, "Content-Disposition", "recipient-list");
			content = linphone_core_create_content(list->lc);
			linphone_content_set_type(content, "application");
			linphone_content_set_subtype(content, "resource-lists+xml");
			linphone_content_set_string_buffer(content, xml_content);
			linphone_event_send_subscribe(list->event, content);
			linphone_content_unref(content);
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
