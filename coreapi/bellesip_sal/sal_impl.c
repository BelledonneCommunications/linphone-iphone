/*
linphone
Copyright (C) 2012  Belledonne Communications, Grenoble, France

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

#include "sal_impl.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


void sal_enable_log(){
	sal_set_log_level(ORTP_MESSAGE);
}

void sal_disable_log() {
	sal_set_log_level(ORTP_ERROR);
}

void sal_set_log_level(OrtpLogLevel level) {
	belle_sip_log_level  belle_sip_level = BELLE_SIP_LOG_MESSAGE;
	if ((level&ORTP_FATAL) != 0) {
		belle_sip_level = BELLE_SIP_LOG_FATAL;
	}
	if ((level&ORTP_ERROR) != 0) {
		belle_sip_level = BELLE_SIP_LOG_ERROR;
	}
	if ((level&ORTP_WARNING) != 0) {
		belle_sip_level = BELLE_SIP_LOG_WARNING;
	}
	if ((level&ORTP_MESSAGE) != 0) {
		belle_sip_level = BELLE_SIP_LOG_MESSAGE;
	}
	if (((level&ORTP_DEBUG) != 0) || ((level&ORTP_TRACE) != 0)) {
		belle_sip_level = BELLE_SIP_LOG_DEBUG;
	}

	belle_sip_set_log_level(belle_sip_level);
}
static BctbxLogFunc _belle_sip_log_handler = bctbx_logv_out;

void sal_set_log_handler(BctbxLogFunc log_handler) {
	_belle_sip_log_handler = log_handler;
	belle_sip_set_log_handler(log_handler);
}

SalAuthInfo* sal_auth_info_create(belle_sip_auth_event_t* event) {
	SalAuthInfo* auth_info = sal_auth_info_new();
	auth_info->realm = ms_strdup(belle_sip_auth_event_get_realm(event));
	auth_info->username = ms_strdup(belle_sip_auth_event_get_username(event));
	auth_info->domain = ms_strdup(belle_sip_auth_event_get_domain(event));
	auth_info->mode = (SalAuthMode)belle_sip_auth_event_get_mode(event);
	auth_info->algorithm = ms_strdup(belle_sip_auth_event_get_algorithm(event));
	return auth_info;
}

SalAuthMode sal_auth_info_get_mode(const SalAuthInfo* auth_info) { return auth_info->mode; }
belle_sip_signing_key_t *sal_auth_info_get_signing_key(const SalAuthInfo* auth_info) { return auth_info->key; }
belle_sip_certificates_chain_t *sal_auth_info_get_certificates_chain(const SalAuthInfo* auth_info) { return auth_info->certificates; }
void sal_auth_info_set_mode(SalAuthInfo* auth_info, SalAuthMode mode) { auth_info->mode = mode; }
void sal_certificates_chain_delete(belle_sip_certificates_chain_t *chain) {
	belle_sip_object_unref((belle_sip_object_t *)chain);
}
void sal_signing_key_delete(belle_sip_signing_key_t *key) {
	belle_sip_object_unref((belle_sip_object_t *)key);
}

int sal_auth_compute_ha1(const char* userid,const char* realm,const char* password, char ha1[33]) {
	return belle_sip_auth_helper_compute_ha1(userid, realm, password, ha1);
}

int sal_auth_compute_ha1_for_algorithm(
	const char *userid,
	const char *realm,
	const char *password,
	char *ha1,
	size_t size,
	const char *algo
) {
	return belle_sip_auth_helper_compute_ha1_for_algorithm(userid, realm, password, ha1, size, algo);
}

SalCustomHeader *sal_custom_header_ref(SalCustomHeader *ch){
	if (ch == NULL) return NULL;
	belle_sip_object_ref(ch);
	return ch;
}

void sal_custom_header_unref(SalCustomHeader *ch){
	if (ch == NULL) return;
	belle_sip_object_unref(ch);
}

SalCustomHeader *sal_custom_header_append(SalCustomHeader *ch, const char *name, const char *value){
	belle_sip_message_t *msg=(belle_sip_message_t*)ch;
	belle_sip_header_t *h;

	if (msg==NULL){
		msg=(belle_sip_message_t*)belle_sip_request_new();
		belle_sip_object_ref(msg);
	}
	h=belle_sip_header_create(name,value);
	if (h==NULL){
		belle_sip_error("Fail to parse custom header.");
		return (SalCustomHeader*)msg;
	}
	belle_sip_message_add_header(msg,h);
	return (SalCustomHeader*)msg;
}

const char *sal_custom_header_find(const SalCustomHeader *ch, const char *name){
	if (ch){
		belle_sip_header_t *h=belle_sip_message_get_header((belle_sip_message_t*)ch,name);

		if (h){
			return belle_sip_header_get_unparsed_value(h);
		}
	}
	return NULL;
}

SalCustomHeader *sal_custom_header_remove(SalCustomHeader *ch, const char *name) {
	belle_sip_message_t *msg=(belle_sip_message_t*)ch;
	if (msg==NULL) return NULL;

	belle_sip_message_remove_header(msg, name);
	return (SalCustomHeader*)msg;
}

void sal_custom_header_free(SalCustomHeader *ch){
	if (ch==NULL) return;
	belle_sip_object_unref((belle_sip_message_t*)ch);
}

SalCustomHeader *sal_custom_header_clone(const SalCustomHeader *ch){
	if (ch==NULL) return NULL;
	return (SalCustomHeader*)belle_sip_object_ref((belle_sip_message_t*)ch);
}

SalCustomSdpAttribute * sal_custom_sdp_attribute_append(SalCustomSdpAttribute *csa, const char *name, const char *value) {
	belle_sdp_session_description_t *desc = (belle_sdp_session_description_t *)csa;
	belle_sdp_attribute_t *attr;

	if (desc == NULL) {
		desc = (belle_sdp_session_description_t *)belle_sdp_session_description_new();
		belle_sip_object_ref(desc);
	}
	attr = BELLE_SDP_ATTRIBUTE(belle_sdp_raw_attribute_create(name, value));
	if (attr == NULL) {
		belle_sip_error("Fail to create custom SDP attribute.");
		return (SalCustomSdpAttribute*)desc;
	}
	belle_sdp_session_description_add_attribute(desc, attr);
	return (SalCustomSdpAttribute *)desc;
}

const char * sal_custom_sdp_attribute_find(const SalCustomSdpAttribute *csa, const char *name) {
	if (csa) {
		return belle_sdp_session_description_get_attribute_value((belle_sdp_session_description_t *)csa, name);
	}
	return NULL;
}

void sal_custom_sdp_attribute_free(SalCustomSdpAttribute *csa) {
	if (csa == NULL) return;
	belle_sip_object_unref((belle_sdp_session_description_t *)csa);
}

SalCustomSdpAttribute * sal_custom_sdp_attribute_clone(const SalCustomSdpAttribute *csa) {
	if (csa == NULL) return NULL;
	return (SalCustomSdpAttribute *)belle_sip_object_ref((belle_sdp_session_description_t *)csa);
}

/** Parse a file containing either a certificate chain order in PEM format or a single DER cert
 * @param auth_info structure where to store the result of parsing
 * @param path path to certificate chain file
 * @param format either PEM or DER
 */
