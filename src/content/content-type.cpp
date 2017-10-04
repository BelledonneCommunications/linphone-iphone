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

#include "linphone/utils/utils.h"

#include "object/clonable-object-p.h"

#include "content-type.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class ContentTypePrivate : public ClonableObjectPrivate {
public:
	string type;
	string subType;
};

// -----------------------------------------------------------------------------

const ContentType ContentType::Cpim("message/cpim");
const ContentType ContentType::FileTransfer("application/vnd.gsma.rcs-ft-http+xml");
const ContentType ContentType::Imdn("message/imdn+xml");
const ContentType ContentType::ImIsComposing("application/im-iscomposing+xml");
const ContentType ContentType::PlainText("text/plain");
const ContentType ContentType::ResourceLists("application/resource-lists+xml");
const ContentType ContentType::Sdp("application/sdp");

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

bool ContentType::operator== (const ContentType &contentType) const {
	return getType() == contentType.getType() && getSubType() == contentType.getSubType();
}

bool ContentType::operator!= (const ContentType &contentType) const {
	return !operator==(contentType);
}

const string &ContentType::getType () const {
	L_D();
	return d->type;
}

bool ContentType::setType (const string &type) {
	L_D();
	if (type.find('/') == string::npos) {
		d->type = Utils::stringToLower(type);
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
		d->subType = Utils::stringToLower(subType);
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

LINPHONE_END_NAMESPACE
