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

// From coreapi.
#include "private.h"

#include "c-wrapper/c-tools.h"
#include "object/object-p.h"

#include "content.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class ContentPrivate : public ObjectPrivate {
public:
	struct Cache {
		string type;
		string subType;
		string customHeaderValue;
		string encoding;
	};

	SalBodyHandler *bodyHandler = nullptr;
	void *cryptoContext = nullptr;
	string name;
	string key;

	mutable Cache cache;
};

// -----------------------------------------------------------------------------

Content::Content () : Object(*new ContentPrivate) {}

const string &Content::getType () const {
	L_D(const Content);
	d->cache.type = sal_body_handler_get_subtype(d->bodyHandler);
	return d->cache.type;
}

void Content::setType (const string &type) {
	L_D(Content);
	sal_body_handler_set_type(d->bodyHandler, L_STRING_TO_C(type));
}

const string &Content::getSubType () const {
	L_D(const Content);
	d->cache.subType = sal_body_handler_get_subtype(d->bodyHandler);
	return d->cache.subType;
}

void Content::setSubType (const string &subType) {
	L_D(Content);
	sal_body_handler_set_subtype(d->bodyHandler, L_STRING_TO_C(subType));
}

const void *Content::getBuffer () const {
	L_D(const Content);
	return sal_body_handler_get_data(d->bodyHandler);
}

void Content::setBuffer (const void *buffer, size_t size) {
	L_D(Content);
	sal_body_handler_set_size(d->bodyHandler, size);
	void *data = belle_sip_malloc(size);
	sal_body_handler_set_data(d->bodyHandler, memcpy(data, buffer, size));
}

size_t Content::getSize () const {
	L_D(const Content);
	return sal_body_handler_get_size(d->bodyHandler);
}

void Content::setSize (size_t size) {
	L_D(Content);
	sal_body_handler_set_data(d->bodyHandler, nullptr);
	sal_body_handler_set_size(d->bodyHandler, size);
}

const string &Content::getEncoding () const {
	L_D(const Content);
	d->cache.encoding = sal_body_handler_get_encoding(d->bodyHandler);
	return d->cache.encoding;
}

void Content::setEncoding (const string &encoding) {
	L_D(Content);
	sal_body_handler_set_encoding(d->bodyHandler, L_STRING_TO_C(encoding));
}

const string &Content::getName () const {
	L_D(const Content);
	return d->name;
}

void Content::setName (const string &name) {
	L_D(Content);
	d->name = name;
}

bool Content::isMultipart () const {
	L_D(const Content);
	return sal_body_handler_is_multipart(d->bodyHandler);
}

shared_ptr<Content> Content::getPart (int index) const {
	L_D(const Content);

	if (!isMultipart())
	  return nullptr;

	SalBodyHandler *bodyHandler = sal_body_handler_get_part(d->bodyHandler, index);
	if (!bodyHandler)
	  return nullptr;

	Content *content = new Content();
	sal_body_handler_ref(bodyHandler);
	content->getPrivate()->bodyHandler = bodyHandler;
	return shared_ptr<Content>(content);
}

shared_ptr<Content> Content::findPartByHeader (const string &headerName, const string &headerValue) const {
	L_D(const Content);

	if (!isMultipart())
		return nullptr;

  SalBodyHandler *bodyHandler = sal_body_handler_find_part_by_header(
		d->bodyHandler,
		L_STRING_TO_C(headerName),
		L_STRING_TO_C(headerValue)
	);
	if (!bodyHandler)
	  return nullptr;

	Content *content = new Content();
	sal_body_handler_ref(bodyHandler);
	content->getPrivate()->bodyHandler = bodyHandler;
	return shared_ptr<Content>(content);
}

const string &Content::getCustomHeaderValue (const string &headerName) const {
	L_D(const Content);
	d->cache.customHeaderValue = sal_body_handler_get_header(d->bodyHandler, L_STRING_TO_C(headerName));
	return d->cache.customHeaderValue;
}

const string &Content::getKey () const {
	L_D(const Content);
	return d->key;
}

void Content::setKey (const string &key) {
	L_D(Content);
	d->key = key;
}

LINPHONE_END_NAMESPACE
