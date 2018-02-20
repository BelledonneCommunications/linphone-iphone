/*
 * magic-search.h
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

#ifndef _L_MAGIC_SEARCH_H_
#define _L_MAGIC_SEARCH_H_

#include <string>
#include <list>
#include <memory>

#include "core/core.h"
#include "core/core-accessor.h"
#include "search-result.h"

LINPHONE_BEGIN_NAMESPACE

class MagicSearchPrivate;

class LINPHONE_PUBLIC MagicSearch : public CoreAccessor, public Object{
public:

	MagicSearch(const std::shared_ptr<Core> &core);
	MagicSearch(const MagicSearch &ms) = delete;
	~MagicSearch();

	/**
	 * Set the minimum value used to calculate the weight in search
	 * @param[in] weight minimum weight
	 **/
	void setMinWeight(const unsigned int weight);

	/**
	 * @return the minimum value used to calculate the weight in search
	 **/
	unsigned int getMinWeight() const;

	/**
	 * Set the maximum value used to calculate the weight in search
	 * @param[in] weight maximum weight
	 **/
	void setMaxWeight(const unsigned int weight);

	/**
	 * @return the maximum value used to calculate the weight in search
	 **/
	unsigned int getMaxWeight() const;

	/**
	 * @return the delimiter used to find matched filter word
	 **/
	const std::string& getDelimiter() const;

	/**
	 * Set the delimiter used to find matched filter word
	 * @param[in] delimiter delimiter (example "-_.,")
	 **/
	void setDelimiter(const std::string &delimiter);

	/**
	 * @return if the delimiter search is used
	 **/
	bool getUseDelimiter() const;

	/**
	 * Enable or disable the delimiter in search
	 * @param[in] enable
	 **/
	void setUseDelimiter(bool enable);

	/**
	 * @return the number of the maximum SearchResult which will be return
	 **/
	unsigned int getSearchLimit() const;

	/**
	 * Set the number of the maximum SearchResult which will be return
	 * @param[in] limit
	 **/
	void setSearchLimit(const unsigned int limit);

	/**
	 * @return if the search is limited
	 **/
	bool getLimitedSearch() const;

	/**
	 * Enable or disable the limited search
	 * @param[in] limited
	 **/
	void setLimitedSearch(const bool limited);

	/**
	 * Reset the cache to begin a new search
	 **/
	void resetSearchCache();

	/**
	 * Create a sorted list of SearchResult from SipUri, Contact name,
	 * Contact displayname, Contact phone number, which match with a filter word
	 * During the first search, a cache is created and used for the next search and so on
	 * Use resetSearchCache() to begin a new search
	 * @param[in] filter word we search
	 * @param[in] withDomain
	 * - "" for searching in all contact
	 * - "*" for searching in contact with sip SipUri
	 * - "yourdomain" for searching in contact from "yourdomain" domain
	 * @return sorted list of SearchResult with "filter" or an empty list if "filter" is empty
	 **/
	std::list<SearchResult> getContactListFromFilter(const std::string &filter, const std::string &withDomain = "");

private:

	/**
	 * @return the cache of precedent result
	 * @private
	 **/
	std::list<SearchResult> *getSearchCache() const;

	/**
	 * Save a result for future search
	 * @param[in] cache result we want to save
	 * @private
	 **/
	void setSearchCache(std::list<SearchResult> *cache);

	/**
	 * Begin the search from friend list
	 * @param[in] filter word we search
	 * @param[in] withDomain domain which we want to search only
	 * @private
	 **/
	std::list<SearchResult> *beginNewSearch(const std::string &filter, const std::string &withDomain);

	/**
	 * Continue the search from the cache of precedent search
	 * @param[in] filter word we search
	 * @param[in] withDomain domain which we want to search only
	 * @private
	 **/
	std::list<SearchResult> *continueSearch(const std::string &filter, const std::string &withDomain);

	/**
	 * Continue the search from the cache of precedent search
	 * @param[in] lFriend friend whose informations will be check
	 * @param[in] filter word we search
	 * @param[in] withDomain domain which we want to search only
	 * @private
	 **/
	SearchResult search(const LinphoneFriend* lFriend, const std::string &filter, const std::string &withDomain);

	/**
	 * Return a weight for a searched in with a filter
	 * @param[in] stringWords which where we are searching
	 * @param[in] filter what we are searching
	 * @return calculate weight
	 * @private
	 **/
	unsigned int getWeight(const std::string &stringWords, const std::string &filter) const;

	L_DECLARE_PRIVATE(MagicSearch);
};

LINPHONE_END_NAMESPACE

#endif //_L_MAGIC_SEARCH_H_
