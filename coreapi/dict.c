/*
linphone
Copyright (C) 2009  Simon MORLAT (simon.morlat@linphone.org)

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

#include "linphone/core.h"
#include "linphone/lpconfig.h"
#include "private.h"

#include <belle-sip/belle-sip.h>
#include <belle-sip/object.h>
#include <belle-sip/dict.h>


/**
 * @addtogroup misc
 * @{
**/


LinphoneDictionary* linphone_dictionary_new()
{
	return belle_sip_dict_create();
}

LinphoneDictionary* linphone_dictionary_clone(const LinphoneDictionary* src)
{
	LinphoneDictionary* cloned = linphone_dictionary_new();
	if( cloned ){
		belle_sip_dict_clone(src, cloned);
	}
	return cloned;
}

LinphoneDictionary* linphone_dictionary_ref(LinphoneDictionary* obj)
{
	return BELLE_SIP_DICT(belle_sip_object_ref(obj));
}

void linphone_dictionary_unref(LinphoneDictionary *obj)
{
	belle_sip_object_unref(obj);
}

void linphone_dictionary_set_int(LinphoneDictionary* obj, const char* key, int value)
{
	belle_sip_dict_set_int(obj, key, value);
}

int linphone_dictionary_get_int(LinphoneDictionary* obj, const char* key, int default_value)
{
	return belle_sip_dict_get_int(obj, key, default_value);
}

void linphone_dictionary_set_string(LinphoneDictionary* obj, const char* key, const char*value)
{
	belle_sip_dict_set_string(obj, key, value);
}

const char* linphone_dictionary_get_string(LinphoneDictionary* obj, const char* key, const char* default_value)
{
	return belle_sip_dict_get_string(obj, key, default_value);
}

void linphone_dictionary_set_int64(LinphoneDictionary* obj, const char* key, int64_t value)
{
	belle_sip_dict_set_int64(obj, key, value);
}

int64_t linphone_dictionary_get_int64(LinphoneDictionary* obj, const char* key, int64_t default_value)
{
	return belle_sip_dict_get_int64(obj, key, default_value);
}

int linphone_dictionary_remove(LinphoneDictionary* obj, const char* key)
{
	return belle_sip_dict_remove(obj, key);
}

void linphone_dictionary_clear(LinphoneDictionary* obj)
{
	belle_sip_dict_clear(obj);
}

int linphone_dictionary_haskey(const LinphoneDictionary* obj, const char* key)
{
	return belle_sip_dict_haskey(obj, key);
}

void linphone_dictionary_foreach(const LinphoneDictionary* obj, void (*apply_func)(const char*, void*, void*), void* userdata)
{
	belle_sip_dict_foreach(obj, apply_func, userdata);
}

struct lp_config_to_dict {
	const char*         section;
	const LpConfig*     config;
	LinphoneDictionary* dict;
};

static void lp_config_section_to_dict_cb(const char*key, struct lp_config_to_dict* userdata)
{
	const char* value = lp_config_get_string(userdata->config, userdata->section, key, "");
	linphone_dictionary_set_string(userdata->dict, key, value);
}

LinphoneDictionary* lp_config_section_to_dict(const LpConfig* lpconfig, const char* section)
{
	LinphoneDictionary* dict = NULL;
	struct lp_config_to_dict fd;
	fd.config = lpconfig;
	fd.section = section;

	dict = linphone_dictionary_new();
	fd.dict = dict;

	lp_config_for_each_entry(lpconfig, section,
							 (void (*)(const char*, void*))lp_config_section_to_dict_cb,
							 &fd);

	return dict;
}

struct lp_config_from_dict {
	const char* section;
	LpConfig*   config;
};

static void lp_config_dict_dump_cb( const char* key, void* value, void* userdata)
{
	struct lp_config_from_dict* fd= (struct lp_config_from_dict*)userdata;
	lp_config_set_string(fd->config, fd->section, key, (const char*)value);
}

void lp_config_load_dict_to_section(LpConfig* lpconfig, const char* section, const LinphoneDictionary* dict)
{
	struct lp_config_from_dict pvdata = { section, lpconfig };
	linphone_dictionary_foreach(dict,lp_config_dict_dump_cb, &pvdata);
}



/**
 * @}
**/
