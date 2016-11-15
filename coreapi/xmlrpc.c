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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "linphone/core.h"
#include "private.h"

#include <string.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>



BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneXmlRpcRequestCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneXmlRpcRequestCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);

static LinphoneXmlRpcRequestCbs * linphone_xml_rpc_request_cbs_new(void) {
	return belle_sip_object_new(LinphoneXmlRpcRequestCbs);
}

LinphoneXmlRpcRequestCbs * linphone_xml_rpc_request_cbs_ref(LinphoneXmlRpcRequestCbs *cbs) {
	belle_sip_object_ref(cbs);
	return cbs;
}

void linphone_xml_rpc_request_cbs_unref(LinphoneXmlRpcRequestCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void *linphone_xml_rpc_request_cbs_get_user_data(const LinphoneXmlRpcRequestCbs *cbs) {
	return cbs->user_data;
}

void linphone_xml_rpc_request_cbs_set_user_data(LinphoneXmlRpcRequestCbs *cbs, void *ud) {
	cbs->user_data = ud;
}

LinphoneXmlRpcRequestCbsResponseCb linphone_xml_rpc_request_cbs_get_response(const LinphoneXmlRpcRequestCbs *cbs) {
	return cbs->response;
}

void linphone_xml_rpc_request_cbs_set_response(LinphoneXmlRpcRequestCbs *cbs, LinphoneXmlRpcRequestCbsResponseCb cb) {
	cbs->response = cb;
}


static void format_request(LinphoneXmlRpcRequest *request) {
	char si[64];
	belle_sip_list_t *arg_ptr = request->arg_list;
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;

	if (request->content != NULL) {
		belle_sip_free(request->content);
		request->content = NULL;
	}

	buf = xmlBufferCreate();
	if (buf == NULL) {
		ms_error("Error creating the XML buffer");
		return;
	}
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == NULL) {
		ms_error("Error creating the XML writer");
		return;
	}

	/* autoindent so that logs are human-readable, as SIP sip on-purpose */
	xmlTextWriterSetIndent(writer, 1);

	err = xmlTextWriterStartDocument(writer, "1.0", "UTF-8", NULL);
	if (err >= 0) {
		err = xmlTextWriterStartElement(writer, (const xmlChar *)"methodCall");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"methodName", (const xmlChar *)request->method);
	}
	if (err >= 0) {
		err = xmlTextWriterStartElement(writer, (const xmlChar *)"params");
	}
	while (arg_ptr != NULL) {
		LinphoneXmlRpcArg *arg = (LinphoneXmlRpcArg *)arg_ptr->data;
		if (err >= 0) {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"param");
		}
		if (err >= 0) {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"value");
		}
		switch (arg->type) {
			case LinphoneXmlRpcArgNone:
				break;
			case LinphoneXmlRpcArgInt:
				memset(si, 0, sizeof(si));
				snprintf(si, sizeof(si), "%i", arg->data.i);
				err = xmlTextWriterWriteElement(writer, (const xmlChar *)"int", (const xmlChar *)si);
				break;
			case LinphoneXmlRpcArgString:
				err = xmlTextWriterWriteElement(writer, (const xmlChar *)"string", arg->data.s ? (const xmlChar *)arg->data.s : (const xmlChar *)"");
				break;
		}
		if (err >= 0) {
			/* Close the "value" element. */
			err = xmlTextWriterEndElement(writer);
		}
		if (err >= 0) {
			/* Close the "param" element. */
			err = xmlTextWriterEndElement(writer);
		}
		arg_ptr = arg_ptr->next;
	}
	if (err >= 0) {
		/* Close the "params" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		/* Close the "methodCall" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		err = xmlTextWriterEndDocument(writer);
	}
	if (err > 0) {
		/* xmlTextWriterEndDocument returns the size of the content. */
		request->content = belle_sip_strdup((const char *)buf->content);
	}
	xmlFreeTextWriter(writer);
	xmlBufferFree(buf);
}

