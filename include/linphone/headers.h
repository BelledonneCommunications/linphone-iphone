/*
content.h
Copyright (C) 2010-2017 Belledonne Communications SARL

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef LINPHONE_HEADERS_H_
#define LINPHONE_HEADERS_H_


#include "linphone/types.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup misc
 * @{
 */

/**
 * Increments ref count.
**/
LINPHONE_PUBLIC LinphoneHeaders * linphone_headers_ref(LinphoneHeaders *obj);

/**
 * Decrements ref count.
**/
LINPHONE_PUBLIC void linphone_headers_unref(LinphoneHeaders *obj);

/**
 * Search for a given header name and return its value.
 * @param obj the LinphoneHeaders object
 * @param name the header's name
 * @return the header's value or NULL if not found.
**/

LINPHONE_PUBLIC const char* linphone_headers_get_value(LinphoneHeaders *obj, const char *header_name);

/**
 * Add given header name and corresponding value.
 * @param obj the LinphoneHeaders object
 * @param name the header's name
 * @param the header's value
**/
LINPHONE_PUBLIC void linphone_headers_add(LinphoneHeaders *obj, const char *name, const char *value);

/**
 * Add given header name and corresponding value.
 * @param obj the LinphoneHeaders object
 * @param name the header's name
 * @param the header's value
**/
LINPHONE_PUBLIC void linphone_headers_remove(LinphoneHeaders *obj, const char *name);


/**
 * @}
**/

#ifdef __cplusplus
}
#endif

#endif
