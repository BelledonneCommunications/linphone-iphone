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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "linphone/core.h"
#include "private.h"

LinphoneCardDavContext* linphone_carddav_context_new(LinphoneFriendList *lfl) {
	LinphoneCardDavContext *carddav_context = NULL;

	if (!linphone_core_vcard_supported()) {
		ms_error("[carddav] vCard isn't available (maybe it wasn't compiled), can't do CardDAV sync");
		return NULL;
	}
	if (!lfl || !lfl->uri) {
		return NULL;
	}

	carddav_context = (LinphoneCardDavContext *)ms_new0(LinphoneCardDavContext, 1);
	carddav_context->friend_list = linphone_friend_list_ref(lfl);
	return carddav_context;
}

void linphone_carddav_context_destroy(LinphoneCardDavContext *cdc) {
	if (cdc) {
		if (cdc->friend_list) {
			linphone_friend_list_unref(cdc->friend_list);
			cdc->friend_list = NULL;
		}
		if (cdc->auth_info) {
			linphone_auth_info_destroy(cdc->auth_info);
			cdc->auth_info = NULL;
		}
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
	cdc->ctag = cdc->friend_list->revision;
	linphone_carddav_get_current_ctag(cdc);
}

static void linphone_carddav_client_to_server_sync_done(LinphoneCardDavContext *cdc, bool_t success, const char *msg) {
	if (!success) {
		ms_error("[carddav] CardDAV client to server sync failure: %s", msg);
	}
	
	if (cdc->sync_done_cb) {
		cdc->sync_done_cb(cdc, success, msg);
	}
}

static void linphone_carddav_server_to_client_sync_done(LinphoneCardDavContext *cdc, bool_t success, const char *msg) {
	if (success) {
		ms_debug("CardDAV sync successful, saving new cTag: %i", cdc->ctag);
		linphone_friend_list_update_revision(cdc->friend_list, cdc->ctag);
	} else {
		ms_error("[carddav] CardDAV server to client sync failure: %s", msg);
	}
	
	if (cdc->sync_done_cb) {
		cdc->sync_done_cb(cdc, success, msg);
	}
}

static int find_matching_friend(LinphoneFriend *lf1, LinphoneFriend *lf2) {
	LinphoneVcard *lvc1 = linphone_friend_get_vcard(lf1);
	LinphoneVcard *lvc2 = linphone_friend_get_vcard(lf2);
	const char *uid1 = NULL, *uid2 = NULL;
	if (!lvc1 || !lvc2) {
		return 1;
	}
	uid1 = linphone_vcard_get_uid(lvc1);
	uid2 = linphone_vcard_get_uid(lvc2);
	if (!uid1 || !uid2) {
		return 1;
	}
	return strcmp(uid1, uid2);
}

static void linphone_carddav_response_free(LinphoneCardDavResponse *response) {
	if (response->etag) ms_free(response->etag);
	if (response->url) ms_free(response->url);
	if (response->vcard) ms_free(response->vcard);
	ms_free(response);
}

static void linphone_carddav_vcards_pulled(LinphoneCardDavContext *cdc, bctbx_list_t *vCards) {
	bctbx_list_t *vCards_remember = vCards;
	if (vCards != NULL && bctbx_list_size(vCards) > 0) {
		bctbx_list_t *friends = cdc->friend_list->friends;
		while (vCards) {
			LinphoneCardDavResponse *vCard = (LinphoneCardDavResponse *)vCards->data;
			if (vCard) {
				LinphoneVcard *lvc = linphone_vcard_context_get_vcard_from_buffer(cdc->friend_list->lc->vcard_context, vCard->vcard);
				LinphoneFriend *lf = NULL;
				bctbx_list_t *local_friend = NULL;
				
				if (lvc) {
					// Compute downloaded vCards' URL and save it (+ eTag)
					char *vCard_name = strrchr(vCard->url, '/');
					char full_url[300];
					snprintf(full_url, sizeof(full_url), "%s%s", cdc->friend_list->uri, vCard_name);
					linphone_vcard_set_url(lvc, full_url);
					linphone_vcard_set_etag(lvc, vCard->etag);
					ms_debug("Downloaded vCard etag/url are %s and %s", vCard->etag, full_url);

					lf = linphone_friend_new_from_vcard(lvc);
					if (lf) {
						local_friend = bctbx_list_find_custom(friends, (int (*)(const void*, const void*))find_matching_friend, lf);
						
						if (local_friend) {
							LinphoneFriend *lf2 = (LinphoneFriend *)local_friend->data;
							lf->storage_id = lf2->storage_id;
							lf->pol = lf2->pol;
							lf->subscribe = lf2->subscribe;
							lf->refkey = ms_strdup(lf2->refkey);
							lf->presence_received = lf2->presence_received;
							lf->lc = lf2->lc;
							lf->friend_list = lf2->friend_list;
							
							if (cdc->contact_updated_cb) {
								ms_debug("Contact updated: %s", linphone_friend_get_name(lf));
								cdc->contact_updated_cb(cdc, lf, lf2);
							}
						} else {
							if (cdc->contact_created_cb) {
								ms_debug("Contact created: %s", linphone_friend_get_name(lf));
								cdc->contact_created_cb(cdc, lf);
							}
						}
						linphone_friend_unref(lf);
					} else {
						ms_error("[carddav] Couldn't create a friend from vCard");
					}
				} else {
					ms_error("[carddav] Couldn't parse vCard %s", vCard->vcard);
				}
			}
			vCards = bctbx_list_next(vCards);
		}
		bctbx_list_free_with_data(vCards_remember, (void (*)(void *))linphone_carddav_response_free);
	}
	linphone_carddav_server_to_client_sync_done(cdc, TRUE, NULL);
}

static bctbx_list_t* parse_vcards_from_xml_response(const char *body) {
	bctbx_list_t *result = NULL;
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
							result = bctbx_list_append(result, response);
							ms_debug("Added vCard object with eTag %s, URL %s and vCard %s", etag, url, vcard);
							linphone_free_xml_text_content(etag);
							linphone_free_xml_text_content(url);
							linphone_free_xml_text_content(vcard);
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
	if (!response->url || !lf || !lf->vcard || !linphone_vcard_get_url(lf->vcard)) {
		return 1;
	}
	return strcmp(response->url, linphone_vcard_get_url(lf->vcard));
}

static void linphone_carddav_vcards_fetched(LinphoneCardDavContext *cdc, bctbx_list_t *vCards) {
	if (vCards != NULL && bctbx_list_size(vCards) > 0) {
		bctbx_list_t *friends = cdc->friend_list->friends;
		bctbx_list_t *friends_to_remove = NULL;
		bctbx_list_t *temp_list = NULL;
		
		while (friends) {
			LinphoneFriend *lf = (LinphoneFriend *)friends->data;
			if (lf) {
				bctbx_list_t *vCard = bctbx_list_find_custom(vCards, (int (*)(const void*, const void*))find_matching_vcard, lf);
				if (!vCard) {
					ms_debug("Local friend %s isn't in the remote vCard list, delete it", linphone_friend_get_name(lf));
					temp_list = bctbx_list_append(temp_list, linphone_friend_ref(lf));
				} else {
					LinphoneCardDavResponse *response = (LinphoneCardDavResponse *)vCard->data;
					ms_debug("Local friend %s is in the remote vCard list, check eTag", linphone_friend_get_name(lf));
					if (response) {
						LinphoneVcard *lvc = linphone_friend_get_vcard(lf);
						const char *etag = linphone_vcard_get_etag(lvc);
						ms_debug("Local friend eTag is %s, remote vCard eTag is %s", etag, response->etag);
						if (lvc && etag && strcmp(etag, response->etag) == 0) {
							bctbx_list_remove(vCards, vCard);
							linphone_carddav_response_free(response);
						}
					}
				}
			}
			friends = bctbx_list_next(friends);
		}
		friends_to_remove = temp_list;
		while(friends_to_remove) {
			LinphoneFriend *lf = (LinphoneFriend *)friends_to_remove->data;
			if (lf) {
				if (cdc->contact_removed_cb) {
					ms_debug("Contact removed: %s", linphone_friend_get_name(lf));
					cdc->contact_removed_cb(cdc, lf);
				}
			}
			friends_to_remove = bctbx_list_next(friends_to_remove);
		}
		temp_list = bctbx_list_free_with_data(temp_list, (void (*)(void *))linphone_friend_unref);
		
		linphone_carddav_pull_vcards(cdc, vCards);
		bctbx_list_free_with_data(vCards, (void (*)(void *))linphone_carddav_response_free);
	}
}

static bctbx_list_t* parse_vcards_etags_from_xml_response(const char *body) {
	bctbx_list_t *result = NULL;
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
							result = bctbx_list_append(result, response);
							ms_debug("Added vCard object with eTag %s and URL %s", etag, url);
							linphone_free_xml_text_content(etag);
							linphone_free_xml_text_content(url);
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
		linphone_carddav_server_to_client_sync_done(cdc, TRUE, "Synchronization skipped because cTag already up to date");
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

static void linphone_carddav_query_free(LinphoneCardDavQuery *query) {
	if (!query) {
		return;
	}
	
	if (query->http_request_listener) {
		belle_sip_object_unref(query->http_request_listener);
		query->http_request_listener = NULL;
	}
	
	// Context will be freed later (in sync_done)
	query->context = NULL;
	
	if (query->url) {
		ms_free(query->url);
	}
	if (query->body) {
		ms_free(query->body);
	}
	
	ms_free(query);
}

static bool_t is_query_client_to_server_sync(LinphoneCardDavQuery *query) {
	if (!query) {
		ms_error("[carddav] query is NULL...");
		return FALSE;
	}
	switch(query->type) {
		case LinphoneCardDavQueryTypePropfind:
		case LinphoneCardDavQueryTypeAddressbookQuery:
		case LinphoneCardDavQueryTypeAddressbookMultiget:
			return FALSE;
		case LinphoneCardDavQueryTypePut:
		case LinphoneCardDavQueryTypeDelete:
			return TRUE;
		default:
			ms_error("[carddav] Unknown request: %i", query->type);
	}
	return FALSE;
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
					LinphoneVcard *lvc = linphone_friend_get_vcard(lf);
					if (lf && lvc) {
						if (header) {
							const char *etag = belle_sip_header_get_unparsed_value(header);
							if (!linphone_vcard_get_etag(lvc)) {
								ms_debug("eTag for newly created vCard is: %s", etag);
							} else {
								ms_debug("eTag for updated vCard is: %s", etag);
							}
							linphone_vcard_set_etag(lvc, etag);

							linphone_carddav_client_to_server_sync_done(query->context, TRUE, NULL);
							linphone_friend_unref(lf);
						} else {
							// For some reason, server didn't return the eTag of the updated/created vCard
							// We need to do a GET on the vCard to get the correct one
							bctbx_list_t *vcard = NULL;
							LinphoneCardDavResponse *response = (LinphoneCardDavResponse *)ms_new0(LinphoneCardDavResponse, 1);
							response->url = ms_strdup(linphone_vcard_get_url(lvc));
							vcard = bctbx_list_append(vcard, response);
							linphone_carddav_pull_vcards(query->context, vcard);
							bctbx_list_free_with_data(vcard, (void (*)(void *))linphone_carddav_response_free);
						}
					}
					else {
						linphone_carddav_client_to_server_sync_done(query->context, FALSE, "No LinphoneFriend found in user_data field of query");
					}
				}
				break;
			case LinphoneCardDavQueryTypeDelete:
				linphone_carddav_client_to_server_sync_done(query->context, TRUE, NULL);
				break;
			default:
				ms_error("[carddav] Unknown request: %i", query->type);
				break;
			}
		} else {
			char msg[100];
			snprintf(msg, sizeof(msg), "Unexpected HTTP response code: %i", code);
			if (is_query_client_to_server_sync(query)) {
				linphone_carddav_client_to_server_sync_done(query->context, FALSE, msg);
			} else {
				linphone_carddav_server_to_client_sync_done(query->context, FALSE, msg);
			}
		}
	} else {
		if (is_query_client_to_server_sync(query)) {
			linphone_carddav_client_to_server_sync_done(query->context, FALSE, "No response found");
		} else {
			linphone_carddav_server_to_client_sync_done(query->context, FALSE, "No response found");
		}
	}
	linphone_carddav_query_free(query);
}

