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

static void linphone_carddav_vcards_fetched(LinphoneCardDavContext *cdc, MSList *vCards) {
	if (vCards != NULL && ms_list_size(vCards) > 0) {
		//MSList *localFriends = linphone_core_fetch_friends_from_db(cdc->lc);
		//TODO: find out if there are new/delete URLs and compare eTags with the current vCards
	}
	if (cdc->sync_done_cb) {
		cdc->sync_done_cb(cdc, TRUE, "TODO: remove lated");
	}
}

static void linphone_carddav_ctag_fetched(LinphoneCardDavContext *cdc, int ctag) {
	if (ctag == -1 || ctag > cdc->ctag) {
		cdc->ctag = ctag;
		linphone_carddav_fetch_vcards(cdc);
	} else {
		ms_message("No changes found on server, skipping sync");
		if (cdc->sync_done_cb) {
			cdc->sync_done_cb(cdc, TRUE, "Synchronization skipped because cTag already up to date");
		}
	}
}

void linphone_carddav_synchronize(LinphoneCardDavContext *cdc) {
	linphone_carddav_get_current_ctag(cdc);
}

static void process_response_from_carddav_request(void *data, const belle_http_response_event_t *event) {
	LinphoneCardDavQuery *query = (LinphoneCardDavQuery *)data;
	
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code == 207 || code == 200) {
			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			ms_message("CardDAV query response body: %s", body);
			query->status = LinphoneCardDavQueryStatusOk;
			switch(query->type) {
			case LinphoneCardDavQueryTypePropfind:
				linphone_carddav_ctag_fetched(query->context, 0);//TODO parse value from body
				break;
			case LinphoneCardDavQueryTypeAddressbookQuery:
				linphone_carddav_vcards_fetched(query->context, NULL);//TODO parse value from body
				break;
			case LinphoneCardDavQueryTypeAddressbookMultiget:
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
	ms_error("I/O Error during CardDAV request sending");
	query->status = LinphoneCardDavQueryStatusFailed;
	ms_free(query);
	if (query->context->sync_done_cb) {
		query->context->sync_done_cb(query->context, FALSE, "I/O Error during CardDAV request sending");
	}
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
		ms_error("Authentication error during CardDAV request sending");
		if (query->context->sync_done_cb) {
			query->context->sync_done_cb(query->context, FALSE, "Authentication error during CardDAV request sending");
		}
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
	query->body = "<card:addressbook-query xmlns:d=\"DAV:\" xmlns:card=\"urn:ietf:params:xml:ns:carddav\"><d:prop><d:getetag /><card:address-data content-type='text-vcard' version='4.0'/></d:prop></card:addressbook-query>";
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
	query->context = cdc;
	query->depth = "1";
	query->method = "REPORT";
	query->url = cdc->server_url;
	query->type = LinphoneCardDavQueryTypeAddressbookMultiget;
	query->status = LinphoneCardDavQueryStatusIdle;
	//TODO: body
	return query;
}

void linphone_carddav_pull_vcards(LinphoneCardDavContext *cdc, MSList *vcards_to_pull) {
	LinphoneCardDavQuery *query = linphone_carddav_create_addressbook_multiget_query(cdc, vcards_to_pull);
	linphone_carddav_send_query(query);
}