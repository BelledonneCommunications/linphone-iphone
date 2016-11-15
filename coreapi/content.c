/*
linphone
Copyright (C) 2010-2014 Belledonne Communications SARL

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



static void linphone_content_set_sal_body_handler(LinphoneContent *content, SalBodyHandler *body_handler) {
	if (content->body_handler != NULL) {
		sal_body_handler_unref(content->body_handler);
		content->body_handler = NULL;
	}
	content->body_handler = sal_body_handler_ref(body_handler);
}

static LinphoneContent * linphone_content_new_with_body_handler(SalBodyHandler *body_handler) {
	LinphoneContent *content = belle_sip_object_new(LinphoneContent);
	belle_sip_object_ref(content);
	content->owned_fields = TRUE;
	content->cryptoContext = NULL; /* this field is managed externally by encryption/decryption functions so be careful to initialise it to NULL */
	if (body_handler == NULL) {
		linphone_content_set_sal_body_handler(content, sal_body_handler_new());
	} else {
		linphone_content_set_sal_body_handler(content, body_handler);
	}
	return content;
}

static void linphone_content_destroy(LinphoneContent *content) {
	if (content->owned_fields == TRUE) {
		if (content->body_handler) sal_body_handler_unref(content->body_handler);
		if (content->name) belle_sip_free(content->name);
		if (content->key) belle_sip_free(content->key);
		/* note : crypto context is allocated/destroyed by the encryption function */
	}
}

static void linphone_content_clone(LinphoneContent *obj, const LinphoneContent *ref) {
	obj->owned_fields = TRUE;
	linphone_content_set_sal_body_handler(obj, sal_body_handler_new());
	if ((linphone_content_get_type(ref) != NULL) || (linphone_content_get_subtype(ref) != NULL)) {
		linphone_content_set_type(obj, linphone_content_get_type(ref));
		linphone_content_set_subtype(obj, linphone_content_get_subtype(ref));
	}
	if (linphone_content_get_encoding(ref) != NULL) {
		linphone_content_set_encoding(obj, linphone_content_get_encoding(ref));
	}
	linphone_content_set_name(obj, linphone_content_get_name(ref));
	linphone_content_set_key(obj, linphone_content_get_key(ref), linphone_content_get_key_size(ref));
	if (linphone_content_get_buffer(ref) != NULL) {
		linphone_content_set_buffer(obj, linphone_content_get_buffer(ref), linphone_content_get_size(ref));
	} else {
		linphone_content_set_size(obj, linphone_content_get_size(ref));
	}
}


BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneContent);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneContent, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_content_destroy,
	(belle_sip_object_clone_t)linphone_content_clone,
	NULL, // marshal
	TRUE
);


LinphoneContent * linphone_core_create_content(LinphoneCore *lc) {
	return linphone_content_new();
}

LinphoneContent * linphone_content_ref(LinphoneContent *content) {
	belle_sip_object_ref(content);
	return content;
}

void linphone_content_unref(LinphoneContent *content) {
	belle_sip_object_unref(content);
}

void *linphone_content_get_user_data(const LinphoneContent *content) {
	return content->user_data;
}

void linphone_content_set_user_data(LinphoneContent *content, void *ud) {
	content->user_data = ud;
}

const char * linphone_content_get_type(const LinphoneContent *content) {
	return sal_body_handler_get_type(content->body_handler);
}

void linphone_content_set_type(LinphoneContent *content, const char *type) {
	sal_body_handler_set_type(content->body_handler, type);
}

const char * linphone_content_get_subtype(const LinphoneContent *content) {
	return sal_body_handler_get_subtype(content->body_handler);
}

void linphone_content_set_subtype(LinphoneContent *content, const char *subtype) {
	sal_body_handler_set_subtype(content->body_handler, subtype);
}

void * linphone_content_get_buffer(const LinphoneContent *content) {
	return sal_body_handler_get_data(content->body_handler);
}

