/*
 * c-content.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "linphone/api/c-content.h"
#include "linphone/wrapper_utils.h"

#include "c-wrapper/c-wrapper.h"

#include "content/content.h"

// =============================================================================

using namespace std;

L_DECLARE_C_CLONABLE_OBJECT_IMPL(Content,
    SalBodyHandler *body_handler;
	void *cryptoContext; /**< crypto context used to encrypt file for RCS file transfer */
)

// =============================================================================
// Reference and user data handling functions.
// =============================================================================

LinphoneContent * linphone_content_ref(LinphoneContent *content) {
    belle_sip_object_ref(content);
	return content;
}

void linphone_content_unref(LinphoneContent *content) {
    belle_sip_object_unref(content);
}

void *linphone_content_get_user_data(const LinphoneContent *content) {
    return L_GET_USER_DATA_FROM_C_OBJECT(content);
}

void linphone_content_set_user_data(LinphoneContent *content, void *ud) {
    return L_SET_USER_DATA_FROM_C_OBJECT(content, ud);
}

// =============================================================================

const char * linphone_content_get_type(const LinphoneContent *content) {
    return NULL;
}

void linphone_content_set_type(LinphoneContent *content, const char *type) {
    
}

const char * linphone_content_get_subtype(const LinphoneContent *content) {
    return NULL;
}

void linphone_content_set_subtype(LinphoneContent *content, const char *subtype) {

}

uint8_t * linphone_content_get_buffer(const LinphoneContent *content) {
    return NULL;
}

void linphone_content_set_buffer(LinphoneContent *content, const uint8_t *buffer, size_t size) {

}

const char * linphone_content_get_string_buffer(const LinphoneContent *content) {
    return NULL;
}

void linphone_content_set_string_buffer(LinphoneContent *content, const char *buffer) {

}

size_t linphone_content_get_size(const LinphoneContent *content) {
    return 0;
}

void linphone_content_set_size(LinphoneContent *content, size_t size) {

}

const char * linphone_content_get_encoding(const LinphoneContent *content) {
    return NULL;
}

void linphone_content_set_encoding(LinphoneContent *content, const char *encoding) {

}

const char * linphone_content_get_name(const LinphoneContent *content) {
    return NULL;
}

void linphone_content_set_name(LinphoneContent *content, const char *name) {

}

bool_t linphone_content_is_multipart(const LinphoneContent *content) {
    return FALSE;
}

LinphoneContent * linphone_content_get_part(const LinphoneContent *content, int idx) {
    return NULL;
}

LinphoneContent * linphone_content_find_part_by_header(const LinphoneContent *content, const char *header_name, const char *header_value) {
    return NULL;
}

const char * linphone_content_get_custom_header(const LinphoneContent *content, const char *header_name) {
    return NULL;
}

const char *linphone_content_get_key(const LinphoneContent *content) {
    return NULL;
}

size_t linphone_content_get_key_size(const LinphoneContent *content) {
    return 0;
}

void linphone_content_set_key(LinphoneContent *content, const char *key, const size_t keyLength) {

}

// =============================================================================
// Private functions.
// =============================================================================

LinphoneContent * linphone_content_new(void) {
	return NULL;
}

LinphoneContent * linphone_content_copy(const LinphoneContent *ref) {
	return NULL;
}

static LinphoneContent * linphone_content_new_with_body_handler(SalBodyHandler *body_handler) {
	return NULL;
}

LinphoneContent * linphone_core_create_content(LinphoneCore *lc) {
	return NULL;
}

/* crypto context is managed(allocated/freed) by the encryption function, so provide the address of field in the private structure */
void ** linphone_content_get_cryptoContext_address(LinphoneContent *content) {
	return &(content->cryptoContext);;
}

LinphoneContent * linphone_content_from_sal_body_handler(SalBodyHandler *body_handler) {
	if (body_handler) {
		return linphone_content_new_with_body_handler(body_handler);
	}
	return NULL;
}

SalBodyHandler * sal_body_handler_from_content(const LinphoneContent *content) {
	return NULL;
}