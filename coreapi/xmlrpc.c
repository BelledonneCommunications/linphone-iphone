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

#include <string.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>



typedef struct _LinphoneXmlRpcArg {
	LinphoneXmlRpcArgType type;
	union {
		int i;
		char *s;
	} data;
} LinphoneXmlRpcArg;

static void format_request(LinphoneXmlRpcRequest *request) {
	char si[64];
	belle_sip_list_t *arg_ptr = request->arg_list;
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;

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
				err = xmlTextWriterWriteElement(writer, (const xmlChar *)"string", (const xmlChar *)arg->data.s);
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

static void process_io_error_from_post_xml_rpc_request(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneXmlRpcSession *session = (LinphoneXmlRpcSession *)data;
	ms_error("I/O Error during XML-RPC request sending to %s", session->url);
	session->request->status = LinphoneXmlRpcStatusFailed;
	ms_cond_signal(&session->cond);
}

static void process_auth_requested_from_post_xml_rpc_request(void *data, belle_sip_auth_event_t *event) {
	LinphoneXmlRpcSession *session = (LinphoneXmlRpcSession *)data;
	ms_error("Error during XML-RPC request sending to connect %s", session->url);
	session->request->status = LinphoneXmlRpcStatusFailed;
	ms_cond_signal(&session->cond);
}

static void parse_valid_xml_rpc_response(LinphoneXmlRpcSession *session, const char *response_body) {
	xmlparsing_context_t *xml_ctx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
	session->request->status = LinphoneXmlRpcStatusFailed;
	xml_ctx->doc = xmlReadDoc((const unsigned char*)response_body, 0, NULL, 0);
	if (xml_ctx->doc != NULL) {
		const char *response_str;
		if (linphone_create_xml_xpath_context(xml_ctx) < 0) goto end;
		response_str = linphone_get_xml_text_content(xml_ctx, "/methodResponse/params/param/value/int");
		if (response_str != NULL) {
			session->request->response = atoi(response_str);
			session->request->status = LinphoneXmlRpcStatusOk;
		}
		linphone_free_xml_text_content(response_str);
	} else {
		ms_warning("Wrongly formatted XML-RPC response: %s", xml_ctx->errorBuffer);
	}
end:
	linphone_xmlparsing_context_destroy(xml_ctx);
	ms_cond_signal(&session->cond);
}

static void notify_xml_rpc_error(LinphoneXmlRpcSession *session) {
	session->request->status = LinphoneXmlRpcStatusOk;
	ms_cond_signal(&session->cond);
}

/**
 * Callback function called when we have a response from server during the upload of the log collection to the server (rcs5.1 recommandation)
 * Note: The first post is empty and the server shall reply a 204 (No content) message, this will trigger a new post request to the server
 * to upload the file. The server response to this second post is processed by this same function
 *
 * @param[in] data The user-defined pointer associated with the request, it contains the LinphoneCore object
 * @param[in] event The response from server
 */
static void process_response_from_post_xml_rpc_request(void *data, const belle_http_response_event_t *event) {
	LinphoneXmlRpcSession *session = (LinphoneXmlRpcSession *)data;

	/* Check the answer code */
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code == 200) { /* Valid response from the server. */
			parse_valid_xml_rpc_response(session, belle_sip_message_get_body((belle_sip_message_t *)event->response));
		} else {
			notify_xml_rpc_error(session);
		}
	}
}


static void linphone_xml_rpc_request_destroy(LinphoneXmlRpcRequest *request) {
	belle_sip_list_free_with_data(request->arg_list, (void (*)(void*))free_arg);
	if (request->content) belle_sip_free(request->content);
	belle_sip_free(request->method);
}

static void linphone_xml_rpc_session_destroy(LinphoneXmlRpcSession *session) {
	belle_sip_free(session->url);
	ms_cond_destroy(&session->cond);
	ms_mutex_destroy(&session->cond_mutex);
	ms_mutex_destroy(&session->mutex);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneXmlRpcRequest);
BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneXmlRpcSession);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneXmlRpcRequest, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_xml_rpc_request_destroy,
	NULL, // clone
	NULL, // marshal
	TRUE
);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneXmlRpcSession, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_xml_rpc_session_destroy,
	NULL, // clone
	NULL, // marshal
	TRUE
);


