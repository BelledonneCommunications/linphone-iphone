/*
 * content.cpp
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "content-type.h"
#include "object/clonable-object-p.h"

#include "content.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class ContentPrivate : public ClonableObjectPrivate {
public:
	vector<char> body;
	ContentType contentType;
	string contentDisposition;
};

// -----------------------------------------------------------------------------

Content::Content () : ClonableObject(*new ContentPrivate) {}

Content::Content (const Content &src) : ClonableObject(*new ContentPrivate) {
	L_D();
	d->body = src.getBody();
	d->contentType = src.getContentType();
}

Content::Content (Content &&src) : ClonableObject(*new ContentPrivate) {
	L_D();
	d->body = move(src.getPrivate()->body);
	d->contentType = move(src.getPrivate()->contentType);
}

Content &Content::operator= (const Content &src) {
	L_D();
	if (this != &src) {
		d->body = src.getBody();
		d->contentType = src.getContentType();
	}

	return *this;
}

Content &Content::operator= (Content &&src) {
	L_D();
	d->body = move(src.getPrivate()->body);
	d->contentType = move(src.getPrivate()->contentType);
	return *this;
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

LINPHONE_END_NAMESPACE