static void process_io_error_from_carddav_request(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)data;
	ms_error("[carddav] I/O error during CardDAV request sending");
	if (is_query_client_to_server_sync(query)) {
		linphone_carddav_client_to_server_sync_done(query->context, FALSE, "I/O error during CardDAV request sending");
	} else {
		linphone_carddav_server_to_client_sync_done(query->context, FALSE, "I/O error during CardDAV request sending");
	}
	linphone_carddav_query_free(query);
}

static void process_auth_requested_from_carddav_request(void *data, belle_sip_auth_event_t *event) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)data;
	LinphoneCardDavContext *cdc = query->context;
	const char *realm = belle_sip_auth_event_get_realm(event);
	belle_generic_uri_t *uri = belle_generic_uri_parse(query->url);
	const char *domain = belle_generic_uri_get_host(uri);
	
	if (cdc->auth_info) {
		belle_sip_auth_event_set_username(event, cdc->auth_info->username);
		belle_sip_auth_event_set_passwd(event, cdc->auth_info->passwd);
		belle_sip_auth_event_set_ha1(event, cdc->auth_info->ha1);
	} else {
		LinphoneCore *lc = cdc->friend_list->lc;
		const bctbx_list_t *auth_infos = linphone_core_get_auth_info_list(lc);
		
		ms_debug("Looking for auth info for domain %s and realm %s", domain, realm);
		while (auth_infos) {
			LinphoneAuthInfo *auth_info = (LinphoneAuthInfo *)auth_infos->data;
			if (auth_info->domain && strcmp(domain, auth_info->domain) == 0) {
				if (!auth_info->realm || strcmp(realm, auth_info->realm) == 0) {
					belle_sip_auth_event_set_username(event, auth_info->username);
					belle_sip_auth_event_set_passwd(event, auth_info->passwd);
					belle_sip_auth_event_set_ha1(event, auth_info->ha1);
					cdc->auth_info = linphone_auth_info_clone(auth_info);
					break;
				}
			}
			auth_infos = bctbx_list_next(auth_infos);
		}
	
		if (!auth_infos) {
			ms_error("[carddav] Authentication requested during CardDAV request sending, and username/password weren't provided");
			if (is_query_client_to_server_sync(query)) {
				linphone_carddav_client_to_server_sync_done(query->context, FALSE, "Authentication requested during CardDAV request sending, and username/password weren't provided");
			} else {
				linphone_carddav_server_to_client_sync_done(query->context, FALSE, "Authentication requested during CardDAV request sending, and username/password weren't provided");
			}
			linphone_carddav_query_free(query);
		}
	}
}