static void free_arg(LinphoneXmlRpcArg *arg) {
	if ((arg->type == LinphoneXmlRpcArgString) && (arg->data.s != NULL)) {
		belle_sip_free(arg->data.s);
	}
	belle_sip_free(arg);
}

static bool_t linphone_xml_rpc_request_aborted(LinphoneXmlRpcRequest *req){
	LinphoneXmlRpcSession *session = (LinphoneXmlRpcSession*) belle_sip_object_data_get(BELLE_SIP_OBJECT(req), "session");
	if (!session){
		ms_error("linphone_xml_rpc_request_aborted(): no session, this should not happen.");
		return FALSE;
	}
	return session->released;
}

static void process_io_error_from_post_xml_rpc_request(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)data;
	ms_error("I/O Error during XML-RPC request sending");
	if (!linphone_xml_rpc_request_aborted(request)){
		request->status = LinphoneXmlRpcStatusFailed;
			if (request->callbacks->response != NULL) {
			request->callbacks->response(request);
		}
	}
	linphone_xml_rpc_request_unref(request);
}

static void process_auth_requested_from_post_xml_rpc_request(void *data, belle_sip_auth_event_t *event) {
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)data;
	ms_error("Authentication error during XML-RPC request sending");
	if (!linphone_xml_rpc_request_aborted(request)){
		request->status = LinphoneXmlRpcStatusFailed;
		if (request->callbacks->response != NULL) {
			request->callbacks->response(request);
		}
	}
	linphone_xml_rpc_request_unref(request);
}

static void parse_valid_xml_rpc_response(LinphoneXmlRpcRequest *request, const char *response_body) {
	xmlparsing_context_t *xml_ctx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
	request->status = LinphoneXmlRpcStatusFailed;
	xml_ctx->doc = xmlReadDoc((const unsigned char*)response_body, 0, NULL, 0);
	if (xml_ctx->doc != NULL) {
		const char *response_str = NULL;
		if (linphone_create_xml_xpath_context(xml_ctx) < 0) goto end;
		switch (request->response.type) {
			case LinphoneXmlRpcArgInt:
				response_str = linphone_get_xml_text_content(xml_ctx, "/methodResponse/params/param/value/int");
				if (response_str != NULL) {
					request->response.data.i = atoi(response_str);
					request->status = LinphoneXmlRpcStatusOk;
				}
				break;
			case LinphoneXmlRpcArgString:
				response_str = linphone_get_xml_text_content(xml_ctx, "/methodResponse/params/param/value/string");
				if (response_str != NULL) {
					request->response.data.s = belle_sip_strdup(response_str);
					request->status = LinphoneXmlRpcStatusOk;
				}
				break;
			default:
				break;
		}
		if (response_str) linphone_free_xml_text_content(response_str);
	} else {
		ms_warning("Wrongly formatted XML-RPC response: %s", xml_ctx->errorBuffer);
	}
end:
	linphone_xmlparsing_context_destroy(xml_ctx);
	if (request->callbacks->response != NULL) {
		request->callbacks->response(request);
	}
}

static void notify_xml_rpc_error(LinphoneXmlRpcRequest *request) {
	request->status = LinphoneXmlRpcStatusFailed;
	if (request->callbacks->response != NULL) {
		request->callbacks->response(request);
	}
}

static void process_response_from_post_xml_rpc_request(void *data, const belle_http_response_event_t *event) {
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)data;

	/* Check the answer code */
	if (!linphone_xml_rpc_request_aborted(request) && event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code == 200) { /* Valid response from the server. */
			parse_valid_xml_rpc_response(request, belle_sip_message_get_body((belle_sip_message_t *)event->response));
		} else {
			ms_error("process_response_from_post_xml_rpc_request(): error code = %i", code);
			notify_xml_rpc_error(request);
		}
	}
	linphone_xml_rpc_request_unref(request);
}


