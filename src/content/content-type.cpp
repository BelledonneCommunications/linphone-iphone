/*
 * content-type.cpp
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

#include "linphone/utils/utils.h"

#include "content-type.h"
#include "object/clonable-object-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class ContentTypePrivate : public ClonableObjectPrivate {
public:
	string type;
	string subType;
	string parameter;
};

// -----------------------------------------------------------------------------

const ContentType ContentType::ConferenceInfo("application/conference-info+xml");
const ContentType ContentType::Cpim("message/cpim");
const ContentType ContentType::ExternalBody("message/external-body");
const ContentType ContentType::FileTransfer("application/vnd.gsma.rcs-ft-http+xml");
const ContentType ContentType::Imdn("message/imdn+xml");
const ContentType ContentType::ImIsComposing("application/im-iscomposing+xml");
const ContentType ContentType::Multipart("multipart/mixed");
const ContentType ContentType::PlainText("text/plain");
const ContentType ContentType::ResourceLists("application/resource-lists+xml");
const ContentType ContentType::Sdp("application/sdp");

// -----------------------------------------------------------------------------

ContentType::ContentType (const string &contentType) : ClonableObject(*new ContentTypePrivate) {
	L_D();

	size_t pos = contentType.find('/');
	size_t posParam = contentType.find(";");
	size_t end = contentType.length();
	if (pos == string::npos)
		return;

	if (setType(Utils::trim(contentType.substr(0, pos)))) {
		if (posParam != string::npos)
			end = posParam;
		if (!setSubType(Utils::trim(contentType.substr(pos + 1, end - (pos + 1)))))
			d->type.clear();
	}

	if (posParam != string::npos)
		setParameter(Utils::trim(contentType.substr(posParam + 1)));
}

ContentType::ContentType (const string &type, const string &subType) : ClonableObject(*new ContentTypePrivate) {
	L_D();

	if (setType(type) && !setSubType(subType))
		d->type.clear();
}

ContentType::ContentType (
	const string &type,
	const string &subType,
	const string &parameter
) : ClonableObject(*new ContentTypePrivate) {
	L_D();

	if (setType(type) && !setSubType(subType))
		d->type.clear();
	setParameter(parameter);
}

ContentType::ContentType (const ContentType &other) : ContentType(other.getType(), other.getSubType(), other.getParameter()) {}

ContentType &ContentType::operator= (const ContentType &other) {
	if (this != &other) {
		setType(other.getType());
		setSubType(other.getSubType());
		setParameter(other.getParameter());
	}

	return *this;
}

bool ContentType::weakEqual (const ContentType &other) const {
	return (getType() == other.getType()) && (getSubType() == other.getSubType());
}

bool ContentType::operator== (const ContentType &other) const {
	return weakEqual(other) && (getParameter() == other.getParameter());
}

bool ContentType::operator!= (const ContentType &other) const {
	return !(*this == other);
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

bool ContentType::isEmpty () const {
	L_D();
	return d->type.empty() && d->subType.empty();
}

bool ContentType::isValid () const {
	L_D();
	return !d->type.empty() && !d->subType.empty();
}

string ContentType::asString () const {
	L_D();
	if (isValid()) {
		string asString = d->type + "/" + d->subType;
		if (!d->parameter.empty())
			asString += "; " + d->parameter;
		return asString;
	}
	return "";
}


bool ContentType::isFile () const {
	// TODO Remove when not needed anymore in step 2.1 of maindb
	return isFile(*this);
}

bool ContentType::isFile (const ContentType &contentType) {
	// TODO Remove when not needed anymore in step 2.1 of maindb
	return contentType != FileTransfer &&
		contentType != PlainText &&
		contentType != ExternalBody &&
		contentType != Imdn &&
		contentType != ImIsComposing &&
		contentType != ResourceLists &&
		contentType != Sdp &&
		contentType != Cpim &&
		contentType != ConferenceInfo;
}

LINPHONE_END_NAMESPACE