static void linphone_carddav_send_query(LinphoneCardDavQuery *query) {
	belle_http_request_listener_callbacks_t cbs = { 0 };
	belle_generic_uri_t *uri = NULL;
	belle_http_request_t *req = NULL;
	belle_sip_memory_body_handler_t *bh = NULL;
	LinphoneCardDavContext *cdc = query->context;
	char* ua = NULL;

	uri = belle_generic_uri_parse(query->url);
	if (!uri) {
		if (cdc && cdc->sync_done_cb) {
			cdc->sync_done_cb(cdc, FALSE, "Could not send request, URL is invalid");
		}
		belle_sip_error("Could not send request, URL %s is invalid", query->url);
		linphone_carddav_query_free(query);
		return;
	}
	req = belle_http_request_create(query->method, uri, belle_sip_header_content_type_create("application", "xml; charset=utf-8"), NULL);
	
	if (!req) {
		if (cdc && cdc->sync_done_cb) {
			cdc->sync_done_cb(cdc, FALSE, "Could not create belle_http_request_t");
		}
		belle_sip_object_unref(uri);
		belle_sip_error("Could not create belle_http_request_t");
		linphone_carddav_query_free(query);
		return;
	}
	
	ua = ms_strdup_printf("%s/%s", linphone_core_get_user_agent(cdc->friend_list->lc), linphone_core_get_version());
	belle_sip_message_add_header((belle_sip_message_t *)req, belle_sip_header_create("User-Agent", ua));
	ms_free(ua);
	if (query->depth) {
		belle_sip_message_add_header((belle_sip_message_t *)req, belle_sip_header_create("Depth", query->depth));
	} else if (query->ifmatch) {
		belle_sip_message_add_header((belle_sip_message_t *)req, belle_sip_header_create("If-Match", query->ifmatch));
	} else if (strcmp(query->method, "PUT")) {
		belle_sip_message_add_header((belle_sip_message_t *)req, belle_sip_header_create("If-None-Match", "*"));
	}
	
	if (query->body) {
		bh = belle_sip_memory_body_handler_new_copy_from_buffer(query->body, strlen(query->body), NULL, NULL);
		belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req), bh ? BELLE_SIP_BODY_HANDLER(bh) : NULL);
	}
	
	cbs.process_response = process_response_from_carddav_request;
	cbs.process_io_error = process_io_error_from_carddav_request;
	cbs.process_auth_requested = process_auth_requested_from_carddav_request;
	query->http_request_listener = belle_http_request_listener_create_from_callbacks(&cbs, query);
	belle_http_provider_send_request(query->context->friend_list->lc->http_provider, req, query->http_request_listener);
}

