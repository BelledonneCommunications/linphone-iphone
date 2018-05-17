/*
 * c-search-result.h
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

#ifndef _L_C_SEARCH_RESULT_H_
#define _L_C_SEARCH_RESULT_H_

#include "linphone/api/c-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup misc
 * @{
 */

/**
 * Increment reference count of LinphoneSearchResult object.
 **/
LINPHONE_PUBLIC LinphoneSearchResult *linphone_search_result_ref(LinphoneSearchResult *searchResult);

/**
 * Decrement reference count of LinphoneSearchResult object. When dropped to zero, memory is freed.
 **/
LINPHONE_PUBLIC void linphone_search_result_unref(LinphoneSearchResult *searchResult);

/**
 * @return LinphoneFriend associed
 **/
LINPHONE_PUBLIC const LinphoneFriend* linphone_search_result_get_friend(const LinphoneSearchResult *searchResult);

/**
 * @return LinphoneAddress associed
 **/
LINPHONE_PUBLIC const LinphoneAddress* linphone_search_result_get_address(const LinphoneSearchResult *searchResult);

/**
 * @return Phone Number associed
 **/
LINPHONE_PUBLIC const char* linphone_search_result_get_phone_number(const LinphoneSearchResult *searchResult);

/**
 * @return the result weight
 **/
LINPHONE_PUBLIC unsigned int linphone_search_result_get_weight(const LinphoneSearchResult *searchResult);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // _L_C_SEARCH_RESULT_H_
