/*
carddav.c
Copyright (C) 2015  Belledonne Communications SARL

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

LinphoneCardDavContext* linphone_core_create_carddav_context(LinphoneCore *lc) {
	LinphoneCardDavContext *carddav_context = NULL;
	
	if (!lc) {
		return NULL;
	}
	
#ifdef VCARD_ENABLED
	carddav_context = (LinphoneCardDavContext *)ms_new0(LinphoneCardDavContext, 1);
	carddav_context->lc = lc;
	carddav_context->server_url = linphone_core_get_carddav_server_url(lc);
	carddav_context->ctag = linphone_core_get_carddav_last_ctag(lc);
	carddav_context->username = linphone_core_get_carddav_username(lc);
	carddav_context->password = linphone_core_get_carddav_password(lc);
	carddav_context->ha1 = linphone_core_get_carddav_ha1(lc);
#else
	ms_error("vCard isn't available (maybe it wasn't compiled), can't do CardDAV sync");
#endif
	return carddav_context;
}

void linphone_carddav_destroy(LinphoneCardDavContext *cdc) {
	if (cdc) {
		ms_free(cdc);
	}
}

void linphone_carddav_set_user_data(LinphoneCardDavContext *cdc, void *ud) {
	cdc->user_data = ud;
}

void* linphone_carddav_get_user_data(LinphoneCardDavContext *cdc) {
	return cdc->user_data;
}

void linphone_carddav_synchronize(LinphoneCardDavContext *cdc) {
	linphone_carddav_get_current_ctag(cdc);
}

static void linphone_carddav_sync_done(LinphoneCardDavContext *cdc, bool_t success, const char *msg) {
	if (success) {
		ms_debug("CardDAV sync successful, saving new cTag: %i", cdc->ctag);
		linphone_core_set_carddav_current_ctag(cdc->lc, cdc->ctag);
	} else {
		ms_error("CardDAV sync failure: %s", msg);
	}
	
	if (cdc->sync_done_cb) {
		cdc->sync_done_cb(cdc, success, msg);
	}
}

static int find_matching_friend(LinphoneFriend *lf1, LinphoneFriend *lf2) {
	LinphoneVCard *lvc1 = linphone_friend_get_vcard(lf1);
	LinphoneVCard *lvc2 = linphone_friend_get_vcard(lf2);
	const char *uid1 = linphone_vcard_get_uid(lvc1);
	const char *uid2 = linphone_vcard_get_uid(lvc2);
	if (!uid1 || !uid2) {
		return 1;
	}
	return strcmp(uid1, uid2);
}

static void linphone_carddav_vcards_pulled(LinphoneCardDavContext *cdc, MSList *vCards) {
	if (vCards != NULL && ms_list_size(vCards) > 0) {
		MSList *localFriends = linphone_core_fetch_friends_from_db(cdc->lc);
		while (vCards) {
			LinphoneCardDavResponse *vCard = (LinphoneCardDavResponse *)vCards->data;
			if (vCard) {
				LinphoneVCard *lvc = linphone_vcard_new_from_vcard4_buffer(vCard->vcard);
				LinphoneFriend *lf = NULL;
				MSList *local_friend = NULL;
				
				if (lvc) {
					// Compute downloaded vCards' URL and save it (+ eTag)
					char *vCard_name = strrchr(vCard->url, '/');
					char full_url[300];
					snprintf(full_url, sizeof(full_url), "%s%s", cdc->server_url, vCard_name);
					linphone_vcard_set_url(lvc, full_url);
					linphone_vcard_set_etag(lvc, vCard->etag);
				}
				lf = linphone_friend_new_from_vcard(lvc);
				local_friend = ms_list_find_custom(localFriends, (int (*)(const void*, const void*))find_matching_friend, lf);
				
				if (local_friend) {
					LinphoneFriend *lf2 = (LinphoneFriend *)local_friend->data;
					if (cdc->contact_updated_cb) {
						ms_debug("Contact updated: %s", linphone_friend_get_name(lf));
						lf2 = linphone_friend_ref(lf2);
						cdc->contact_updated_cb(cdc, lf, lf2);
					}
				} else {
					if (cdc->contact_created_cb) {
						ms_debug("Contact created: %s", linphone_friend_get_name(lf));
						cdc->contact_created_cb(cdc, lf);
					}
				}
			}
			vCards = ms_list_next(vCards);
		}
		localFriends = ms_list_free_with_data(localFriends, (void (*)(void *))linphone_friend_unref);
	}
	ms_list_free(vCards);
	linphone_carddav_sync_done(cdc, TRUE, "");
}

static MSList* parse_vcards_from_xml_response(const char *body) {
	MSList *result = NULL;
	xmlparsing_context_t *xml_ctx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
	xml_ctx->doc = xmlReadDoc((const unsigned char*)body, 0, NULL, 0);
	if (xml_ctx->doc != NULL) {
		if (linphone_create_xml_xpath_context(xml_ctx) < 0) goto end;
		linphone_xml_xpath_context_init_carddav_ns(xml_ctx);
		{
			xmlXPathObjectPtr responses = linphone_get_xml_xpath_object_for_node_list(xml_ctx, "/d:multistatus/d:response");
			if (responses != NULL && responses->nodesetval != NULL) {
				xmlNodeSetPtr responses_nodes = responses->nodesetval;
				if (responses_nodes->nodeNr >= 1) {
					int i;
					for (i = 0; i < responses_nodes->nodeNr; i++) {
						xmlNodePtr response_node = responses_nodes->nodeTab[i];
						xml_ctx->xpath_ctx->node = response_node;
						{
							char *etag = linphone_get_xml_text_content(xml_ctx, "d:propstat/d:prop/d:getetag");
							char *url =  linphone_get_xml_text_content(xml_ctx, "d:href");
							char *vcard = linphone_get_xml_text_content(xml_ctx, "d:propstat/d:prop/card:address-data");
							LinphoneCardDavResponse *response = ms_new0(LinphoneCardDavResponse, 1);
							response->etag = ms_strdup(etag);
							response->url = ms_strdup(url);
							response->vcard = ms_strdup(vcard);
							result = ms_list_append(result, response);
							ms_debug("Added vCard object with eTag %s, URL %s and vCard %s", etag, url, vcard);
						}
					}
				}
				xmlXPathFreeObject(responses);
			}
		}
	}
end:
	linphone_xmlparsing_context_destroy(xml_ctx);
	return result;
}

static int find_matching_vcard(LinphoneCardDavResponse *response, LinphoneFriend *lf) {
	LinphoneVCard *lvc1 = linphone_vcard_new_from_vcard4_buffer(response->vcard);
	LinphoneVCard *lvc2 = linphone_friend_get_vcard(lf);
	const char *uid1 = linphone_vcard_get_uid(lvc1);
	const char *uid2 = linphone_vcard_get_uid(lvc2);
	linphone_vcard_free(lvc1);
	if (!uid1 || !uid2) {
		return 1;
	}
	return strcmp(uid1, uid2);
}

static void linphone_carddav_vcards_fetched(LinphoneCardDavContext *cdc, MSList *vCards) {
	if (vCards != NULL && ms_list_size(vCards) > 0) {
		MSList *localFriends = linphone_core_fetch_friends_from_db(cdc->lc);
		MSList *friends = localFriends;
		while (friends) {
			LinphoneFriend *lf = (LinphoneFriend *)friends->data;
			if (lf) {
				MSList *vCard = ms_list_find_custom(vCards, (int (*)(const void*, const void*))find_matching_vcard, lf);
				if (!vCard) {
					ms_debug("Local friend %s isn't in the remote vCard list, delete it", linphone_friend_get_name(lf));
					if (cdc->contact_removed_cb) {
						ms_debug("Contact removed: %s", linphone_friend_get_name(lf));
						lf = linphone_friend_ref(lf);
						cdc->contact_removed_cb(cdc, lf);
					}
				} else {
					LinphoneCardDavResponse *response = (LinphoneCardDavResponse *)vCard->data;
					ms_debug("Local friend %s is in the remote vCard list, check eTag", linphone_friend_get_name(lf));
					if (response) {
						LinphoneVCard *lvc = linphone_friend_get_vcard(lf);
						ms_debug("Local friend eTag is %s, remote vCard eTag is %s", linphone_vcard_get_etag(lvc), response->etag);
						if (lvc && strcmp(linphone_vcard_get_etag(lvc), response->etag) == 0) {
							ms_list_remove(vCards, vCard);
							ms_free(response);
						}
					}
				}
			}
			friends = ms_list_next(friends);
		}
		localFriends = ms_list_free_with_data(localFriends, (void (*)(void *))linphone_friend_unref);
		linphone_carddav_pull_vcards(cdc, vCards);
	}
	ms_list_free(vCards);
}

static MSList* parse_vcards_etags_from_xml_response(const char *body) {
	MSList *result = NULL;
	xmlparsing_context_t *xml_ctx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
	xml_ctx->doc = xmlReadDoc((const unsigned char*)body, 0, NULL, 0);
	if (xml_ctx->doc != NULL) {
		if (linphone_create_xml_xpath_context(xml_ctx) < 0) goto end;
		linphone_xml_xpath_context_init_carddav_ns(xml_ctx);
		{
			xmlXPathObjectPtr responses = linphone_get_xml_xpath_object_for_node_list(xml_ctx, "/d:multistatus/d:response");
			if (responses != NULL && responses->nodesetval != NULL) {
				xmlNodeSetPtr responses_nodes = responses->nodesetval;
				if (responses_nodes->nodeNr >= 1) {
					int i;
					for (i = 0; i < responses_nodes->nodeNr; i++) {
						xmlNodePtr response_node = responses_nodes->nodeTab[i];
						xml_ctx->xpath_ctx->node = response_node;
						{
							char *etag = linphone_get_xml_text_content(xml_ctx, "d:propstat/d:prop/d:getetag");
							char *url =  linphone_get_xml_text_content(xml_ctx, "d:href");
							LinphoneCardDavResponse *response = ms_new0(LinphoneCardDavResponse, 1);
							response->etag = ms_strdup(etag);
							response->url = ms_strdup(url);
							result = ms_list_append(result, response);
							ms_debug("Added vCard object with eTag %s and URL %s", etag, url);
						}
					}
				}
				xmlXPathFreeObject(responses);
			}
		}
	}
end:
	linphone_xmlparsing_context_destroy(xml_ctx);
	return result;
}

static void linphone_carddav_ctag_fetched(LinphoneCardDavContext *cdc, int ctag) {
	ms_debug("Remote cTag for CardDAV addressbook is %i, local one is %i", ctag, cdc->ctag);
	if (ctag == -1 || ctag > cdc->ctag) {
		cdc->ctag = ctag;
		linphone_carddav_fetch_vcards(cdc);
	} else {
		ms_message("No changes found on server, skipping sync");
		linphone_carddav_sync_done(cdc, TRUE, "Synchronization skipped because cTag already up to date");
	}
}

static int parse_ctag_value_from_xml_response(const char *body) {
	int result = -1;
	xmlparsing_context_t *xml_ctx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
	xml_ctx->doc = xmlReadDoc((const unsigned char*)body, 0, NULL, 0);
	if (xml_ctx->doc != NULL) {
		char *response = NULL;
		if (linphone_create_xml_xpath_context(xml_ctx) < 0) goto end;
		linphone_xml_xpath_context_init_carddav_ns(xml_ctx);
		response = linphone_get_xml_text_content(xml_ctx, "/d:multistatus/d:response/d:propstat/d:prop/x1:getctag");
		if (response) {
			result = atoi(response);
			linphone_free_xml_text_content(response);
		}
	}
end:
	linphone_xmlparsing_context_destroy(xml_ctx);
	return result;
}

static void process_response_from_carddav_request(void *data, const belle_http_response_event_t *event) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)data;
	
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code == 207 || code == 200 || code == 201 || code == 204) {
			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			switch(query->type) {
			case LinphoneCardDavQueryTypePropfind:
				linphone_carddav_ctag_fetched(query->context, parse_ctag_value_from_xml_response(body));
				break;
			case LinphoneCardDavQueryTypeAddressbookQuery:
				linphone_carddav_vcards_fetched(query->context, parse_vcards_etags_from_xml_response(body));
				break;
			case LinphoneCardDavQueryTypeAddressbookMultiget:
				linphone_carddav_vcards_pulled(query->context, parse_vcards_from_xml_response(body));
				break;
			case LinphoneCardDavQueryTypePut:
				{
					belle_sip_header_t *header = belle_sip_message_get_header((belle_sip_message_t *)event->response, "ETag");
					LinphoneFriend *lf = (LinphoneFriend *)query->user_data;
					if (lf) {
						LinphoneVCard *lvc = linphone_friend_get_vcard(lf);
						if (header && lvc && !linphone_vcard_get_etag(lvc)) {
							const char *etag = belle_sip_header_get_unparsed_value(header);
							ms_debug("eTag for newly created vCard is: %s", etag);
							linphone_vcard_set_etag(lvc, etag);
						}
						linphone_carddav_sync_done(query->context, TRUE, "");
						linphone_friend_unref(lf);
					} else {
						linphone_carddav_sync_done(query->context, FALSE, "No LinphoneFriend found in user_date field of query");
					}
				}
				break;
			case LinphoneCardDavQueryTypeDelete:
				linphone_carddav_sync_done(query->context, TRUE, "");
				break;
			default:
				ms_error("Unknown request: %i", query->type);
				break;
			}
		} else {
			char msg[100];
			snprintf(msg, sizeof(msg), "Unexpected HTTP response code: %i", code);
			linphone_carddav_sync_done(query->context, FALSE, msg);
		}
	} else {
		linphone_carddav_sync_done(query->context, FALSE, "No response found");
	}
	ms_free(query);
}

static void process_io_error_from_carddav_request(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)data;
	ms_error("I/O error during CardDAV request sending");
	ms_free(query);
	linphone_carddav_sync_done(query->context, FALSE, "I/O error during CardDAV request sending");
}

static void process_auth_requested_from_carddav_request(void *data, belle_sip_auth_event_t *event) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)data;
	LinphoneCardDavContext *context = query->context;
	
	if (context->username && (context->password || context->ha1)) {
		belle_sip_auth_event_set_username(event, context->username);
		if (context->password) {
			belle_sip_auth_event_set_passwd(event, context->password);
		}
		if (context->ha1) {
			belle_sip_auth_event_set_ha1(event, context->ha1);
		}
	} else {
		ms_error("Authentication requested during CardDAV request sending, and username/password weren't provided");
		linphone_carddav_sync_done(query->context, FALSE, "Authentication requested during CardDAV request sending, and username/password weren't provided");
	}
}

static void linphone_carddav_send_query(LinphoneCardDavQuery *query) {
	belle_http_request_listener_callbacks_t cbs = { 0 };
	belle_http_request_listener_t *l = NULL;
	belle_generic_uri_t *uri = NULL;
	belle_http_request_t *req = NULL;
	belle_sip_memory_body_handler_t *bh = NULL;

	uri = belle_generic_uri_parse(query->url);
	if (!uri) {
		LinphoneCardDavContext *cdc = query->context;
		if (cdc && cdc->sync_done_cb) {
			cdc->sync_done_cb(cdc, FALSE, "Could not send request, URL is invalid");
		}
		belle_sip_error("Could not send request, URL %s is invalid", query->url);
		return;
	}
	if (query->depth) {
		req = belle_http_request_create(query->method, uri, belle_sip_header_content_type_create("application", "xml; charset=utf-8"), belle_sip_header_create("Depth", query->depth), NULL);
	} else if (query->ifmatch) {
		req = belle_http_request_create(query->method, uri, belle_sip_header_content_type_create("application", "xml; charset=utf-8"), belle_sip_header_create("If-Match", query->ifmatch), NULL);
	} else {
		if (strcmp(query->method, "PUT")) {
			req = belle_http_request_create(query->method, uri, belle_sip_header_content_type_create("application", "xml; charset=utf-8"), belle_sip_header_create("If-None-Match", "*"), NULL);
		} else {
			req = belle_http_request_create(query->method, uri, belle_sip_header_content_type_create("application", "xml; charset=utf-8"), NULL);
		}
	}
	if (!req) {
		LinphoneCardDavContext *cdc = query->context;
		if (cdc && cdc->sync_done_cb) {
			cdc->sync_done_cb(cdc, FALSE, "Could not create belle_http_request_t");
		}
		belle_sip_object_unref(uri);
		belle_sip_error("Could not create belle_http_request_t");
		return;
	}
	
	if (query->body) {
		bh = belle_sip_memory_body_handler_new_copy_from_buffer(query->body, strlen(query->body), NULL, NULL);
		belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req), bh ? BELLE_SIP_BODY_HANDLER(bh) : NULL);
	}
	
	cbs.process_response = process_response_from_carddav_request;
	cbs.process_io_error = process_io_error_from_carddav_request;
	cbs.process_auth_requested = process_auth_requested_from_carddav_request;
	l = belle_http_request_listener_create_from_callbacks(&cbs, query);
	belle_http_provider_send_request(query->context->lc->http_provider, req, l);
}

static LinphoneCardDavQuery* linphone_carddav_create_put_query(LinphoneCardDavContext *cdc, LinphoneVCard *lvc) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)ms_new0(LinphoneCardDavQuery, 1);
	query->context = cdc;
	query->depth = NULL;
	query->ifmatch = linphone_vcard_get_etag(lvc);
	query->body = linphone_vcard_as_vcard4_string(lvc);
	query->method = "PUT";
	query->url = linphone_vcard_get_url(lvc);
	query->type = LinphoneCardDavQueryTypePut;
	return query;
}

static char* generate_url_from_server_address_and_uid(const char *server_url) {
	if (server_url) {
		char uuid[32];
		if (sal_generate_uuid(uuid, sizeof(uuid)) == 0) {
			char url[300];
			snprintf(url, sizeof(url), "%s/linphone-%s.vcf", server_url, uuid);
			ms_debug("Generated url is %s", url);
			return ms_strdup(url);
		}
	}
	return NULL;
}

void linphone_carddav_put_vcard(LinphoneCardDavContext *cdc, LinphoneFriend *lf) {
	LinphoneVCard *lvc = linphone_friend_get_vcard(lf);
	if (lvc && linphone_vcard_get_uid(lvc)) {
		LinphoneCardDavQuery *query = NULL;
		
		if (!linphone_vcard_get_url(lvc)) {
			char *url = generate_url_from_server_address_and_uid(cdc->server_url);
			linphone_vcard_set_url(lvc, url);
			ms_free(url);
		}
		
		query = linphone_carddav_create_put_query(cdc, lvc);
		query->user_data = linphone_friend_ref(lf);
		linphone_carddav_send_query(query);
	} else {
		const char *msg = NULL;
		if (!lvc) {
			msg = "LinphoneVCard is NULL";
		} else if (!linphone_vcard_get_url(lvc)) {
			msg = "LinphoneVCard doesn't have an UID";
		}
		
		if (msg) {
			ms_error("%s", msg);
		}
		
		if (cdc && cdc->sync_done_cb) {
			cdc->sync_done_cb(cdc, FALSE, msg);
		}
	}
}

static LinphoneCardDavQuery* linphone_carddav_create_delete_query(LinphoneCardDavContext *cdc, LinphoneVCard *lvc) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)ms_new0(LinphoneCardDavQuery, 1);
	query->context = cdc;
	query->depth = NULL;
	query->ifmatch = linphone_vcard_get_etag(lvc);
	query->body = NULL;
	query->method = "DELETE";
	query->url = linphone_vcard_get_url(lvc);
	query->type = LinphoneCardDavQueryTypeDelete;
	return query;
}

void linphone_carddav_delete_vcard(LinphoneCardDavContext *cdc, LinphoneFriend *lf) {
	LinphoneVCard *lvc = linphone_friend_get_vcard(lf);
	if (lvc && linphone_vcard_get_uid(lvc) && linphone_vcard_get_etag(lvc)) {
		LinphoneCardDavQuery *query = NULL;
		
		if (!linphone_vcard_get_url(lvc)) {
			char *url = generate_url_from_server_address_and_uid(cdc->server_url);
			linphone_vcard_set_url(lvc, url);
			ms_free(url);
		}
		
		query = linphone_carddav_create_delete_query(cdc, lvc);
		linphone_carddav_send_query(query);
	} else {
		const char *msg = NULL;
		if (!lvc) {
			msg = "LinphoneVCard is NULL";
		} else if (!linphone_vcard_get_uid(lvc)) {
			msg = "LinphoneVCard doesn't have an UID";
		} else if (!linphone_vcard_get_etag(lvc)) {
			msg = "LinphoneVCard doesn't have an eTag";
		}
		
		if (msg) {
			ms_error("%s", msg);
		}
		
		if (cdc && cdc->sync_done_cb) {
			cdc->sync_done_cb(cdc, FALSE, msg);
		}
	}
}

void linphone_carddav_set_synchronization_done_callback(LinphoneCardDavContext *cdc, LinphoneCardDavSynchronizationDoneCb cb) {
	cdc->sync_done_cb = cb;
}

void linphone_carddav_set_new_contact_callback(LinphoneCardDavContext *cdc, LinphoneCardDavContactCreatedCb cb) {
	cdc->contact_created_cb = cb;
}

void linphone_carddav_set_updated_contact_callback(LinphoneCardDavContext *cdc, LinphoneCardDavContactUpdatedCb cb) {
	cdc->contact_updated_cb = cb;
}

void linphone_carddav_set_removed_contact_callback(LinphoneCardDavContext *cdc, LinphoneCardDavContactRemovedCb cb) {
	cdc->contact_removed_cb = cb;
}

static LinphoneCardDavQuery* linphone_carddav_create_propfind_query(LinphoneCardDavContext *cdc) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)ms_new0(LinphoneCardDavQuery, 1);
	query->context = cdc;
	query->depth = "0";
	query->ifmatch = NULL;
	query->body = "<d:propfind xmlns:d=\"DAV:\" xmlns:cs=\"http://calendarserver.org/ns/\"><d:prop><cs:getctag /></d:prop></d:propfind>";
	query->method = "PROPFIND";
	query->url = cdc->server_url;
	query->type = LinphoneCardDavQueryTypePropfind;
	return query;
}

void linphone_carddav_get_current_ctag(LinphoneCardDavContext *cdc) {
	LinphoneCardDavQuery *query = linphone_carddav_create_propfind_query(cdc);
	linphone_carddav_send_query(query);
}

static LinphoneCardDavQuery* linphone_carddav_create_addressbook_query(LinphoneCardDavContext *cdc) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)ms_new0(LinphoneCardDavQuery, 1);
	query->context = cdc;
	query->depth = "1";
	query->ifmatch = NULL;
	query->body = "<card:addressbook-query xmlns:d=\"DAV:\" xmlns:card=\"urn:ietf:params:xml:ns:carddav\"><d:prop><d:getetag /></d:prop></card:addressbook-query>";
	query->method = "REPORT";
	query->url = cdc->server_url;
	query->type = LinphoneCardDavQueryTypeAddressbookQuery;
	return query;
}

void linphone_carddav_fetch_vcards(LinphoneCardDavContext *cdc) {
	LinphoneCardDavQuery *query = linphone_carddav_create_addressbook_query(cdc);
	linphone_carddav_send_query(query);
}

static LinphoneCardDavQuery* linphone_carddav_create_addressbook_multiget_query(LinphoneCardDavContext *cdc, MSList *vcards) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)ms_new0(LinphoneCardDavQuery, 1);
	char body[(ms_list_size(vcards)+1)*300];
	MSList *iterator = vcards;
	
	query->context = cdc;
	query->depth = "1";
	query->ifmatch = NULL;
	query->method = "REPORT";
	query->url = cdc->server_url;
	query->type = LinphoneCardDavQueryTypeAddressbookMultiget;

	sprintf(body, "%s", "<card:addressbook-multiget xmlns:d=\"DAV:\" xmlns:card=\"urn:ietf:params:xml:ns:carddav\"><d:prop><d:getetag /><card:address-data content-type='text-vcard' version='4.0'/></d:prop>");
	while (iterator) {
		LinphoneCardDavResponse *response = (LinphoneCardDavResponse *)iterator->data;
		if (response) {
			char temp_body[300];
			snprintf(temp_body, sizeof(temp_body), "<d:href>%s</d:href>", response->url);
			sprintf(body, "%s%s", body, temp_body);
			iterator = ms_list_next(iterator);
		}
	}
	sprintf(body, "%s%s", body, "</card:addressbook-multiget>");
	query->body = ms_strdup(body);
	
	return query;
}

void linphone_carddav_pull_vcards(LinphoneCardDavContext *cdc, MSList *vcards_to_pull) {
	LinphoneCardDavQuery *query = linphone_carddav_create_addressbook_multiget_query(cdc, vcards_to_pull);
	linphone_carddav_send_query(query);
}