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
};

// -----------------------------------------------------------------------------

const ContentDisposition ContentDisposition::RecipientList("recipient-list");

// -----------------------------------------------------------------------------

ContentDisposition::ContentDisposition (const string &disposition) : ClonableObject(*new ContentDispositionPrivate) {
	L_D();
	d->disposition = disposition;
}

ContentDisposition::ContentDisposition (const ContentDisposition &other)
	: ContentDisposition(other.getPrivate()->disposition) {}

ContentDisposition &ContentDisposition::operator= (const ContentDisposition &other) {
	L_D();
	if (this != &other) {
		d->disposition = other.getPrivate()->disposition;
	}
	return *this;
}

bool ContentDisposition::operator== (const ContentDisposition &other) const {
	L_D();
	return d->disposition == other.getPrivate()->disposition;
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

string ContentDisposition::asString () const {
	L_D();
	if (isValid())
		return d->disposition;
	return "";
}

LINPHONE_END_NAMESPACE
