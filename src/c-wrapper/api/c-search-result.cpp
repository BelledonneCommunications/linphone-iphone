/*
 * c-search-result.cpp
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

#include "search/search-result.h"
#include "c-wrapper/c-wrapper.h"

L_DECLARE_C_CLONABLE_OBJECT_IMPL(SearchResult);

LinphoneSearchResult *linphone_search_result_ref(LinphoneSearchResult *searchResult) {
	belle_sip_object_ref(searchResult);
	return searchResult;
}

void linphone_search_result_unref(LinphoneSearchResult *searchResult) {
	belle_sip_object_unref(searchResult);
}

const LinphoneFriend* linphone_search_result_get_friend(const LinphoneSearchResult *searchResult) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(searchResult)->getFriend();
}

const LinphoneAddress* linphone_search_result_get_address(const LinphoneSearchResult *searchResult) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(searchResult)->getAddress();
}

const char* linphone_search_result_get_phone_number(const LinphoneSearchResult *searchResult) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(searchResult)->getPhoneNumber());
}

unsigned int linphone_search_result_get_weight(const LinphoneSearchResult *searchResult) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(searchResult)->getWeight();
}