void sal_certificates_chain_parse_file(SalAuthInfo* auth_info, const char* path, SalCertificateRawFormat format) {
	auth_info->certificates = (belle_sip_certificates_chain_t*) belle_sip_certificates_chain_parse_file(path, (belle_sip_certificate_raw_format_t)format);
	if (auth_info->certificates) belle_sip_object_ref((belle_sip_object_t *) auth_info->certificates);
}

/**
 * Parse a file containing either a private or public rsa key
 * @param auth_info structure where to store the result of parsing
 * @param passwd password (optionnal)
 */
void sal_signing_key_parse_file(SalAuthInfo* auth_info, const char* path, const char *passwd) {
	auth_info->key = (belle_sip_signing_key_t *) belle_sip_signing_key_parse_file(path, passwd);
	if (auth_info->key) belle_sip_object_ref((belle_sip_object_t *) auth_info->key);
}

/** Parse a buffer containing either a certificate chain order in PEM format or a single DER cert
 * @param auth_info structure where to store the result of parsing
 * @param buffer the buffer to parse
 * @param format either PEM or DER
 */
void sal_certificates_chain_parse(SalAuthInfo* auth_info, const char* buffer, SalCertificateRawFormat format) {
	size_t len = buffer != NULL ? strlen(buffer) : 0;
	auth_info->certificates = (belle_sip_certificates_chain_t*) belle_sip_certificates_chain_parse(buffer, len, (belle_sip_certificate_raw_format_t)format);
	if (auth_info->certificates) belle_sip_object_ref((belle_sip_object_t *) auth_info->certificates);
}

