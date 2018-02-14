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
	 * Create a sorted list of SearchResult from SipUri, Contact name,
	 * Contact displayname, Contact phone number, which match with a filter word
	 * @param[in] filter word we search
	 * @param[in] withDomain domain which we want to search only
	 * @return sorted list of SearchResult with "filter"
	 **/
	std::list<SearchResult> getContactListFromFilter(const std::string &filter, const std::string &withDomain = "") const;

private:
	/**
	 * Return a weight for a searched in with a filter
	 * @param[in] stringWords which where we are searching
	 * @param[in] filter what we are searching
	 * @return calculate weight
	 **/
	unsigned int getWeight(const std::string &stringWords, const std::string &filter) const;

	L_DECLARE_PRIVATE(MagicSearch);
};

LINPHONE_END_NAMESPACE

#endif //_L_MAGIC_SEARCH_H_
