/*
 * content-disposition.cpp
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

#include "content-disposition.h"
#include "object/clonable-object-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class ContentDispositionPrivate : public ClonableObjectPrivate {
public:
	string disposition;
	string parameter;
};

// -----------------------------------------------------------------------------

const ContentDisposition ContentDisposition::Notification("notification");
const ContentDisposition ContentDisposition::RecipientList("recipient-list");
const ContentDisposition ContentDisposition::RecipientListHistory("recipient-list-history; handling=optional");

// -----------------------------------------------------------------------------

ContentDisposition::ContentDisposition (const string &disposition) : ClonableObject(*new ContentDispositionPrivate) {
	L_D();
	size_t posParam = disposition.find(";");
	d->disposition = Utils::trim(disposition.substr(0, posParam));
	if (posParam != string::npos)
		setParameter(Utils::trim(disposition.substr(posParam + 1)));
}

ContentDisposition::ContentDisposition (const ContentDisposition &other)
	: ContentDisposition(other.asString()) {}

ContentDisposition &ContentDisposition::operator= (const ContentDisposition &other) {
	L_D();
	if (this != &other) {
		d->disposition = other.getPrivate()->disposition;
		setParameter(other.getParameter());
	}
	return *this;
}

bool ContentDisposition::weakEqual (const ContentDisposition &other) const {
	L_D();
	return d->disposition == other.getPrivate()->disposition;
}

bool ContentDisposition::operator== (const ContentDisposition &other) const {
	return weakEqual(other) && (getParameter() == other.getParameter());
}

bool ContentDisposition::operator!= (const ContentDisposition &other) const {
	return !(*this == other);
}

bool ContentDisposition::isEmpty () const {
	L_D();
	return d->disposition.empty();
}

bool ContentDisposition::isValid () const {
	L_D();
	return !d->disposition.empty();
}

const string &ContentDisposition::getParameter () const {
	L_D();
	return d->parameter;
}

void ContentDisposition::setParameter (const string &parameter) {
	L_D();
	d->parameter = parameter;
}

string ContentDisposition::asString () const {
	L_D();
	if (isValid()) {
		string asString = d->disposition;
		if (!d->parameter.empty())
			asString += ";" + d->parameter;
		return asString;
	}
	return "";
}

LINPHONE_END_NAMESPACE
