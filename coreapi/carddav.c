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
		linphone_core_set_carddav_current_ctag(cdc->lc, cdc->ctag);
	}
	
	if (cdc->sync_done_cb) {
		cdc->sync_done_cb(cdc, success, msg);
	}
}

static void linphone_carddav_vcards_pulled(LinphoneCardDavContext *cdc, MSList *vCards) {
	if (vCards != NULL && ms_list_size(vCards) > 0) {
		//TODO: find out which one is new and which one is an update and call the according callback
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
			xmlXPathObjectPtr etags = linphone_get_xml_xpath_object_for_node_list(xml_ctx, "/d:multistatus/d:response/d:propstat/d:prop/d:getetag");
			xmlXPathObjectPtr hrefs = linphone_get_xml_xpath_object_for_node_list(xml_ctx, "/d:multistatus/d:response/d:href");
			xmlXPathObjectPtr vcards = linphone_get_xml_xpath_object_for_node_list(xml_ctx, "/d:multistatus/d:response/d:propstat/d:prop/card:address-data");
			if (etags != NULL && hrefs != NULL && vcards != NULL) {
				if (etags->nodesetval != NULL && hrefs->nodesetval != NULL && vcards->nodesetval != NULL) {
					xmlNodeSetPtr etags_nodes = etags->nodesetval;
					xmlNodeSetPtr hrefs_nodes = hrefs->nodesetval;
					xmlNodeSetPtr vcards_nodes = vcards->nodesetval;
					if (etags_nodes->nodeNr >= 1 && hrefs_nodes->nodeNr == etags_nodes->nodeNr && vcards_nodes->nodeNr == etags_nodes->nodeNr) {
						int i;
						for (i = 0; i < vcards_nodes->nodeNr; i++) {
							xmlNodePtr etag_node = etags_nodes->nodeTab[i];
							xmlNodePtr href_node = hrefs_nodes->nodeTab[i];
							xmlNodePtr vcard_node = vcards_nodes->nodeTab[i];
							if (vcard_node->children != NULL) {
								char *etag = (char *)xmlNodeListGetString(xml_ctx->doc, etag_node->children, 1);
								char *url = (char *)xmlNodeListGetString(xml_ctx->doc, href_node->children, 1);
								char *vcard = (char *)xmlNodeListGetString(xml_ctx->doc, vcard_node->children, 1);
								LinphoneCardDavResponse *response = ms_new0(LinphoneCardDavResponse, 1);
								response->etag = ms_strdup(etag);
								response->url = ms_strdup(url);
								response->vcard = ms_strdup(vcard);
								result = ms_list_append(result, response);
								ms_debug("Added vCard object with eTag %s, URL %s and vCard %s", etag, url, vcard);
							}
						}
					}
				}
				xmlXPathFreeObject(vcards);
				xmlXPathFreeObject(etags);
				xmlXPathFreeObject(hrefs);
			}
		}
	}
end:
	linphone_xmlparsing_context_destroy(xml_ctx);
	return result;
}

static void linphone_carddav_vcards_fetched(LinphoneCardDavContext *cdc, MSList *vCards) {
	if (vCards != NULL && ms_list_size(vCards) > 0) {
		//MSList *localFriends = linphone_core_fetch_friends_from_db(cdc->lc);
		//TODO: call onDelete from the ones that are in localFriends but not in vCards
		//TODO: remove from vCards the one that are in localFriends and for which the eTag hasn't changed
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
			xmlXPathObjectPtr etags = linphone_get_xml_xpath_object_for_node_list(xml_ctx, "/d:multistatus/d:response/d:propstat/d:prop/d:getetag");
			xmlXPathObjectPtr hrefs = linphone_get_xml_xpath_object_for_node_list(xml_ctx, "/d:multistatus/d:response/d:href");
			if (etags != NULL && hrefs != NULL) {
				if (etags->nodesetval != NULL && hrefs->nodesetval != NULL) {
					xmlNodeSetPtr etags_nodes = etags->nodesetval;
					xmlNodeSetPtr hrefs_nodes = hrefs->nodesetval;
					if (etags_nodes->nodeNr >= 1 && hrefs_nodes->nodeNr == etags_nodes->nodeNr) {
						int i;
						for (i = 0; i < etags_nodes->nodeNr; i++) {
							xmlNodePtr etag_node = etags_nodes->nodeTab[i];
							xmlNodePtr href_node = hrefs_nodes->nodeTab[i];
							if (etag_node->children != NULL && href_node->children != NULL) {
								char *etag = (char *)xmlNodeListGetString(xml_ctx->doc, etag_node->children, 1);
								char *url = (char *)xmlNodeListGetString(xml_ctx->doc, href_node->children, 1);
								LinphoneCardDavResponse *response = ms_new0(LinphoneCardDavResponse, 1);
								response->etag = ms_strdup(etag);
								response->url = ms_strdup(url);
								result = ms_list_append(result, response);
								ms_debug("Added vCard object with eTag %s and URL %s", etag, url);
							}
						}
					}
				}
				xmlXPathFreeObject(etags);
				xmlXPathFreeObject(hrefs);
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
		if (code == 207 || code == 200) {
			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			query->status = LinphoneCardDavQueryStatusOk;
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
			}
		} else {
			query->status = LinphoneCardDavQueryStatusFailed;
		}
	}
	ms_free(query);
}