/**
 * Parse a buffer containing either a private or public rsa key
 * @param auth_info structure where to store the result of parsing
 * @param passwd password (optionnal)
 */
void sal_signing_key_parse(SalAuthInfo* auth_info, const char* buffer, const char *passwd) {
	size_t len = buffer != NULL ? strlen(buffer) : 0;
	auth_info->key = (belle_sip_signing_key_t *) belle_sip_signing_key_parse(buffer, len, passwd);
	if (auth_info->key) belle_sip_object_ref((belle_sip_object_t *) auth_info->key);
}

/**
 * Parse a directory to get a certificate with the given subject common name
 *
 */
void sal_certificates_chain_parse_directory(char **certificate_pem, char **key_pem, char **fingerprint, const char* path, const char *subject, SalCertificateRawFormat format, bool_t generate_certificate, bool_t generate_dtls_fingerprint) {
	belle_sip_certificates_chain_t *certificate = NULL;
	belle_sip_signing_key_t *key = NULL;
	*certificate_pem = NULL;
	*key_pem = NULL;
	if (belle_sip_get_certificate_and_pkey_in_dir(path, subject, &certificate, &key, (belle_sip_certificate_raw_format_t)format) == 0) {
		*certificate_pem = belle_sip_certificates_chain_get_pem(certificate);
		*key_pem = belle_sip_signing_key_get_pem(key);
		ms_message("Retrieve certificate with CN=%s successful\n", subject);
	} else {
		if (generate_certificate == TRUE) {
			if ( belle_sip_generate_self_signed_certificate(path, subject, &certificate, &key) == 0) {
				*certificate_pem = belle_sip_certificates_chain_get_pem(certificate);
				*key_pem = belle_sip_signing_key_get_pem(key);
				ms_message("Generate self-signed certificate with CN=%s successful\n", subject);
			}
		}
	}
	/* generate the fingerprint as described in RFC4572 if needed */
	if ((generate_dtls_fingerprint == TRUE) && (fingerprint != NULL)) {
		if (*fingerprint != NULL) {
			ms_free(*fingerprint);
		}
		*fingerprint = belle_sip_certificates_chain_get_fingerprint(certificate);
	}

	/* free key and certificate */
	if ( certificate != NULL ) {
		belle_sip_object_unref(certificate);
	}
	if ( key != NULL ) {
		belle_sip_object_unref(key);
	}
}

unsigned char * sal_get_random_bytes(unsigned char *ret, size_t size){
	return belle_sip_random_bytes(ret,size);
}

char *sal_get_random_token(int size){
	return belle_sip_random_token(reinterpret_cast<char *>(ms_malloc((size_t)size)),(size_t)size);
}

unsigned int sal_get_random(void){
	unsigned int ret=0;
	belle_sip_random_bytes((unsigned char*)&ret,4);
	return ret;
}

unsigned long sal_begin_background_task(const char *name, void (*max_time_reached)(void *), void *data){
	return belle_sip_begin_background_task(name, max_time_reached, data);
}

void sal_end_background_task(unsigned long id){
	belle_sip_end_background_task(id);
}

static belle_sip_header_t * sal_body_handler_find_header(const SalBodyHandler *body_handler, const char *header_name) {
	belle_sip_body_handler_t *bsbh = BELLE_SIP_BODY_HANDLER(body_handler);
	const belle_sip_list_t *l = belle_sip_body_handler_get_headers(bsbh);
	for (; l != NULL; l = l->next) {
		belle_sip_header_t *header = BELLE_SIP_HEADER(l->data);
		if (strcmp(belle_sip_header_get_name(header), header_name) == 0) {
			return header;
		}
	}
	return NULL;
}

