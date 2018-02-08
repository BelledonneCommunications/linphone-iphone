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
#include "content/content-type.h"
#include "content/file-content.h"
#include "content/file-transfer-content.h"

// =============================================================================

using namespace std;

L_DECLARE_C_CLONABLE_OBJECT_IMPL(Content,
    SalBodyHandler *body_handler;
	void *cryptoContext; /**< crypto context used to encrypt file for RCS file transfer */
	mutable char *name;
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
    return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(content)->getContentType().getType());
}

void linphone_content_set_type(LinphoneContent *content, const char *type) {
    LinphonePrivate::ContentType ct = L_GET_CPP_PTR_FROM_C_OBJECT(content)->getContentType();
    ct.setType(L_C_TO_STRING(type));
    L_GET_CPP_PTR_FROM_C_OBJECT(content)->setContentType(ct);
}

const char * linphone_content_get_subtype(const LinphoneContent *content) {
    return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(content)->getContentType().getSubType());
}

void linphone_content_set_subtype(LinphoneContent *content, const char *subtype) {
    LinphonePrivate::ContentType ct = L_GET_CPP_PTR_FROM_C_OBJECT(content)->getContentType();
    ct.setSubType(L_C_TO_STRING(subtype));
    L_GET_CPP_PTR_FROM_C_OBJECT(content)->setContentType(ct);
}

uint8_t * linphone_content_get_buffer(const LinphoneContent *content) {
    return (uint8_t *)L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(content)->getBodyAsString());
}

void linphone_content_set_buffer(LinphoneContent *content, const uint8_t *buffer, size_t size) {
    L_GET_CPP_PTR_FROM_C_OBJECT(content)->setBody(buffer, size);
}

const char * linphone_content_get_string_buffer(const LinphoneContent *content) {
    return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(content)->getBodyAsString());
}

void linphone_content_set_string_buffer(LinphoneContent *content, const char *buffer) {
    L_GET_CPP_PTR_FROM_C_OBJECT(content)->setBody(L_C_TO_STRING(buffer));
}

size_t linphone_content_get_size(const LinphoneContent *content) {
    size_t size = L_GET_CPP_PTR_FROM_C_OBJECT(content)->getSize();
    if (size == 0) {
        size = sal_body_handler_get_size(content->body_handler);
    }
    return size;
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
    const LinphonePrivate::Content *c = L_GET_CPP_PTR_FROM_C_OBJECT(content);
    if (c->isFile()) {
        const LinphonePrivate::FileContent *fc = static_cast<const LinphonePrivate::FileContent *>(c);
        if (content->name) ms_free(content->name);
        content->name = ms_strdup(L_STRING_TO_C(fc->getFileName()));
    } else if (c->getContentType() == LinphonePrivate::ContentType::FileTransfer) {
        const LinphonePrivate::FileTransferContent *ftc = static_cast<const LinphonePrivate::FileTransferContent *>(c);
        if (content->name) ms_free(content->name);
        content->name = ms_strdup(L_STRING_TO_C(ftc->getFileName()));
    }
    return content->name;
}

void linphone_content_set_name(LinphoneContent *content, const char *name) {
    LinphonePrivate::Content *c = L_GET_CPP_PTR_FROM_C_OBJECT(content);
    if (c->isFile()) {
        LinphonePrivate::FileContent *fc = static_cast<LinphonePrivate::FileContent *>(c);
        fc->setFileName(L_C_TO_STRING(name));
    } else if (c->getContentType() == LinphonePrivate::ContentType::FileTransfer) {
        LinphonePrivate::FileTransferContent *ftc = static_cast<LinphonePrivate::FileTransferContent *>(c);
        ftc->setFileName(L_C_TO_STRING(name));
    }
    content->name = ms_strdup(name);
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

const char *linphone_content_get_key(const LinphoneContent *content) {
    const LinphonePrivate::Content *c = L_GET_CPP_PTR_FROM_C_OBJECT(content);
    if (c->isFile()) {
        const LinphonePrivate::FileContent *fc = static_cast<const LinphonePrivate::FileContent *>(c);
        return L_STRING_TO_C(fc->getFileKey());
    }
    return NULL;
}

size_t linphone_content_get_key_size(const LinphoneContent *content) {
    const LinphonePrivate::Content *c = L_GET_CPP_PTR_FROM_C_OBJECT(content);
    if (c->isFile()) {
        const LinphonePrivate::FileContent *fc = static_cast<const LinphonePrivate::FileContent *>(c);
        return fc->getFileKey().length();
    }
    return 0;
}

void linphone_content_set_key(LinphoneContent *content, const char *key, const size_t keyLength) {
    LinphonePrivate::Content *c = L_GET_CPP_PTR_FROM_C_OBJECT(content);
    if (c->isFile()) {
        LinphonePrivate::FileContent *fc = static_cast<LinphonePrivate::FileContent *>(c);
        fc->setFileKey(L_C_TO_STRING(key));
    }
}

// =============================================================================
// Private functions.
// =============================================================================

static void linphone_content_set_sal_body_handler(LinphoneContent *content, SalBodyHandler *body_handler) {
	if (content->body_handler != NULL) {
		sal_body_handler_unref(content->body_handler);
		content->body_handler = NULL;
	}
	content->body_handler = sal_body_handler_ref(body_handler);
}

static LinphoneContent * linphone_content_new_with_body_handler(SalBodyHandler *body_handler) {
	LinphoneContent *content = L_INIT(Content);
    content->cryptoContext = NULL;
    if (body_handler == NULL) {
		linphone_content_set_sal_body_handler(content, sal_body_handler_new());
	} else {
		linphone_content_set_sal_body_handler(content, body_handler);
	}
    LinphonePrivate::Content *c = new LinphonePrivate::Content();
    L_SET_CPP_PTR_FROM_C_OBJECT(content, c);
    return content;
}

LinphoneContent * linphone_content_new(void) {
	return linphone_content_new_with_body_handler(NULL);
}

LinphoneContent * linphone_content_copy(const LinphoneContent *ref) {
    //TODO
	return (LinphoneContent *)belle_sip_object_ref(belle_sip_object_clone(BELLE_SIP_OBJECT(ref)));
}

LinphoneContent * linphone_core_create_content(LinphoneCore *lc) {
	return linphone_content_new();
}

/* crypto context is managed(allocated/freed) by the encryption function, so provide the address of field in the private structure */
void ** linphone_content_get_cryptoContext_address(LinphoneContent *content) {
	return &(content->cryptoContext);
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