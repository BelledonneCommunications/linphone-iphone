/*
remote_provisioning.c
Copyright (C) 2010  Belledonne Communications SARL

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
#include "private.h"
#include "xml2lpc.h"
#include "linphone/lpconfig.h"

#define XML2LPC_CALLBACK_BUFFER_SIZE  1024


static void linphone_remote_provisioning_apply(LinphoneCore *lc, const char *xml) {
	const char* error_msg = _linphone_config_load_from_xml_string(linphone_core_get_config(lc), xml);

	linphone_configuring_terminated(lc
									,error_msg ? LinphoneConfiguringFailed : LinphoneConfiguringSuccessful
									, error_msg);
}

int linphone_remote_provisioning_load_file( LinphoneCore* lc, const char* file_path){
	int status = -1;
	char* provisioning=ms_load_path_content(file_path, NULL);

	if (provisioning){
		linphone_remote_provisioning_apply(lc, provisioning);
		status = 0;
		ms_free(provisioning);
	}
	return status;
}

static void belle_request_process_response_event(void *ctx, const belle_http_response_event_t *event) {
	LinphoneCore *lc = (LinphoneCore *)ctx;
	belle_sip_message_t *message = BELLE_SIP_MESSAGE(event->response);
	const char *body = belle_sip_message_get_body(message);

	if (belle_http_response_get_status_code(event->response) == 200) {
		linphone_remote_provisioning_apply(lc, body);
	} else {
		linphone_configuring_terminated(lc, LinphoneConfiguringFailed, "http error");
	}
}

static void belle_request_process_io_error(void *ctx, const belle_sip_io_error_event_t *event) {
	LinphoneCore *lc = (LinphoneCore *)ctx;
	linphone_configuring_terminated(lc, LinphoneConfiguringFailed, "http io error");
}

static void belle_request_process_timeout(void *ctx, const belle_sip_timeout_event_t *event) {
	LinphoneCore *lc = (LinphoneCore *)ctx;
	linphone_configuring_terminated(lc, LinphoneConfiguringFailed, "http timeout");
}

static void belle_request_process_auth_requested(void *ctx, belle_sip_auth_event_t *event) {
	LinphoneCore *lc = (LinphoneCore *)ctx;
	linphone_configuring_terminated(lc, LinphoneConfiguringFailed, "http auth requested");
}

int linphone_remote_provisioning_download_and_apply(LinphoneCore *lc, const char *remote_provisioning_uri) {

	belle_generic_uri_t *uri=belle_generic_uri_parse(remote_provisioning_uri);
	const char* scheme = uri ? belle_generic_uri_get_scheme(uri) : NULL;
	const char *host = uri ? belle_generic_uri_get_host(uri) : NULL;

	if( scheme && (strcmp(scheme,"file") == 0) ){
		// We allow for 'local remote-provisioning' in case the file is to be opened from the hard drive.
		const char* file_path = remote_provisioning_uri + strlen("file://"); // skip scheme
		if (uri) {
			belle_sip_object_unref(uri);
		}
		return linphone_remote_provisioning_load_file(lc, file_path);
	} else if( scheme && strncmp(scheme, "http", 4) == 0 && host && strlen(host) > 0) {
		belle_http_request_listener_callbacks_t belle_request_listener={0};
		belle_http_request_t *request;

		belle_request_listener.process_response=belle_request_process_response_event;
		belle_request_listener.process_auth_requested=belle_request_process_auth_requested;
		belle_request_listener.process_io_error=belle_request_process_io_error;
		belle_request_listener.process_timeout=belle_request_process_timeout;

		lc->provisioning_http_listener = belle_http_request_listener_create_from_callbacks(&belle_request_listener, lc);

		request=belle_http_request_create("GET"
											, uri
											, belle_sip_header_create("User-Agent",linphone_core_get_user_agent(lc))
											, NULL);

		return belle_http_provider_send_request(lc->http_provider, request, lc->provisioning_http_listener);
	} else {
		ms_error("Invalid provisioning URI [%s] (missing scheme or host ?)",remote_provisioning_uri);
		if (uri) {
			belle_sip_object_unref(uri);
		}
		return -1;
	}
}

LinphoneStatus linphone_core_set_provisioning_uri(LinphoneCore *lc, const char *remote_provisioning_uri) {
	belle_generic_uri_t *uri=remote_provisioning_uri?belle_generic_uri_parse(remote_provisioning_uri):NULL;
	if (!remote_provisioning_uri||uri) {
		lp_config_set_string(lc->config,"misc","config-uri",remote_provisioning_uri);
		if (uri) {
			belle_sip_object_unref(uri);
		}
		return 0;
	}
	ms_error("Invalid provisioning URI [%s] (could not be parsed)",remote_provisioning_uri);
	return -1;

}

const char*linphone_core_get_provisioning_uri(const LinphoneCore *lc){
	return lp_config_get_string(lc->config,"misc","config-uri",NULL);
}

bool_t linphone_core_is_provisioning_transient(LinphoneCore *lc) {
	return lp_config_get_int(lc->config, "misc", "transient_provisioning", 0) == 1;
}
