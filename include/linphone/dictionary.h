/*
dictionary.h
Copyright (C) 2016  Belledonne Communications SARL

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

#ifndef LINPHONE_DICTIONARY_H
#define LINPHONE_DICTIONARY_H

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

LINPHONE_PUBLIC LinphoneDictionary* linphone_dictionary_new(void);

LINPHONE_PUBLIC LinphoneDictionary * linphone_dictionary_clone(const LinphoneDictionary* src);

LINPHONE_PUBLIC LinphoneDictionary * linphone_dictionary_ref(LinphoneDictionary* obj);

LINPHONE_PUBLIC void linphone_dictionary_unref(LinphoneDictionary* obj);

LINPHONE_PUBLIC void linphone_dictionary_set_int(LinphoneDictionary* obj, const char* key, int value);

LINPHONE_PUBLIC int linphone_dictionary_get_int(LinphoneDictionary* obj, const char* key, int default_value);

LINPHONE_PUBLIC void linphone_dictionary_set_string(LinphoneDictionary* obj, const char* key, const char*value);

LINPHONE_PUBLIC const char* linphone_dictionary_get_string(LinphoneDictionary* obj, const char* key, const char* default_value);

LINPHONE_PUBLIC void linphone_dictionary_set_int64(LinphoneDictionary* obj, const char* key, int64_t value);

LINPHONE_PUBLIC int64_t linphone_dictionary_get_int64(LinphoneDictionary* obj, const char* key, int64_t default_value);

LINPHONE_PUBLIC LinphoneStatus linphone_dictionary_remove(LinphoneDictionary* obj, const char* key);

LINPHONE_PUBLIC void linphone_dictionary_clear(LinphoneDictionary* obj);

LINPHONE_PUBLIC LinphoneStatus linphone_dictionary_haskey(const LinphoneDictionary* obj, const char* key);

LINPHONE_PUBLIC void linphone_dictionary_foreach( const LinphoneDictionary* obj, void (*apply_func)(const char*key, void* value, void* userdata), void* userdata);

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_DICTIONARY_H */
