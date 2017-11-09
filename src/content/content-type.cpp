/*
 * content-type.cpp
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

#include "linphone/utils/utils.h"

#include "object/clonable-object-p.h"
#include "logger/logger.h"
#include "content-type.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class ContentTypePrivate : public ClonableObjectPrivate {
public:
	string type;
	string subType;
	string parameter;
};

// -----------------------------------------------------------------------------

const ContentType ContentType::Cpim("message/cpim");
const ContentType ContentType::ExternalBody("message/external-body");
const ContentType ContentType::FileTransfer("application/vnd.gsma.rcs-ft-http+xml");
const ContentType ContentType::Imdn("message/imdn+xml");
const ContentType ContentType::ImIsComposing("application/im-iscomposing+xml");
const ContentType ContentType::PlainText("text/plain");
const ContentType ContentType::ResourceLists("application/resource-lists+xml");
const ContentType ContentType::Sdp("application/sdp");
const ContentType ContentType::ConferenceInfo("application/conference-info+xml");

// -----------------------------------------------------------------------------

ContentType::ContentType (const string &contentType) : ClonableObject(*new ContentTypePrivate) {
	L_D();

	size_t pos = contentType.find('/');
	size_t posParam = contentType.find("; ");
	size_t end = contentType.length();
	if (pos == string::npos)
		return;

	if (setType(contentType.substr(0, pos))) {
		if (posParam != string::npos) {
			end = posParam;
		}
		if (!setSubType(contentType.substr(pos + 1, end - (pos + 1))))
			d->type.clear();
	}

	if (posParam != string::npos) {
		setParameter(contentType.substr(posParam + 2)); // We remove the blankspace after the ;
	}
}

ContentType::ContentType (const string &type, const string &subType) : ClonableObject(*new ContentTypePrivate) {
	L_D();

	if (setType(type)) {
		if (!setSubType(subType))
			d->type.clear();
	}
}

ContentType::ContentType (const string &type, const string &subType, const string &parameter) : ClonableObject(*new ContentTypePrivate) {
	L_D();

	if (setType(type)) {
		if (!setSubType(subType))
			d->type.clear();
	}
	setParameter(parameter);
}

ContentType::ContentType (const ContentType &src) : ContentType(src.getType(), src.getSubType(), src.getParameter()) {}

ContentType &ContentType::operator= (const ContentType &src) {
	if (this != &src) {
		setType(src.getType());
		setSubType(src.getSubType());
		setParameter(src.getParameter());
	}

	return *this;
}

bool ContentType::operator== (const ContentType &contentType) const {
	return getType() == contentType.getType() && getSubType() == contentType.getSubType() && getParameter() == contentType.getParameter();
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

const string &ContentType::getParameter () const {
	L_D();
	return d->parameter;
}

void ContentType::setParameter (const string &parameter) {
	L_D();
	d->parameter = parameter;
}

bool ContentType::isValid () const {
	L_D();
	return !d->type.empty() && !d->subType.empty();
}

bool ContentType::isFile() const {
	//TODO Improve
	if (*this != ContentType::FileTransfer && *this != ContentType::PlainText &&
		*this != ContentType::ExternalBody && *this != ContentType::Imdn &&
		*this != ContentType::ImIsComposing && *this != ContentType::ResourceLists &&
		*this != ContentType::Sdp && *this != ContentType::Cpim &&
		*this != ContentType::ConferenceInfo) {
			return true;
	}
	return false;
}

string ContentType::asString () const {
	L_D();
	if (isValid()) {
		string asString = d->type + "/" + d->subType;
		if (!d->parameter.empty()) {
			asString += "; " + d->parameter;
		}
		return asString;
	}
	return "";
}

LINPHONE_END_NAMESPACE
