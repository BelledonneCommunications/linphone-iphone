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
#include "content/header/header-param.h"
#include "content/header/header.h"
#include "content/content-manager.h"
#include "content/file-content.h"
#include "content/file-transfer-content.h"

// =============================================================================

using namespace std;

L_DECLARE_C_CLONABLE_OBJECT_IMPL(Content,
	void *cryptoContext; /**< crypto context used to encrypt file for RCS file transfer */
	mutable char *name;
    mutable char *type;
    mutable char *subtype;
    mutable char *body;
    mutable size_t size;
    mutable char *encoding;
    mutable char *key;
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
    if (content->type) ms_free(content->type);
    content->type = ms_strdup(L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(content)->getContentType().getType()));
    return content->type;
}

void linphone_content_set_type(LinphoneContent *content, const char *type) {
    LinphonePrivate::ContentType ct = L_GET_CPP_PTR_FROM_C_OBJECT(content)->getContentType();
    ct.setType(L_C_TO_STRING(type));
    L_GET_CPP_PTR_FROM_C_OBJECT(content)->setContentType(ct);
}

const char * linphone_content_get_subtype(const LinphoneContent *content) {
    if (content->subtype) ms_free(content->subtype);
    content->subtype = ms_strdup(L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(content)->getContentType().getSubType()));
    return content->subtype;
}

void linphone_content_set_subtype(LinphoneContent *content, const char *subtype) {
    LinphonePrivate::ContentType ct = L_GET_CPP_PTR_FROM_C_OBJECT(content)->getContentType();
    ct.setSubType(L_C_TO_STRING(subtype));
    L_GET_CPP_PTR_FROM_C_OBJECT(content)->setContentType(ct);
}

void linphone_content_add_content_type_parameter(LinphoneContent *content, const char *name, const char *value) {
    LinphonePrivate::ContentType ct = L_GET_CPP_PTR_FROM_C_OBJECT(content)->getContentType();
    ct.addParameter(L_C_TO_STRING(name), L_C_TO_STRING(value));
    L_GET_CPP_PTR_FROM_C_OBJECT(content)->setContentType(ct);
}

uint8_t * linphone_content_get_buffer(const LinphoneContent *content) {
    return (uint8_t *)linphone_content_get_string_buffer(content);
}

void linphone_content_set_buffer(LinphoneContent *content, const uint8_t *buffer, size_t size) {
    L_GET_CPP_PTR_FROM_C_OBJECT(content)->setBody(buffer, size);
}

const char * linphone_content_get_string_buffer(const LinphoneContent *content) {
    if (content->body) ms_free(content->body);
    content->body = ms_strdup(L_GET_CPP_PTR_FROM_C_OBJECT(content)->getBodyAsUtf8String().c_str());
    return content->body;
}

void linphone_content_set_string_buffer(LinphoneContent *content, const char *buffer) {
    L_GET_CPP_PTR_FROM_C_OBJECT(content)->setBodyFromUtf8(L_C_TO_STRING(buffer));
}

size_t linphone_content_get_size(const LinphoneContent *content) {
    size_t size = L_GET_CPP_PTR_FROM_C_OBJECT(content)->getSize();
    if (size == 0) {
        size = content->size;
    }
    return size;
}

void linphone_content_set_size(LinphoneContent *content, size_t size) {
    content->size = size;
}

const char * linphone_content_get_encoding(const LinphoneContent *content) {
	return content->encoding;
}

void linphone_content_set_encoding(LinphoneContent *content, const char *encoding) {
    if (content->encoding) ms_free(content->encoding);
	content->encoding = ms_strdup(encoding);
}

const char * linphone_content_get_name(const LinphoneContent *content) {
    const LinphonePrivate::Content *c = L_GET_CPP_PTR_FROM_C_OBJECT(content);
    if (c->isFile()) {
        const LinphonePrivate::FileContent *fc = static_cast<const LinphonePrivate::FileContent *>(c);
        if (content->name) ms_free(content->name);
        content->name = ms_strdup(L_STRING_TO_C(fc->getFileName()));
    } else if (c->isFileTransfer()) {
        const LinphonePrivate::FileTransferContent *ftc = static_cast<const LinphonePrivate::FileTransferContent *>(c);
        if (content->name) ms_free(content->name);
        content->name = ms_strdup(L_STRING_TO_C(ftc->getFileName()));
    }
    return content->name;
}

