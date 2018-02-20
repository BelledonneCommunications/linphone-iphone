/*
 * magic-search.cpp
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

#include "magic-search-p.h"

#include <bctoolbox/list.h>

#include "c-wrapper/internal/c-tools.h"
#include "linphone/utils/utils.h"
#include "linphone/core.h"
#include "linphone/types.h"
#include "private.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

MagicSearch::MagicSearch(const std::shared_ptr<Core> &core) : CoreAccessor(core), Object(*new MagicSearchPrivate){
	L_D();
	d->mMinWeight = 0;
	d->mMaxWeight = 1000;
	d->mSearchLimit = 30;
	d->mLimitedSearch = true;
	d->mDelimiter = "+_-";
	d->mUseDelimiter = true;
	d->mCacheResult = nullptr;
}

MagicSearch::~MagicSearch() {
	resetSearchCache();
}

void MagicSearch::setMinWeight(const unsigned int weight) {
	L_D();
	d->mMinWeight = weight;
}

unsigned int MagicSearch::getMinWeight() const {
	L_D();
	return d->mMinWeight;
}

void MagicSearch::setMaxWeight(const unsigned int weight) {
	L_D();
	d->mMaxWeight = weight;
}

unsigned int MagicSearch::getMaxWeight() const {
	L_D();
	return d->mMaxWeight;
}

const string &MagicSearch::getDelimiter() const {
	L_D();
	return d->mDelimiter;
}

void MagicSearch::setDelimiter(const string &delimiter) {
	L_D();
	d->mDelimiter = delimiter;
}

bool MagicSearch::getUseDelimiter() const {
	L_D();
	return d->mUseDelimiter;
}

void MagicSearch::setUseDelimiter(bool enable) {
	L_D();
	d->mUseDelimiter = enable;
}

unsigned int MagicSearch::getSearchLimit() const {
	L_D();
	return d->mSearchLimit;
}

void MagicSearch::setSearchLimit(const unsigned int limit) {
	L_D();
	d->mSearchLimit = limit;
}

bool MagicSearch::getLimitedSearch() const {
	L_D();
	return d->mLimitedSearch;
}

void MagicSearch::setLimitedSearch(const bool limited) {
	L_D();
	d->mLimitedSearch = limited;
}

void MagicSearch::resetSearchCache() {
	L_D();
	if (d->mCacheResult) {
		delete d->mCacheResult;
		d->mCacheResult = nullptr;
	}
}

list<SearchResult> MagicSearch::getContactListFromFilter(const string &filter, const string &withDomain) {
	list<SearchResult> *resultList;

	if (filter.empty()) return list<SearchResult>();

	// We begin a new search if the last result size is superior or equal of the search limit size
	if (getSearchCache() != nullptr &&
		(getLimitedSearch() && getSearchLimit() > getSearchCache()->size())
	) {
		resultList = continueSearch(filter, withDomain);
	} else {
		resultList = beginNewSearch(filter, withDomain);
	}

	resultList->sort([](const SearchResult& lsr, const SearchResult& rsr){
		return lsr >= rsr;
	});

	setSearchCache(resultList);

	return *resultList;
}

/////////////////////
// Private Methods //
/////////////////////

list<SearchResult> *MagicSearch::getSearchCache() const {
	L_D();
	return d->mCacheResult;
}

void MagicSearch::setSearchCache(list<SearchResult> *cache) {
	L_D();
	if (d->mCacheResult != cache) resetSearchCache();
	d->mCacheResult = cache;
}

list<SearchResult> *MagicSearch::beginNewSearch(const string &filter, const string &withDomain) {
	list<SearchResult> *resultList = new list<SearchResult>();
	LinphoneFriendList* list = linphone_core_get_default_friend_list(this->getCore()->getCCore());

	// For all friends or when we reach the search limit
	for (bctbx_list_t *f = list->friends;
		 f != nullptr && (!getLimitedSearch() || resultList->size() < getSearchLimit());
	f = bctbx_list_next(f)
	) {
		SearchResult result = search(reinterpret_cast<LinphoneFriend*>(f->data), filter, withDomain);
		if (result.getWeight() > getMinWeight()) {
			resultList->push_back(result);
		}
	}

	return resultList;
}

list<SearchResult> *MagicSearch::continueSearch(const string &filter, const string &withDomain) {
	list<SearchResult> *resultList = new list<SearchResult>();
	const list <SearchResult> *cacheList = getSearchCache();

	for (const auto sr : *cacheList) {
		if (sr.getFriend()) {
			SearchResult result = search(sr.getFriend(), filter, withDomain);
			if (result.getWeight() > getMinWeight()) {
				resultList->push_back(result);
			}
		}
	}

	return resultList;
}

SearchResult MagicSearch::search(const LinphoneFriend* lFriend, const string &filter, const string &withDomain) {
	unsigned int weight = getMinWeight();
	const LinphoneAddress* lAddress = linphone_friend_get_address(lFriend);
	bool allDomain = !withDomain.empty() && withDomain.compare("*") == 0;

	if (allDomain &&
		lAddress == nullptr &&
		linphone_friend_get_presence_model(lFriend) == nullptr
	) return SearchResult(weight, nullptr);

	// Check domain
	if (!allDomain &&
		!withDomain.empty() &&
		lAddress != nullptr &&
		withDomain.compare(linphone_address_get_domain(lAddress)) != 0
	) return SearchResult(weight, nullptr);

	if (lAddress != nullptr) {
		// SIPURI
		if (linphone_address_get_username(lAddress) != nullptr) {
			weight += getWeight(linphone_address_get_username(lAddress), filter);
		}
		// DISPLAYNAME
		if (linphone_address_get_display_name(lAddress) != nullptr) {
			weight += getWeight(linphone_address_get_display_name(lAddress), filter);
		}
	}

	// NAME
	if (linphone_friend_get_name(lFriend) != nullptr) {
		weight += getWeight(linphone_friend_get_name(lFriend), filter);
	}

	// PHONE NUMBER
	bctbx_list_t *begin, *phoneNumbers = linphone_friend_get_phone_numbers(lFriend);
	begin = phoneNumbers;
	while (phoneNumbers != nullptr && phoneNumbers->data != nullptr) {
		string number = static_cast<const char*>(phoneNumbers->data);
		const LinphonePresenceModel *presence = linphone_friend_get_presence_model_for_uri_or_tel(lFriend, number.c_str());
		weight += getWeight(number, filter);
		if (presence != nullptr) {
			weight += getWeight(linphone_presence_model_get_contact(presence), filter);
		}
		phoneNumbers = phoneNumbers->next;
	}
	if (begin) bctbx_list_free(begin);

	return SearchResult(weight, lAddress, lFriend);
}

unsigned int MagicSearch::getWeight(const string &stringWords, const string &filter) const {
	size_t weight = string::npos;

	// Finding all occurrences of "filter" in "stringWords"
	for (size_t w = stringWords.find(filter);
		 w != string::npos;
		 w = stringWords.find(filter, w + filter.length())
		) {
		// weight max if occurence find at beginning
		if (w == 0) {
			weight = getMaxWeight();
		} else {
			bool isDelimiter = false;
			if (getUseDelimiter()) {
				// get the char before the matched filter
				const char l = stringWords.at(w - 1);
				// Check if it's a delimiter
				for (const char d : getDelimiter()) {
					if (l == d) {
						isDelimiter = true;
						break;
					}
				}
			}
			unsigned int newWeight = getMaxWeight() - (unsigned int)((isDelimiter) ? 1 : w + 1);
			weight = (weight != string::npos) ? weight + newWeight : newWeight;
		}
		// Only one search on the stringWords for the moment
		// due to weight calcul which dos not take into the case of multiple occurence
		break;
	}

	return (weight != string::npos) ? (unsigned int)(weight) : getMinWeight();
}

LINPHONE_END_NAMESPACE
