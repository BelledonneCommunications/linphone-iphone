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

#include "search-result-p.h"
#include "linphone/utils/utils.h"

using namespace LinphonePrivate;

LINPHONE_BEGIN_NAMESPACE

SearchResult::SearchResult(const unsigned int weight, const LinphoneAddress *a, const LinphoneFriend *f) : ClonableObject(*new SearchResultPrivate) {
	L_D();
	d->mWeight = weight;
	d->mAddress = a;
	d->mFriend = f;
}

SearchResult::SearchResult(const SearchResult &sr) : ClonableObject(*new SearchResultPrivate) {
	L_D();
	d->mWeight = sr.getWeight();
	d->mAddress = sr.getAddress();
	d->mFriend = sr.getFriend();
}

SearchResult::~SearchResult() {};

bool SearchResult::operator<(const SearchResult& rsr) const{
	return this->getWeight() < rsr.getWeight();
}

bool SearchResult::operator>(const SearchResult& rsr) const{
	return this->getWeight() > rsr.getWeight();
}

bool SearchResult::operator>=(const SearchResult& rsr) const{
	return this->getWeight() >= rsr.getWeight();
}

bool SearchResult::operator=(const SearchResult& rsr) const{
	return this->getWeight() == rsr.getWeight();
}

const LinphoneFriend* SearchResult::getFriend() const {
	L_D();
	return d->mFriend;
}

const LinphoneAddress *SearchResult::getAddress() const {
	L_D();
	return d->mAddress;
}

unsigned int SearchResult::getWeight() const {
	L_D();
	return d->mWeight;
}

LINPHONE_END_NAMESPACE
