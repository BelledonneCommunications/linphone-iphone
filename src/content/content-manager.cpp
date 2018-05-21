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

#include "c-wrapper/c-wrapper.h"

#include "linphone/api/c-content.h"

#include "content-disposition.h"
#include "content-manager.h"
#include "content-type.h"
#include "content/content.h"
#include "content/header/header-param.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

list<Content> ContentManager::multipartToContentList (const Content &content) {
	LinphoneContent *cContent = L_GET_C_BACK_PTR(&content);
	SalBodyHandler *sbh = sal_body_handler_from_content(cContent);

	list<Content> contents;
	for (const belle_sip_list_t *parts = sal_body_handler_get_parts(sbh); parts; parts = parts->next) {
		SalBodyHandler *part = (SalBodyHandler *)parts->data;
		LinphoneContent *cContent = linphone_content_from_sal_body_handler(part, false);
		Content *cppContent = L_GET_CPP_PTR_FROM_C_OBJECT(cContent);
		if (content.getContentDisposition().isValid())
			cppContent->setContentDisposition(content.getContentDisposition());
		contents.push_back(*cppContent);
		linphone_content_unref(cContent);
	}

	sal_body_handler_unref(sbh);
	return contents;
}

Content ContentManager::contentListToMultipart (const list<Content *> &contents, const string &boundary) {
	belle_sip_multipart_body_handler_t *mpbh = belle_sip_multipart_body_handler_new(
		nullptr, nullptr, nullptr, boundary.c_str()
	);
	mpbh = (belle_sip_multipart_body_handler_t *)belle_sip_object_ref(mpbh);

	ContentDisposition disposition;
	for (Content *content : contents) {
		// Is this content-disposition stuff generic or only valid for notification content-disposition?
		if (content->getContentDisposition().isValid())
			disposition = content->getContentDisposition();

		LinphoneContent *cContent = L_GET_C_BACK_PTR(content);
		SalBodyHandler *sbh = sal_body_handler_from_content(cContent, false);
		belle_sip_multipart_body_handler_add_part(mpbh, BELLE_SIP_BODY_HANDLER(sbh));
	}

	SalBodyHandler *sbh = (SalBodyHandler *)mpbh;
	sal_body_handler_set_type(sbh, ContentType::Multipart.getType().c_str());
	sal_body_handler_set_subtype(sbh, ContentType::Multipart.getSubType().c_str());
	sal_body_handler_set_content_type_parameter(sbh, "boundary", boundary.c_str());

	LinphoneContent *cContent = linphone_content_from_sal_body_handler(sbh);
	belle_sip_object_unref(mpbh);

	Content content = *L_GET_CPP_PTR_FROM_C_OBJECT(cContent);
	if (disposition.isValid())
		content.setContentDisposition(disposition);
	linphone_content_unref(cContent);
	return content;
}

LINPHONE_END_NAMESPACE
