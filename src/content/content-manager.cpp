/*
 * content-manager.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#include <sstream>

#include "belle-sip/belle-sip.h"

#include "content-manager.h"
#include "content-type.h"
#include "linphone/content.h"
#include "logger/logger.h"

// =============================================================================

#define MULTIPART_BOUNDARY "---------------------------14737809831466499882746641449"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

list<Content> ContentManager::multipartToContentLists (const Content &content) const {
	belle_sip_multipart_body_handler_t *mpbh = belle_sip_multipart_body_handler_new_from_buffer((void *)content.getBodyAsString().c_str(), content.getBodyAsString().length(), MULTIPART_BOUNDARY);
	belle_sip_object_ref(mpbh);

	const belle_sip_list_t *parts = belle_sip_multipart_body_handler_get_parts(mpbh);
	list<Content> contentsList = list<Content>();
	while (parts) {
		belle_sip_body_handler_t *part = BELLE_SIP_BODY_HANDLER(parts->data);
		const belle_sip_list_t *part_headers = belle_sip_body_handler_get_headers(part);
		belle_sip_list_t *it;
		belle_sip_header_content_type_t *part_content_type=NULL;;
		for(it = (belle_sip_list_t *)part_headers;it!=NULL;it=it->next) {
			belle_sip_header_t *header = BELLE_SIP_HEADER(it->data);
			if(strcasecmp("Content-Type",belle_sip_header_get_name(header)) == 0) {
				part_content_type=BELLE_SIP_HEADER_CONTENT_TYPE(header);
				break;
			}
		}
		belle_sip_header_content_type_get_type(part_content_type);
		belle_sip_memory_body_handler_get_buffer(BELLE_SIP_MEMORY_BODY_HANDLER(part));
		Content retContent = Content();
		ContentType type(belle_sip_header_content_type_get_type(part_content_type), belle_sip_header_content_type_get_subtype(part_content_type));
		retContent.setBody((const char *)belle_sip_memory_body_handler_get_buffer(BELLE_SIP_MEMORY_BODY_HANDLER(part)));
		retContent.setContentType(type);
		contentsList.push_back(retContent);
		parts = parts->next;
	}

	belle_sip_object_unref(mpbh);
	return contentsList;
}

Content ContentManager::contentsListToMultipart (const list<Content> &contents) const {
	char *desc;
	string sub;
	belle_sip_memory_body_handler_t *mbh = NULL;
	belle_sip_multipart_body_handler_t *mpbh = belle_sip_multipart_body_handler_new(NULL, NULL, NULL, MULTIPART_BOUNDARY);
	belle_sip_object_ref(mpbh);
	for (const auto &content : contents) {
		const ContentType &contentType = content.getContentType();
		stringstream subtype;
		sub = contentType.getSubType();
		subtype << sub << "; charset=\"UTF-8\"";
		belle_sip_header_t *content_type = BELLE_SIP_HEADER(
			belle_sip_header_content_type_create(
				contentType.getType().c_str(),
				subtype.str().c_str()
			)
		);
		mbh = belle_sip_memory_body_handler_new_copy_from_buffer(
			(void *)content.getBodyAsString().c_str(),
			content.getBodyAsString().length(),
			NULL,
			NULL
		);
		belle_sip_body_handler_add_header(BELLE_SIP_BODY_HANDLER(mbh), content_type);
		belle_sip_multipart_body_handler_add_part(mpbh, BELLE_SIP_BODY_HANDLER(mbh));
	}
	desc = belle_sip_object_to_string(mpbh);
	belle_sip_object_unref(mpbh);

	Content retContent = Content();
	ContentType type("application", sub);
	retContent.setBody(desc);
	retContent.setContentType(type);
	return retContent;
}

LINPHONE_END_NAMESPACE
