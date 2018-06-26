/*
 * search-result.h
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

#ifndef _L_SEARCH_RESULT_H_
#define _L_SEARCH_RESULT_H_

#include "linphone/utils/general.h"
#include "linphone/types.h"

#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class SearchResultPrivate;

class LINPHONE_PUBLIC SearchResult : public ClonableObject {
public:
	// TODO: Use C++ Address! Not LinphoneAddress.
	SearchResult(const unsigned int weight, const LinphoneAddress *a, const std::string &pn, const LinphoneFriend *f = nullptr);
	SearchResult(const SearchResult &other);
	~SearchResult();

	bool operator<(const SearchResult &other) const;
	bool operator>(const SearchResult &other) const;
	bool operator>=(const SearchResult &other) const;
	bool operator=(const SearchResult &other) const;

	/**
	 * @return LinphoneFriend associed
	 **/
	const LinphoneFriend *getFriend()const;

	/**
	 * @return LinphoneAddress associed
	 **/
	const LinphoneAddress *getAddress() const;

	/**
	 * @return Phone Number associed
	 **/
	const std::string &getPhoneNumber() const;

	/**
	 * @return the result weight
	 **/
	unsigned int getWeight() const;

private:
	L_DECLARE_PRIVATE(SearchResult);
};

LINPHONE_END_NAMESPACE

#endif //_L_SEARCH_RESULT_H_