SalBodyHandler * sal_body_handler_new(void) {
	belle_sip_memory_body_handler_t *body_handler = belle_sip_memory_body_handler_new(NULL, NULL);
	return (SalBodyHandler *)BELLE_SIP_BODY_HANDLER(body_handler);
}

SalBodyHandler * sal_body_handler_ref(SalBodyHandler *body_handler) {
	return (SalBodyHandler *)belle_sip_object_ref(BELLE_SIP_OBJECT(body_handler));
}

void sal_body_handler_unref(SalBodyHandler *body_handler) {
	belle_sip_object_unref(BELLE_SIP_OBJECT(body_handler));
}

const char * sal_body_handler_get_type(const SalBodyHandler *body_handler) {
	belle_sip_header_content_type_t *content_type = BELLE_SIP_HEADER_CONTENT_TYPE(sal_body_handler_find_header(body_handler, "Content-Type"));
	if (content_type != NULL) {
		return belle_sip_header_content_type_get_type(content_type);
	}
	return NULL;
}

void sal_body_handler_set_type(SalBodyHandler *body_handler, const char *type) {
	belle_sip_header_content_type_t *content_type = BELLE_SIP_HEADER_CONTENT_TYPE(sal_body_handler_find_header(body_handler, "Content-Type"));
	if (content_type == NULL) {
		content_type = belle_sip_header_content_type_new();
		belle_sip_body_handler_add_header(BELLE_SIP_BODY_HANDLER(body_handler), BELLE_SIP_HEADER(content_type));
	}
	belle_sip_header_content_type_set_type(content_type, type);
}

const char * sal_body_handler_get_subtype(const SalBodyHandler *body_handler) {
	belle_sip_header_content_type_t *content_type = BELLE_SIP_HEADER_CONTENT_TYPE(sal_body_handler_find_header(body_handler, "Content-Type"));
	if (content_type != NULL) {
		return belle_sip_header_content_type_get_subtype(content_type);
	}
	return NULL;
}

void sal_body_handler_set_subtype(SalBodyHandler *body_handler, const char *subtype) {
	belle_sip_header_content_type_t *content_type = BELLE_SIP_HEADER_CONTENT_TYPE(sal_body_handler_find_header(body_handler, "Content-Type"));
	if (content_type == NULL) {
		content_type = belle_sip_header_content_type_new();
		belle_sip_body_handler_add_header(BELLE_SIP_BODY_HANDLER(body_handler), BELLE_SIP_HEADER(content_type));
	}
	belle_sip_header_content_type_set_subtype(content_type, subtype);
}

const belle_sip_list_t * sal_body_handler_get_content_type_parameters_names(const SalBodyHandler *body_handler) {
	belle_sip_header_content_type_t *content_type = BELLE_SIP_HEADER_CONTENT_TYPE(sal_body_handler_find_header(body_handler, "Content-Type"));
	if (content_type != NULL) {
		return belle_sip_parameters_get_parameter_names(BELLE_SIP_PARAMETERS(content_type));
	}
	return NULL;
}

const char * sal_body_handler_get_content_type_parameter(const SalBodyHandler *body_handler, const char *name) {
	belle_sip_header_content_type_t *content_type = BELLE_SIP_HEADER_CONTENT_TYPE(sal_body_handler_find_header(body_handler, "Content-Type"));
	if (content_type != NULL) {
		return belle_sip_parameters_get_parameter(BELLE_SIP_PARAMETERS(content_type), name);
	}
	return NULL;
}

void sal_body_handler_set_content_type_parameter(SalBodyHandler *body_handler, const char *paramName, const char *paramValue) {
	belle_sip_header_content_type_t *content_type = BELLE_SIP_HEADER_CONTENT_TYPE(sal_body_handler_find_header(body_handler, "Content-Type"));
	if (content_type != NULL) {
		belle_sip_parameters_set_parameter(BELLE_SIP_PARAMETERS(content_type), paramName, paramValue);
	}
}

const char * sal_body_handler_get_encoding(const SalBodyHandler *body_handler) {
	belle_sip_header_t *content_encoding = sal_body_handler_find_header(body_handler, "Content-Encoding");
	if (content_encoding != NULL) {
		return belle_sip_header_get_unparsed_value(content_encoding);
	}
	return NULL;
}

