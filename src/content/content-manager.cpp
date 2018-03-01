/*
 * content-manager.cpp
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

#include <belle-sip/belle-sip.h>

#include "content-manager.h"
#include "content-type.h"
#include "content/content.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

namespace {
	constexpr const char MultipartBoundary[] = "---------------------------14737809831466499882746641449";
}

// -----------------------------------------------------------------------------

list<Content> ContentManager::multipartToContentList (const Content &content) {
	const string body = content.getBodyAsString();
	belle_sip_multipart_body_handler_t *mpbh = belle_sip_multipart_body_handler_new_from_buffer(
		body.c_str(), body.length(), MultipartBoundary
	);
	belle_sip_object_ref(mpbh);

	list<Content> contents;
	for (const belle_sip_list_t *parts = belle_sip_multipart_body_handler_get_parts(mpbh); parts; parts = parts->next) {
		belle_sip_body_handler_t *part = BELLE_SIP_BODY_HANDLER(parts->data);
		belle_sip_header_content_type_t *partContentType = nullptr;
		for (const belle_sip_list_t *it = belle_sip_body_handler_get_headers(part); it; it = it->next) {
			belle_sip_header_t *header = BELLE_SIP_HEADER(it->data);
			if (strcasecmp("Content-Type", belle_sip_header_get_name(header)) == 0) {
				partContentType = BELLE_SIP_HEADER_CONTENT_TYPE(header);
				break;
			}
		}

		Content content;
		content.setBody(static_cast<const char *>(
			belle_sip_memory_body_handler_get_buffer(BELLE_SIP_MEMORY_BODY_HANDLER(part))
		));
		content.setContentType(ContentType(
			belle_sip_header_content_type_get_type(partContentType),
			belle_sip_header_content_type_get_subtype(partContentType)
		));
		contents.push_back(move(content));
	}

	belle_sip_object_unref(mpbh);
	return contents;
}

Content ContentManager::contentListToMultipart (const list<Content> &contents) {
	belle_sip_multipart_body_handler_t *mpbh = belle_sip_multipart_body_handler_new(
		nullptr, nullptr, nullptr, MultipartBoundary
	);
	belle_sip_object_ref(mpbh);

	for (const auto &content : contents) {
		const ContentType &contentType = content.getContentType();
		belle_sip_header_t *cContentType = BELLE_SIP_HEADER(
			belle_sip_header_content_type_create(
				contentType.getType().c_str(),
				string(contentType.getSubType() + "; charset=\"UTF-8\"").c_str()
			)
		);

		const string body = content.getBodyAsString();
		belle_sip_memory_body_handler_t *mbh = belle_sip_memory_body_handler_new_copy_from_buffer(
			body.c_str(), body.length(), nullptr, nullptr
		);
		belle_sip_body_handler_add_header(BELLE_SIP_BODY_HANDLER(mbh), cContentType);

		for (const auto &header : content.getHeaders()) {
			belle_sip_header_t *additionalHeader = BELLE_SIP_HEADER(
				belle_sip_header_create(
					header.first.c_str(),
					header.second.c_str()
				)
			);
			belle_sip_body_handler_add_header(BELLE_SIP_BODY_HANDLER(mbh), additionalHeader);
		}

		belle_sip_multipart_body_handler_add_part(mpbh, BELLE_SIP_BODY_HANDLER(mbh));
	}

	char *desc = belle_sip_object_to_string(mpbh);
	Content content;
	content.setBody(desc);
	belle_sip_free(desc);
	belle_sip_object_unref(mpbh);

	ContentType contentType = ContentType::Multipart;
	contentType.setParameter("boundary=" + string(MultipartBoundary));
	content.setContentType(contentType);

	return content;
}

LINPHONE_END_NAMESPACE