static LinphoneXmlRpcRequest * _linphone_xml_rpc_request_new(const char *method, LinphoneXmlRpcArgType return_type) {
	LinphoneXmlRpcRequest *request = belle_sip_object_new(LinphoneXmlRpcRequest);
	request->callbacks = linphone_xml_rpc_request_cbs_new();
	request->status = LinphoneXmlRpcStatusPending;
	request->response.type = return_type;
	request->method = belle_sip_strdup(method);
	return request;
}

static void _linphone_xml_rpc_request_add_int_arg(LinphoneXmlRpcRequest *request, int value) {
	LinphoneXmlRpcArg *arg = belle_sip_malloc0(sizeof(LinphoneXmlRpcArg));
	arg->type = LinphoneXmlRpcArgInt;
	arg->data.i = value;
	request->arg_list = belle_sip_list_append(request->arg_list, arg);
}

static void _linphone_xml_rpc_request_add_string_arg(LinphoneXmlRpcRequest *request, const char *value) {
	LinphoneXmlRpcArg *arg = belle_sip_malloc0(sizeof(LinphoneXmlRpcArg));
	arg->type = LinphoneXmlRpcArgString;
	arg->data.s = belle_sip_strdup(value);
	request->arg_list = belle_sip_list_append(request->arg_list, arg);
}

static void _linphone_xml_rpc_request_destroy(LinphoneXmlRpcRequest *request) {
	belle_sip_list_free_with_data(request->arg_list, (void (*)(void*))free_arg);
	if ((request->response.type == LinphoneXmlRpcArgString) && (request->response.data.s != NULL)) {
		belle_sip_free(request->response.data.s);
	}
	if (request->content) belle_sip_free(request->content);
	belle_sip_free(request->method);
	linphone_xml_rpc_request_cbs_unref(request->callbacks);
}

static void _linphone_xml_rpc_session_destroy(LinphoneXmlRpcSession *session) {
	belle_sip_free(session->url);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneXmlRpcRequest);
BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneXmlRpcSession);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneXmlRpcRequest, belle_sip_object_t,
	(belle_sip_object_destroy_t)_linphone_xml_rpc_request_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneXmlRpcSession, belle_sip_object_t,
	(belle_sip_object_destroy_t)_linphone_xml_rpc_session_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);


LinphoneXmlRpcRequest * linphone_xml_rpc_request_new(const char *method, LinphoneXmlRpcArgType return_type) {
	LinphoneXmlRpcRequest *request = _linphone_xml_rpc_request_new(method, return_type);
	format_request(request);
	return request;
}

LinphoneXmlRpcRequest * linphone_xml_rpc_request_new_with_args(const char *method, LinphoneXmlRpcArgType return_type, ...) {
	bool_t cont = TRUE;
	va_list args;
	LinphoneXmlRpcArgType arg_type;
	LinphoneXmlRpcRequest *request = _linphone_xml_rpc_request_new(method, return_type);
	va_start(args, return_type);
	while (cont) {
		arg_type = va_arg(args, LinphoneXmlRpcArgType);
		switch (arg_type) {
			case LinphoneXmlRpcArgNone:
				cont = FALSE;
				break;
			case LinphoneXmlRpcArgInt:
				_linphone_xml_rpc_request_add_int_arg(request, va_arg(args, int));
				break;
			case LinphoneXmlRpcArgString:
				_linphone_xml_rpc_request_add_string_arg(request, va_arg(args, char *));
				break;
		}
	}
	va_end(args);
	format_request(request);
	return request;
}

LinphoneXmlRpcRequest * linphone_xml_rpc_request_ref(LinphoneXmlRpcRequest *request) {
	belle_sip_object_ref(request);
	return request;
}

void linphone_xml_rpc_request_unref(LinphoneXmlRpcRequest *request) {
	belle_sip_object_unref(request);
}

void *linphone_xml_rpc_request_get_user_data(const LinphoneXmlRpcRequest *request) {
	return request->user_data;
}

void linphone_xml_rpc_request_set_user_data(LinphoneXmlRpcRequest *request, void *ud) {
	request->user_data = ud;
}