void sal_body_handler_set_encoding(SalBodyHandler *body_handler, const char *encoding) {
	belle_sip_header_t *content_encoding = sal_body_handler_find_header(body_handler, "Content-Encoding");
	if (content_encoding != NULL) {
		belle_sip_body_handler_remove_header_from_ptr(BELLE_SIP_BODY_HANDLER(body_handler), content_encoding);
	}
	belle_sip_body_handler_add_header(BELLE_SIP_BODY_HANDLER(body_handler), belle_sip_header_create("Content-Encoding", encoding));
}

void * sal_body_handler_get_data(const SalBodyHandler *body_handler) {
	return belle_sip_memory_body_handler_get_buffer(BELLE_SIP_MEMORY_BODY_HANDLER(body_handler));
}

void sal_body_handler_set_data(SalBodyHandler *body_handler, void *data) {
	belle_sip_memory_body_handler_set_buffer(BELLE_SIP_MEMORY_BODY_HANDLER(body_handler), data);
}

size_t sal_body_handler_get_size(const SalBodyHandler *body_handler) {
	return belle_sip_body_handler_get_size(BELLE_SIP_BODY_HANDLER(body_handler));
}

void sal_body_handler_set_size(SalBodyHandler *body_handler, size_t size) {
	belle_sip_header_content_length_t *content_length = BELLE_SIP_HEADER_CONTENT_LENGTH(sal_body_handler_find_header(body_handler, "Content-Length"));
	if (content_length == NULL) {
		content_length = belle_sip_header_content_length_new();
		belle_sip_body_handler_add_header(BELLE_SIP_BODY_HANDLER(body_handler), BELLE_SIP_HEADER(content_length));
	}
	belle_sip_header_content_length_set_content_length(content_length, size);
	belle_sip_body_handler_set_size(BELLE_SIP_BODY_HANDLER(body_handler), size);
}

bool_t sal_body_handler_is_multipart(const SalBodyHandler *body_handler) {
	if (BELLE_SIP_IS_INSTANCE_OF(body_handler, belle_sip_multipart_body_handler_t)) return TRUE;
	return FALSE;
}

SalBodyHandler * sal_body_handler_get_part(const SalBodyHandler *body_handler, int idx) {
	const belle_sip_list_t *l = belle_sip_multipart_body_handler_get_parts(BELLE_SIP_MULTIPART_BODY_HANDLER(body_handler));
	return (SalBodyHandler *)belle_sip_list_nth_data(l, idx);
}

const belle_sip_list_t * sal_body_handler_get_parts(const SalBodyHandler *body_handler) {
	if (!sal_body_handler_is_multipart(body_handler)) return NULL;
	return belle_sip_multipart_body_handler_get_parts(BELLE_SIP_MULTIPART_BODY_HANDLER(body_handler));
}

SalBodyHandler * sal_body_handler_find_part_by_header(const SalBodyHandler *body_handler, const char *header_name, const char *header_value) {
	const belle_sip_list_t *l = belle_sip_multipart_body_handler_get_parts(BELLE_SIP_MULTIPART_BODY_HANDLER(body_handler));
	for (; l != NULL; l = l->next) {
		belle_sip_body_handler_t *bsbh = BELLE_SIP_BODY_HANDLER(l->data);
		const belle_sip_list_t *headers = belle_sip_body_handler_get_headers(bsbh);
		for (; headers != NULL; headers = headers->next) {
			belle_sip_header_t *header = BELLE_SIP_HEADER(headers->data);
			if ((strcmp(belle_sip_header_get_name(header), header_name) == 0) && (strcmp(belle_sip_header_get_unparsed_value(header), header_value) == 0)) {
				return (SalBodyHandler *)bsbh;
			}
		}
	}
	return NULL;
}

const char * sal_body_handler_get_header(const SalBodyHandler *body_handler, const char *header_name) {
	belle_sip_header_t *header = sal_body_handler_find_header(body_handler, header_name);
	if (header != NULL) {
		return belle_sip_header_get_unparsed_value(header);
	}
	return NULL;
}

const belle_sip_list_t* sal_body_handler_get_headers(const SalBodyHandler *body_handler) {
	return belle_sip_body_handler_get_headers(BELLE_SIP_BODY_HANDLER(body_handler));
}
