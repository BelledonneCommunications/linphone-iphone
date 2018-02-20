/*
 * c-magic-search.h
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

#ifndef _L_C_MAGIC_SEARCH_H_
#define _L_C_MAGIC_SEARCH_H_

#include "linphone/api/c-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup misc
 * @{
 */

/**
 * Constructs a LinphoneMagicSearch object
 **/
LINPHONE_PUBLIC LinphoneMagicSearch *linphone_magic_search_new(LinphoneCore *lc);

/**
 * Increment reference count of LinphoneMagicSearch object.
 **/
LINPHONE_PUBLIC LinphoneMagicSearch *linphone_magic_search_ref(LinphoneMagicSearch *magicSearch);

/**
 * Decrement reference count of LinphoneMagicSearch object. When dropped to zero, memory is freed.
 **/
LINPHONE_PUBLIC void linphone_magic_search_unref(LinphoneMagicSearch *magicSearch);

/**
 * Set the minimum value used to calculate the weight in search
 * @param[in] weight minimum weight
 **/
LINPHONE_PUBLIC void linphone_magic_search_set_min_weight(LinphoneMagicSearch *magicSearch, const unsigned int weight);

/**
 * @return the minimum value used to calculate the weight in search
 **/
LINPHONE_PUBLIC unsigned int linphone_magic_search_get_min_weight(const LinphoneMagicSearch *magicSearch);

/**
 * Set the maximum value used to calculate the weight in search
 * @param[in] weight maximum weight
 **/
LINPHONE_PUBLIC void linphone_magic_search_set_max_weight(LinphoneMagicSearch *magicSearch, const unsigned int weight);

/**
 * @return the maximum value used to calculate the weight in search
 **/
LINPHONE_PUBLIC unsigned int linphone_magic_search_get_max_weight(const LinphoneMagicSearch *magicSearch);

/**
 * @return the delimiter used to find matched filter word
 **/
LINPHONE_PUBLIC const char *linphone_magic_search_get_delimiter(const LinphoneMagicSearch *magicSearch);

/**
 * Set the delimiter used to find matched filter word
 * @param[in] delimiter delimiter (example "-_.,")
 **/
LINPHONE_PUBLIC void linphone_magic_search_set_delimiter(LinphoneMagicSearch *magicSearch, const char *delimiter);

/**
 * @return if the delimiter search is used
 **/
LINPHONE_PUBLIC bool_t linphone_magic_search_get_use_delimiter(LinphoneMagicSearch *magicSearch);

/**
 * Enable or disable the delimiter in search
 * @param[in] enable
 **/
LINPHONE_PUBLIC void linphone_magic_search_set_use_delimiter(LinphoneMagicSearch *magicSearch, bool_t enable);

/**
 * @return the number of the maximum SearchResult which will be return
 **/
LINPHONE_PUBLIC unsigned int linphone_magic_search_get_search_limit(const LinphoneMagicSearch *magicSearch);

/**
 * Set the number of the maximum SearchResult which will be return
 * @param[in] limit
 **/
LINPHONE_PUBLIC void linphone_magic_search_set_search_limit(LinphoneMagicSearch *magicSearch, const unsigned int limit);

/**
 * @return if the search is limited
 **/
LINPHONE_PUBLIC bool_t linphone_magic_search_get_limited_search(const LinphoneMagicSearch *magicSearch);

/**
 * Enable or disable the limited search
 * @param[in] limited
 **/
LINPHONE_PUBLIC void linphone_magic_search_set_limited_search(LinphoneMagicSearch *magicSearch, const bool_t limited);

/**
 * Reset the cache to begin a new search
 **/
LINPHONE_PUBLIC void linphone_magic_search_reset_search_cache(LinphoneMagicSearch *magicSearch);

/**
 * Create a sorted list of SearchResult from SipUri, Contact name,
 * Contact displayname, Contact phone number, which match with a filter word
 * During the first search, a cache is created and used for the next search and so on
 * Use linphone_magic_search_reset_search_cache() to begin a new search
 * @param[in] filter word we search
 * @param[in] withDomain domain which we want to search only
 * - "" for searching in all contact
 * - "*" for searching in contact with sip SipUri
 * - "yourdomain" for searching in contact from "yourdomain" domain
 * @return sorted list of \bctbx_list{LinphoneSearchResult}
 **/
LINPHONE_PUBLIC bctbx_list_t* linphone_magic_search_get_contact_list_from_filter(LinphoneMagicSearch *magicSearch, const char *filter, const char *withDomain);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // _L_C_MAGIC_SEARCH_H_