void linphone_content_set_name(LinphoneContent *content, const char *name) {
    if (content->name) ms_free(content->name);

    LinphonePrivate::Content *c = L_GET_CPP_PTR_FROM_C_OBJECT(content);
    if (c->isFile()) {
        LinphonePrivate::FileContent *fc = static_cast<LinphonePrivate::FileContent *>(c);
        fc->setFileName(L_C_TO_STRING(name));
    } else if (c->isFileTransfer()) {
        LinphonePrivate::FileTransferContent *ftc = static_cast<LinphonePrivate::FileTransferContent *>(c);
        ftc->setFileName(L_C_TO_STRING(name));
    }

    content->name = ms_strdup(name);
}

bool_t linphone_content_is_multipart(const LinphoneContent *content) {
    return L_GET_CPP_PTR_FROM_C_OBJECT(content)->getContentType().isMultipart();
}

LinphoneContent * linphone_content_get_part(const LinphoneContent *content, int idx) {
	SalBodyHandler *part_body_handler;
	SalBodyHandler *body_handler = sal_body_handler_from_content(content);
    if (!sal_body_handler_is_multipart(body_handler)) {
        sal_body_handler_unref(body_handler);
        return NULL;
    }
	part_body_handler = sal_body_handler_get_part(body_handler, idx);
	LinphoneContent *result = linphone_content_from_sal_body_handler(part_body_handler);
    sal_body_handler_unref(body_handler);
    return result;
}

LinphoneContent * linphone_content_find_part_by_header(const LinphoneContent *content, const char *header_name, const char *header_value) {
	SalBodyHandler *part_body_handler;
	SalBodyHandler *body_handler = sal_body_handler_from_content(content);
    if (!sal_body_handler_is_multipart(body_handler)) {
        sal_body_handler_unref(body_handler);
        return NULL;
    }
	part_body_handler = sal_body_handler_find_part_by_header(body_handler, header_name, header_value);
    LinphoneContent *result = linphone_content_from_sal_body_handler(part_body_handler);
    sal_body_handler_unref(body_handler);
    return result;
}

const char * linphone_content_get_custom_header(const LinphoneContent *content, const char *header_name) {
    SalBodyHandler *body_handler = sal_body_handler_from_content(content);
    const char *header = sal_body_handler_get_header(body_handler, header_name);
    sal_body_handler_unref(body_handler);
    return header;
}

const char *linphone_content_get_key(const LinphoneContent *content) {
    if (content->key) ms_free(content->key);

    const LinphonePrivate::Content *c = L_GET_CPP_PTR_FROM_C_OBJECT(content);
    if (c->isFileTransfer()) {
        const LinphonePrivate::FileTransferContent *ftc = static_cast<const LinphonePrivate::FileTransferContent *>(c);
        content->key = ms_strdup(ftc->getFileKeyAsString());
    }

    return content->key;
}

size_t linphone_content_get_key_size(const LinphoneContent *content) {
    const LinphonePrivate::Content *c = L_GET_CPP_PTR_FROM_C_OBJECT(content);
    if (c->isFileTransfer()) {
        const LinphonePrivate::FileTransferContent *ftc = static_cast<const LinphonePrivate::FileTransferContent *>(c);
        return ftc->getFileKeySize();
    }
    return 0;
}

void linphone_content_set_key(LinphoneContent *content, const char *key, const size_t keyLength) {
    LinphonePrivate::Content *c = L_GET_CPP_PTR_FROM_C_OBJECT(content);
    if (c->isFileTransfer()) {
        LinphonePrivate::FileTransferContent *ftc = static_cast<LinphonePrivate::FileTransferContent *>(c);
        ftc->setFileKey(key, keyLength);
    }
}

// =============================================================================
// Private functions.
// =============================================================================

