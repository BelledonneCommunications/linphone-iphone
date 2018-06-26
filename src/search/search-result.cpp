/*
 * search-result.cpp
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

#include "linphone/api/c-address.h"

#include "object/clonable-object-p.h"
#include "search-result.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class SearchResultPrivate : public ClonableObjectPrivate {
private:
	const LinphoneFriend *mFriend;
	const LinphoneAddress *mAddress;
	std::string mPhoneNumber;
	unsigned int mWeight;

	L_DECLARE_PUBLIC(SearchResult);
};

using namespace std;

SearchResult::SearchResult(const unsigned int weight, const LinphoneAddress *a, const string &pn, const LinphoneFriend *f) : ClonableObject(*new SearchResultPrivate) {
	L_D();
	d->mWeight = weight;
	d->mAddress = a;
	if (d->mAddress) linphone_address_ref(const_cast<LinphoneAddress *>(d->mAddress));
	d->mFriend = f;
	d->mPhoneNumber = pn;
}

SearchResult::SearchResult(const SearchResult &sr) : ClonableObject(*new SearchResultPrivate) {
	L_D();
	d->mWeight = sr.getWeight();
	d->mAddress = sr.getAddress();
	if (d->mAddress) linphone_address_ref(const_cast<LinphoneAddress *>(d->mAddress));
	d->mFriend = sr.getFriend();
	d->mPhoneNumber = sr.getPhoneNumber();
}

SearchResult::~SearchResult() {
	L_D();
	// FIXME: Ugly temporary workaround to solve weak. Remove me later.
	if (d->mAddress) linphone_address_unref(const_cast<LinphoneAddress *>(d->mAddress));
};

bool SearchResult::operator<(const SearchResult &other) const {
	return getWeight() < other.getWeight();
}

bool SearchResult::operator>(const SearchResult &other) const {
	return getWeight() > other.getWeight();
}

bool SearchResult::operator>=(const SearchResult &other) const {
	return getWeight() >= other.getWeight();
}

bool SearchResult::operator=(const SearchResult &other) const {
	return getWeight() == other.getWeight();
}

const LinphoneFriend *SearchResult::getFriend() const {
	L_D();
	return d->mFriend;
}

const LinphoneAddress *SearchResult::getAddress() const {
	L_D();
	return d->mAddress;
}

const string &SearchResult::getPhoneNumber() const {
	L_D();
	return d->mPhoneNumber;
}

unsigned int SearchResult::getWeight() const {
	L_D();
	return d->mWeight;
}

LINPHONE_END_NAMESPACE
