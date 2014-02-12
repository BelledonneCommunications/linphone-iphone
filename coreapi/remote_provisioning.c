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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "private.h"
#include "xml2lpc.h"

#define XML2LPC_CALLBACK_BUFFER_SIZE  1024

static ConfiguringCallback linphone_callback = NULL;
static bool_t waiting_response = FALSE;

static void xml2lpc_callback(void *ctx, xml2lpc_log_level level, const char *fmt, va_list list) {
	char buffer[XML2LPC_CALLBACK_BUFFER_SIZE];
	vsnprintf(buffer, XML2LPC_CALLBACK_BUFFER_SIZE, fmt, list);

	if (level == XML2LPC_ERROR)
		ms_error("%s", buffer);
	else if (level == XML2LPC_WARNING)
		ms_warning("%s", buffer);
	/*else
		ms_message("%s", buffer); // Don't log debug messages */
}

static void linphone_remote_provisioning_apply(LinphoneCore *lc, const char *xml) {
	xml2lpc_context *context = xml2lpc_context_new(xml2lpc_callback, lc);
	int result = xml2lpc_set_xml_string(context, xml);
	if (result == 0) {
		result = xml2lpc_convert(context, linphone_core_get_config(lc));
		if (result == 0) {
			lp_config_sync(linphone_core_get_config(lc));
			xml2lpc_context_destroy(context);
			if (linphone_callback)
				linphone_callback(lc, LinphoneConfiguringSuccessful, NULL);
		} else {
			xml2lpc_context_destroy(context);
			if (linphone_callback)
				linphone_callback(lc, LinphoneConfiguringFailed, "convert failed");
		}
	} else {
		xml2lpc_context_destroy(context);
		if (linphone_callback)
			linphone_callback(lc, LinphoneConfiguringFailed, "set xml string failed");
	}
}

static void belle_request_process_response_event(void *ctx, const belle_http_response_event_t *event) {
	waiting_response = FALSE;
	LinphoneCore *lc = (LinphoneCore *)ctx;
	belle_sip_message_t *body = BELLE_SIP_MESSAGE(event->response);
	const char *message = belle_sip_message_get_body(body);
	
	if (belle_http_response_get_status_code(event->response) == 200) {
		linphone_remote_provisioning_apply(lc, message);
	} else {
		if (linphone_callback)
			linphone_callback(lc, LinphoneConfiguringFailed, message);
	}
}

static void belle_request_process_io_error(void *ctx, const belle_sip_io_error_event_t *event) {
	waiting_response = FALSE;
	LinphoneCore *lc = (LinphoneCore *)ctx;
	
	if (linphone_callback)
		linphone_callback(lc, LinphoneConfiguringFailed, "io error");
}

static void belle_request_process_timeout(void *ctx, const belle_sip_timeout_event_t *event) {
	waiting_response = FALSE;
	LinphoneCore *lc = (LinphoneCore *)ctx;
	
	if (linphone_callback)
		linphone_callback(lc, LinphoneConfiguringFailed, "timeout");
}

static void belle_request_process_auth_requested(void *ctx, belle_sip_auth_event_t *event) {
	waiting_response = FALSE;
	LinphoneCore *lc = (LinphoneCore *)ctx;
	
	if (linphone_callback)
		linphone_callback(lc, LinphoneConfiguringFailed, "auth requested");
}

static belle_http_request_listener_callbacks_t belle_request_listener = {
	belle_request_process_response_event,
	belle_request_process_io_error,
	belle_request_process_timeout,
	belle_request_process_auth_requested
};

static void linphone_remote_provisioning_download(LinphoneCore *lc, const char *remote_provisioning_uri) {
	belle_sip_object_pool_t *pool = belle_sip_object_pool_push();
	belle_sip_stack_t *stack = belle_sip_stack_new(NULL);
	
	belle_http_request_listener_t *listener = belle_http_request_listener_create_from_callbacks(&belle_request_listener, lc);
	belle_http_provider_t *provider = belle_sip_stack_create_http_provider(stack, "0.0.0.0");
	
	belle_http_request_t *request = belle_http_request_create(
		"GET",
		belle_generic_uri_parse(remote_provisioning_uri),  
		NULL
	);
	
	belle_http_provider_send_request(provider, request, listener);
	
	waiting_response = TRUE;
	while (waiting_response) {
		belle_sip_stack_sleep(stack, 10);
	}
	
	belle_sip_object_unref(pool);
	belle_sip_object_unref(provider);
	belle_sip_object_unref(stack);
}

/**
 * Fetches the remote provisioning from the given URI and tries to apply it to the current LpConfig
 * @param lc the LinphoneCore
 * @param remote_provisioning_uri the URI at which the provisioning is available
 */
void linphone_remote_provisioning_download_and_apply(LinphoneCore *lc, const char *remote_provisioning_uri, ConfiguringCallback cb) {
	linphone_callback = cb;
	linphone_remote_provisioning_download(lc, remote_provisioning_uri);
}

void linphone_core_set_provisioning_uri(LinphoneCore *lc, const char*uri){
	if (linphone_core_ready(lc)){
		lp_config_set_string(lc->config,"misc","config-uri",uri);
	}
}

const char*linphone_core_get_provisioning_uri(const LinphoneCore *lc){
	return lp_config_get_string(lc->config,"misc","config-uri",NULL);
}