void linphone_xml_rpc_request_add_int_arg(LinphoneXmlRpcRequest *request, int value) {
	_linphone_xml_rpc_request_add_int_arg(request, value);
	format_request(request);
}

void linphone_xml_rpc_request_add_string_arg(LinphoneXmlRpcRequest *request, const char *value) {
	_linphone_xml_rpc_request_add_string_arg(request, value);
	format_request(request);
}

LinphoneXmlRpcRequestCbs * linphone_xml_rpc_request_get_callbacks(const LinphoneXmlRpcRequest *request) {
	return request->callbacks;
}

const char * linphone_xml_rpc_request_get_content(const LinphoneXmlRpcRequest *request) {
	return request->content;
}

LinphoneXmlRpcStatus linphone_xml_rpc_request_get_status(const LinphoneXmlRpcRequest *request) {
	return request->status;
}

int linphone_xml_rpc_request_get_int_response(const LinphoneXmlRpcRequest *request) {
	return request->response.data.i;
}

const char * linphone_xml_rpc_request_get_string_response(const LinphoneXmlRpcRequest *request) {
	return request->response.data.s;
}


LinphoneXmlRpcSession * linphone_xml_rpc_session_new(LinphoneCore *core, const char *url) {
	LinphoneXmlRpcSession *session = belle_sip_object_new(LinphoneXmlRpcSession);
	session->core = core;
	session->url = belle_sip_strdup(url);
	return session;
}

LinphoneXmlRpcSession * linphone_xml_rpc_session_ref(LinphoneXmlRpcSession *session) {
	belle_sip_object_ref(session);
	return session;
}

void linphone_xml_rpc_session_unref(LinphoneXmlRpcSession *session) {
	belle_sip_object_unref(session);
}

void *linphone_xml_rpc_session_get_user_data(const LinphoneXmlRpcSession *session) {
	return session->user_data;
}

void linphone_xml_rpc_session_set_user_data(LinphoneXmlRpcSession *session, void *ud) {
	session->user_data = ud;
}

void linphone_xml_rpc_session_send_request(LinphoneXmlRpcSession *session, LinphoneXmlRpcRequest *request) {
	belle_http_request_listener_callbacks_t cbs = { 0 };
	belle_http_request_listener_t *l;
	belle_generic_uri_t *uri;
	belle_http_request_t *req;
	belle_sip_memory_body_handler_t *bh;
	const char *data;
	linphone_xml_rpc_request_ref(request);

	uri = belle_generic_uri_parse(session->url);
	if (!uri) {
		ms_error("Could not send request, URL %s is invalid", session->url);
		process_io_error_from_post_xml_rpc_request(request, NULL);
		return;
	}
	req = belle_http_request_create("POST", uri, belle_sip_header_content_type_create("text", "xml"), NULL);
	if (!req) {
		belle_sip_object_unref(uri);
		process_io_error_from_post_xml_rpc_request(request, NULL);
		return;
	}
	data = linphone_xml_rpc_request_get_content(request);
	bh = belle_sip_memory_body_handler_new_copy_from_buffer(data, strlen(data), NULL, NULL);
	belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req), BELLE_SIP_BODY_HANDLER(bh));
	cbs.process_response = process_response_from_post_xml_rpc_request;
	cbs.process_io_error = process_io_error_from_post_xml_rpc_request;
	cbs.process_auth_requested = process_auth_requested_from_post_xml_rpc_request;
	l = belle_http_request_listener_create_from_callbacks(&cbs, request);
	belle_http_provider_send_request(session->core->http_provider, req, l);
	/*ensure that the listener object will be destroyed with the request*/
	belle_sip_object_data_set(BELLE_SIP_OBJECT(request), "listener", l, belle_sip_object_unref);
	/*prevent destruction of the session while there are still pending http requests*/
	belle_sip_object_data_set(BELLE_SIP_OBJECT(request), "session", belle_sip_object_ref(session), belle_sip_object_unref);
}

void linphone_xml_rpc_session_release(LinphoneXmlRpcSession *session){
	session->released = TRUE;
	belle_sip_object_unref(session);
}