static LinphoneCardDavQuery* linphone_carddav_create_put_query(LinphoneCardDavContext *cdc, LinphoneVcard *lvc) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)ms_new0(LinphoneCardDavQuery, 1);
	query->context = cdc;
	query->depth = NULL;
	query->ifmatch = linphone_vcard_get_etag(lvc);
	query->body = ms_strdup(linphone_vcard_as_vcard4_string(lvc));
	query->method = "PUT";
	query->url = ms_strdup(linphone_vcard_get_url(lvc));
	query->type = LinphoneCardDavQueryTypePut;
	return query;
}

static char* generate_url_from_server_address_and_uid(const char *server_url) {
	char *result = NULL;
	if (server_url) {
		char *uuid = ms_malloc(64);
		if (sal_generate_uuid(uuid, 64) == 0) {
			char *url = ms_malloc(300);
			snprintf(url, 300, "%s/linphone-%s.vcf", server_url, uuid);
			ms_debug("Generated url is %s", url);
			result = ms_strdup(url);
			ms_free(url);
		}
		ms_free(uuid);
	}
	return result;
}

void linphone_carddav_put_vcard(LinphoneCardDavContext *cdc, LinphoneFriend *lf) {
	LinphoneVcard *lvc = linphone_friend_get_vcard(lf);
	if (lvc) {
		LinphoneCardDavQuery *query = NULL;
		if (!linphone_vcard_get_uid(lvc)) {
			linphone_vcard_generate_unique_id(lvc);
		}
		
		if (!linphone_vcard_get_url(lvc)) {
			char *url = generate_url_from_server_address_and_uid(cdc->friend_list->uri);
			if (url) {
				linphone_vcard_set_url(lvc, url);
				ms_free(url);
			} else {
				const char *msg = "vCard doesn't have an URL, and friendlist doesn't have a CardDAV server set either, can't push it";
				ms_warning("%s", msg);
				if (cdc && cdc->sync_done_cb) {
					cdc->sync_done_cb(cdc, FALSE, msg);
				}
				return;
			}
		}
		
		query = linphone_carddav_create_put_query(cdc, lvc);
		query->user_data = linphone_friend_ref(lf);
		linphone_carddav_send_query(query);
	} else {
		const char *msg = NULL;
		if (!lvc) {
			msg = "LinphoneVcard is NULL";
		} else {
			msg = "Unknown error";
		}
		
		if (msg) {
			ms_error("[carddav] %s", msg);
		}
		
		if (cdc && cdc->sync_done_cb) {
			cdc->sync_done_cb(cdc, FALSE, msg);
		}
	}
}