LinphoneXmlRpcRequest * linphone_xml_rpc_request_new(const char *method, ...) {
	bool_t cont = TRUE;
	va_list args;
	LinphoneXmlRpcArgType arg_type;
	LinphoneXmlRpcArg *arg = NULL;
	LinphoneXmlRpcRequest *request = belle_sip_object_new(LinphoneXmlRpcRequest);
	belle_sip_object_ref(request);
	request->status = LinphoneXmlRpcStatusPending;
	request->response = -1;
	request->method = belle_sip_strdup(method);
	va_start(args, method);
	while (cont) {
		arg_type = va_arg(args, LinphoneXmlRpcArgType);
		switch (arg_type) {
			case LinphoneXmlRpcArgNone:
				cont = FALSE;
				break;
			case LinphoneXmlRpcArgInt:
				arg = belle_sip_malloc0(sizeof(LinphoneXmlRpcArg));
				arg->type = LinphoneXmlRpcArgInt;
				arg->data.i = va_arg(args, int);
				request->arg_list = belle_sip_list_append(request->arg_list, arg);
				break;
			case LinphoneXmlRpcArgString:
				arg = belle_sip_malloc0(sizeof(LinphoneXmlRpcArg));
				arg->type = LinphoneXmlRpcArgString;
				arg->data.s = belle_sip_strdup(va_arg(args, char *));
				request->arg_list = belle_sip_list_append(request->arg_list, arg);
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

const char * linphone_xml_rpc_request_get_content(const LinphoneXmlRpcRequest *request) {
	return request->content;
}

LinphoneXmlRpcStatus linphone_xml_rpc_request_get_status(const LinphoneXmlRpcRequest *request) {
	return request->status;
}

int linphone_xml_rpc_request_get_response(const LinphoneXmlRpcRequest *request) {
	return request->response;
}


LinphoneXmlRpcSession * linphone_xml_rpc_session_new(LinphoneCore *core, const char *url) {
	LinphoneXmlRpcSession *session = belle_sip_object_new(LinphoneXmlRpcSession);
	belle_sip_object_ref(session);
	session->core = core;
	session->url = belle_sip_strdup(url);
	ms_cond_init(&session->cond, NULL);
	ms_mutex_init(&session->cond_mutex, NULL);
	ms_mutex_init(&session->mutex, NULL);
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

LinphoneXmlRpcStatus linphone_xml_rpc_session_send_request(LinphoneXmlRpcSession *session, LinphoneXmlRpcRequest *request) {
	belle_http_request_listener_callbacks_t cbs = { 0 };
	belle_http_request_listener_t *l;
	belle_generic_uri_t *uri;
	belle_http_request_t *req;
	belle_sip_memory_body_handler_t *bh;
	const char *data;
	LinphoneXmlRpcStatus status;

	ms_mutex_lock(&session->mutex);
	session->request = linphone_xml_rpc_request_ref(request);
	session->content = linphone_content_new();
	linphone_content_set_type(session->content, "text");
	linphone_content_set_subtype(session->content,"xml");
	linphone_content_set_string_buffer(session->content, linphone_xml_rpc_request_get_content(request));
	uri = belle_generic_uri_parse(session->url);
	req = belle_http_request_create("POST", uri, belle_sip_header_content_type_create("text", "xml"), NULL);
	data = linphone_xml_rpc_request_get_content(request);
	bh = belle_sip_memory_body_handler_new_copy_from_buffer(data, strlen(data), NULL, session);
	belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req), BELLE_SIP_BODY_HANDLER(bh));
	cbs.process_response = process_response_from_post_xml_rpc_request;
	cbs.process_io_error = process_io_error_from_post_xml_rpc_request;
	cbs.process_auth_requested = process_auth_requested_from_post_xml_rpc_request;
	l = belle_http_request_listener_create_from_callbacks(&cbs, session);
	belle_http_provider_send_request(session->core->http_provider, req, l);

	ms_mutex_lock(&session->cond_mutex);
	ms_cond_wait(&session->cond, &session->cond_mutex);
	ms_mutex_unlock(&session->cond_mutex);
	status = session->request->status;
	linphone_xml_rpc_request_unref(session->request);
	session->request = NULL;
	linphone_content_unref(session->content);
	session->content = NULL;
	ms_mutex_unlock(&session->mutex);
	return status;
}
