/*
 * content.cpp
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

// TODO: Remove me later.
#include "linphone/core.h"

#include "content-p.h"
#include "content-type.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

Content::Content () : ClonableObject(*new ContentPrivate) {}

Content::Content (const Content &src) : ClonableObject(*new ContentPrivate), AppDataContainer(src) {
	L_D();
	d->body = src.getBody();
	d->contentType = src.getContentType();
	d->contentDisposition = src.getContentDisposition();
}

Content::Content (Content &&src) : ClonableObject(*new ContentPrivate), AppDataContainer(move(src)) {
	L_D();
	d->body = move(src.getPrivate()->body);
	d->contentType = move(src.getPrivate()->contentType);
	d->contentDisposition = move(src.getPrivate()->contentDisposition);
}

Content::Content (ContentPrivate &p) : ClonableObject(p) {}

Content::~Content () {
	L_D();
	/*
	 * Fills the body with zeros before releasing since it may contain
	 * private data like cipher keys or decoded messages.
	 */
	d->body.assign(d->body.size(), 0);
}

Content &Content::operator= (const Content &src) {
	L_D();
	if (this != &src) {
		d->body = src.getBody();
		d->contentType = src.getContentType();
		d->contentDisposition = src.getContentDisposition();
		AppDataContainer::operator=(src);
	}

	return *this;
}

Content &Content::operator= (Content &&src) {
	L_D();
	d->body = move(src.getPrivate()->body);
	d->contentType = move(src.getPrivate()->contentType);
	d->contentDisposition = move(src.getPrivate()->contentDisposition);
	AppDataContainer::operator=(move(src));
	return *this;
}

bool Content::operator== (const Content &content) const {
	L_D();
	return d->contentType == content.getContentType() &&
		d->body == content.getBody() &&
		d->contentDisposition == content.getContentDisposition();
}

const ContentType &Content::getContentType () const {
	L_D();
	return d->contentType;
}

void Content::setContentType (const ContentType &contentType) {
	L_D();
	d->contentType = contentType;
}

void Content::setContentType (const string &contentType) {
	L_D();
	d->contentType = ContentType(contentType);
}

const string &Content::getContentDisposition () const {
	L_D();
	return d->contentDisposition;
}

void Content::setContentDisposition (const string &contentDisposition) {
	L_D();
	d->contentDisposition = contentDisposition;
}

const vector<char> &Content::getBody () const {
	L_D();
	return d->body;
}

string Content::getBodyAsString () const {
	L_D();
	return string(d->body.begin(), d->body.end());
}

void Content::setBody (const vector<char> &body) {
	L_D();
	d->body = body;
}

void Content::setBody (vector<char> &&body) {
	L_D();
	d->body = move(body);
}

void Content::setBody (const string &body) {
	L_D();
	d->body = vector<char>(body.cbegin(), body.cend());
}

void Content::setBody (const void *buffer, size_t size) {
	L_D();
	const char *start = static_cast<const char *>(buffer);
	d->body = vector<char>(start, start + size);
}

size_t Content::getSize () const {
	L_D();
	return d->body.size();
}

bool Content::isEmpty () const {
	return getSize() == 0;
}

bool Content::isValid () const {
	L_D();
	return d->contentType.isValid() || (d->contentType.isEmpty() && d->body.empty());
}

LinphoneContent *Content::toLinphoneContent () const {
	LinphoneContent *content = linphone_core_create_content(nullptr);
	linphone_content_set_type(content, getContentType().getType().c_str());
	linphone_content_set_subtype(content, getContentType().getSubType().c_str());
	return content;
}

LINPHONE_END_NAMESPACE