static LinphoneContent * linphone_content_new_with_body_handler(SalBodyHandler *body_handler) {
	LinphoneContent *content = L_INIT(Content);
    content->cryptoContext = NULL;
    LinphonePrivate::Content *c = new LinphonePrivate::Content();
    L_SET_CPP_PTR_FROM_C_OBJECT(content, c);

    if (body_handler != NULL) {
        linphone_content_set_type(content, sal_body_handler_get_type(body_handler));
        linphone_content_set_subtype(content, sal_body_handler_get_subtype(body_handler));
        for (const belle_sip_list_t *params = sal_body_handler_get_content_type_parameters_names(body_handler); params; params = params->next) {
            const char *paramName = (const char *)(params->data);
            const char *paramValue = sal_body_handler_get_content_type_parameter(body_handler, paramName);
            linphone_content_add_content_type_parameter(content, paramName, paramValue);
        }

        if (!linphone_content_is_multipart(content)) {
            linphone_content_set_string_buffer(content, (char *)sal_body_handler_get_data(body_handler));
        } else {
            belle_sip_multipart_body_handler_t *mpbh = BELLE_SIP_MULTIPART_BODY_HANDLER(body_handler);
            char *body = belle_sip_object_to_string(mpbh);
            linphone_content_set_string_buffer(content, body);
            belle_sip_free(body);
        }

        belle_sip_list_t *headers = (belle_sip_list_t *)sal_body_handler_get_headers(body_handler);
        while (headers) {
            belle_sip_header_t *cHeader = BELLE_SIP_HEADER(headers->data);
            LinphonePrivate::Header header = LinphonePrivate::Header(belle_sip_header_get_name(cHeader), belle_sip_header_get_unparsed_value(cHeader));
            L_GET_CPP_PTR_FROM_C_OBJECT(content)->addHeader(header);
            headers = headers->next;
        }
        if (sal_body_handler_get_encoding(body_handler)) linphone_content_set_encoding(content, sal_body_handler_get_encoding(body_handler));
	}

    return content;
}

LinphoneContent * linphone_content_new(void) {
	return linphone_content_new_with_body_handler(NULL);
}

LinphoneContent * linphone_content_copy(const LinphoneContent *ref) {
	return (LinphoneContent *)(belle_sip_object_clone(BELLE_SIP_OBJECT(ref)));
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

SalBodyHandler * sal_body_handler_from_content(const LinphoneContent *content, bool parseMultipart) {
	if (content == NULL) return NULL;

    SalBodyHandler *body_handler;
    LinphonePrivate::ContentType contentType = L_GET_CPP_PTR_FROM_C_OBJECT(content)->getContentType();

    if (contentType.isMultipart() && parseMultipart) {
        size_t size = linphone_content_get_size(content);
        char *buffer = ms_strdup(L_GET_CPP_PTR_FROM_C_OBJECT(content)->getBodyAsUtf8String().c_str());
        const char *boundary = L_STRING_TO_C(contentType.getParameter("boundary").getValue());
        belle_sip_multipart_body_handler_t *bh = belle_sip_multipart_body_handler_new_from_buffer(buffer, size, boundary);
        body_handler = (SalBodyHandler *)BELLE_SIP_BODY_HANDLER(bh);
    } else {
        body_handler = sal_body_handler_new();
	    sal_body_handler_set_data(body_handler, belle_sip_strdup(linphone_content_get_string_buffer(content)));
    }

    for (const auto &header : L_GET_CPP_PTR_FROM_C_OBJECT(content)->getHeaders()) {
        belle_sip_header_t *additionalHeader = belle_sip_header_parse(header.asString().c_str());
        belle_sip_body_handler_add_header(BELLE_SIP_BODY_HANDLER(body_handler), additionalHeader);
    }

    sal_body_handler_set_type(body_handler, contentType.getType().c_str());
    sal_body_handler_set_subtype(body_handler, contentType.getSubType().c_str());
    sal_body_handler_set_size(body_handler, linphone_content_get_size(content));
    for (const auto &param : contentType.getParameters()) {
        sal_body_handler_set_content_type_parameter(body_handler, param.getName().c_str(), param.getValue().c_str());
    }
    if (content->encoding) sal_body_handler_set_encoding(body_handler, linphone_content_get_encoding(content));

	return body_handler;
}