static void process_io_error_from_carddav_request(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)data;
	ms_error("I/O error during CardDAV request sending");
	query->status = LinphoneCardDavQueryStatusFailed;
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
		query->status = LinphoneCardDavQueryStatusFailed;
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
		belle_sip_error("Could not send request, URL %s is invalid", query->url);
		query->status = LinphoneCardDavQueryStatusFailed;
		return;
	}
	req = belle_http_request_create(query->method, uri, belle_sip_header_content_type_create("application", "xml; charset=utf-8"), belle_sip_header_create("Depth", query->depth), NULL);
	if (!req) {
		belle_sip_object_unref(uri);
		belle_sip_error("Could not create belle_http_request_t");
		query->status = LinphoneCardDavQueryStatusFailed;
		return;
	}
	query->status = LinphoneCardDavQueryStatusPending;
	
	bh = belle_sip_memory_body_handler_new_copy_from_buffer(query->body, strlen(query->body), NULL, NULL);
	belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req), BELLE_SIP_BODY_HANDLER(bh));
	
	cbs.process_response = process_response_from_carddav_request;
	cbs.process_io_error = process_io_error_from_carddav_request;
	cbs.process_auth_requested = process_auth_requested_from_carddav_request;
	l = belle_http_request_listener_create_from_callbacks(&cbs, query);
	belle_http_provider_send_request(query->context->lc->http_provider, req, l);
}

void linphone_carddav_put_vcard(LinphoneCardDavContext *cdc, LinphoneFriend *lf) {
	//TODO
}

void linphone_carddav_delete_vcard(LinphoneCardDavContext *cdc, LinphoneFriend *lf) {
	//TODO
}

void linphone_carddav_set_synchronization_done_callback(LinphoneCardDavContext *cdc, LinphoneCardDavSynchronizationDoneCb cb) {
	cdc->sync_done_cb = cb;
}

static LinphoneCardDavQuery* linphone_carddav_create_propfind_query(LinphoneCardDavContext *cdc) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)ms_new0(LinphoneCardDavQuery, 1);
	query->context = cdc;
	query->depth = "0";
	query->body = "<d:propfind xmlns:d=\"DAV:\" xmlns:cs=\"http://calendarserver.org/ns/\"><d:prop><cs:getctag /></d:prop></d:propfind>";
	query->method = "PROPFIND";
	query->url = cdc->server_url;
	query->type = LinphoneCardDavQueryTypePropfind;
	query->status = LinphoneCardDavQueryStatusIdle;
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
	query->body = "<card:addressbook-query xmlns:d=\"DAV:\" xmlns:card=\"urn:ietf:params:xml:ns:carddav\"><d:prop><d:getetag /></d:prop></card:addressbook-query>";
	query->method = "REPORT";
	query->url = cdc->server_url;
	query->type = LinphoneCardDavQueryTypeAddressbookQuery;
	query->status = LinphoneCardDavQueryStatusIdle;
	return query;
}

void linphone_carddav_fetch_vcards(LinphoneCardDavContext *cdc) {
	LinphoneCardDavQuery *query = linphone_carddav_create_addressbook_query(cdc);
	linphone_carddav_send_query(query);
}

static LinphoneCardDavQuery* linphone_carddav_create_addressbook_multiget_query(LinphoneCardDavContext *cdc, MSList *vcards) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)ms_new0(LinphoneCardDavQuery, 1);
	char body[(ms_list_size(vcards)+1)*100];
	MSList *iterator = vcards;
	
	query->context = cdc;
	query->depth = "1";
	query->method = "REPORT";
	query->url = cdc->server_url;
	query->type = LinphoneCardDavQueryTypeAddressbookMultiget;
	query->status = LinphoneCardDavQueryStatusIdle;

	sprintf(body, "%s", "<card:addressbook-multiget xmlns:d=\"DAV:\" xmlns:card=\"urn:ietf:params:xml:ns:carddav\"><d:prop><d:getetag /><card:address-data content-type='text-vcard' version='4.0'/></d:prop>");
	while (iterator) {
		LinphoneCardDavResponse *response = (LinphoneCardDavResponse *)iterator->data;
		char temp_body[100];
		sprintf(temp_body, "<d:href>%s</d:href>", response->url);
		sprintf(body, "%s%s", body, temp_body);
		iterator = ms_list_next(iterator);
	}
	sprintf(body, "%s%s", body, "</card:addressbook-multiget>");
	query->body = ms_strdup(body);
	
	return query;
}

void linphone_carddav_pull_vcards(LinphoneCardDavContext *cdc, MSList *vcards_to_pull) {
	LinphoneCardDavQuery *query = linphone_carddav_create_addressbook_multiget_query(cdc, vcards_to_pull);
	linphone_carddav_send_query(query);
}