/*
 * content-type.cpp
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

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class ContentTypePrivate : public ClonableObjectPrivate {
public:
	string type;
	string subType;
};

// -----------------------------------------------------------------------------

ContentType::ContentType (const string &contentType) : ClonableObject(*new ContentTypePrivate) {
	L_D();

	size_t pos = contentType.find('/');
	if (pos == string::npos)
		return;

	if (setType(contentType.substr(0, pos))) {
		if (!setSubType(contentType.substr(pos + 1)))
			d->type.clear();
	}
}

ContentType::ContentType (const string &type, const string &subType) : ClonableObject(*new ContentTypePrivate) {
	L_D();

	if (setType(type)) {
		if (!setSubType(subType))
			d->type.clear();
	}
}

ContentType::ContentType (const ContentType &src) : ContentType(src.getType(), src.getSubType()) {}

ContentType &ContentType::operator= (const ContentType &src) {
	if (this != &src) {
		setType(src.getType());
		setSubType(src.getSubType());
	}

	return *this;
}

bool ContentType::operator== (const ContentType &contentType) {
	return getType() == contentType.getType() && getSubType() == contentType.getSubType();
}

bool ContentType::operator== (const string &contentType) {
	return *this == ContentType(contentType);
}

const string &ContentType::getType () const {
	L_D();
	return d->type;
}

bool ContentType::setType (const string &type) {
	L_D();
	if (type.find('/') == string::npos) {
		d->type = type;
		return true;
	}
	return false;
}

const string &ContentType::getSubType () const {
	L_D();
	return d->subType;
}

bool ContentType::setSubType (const string &subType) {
	L_D();
	if (subType.find('/') == string::npos) {
		d->subType = subType;
		return true;
	}
	return false;
}

bool ContentType::isValid () const {
	L_D();
	return !d->type.empty() && !d->subType.empty();
}

string ContentType::asString () const {
	L_D();
	return isValid() ? d->type + "/" + d->subType : "";
}

bool ContentType::isFileTransfer () const {
	return isFileTransfer(asString());
}

bool ContentType::isImIsComposing () const {
	return isFileTransfer(asString());
}

bool ContentType::isImdn () const {
	return isImdn(asString());
}

bool ContentType::isText () const {
	return isText(asString());
}

bool ContentType::isFileTransfer (const string &contentType) {
	return contentType == "application/vnd.gsma.rcs-ft-http+xml";
}

bool ContentType::isImIsComposing (const string &contentType) {
	return contentType == "application/im-iscomposing+xml";
}

bool ContentType::isImdn (const string &contentType) {
	return contentType == "message/imdn+xml";
}

bool ContentType::isText (const string &contentType) {
	return contentType == "text/plain";
}

LINPHONE_END_NAMESPACE