void linphone_content_set_buffer(LinphoneContent *content, const void *buffer, size_t size) {
	void *data;
	sal_body_handler_set_size(content->body_handler, size);
	data = belle_sip_malloc(size + 1);
	memcpy(data, buffer, size);
	((char *)data)[size] = '\0';
	sal_body_handler_set_data(content->body_handler, data);
}

const char * linphone_content_get_string_buffer(const LinphoneContent *content) {
	return (const char *)linphone_content_get_buffer(content);
}

void linphone_content_set_string_buffer(LinphoneContent *content, const char *buffer) {
	sal_body_handler_set_size(content->body_handler, strlen(buffer));
	sal_body_handler_set_data(content->body_handler, belle_sip_strdup(buffer));
}

size_t linphone_content_get_size(const LinphoneContent *content) {
	return sal_body_handler_get_size(content->body_handler);
}

void linphone_content_set_size(LinphoneContent *content, size_t size) {
	sal_body_handler_set_size(content->body_handler, size);
}

const char * linphone_content_get_encoding(const LinphoneContent *content) {
	return sal_body_handler_get_encoding(content->body_handler);
}

void linphone_content_set_encoding(LinphoneContent *content, const char *encoding) {
	sal_body_handler_set_encoding(content->body_handler, encoding);
}

const char * linphone_content_get_name(const LinphoneContent *content) {
	return content->name;
}

void linphone_content_set_name(LinphoneContent *content, const char *name) {
	if (content->name != NULL) {
		belle_sip_free(content->name);
		content->name = NULL;
	}
	if (name != NULL) {
		content->name = belle_sip_strdup(name);
	}
}

size_t linphone_content_get_key_size(const LinphoneContent *content) {
	return content->keyLength;
}

const char * linphone_content_get_key(const LinphoneContent *content) {
	return content->key;
}

void linphone_content_set_key(LinphoneContent *content, const char *key, const size_t keyLength) {
	if (content->key != NULL) {
		belle_sip_free(content->key);
		content->key = NULL;
	}
	if (key != NULL) {
		content->key = belle_sip_malloc(keyLength);
		memcpy(content->key, key, keyLength);
		content->keyLength = keyLength;
	}
}

/* crypto context is managed(allocated/freed) by the encryption function, so provide the address of field in the private structure */
void ** linphone_content_get_cryptoContext_address(LinphoneContent *content) {
	return &(content->cryptoContext);
}

bool_t linphone_content_is_multipart(const LinphoneContent *content) {
	return sal_body_handler_is_multipart(content->body_handler);
}

LinphoneContent * linphone_content_get_part(const LinphoneContent *content, int idx) {
	SalBodyHandler *part_body_handler;
	if (!linphone_content_is_multipart(content)) return NULL;
	part_body_handler = sal_body_handler_get_part(content->body_handler, idx);
	return linphone_content_from_sal_body_handler(part_body_handler);
}

LinphoneContent * linphone_content_find_part_by_header(const LinphoneContent *content, const char *header_name, const char *header_value) {
	SalBodyHandler *part_body_handler;
	if (!linphone_content_is_multipart(content)) return NULL;
	part_body_handler = sal_body_handler_find_part_by_header(content->body_handler, header_name, header_value);
	return linphone_content_from_sal_body_handler(part_body_handler);
}

const char * linphone_content_get_custom_header(const LinphoneContent *content, const char *header_name) {
	return sal_body_handler_get_header(content->body_handler, header_name);
}


LinphoneContent * linphone_content_new(void) {
	return linphone_content_new_with_body_handler(NULL);
}

LinphoneContent * linphone_content_copy(const LinphoneContent *ref) {
	return (LinphoneContent *)belle_sip_object_ref(belle_sip_object_clone(BELLE_SIP_OBJECT(ref)));
}

LinphoneContent * linphone_content_from_sal_body_handler(SalBodyHandler *body_handler) {
	if (body_handler) {
		return linphone_content_new_with_body_handler(body_handler);
	}
	return NULL;
}

SalBodyHandler * sal_body_handler_from_content(const LinphoneContent *content) {
	if (content == NULL) return NULL;
	return content->body_handler;
}
