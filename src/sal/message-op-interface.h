/*
 * message-op-interface.h
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

#ifndef _L_SAL_MESSAGE_OP_INTERFACE_H_
#define _L_SAL_MESSAGE_OP_INTERFACE_H_

#include <ctime>

#include "content/content.h"
#include "content/content-type.h"

LINPHONE_BEGIN_NAMESPACE

class SalMessageOpInterface {
public:
	virtual ~SalMessageOpInterface() = default;

	virtual int sendMessage (const Content &content) = 0;
	virtual int reply (SalReason reason) = 0;

protected:
	void prepareMessageRequest (belle_sip_request_t *req, const Content &content) {
		time_t curtime = std::time(nullptr);
		belle_sip_message_add_header(
			BELLE_SIP_MESSAGE(req),
			BELLE_SIP_HEADER(belle_sip_header_date_create_from_time(&curtime))
		);
		std::string contentEncoding = content.getContentEncoding();
		if (!contentEncoding.empty())
			belle_sip_message_add_header(
				BELLE_SIP_MESSAGE(req),
				belle_sip_header_create("Content-Encoding", contentEncoding.c_str())
			);
		const ContentType &contentType = content.getContentType();
		std::string contentTypeStr = std::string(BELLE_SIP_CONTENT_TYPE ": ") + contentType.asString();
		belle_sip_message_add_header(
			BELLE_SIP_MESSAGE(req),
			BELLE_SIP_HEADER(belle_sip_header_content_type_parse(contentTypeStr.c_str()))
		);
		if (content.isEmpty()) {
			belle_sip_message_add_header(
				BELLE_SIP_MESSAGE(req),
				BELLE_SIP_HEADER(belle_sip_header_content_length_create(0))
			);
		} else {
			std::string body = content.getBodyAsUtf8String();
			size_t contentLength = body.size();
			belle_sip_message_add_header(
				BELLE_SIP_MESSAGE(req),
				BELLE_SIP_HEADER(belle_sip_header_content_length_create(contentLength))
			);
			belle_sip_message_set_body(BELLE_SIP_MESSAGE(req), body.c_str(), contentLength);
		}
	}

};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SAL_MESSAGE_OP_INTERFACE_H_
