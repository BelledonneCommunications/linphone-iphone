/*
 * c-magic-search.cpp
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

#include "search/magic-search.h"
#include "c-wrapper/c-wrapper.h"

using namespace std;

L_DECLARE_C_OBJECT_IMPL(MagicSearch);

LinphoneMagicSearch *linphone_magic_search_new(LinphoneCore *lc) {
	shared_ptr<LinphonePrivate::MagicSearch> cppPtr = make_shared<LinphonePrivate::MagicSearch>(L_GET_CPP_PTR_FROM_C_OBJECT(lc));

	LinphoneMagicSearch *object = L_INIT(MagicSearch);
	L_SET_CPP_PTR_FROM_C_OBJECT(object, cppPtr);
	return object;
}

LinphoneMagicSearch *linphone_magic_search_ref(LinphoneMagicSearch *magicSearch) {
	belle_sip_object_ref(magicSearch);
	return magicSearch;
}

void linphone_magic_search_unref(LinphoneMagicSearch *magicSearch) {
	belle_sip_object_unref(magicSearch);
}

void linphone_magic_search_set_min_weight(LinphoneMagicSearch *magicSearch, const unsigned int weight) {
	L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->setMinWeight(weight);
}

unsigned int linphone_magic_search_get_min_weight(const LinphoneMagicSearch *magicSearch) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->getMinWeight();
}

void linphone_magic_search_set_max_weight(LinphoneMagicSearch *magicSearch, const unsigned int weight) {
	L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->setMaxWeight(weight);
}

unsigned int linphone_magic_search_get_max_weight(const LinphoneMagicSearch *magicSearch) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->getMaxWeight();
}

const char *linphone_magic_search_get_delimiter(const LinphoneMagicSearch *magicSearch) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->getDelimiter());
}

void linphone_magic_search_set_delimiter(LinphoneMagicSearch *magicSearch, const char *delimiter) {
	L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->setDelimiter(L_C_TO_STRING(delimiter));
}

bool_t linphone_magic_search_get_use_delimiter(LinphoneMagicSearch *magicSearch) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->getUseDelimiter();
}

void linphone_magic_search_set_use_delimiter(LinphoneMagicSearch *magicSearch, bool_t enable) {
	L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->setUseDelimiter(enable);
}

unsigned int linphone_magic_search_get_search_limit(const LinphoneMagicSearch *magicSearch) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->getSearchLimit();
}

void linphone_magic_search_set_search_limit(LinphoneMagicSearch *magicSearch, const unsigned int limit) {
	L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->setSearchLimit(limit);
}

bool_t linphone_magic_search_get_limited_search(const LinphoneMagicSearch *magicSearch) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->getLimitedSearch();
}

void linphone_magic_search_set_limited_search(LinphoneMagicSearch *magicSearch, const bool_t limited) {
	L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->setLimitedSearch(limited);
}

void linphone_magic_search_reset_search_cache(LinphoneMagicSearch *magicSearch) {
	L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->resetSearchCache();
}

bctbx_list_t* linphone_magic_search_get_contact_list_from_filter(LinphoneMagicSearch *magicSearch, const char *filter, const char *withDomain) {
	return L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(L_GET_CPP_PTR_FROM_C_OBJECT(magicSearch)->getContactListFromFilter(L_C_TO_STRING(filter), L_C_TO_STRING(withDomain)));
}