static LinphoneCardDavQuery* linphone_carddav_create_delete_query(LinphoneCardDavContext *cdc, LinphoneVcard *lvc) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)ms_new0(LinphoneCardDavQuery, 1);
	query->context = cdc;
	query->depth = NULL;
	query->ifmatch = linphone_vcard_get_etag(lvc);
	query->body = NULL;
	query->method = "DELETE";
	query->url = ms_strdup(linphone_vcard_get_url(lvc));
	query->type = LinphoneCardDavQueryTypeDelete;
	return query;
}

void linphone_carddav_delete_vcard(LinphoneCardDavContext *cdc, LinphoneFriend *lf) {
	LinphoneVcard *lvc = linphone_friend_get_vcard(lf);
	if (lvc && linphone_vcard_get_uid(lvc) && linphone_vcard_get_etag(lvc)) {
		LinphoneCardDavQuery *query = NULL;
		
		if (!linphone_vcard_get_url(lvc)) {
			char *url = generate_url_from_server_address_and_uid(cdc->friend_list->uri);
			if (url) {
				linphone_vcard_set_url(lvc, url);
				ms_free(url);
			} else {
				const char *msg = "vCard doesn't have an URL, and friendlist doesn't have a CardDAV server set either, can't delete it";
				ms_warning("%s", msg);
				if (cdc && cdc->sync_done_cb) {
					cdc->sync_done_cb(cdc, FALSE, msg);
				}
				return;
			}
		}
		
		query = linphone_carddav_create_delete_query(cdc, lvc);
		linphone_carddav_send_query(query);
	} else {
		const char *msg = NULL;
		if (!lvc) {
			msg = "LinphoneVcard is NULL";
		} else if (!linphone_vcard_get_uid(lvc)) {
			msg = "LinphoneVcard doesn't have an UID";
		} else if (!linphone_vcard_get_etag(lvc)) {
			msg = "LinphoneVcard doesn't have an eTag";
		}
		
		if (msg) {
			ms_error("[carddav] %s", msg);
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
	query->body = ms_strdup("<d:propfind xmlns:d=\"DAV:\" xmlns:cs=\"http://calendarserver.org/ns/\"><d:prop><cs:getctag /></d:prop></d:propfind>");
	query->method = "PROPFIND";
	query->url = ms_strdup(cdc->friend_list->uri);
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
	query->body = ms_strdup("<card:addressbook-query xmlns:d=\"DAV:\" xmlns:card=\"urn:ietf:params:xml:ns:carddav\"><d:prop><d:getetag /></d:prop><card:filter></card:filter></card:addressbook-query>");
	query->method = "REPORT";
	query->url = ms_strdup(cdc->friend_list->uri);
	query->type = LinphoneCardDavQueryTypeAddressbookQuery;
	return query;
}

void linphone_carddav_fetch_vcards(LinphoneCardDavContext *cdc) {
	LinphoneCardDavQuery *query = linphone_carddav_create_addressbook_query(cdc);
	linphone_carddav_send_query(query);
}

static LinphoneCardDavQuery* linphone_carddav_create_addressbook_multiget_query(LinphoneCardDavContext *cdc, bctbx_list_t *vcards) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)ms_new0(LinphoneCardDavQuery, 1);
	char *body = (char *)ms_malloc((bctbx_list_size(vcards) + 1) * 300 * sizeof(char));
	bctbx_list_t *iterator = vcards;
	
	query->context = cdc;
	query->depth = "1";
	query->ifmatch = NULL;
	query->method = "REPORT";
	query->url = ms_strdup(cdc->friend_list->uri);
	query->type = LinphoneCardDavQueryTypeAddressbookMultiget;

	sprintf(body, "%s", "<card:addressbook-multiget xmlns:d=\"DAV:\" xmlns:card=\"urn:ietf:params:xml:ns:carddav\"><d:prop><d:getetag /><card:address-data content-type='text/vcard' version='4.0'/></d:prop>");
	while (iterator) {
		LinphoneCardDavResponse *response = (LinphoneCardDavResponse *)iterator->data;
		if (response) {
			char temp_body[300];
			snprintf(temp_body, sizeof(temp_body), "<d:href>%s</d:href>", response->url);
			strcat(body, temp_body);
			iterator = bctbx_list_next(iterator);
		}
	}
	strcat(body, "</card:addressbook-multiget>");
	query->body = ms_strdup(body);
	ms_free(body);
	
	return query;
}

void linphone_carddav_pull_vcards(LinphoneCardDavContext *cdc, bctbx_list_t *vcards_to_pull) {
	LinphoneCardDavQuery *query = linphone_carddav_create_addressbook_multiget_query(cdc, vcards_to_pull);
	linphone_carddav_send_query(query);
}
