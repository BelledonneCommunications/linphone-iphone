/*
vcard_stubs.c
Copyright (C) 2015  Belledonne Communications SARL

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
#include "vcard_private.h"

struct _LinphoneVcardContext {
	void *user_data;
};

LinphoneVcardContext* linphone_vcard_context_new(void) {
	LinphoneVcardContext* context = ms_new0(LinphoneVcardContext, 1);
	context->user_data = NULL;
	return context;
}

void linphone_vcard_context_destroy(LinphoneVcardContext *context) {
	if (context) {
		ms_free(context);
	}
}


void* linphone_vcard_context_get_user_data(const LinphoneVcardContext *context) {
	return context ? context->user_data : NULL;
}

void linphone_vcard_context_set_user_data(LinphoneVcardContext *context, void *data) {
	if (context) context->user_data = data;
}

struct _LinphoneVcard {
	void *dummy;
};

LinphoneVcard* linphone_vcard_new(void) {
	return NULL;
}

LinphoneVcard *linphone_factory_create_vcard(LinphoneFactory *factory) {
	return NULL;
}

void linphone_vcard_free(LinphoneVcard *vCard) {
}

LinphoneVcard *linphone_vcard_ref(LinphoneVcard *vCard) {
	return NULL;
}

void linphone_vcard_unref(LinphoneVcard *vCard) {
}

bctbx_list_t* linphone_vcard_context_get_vcard_list_from_file(LinphoneVcardContext *context, const char *filename) {
	return NULL;
}

bctbx_list_t* linphone_vcard_context_get_vcard_list_from_buffer(LinphoneVcardContext *context, const char *buffer) {
	return NULL;
}

LinphoneVcard* linphone_vcard_context_get_vcard_from_buffer(LinphoneVcardContext *context, const char *buffer) {
	return NULL;
}

const char * linphone_vcard_as_vcard4_string(LinphoneVcard *vCard) {
	return NULL;
}

void linphone_vcard_set_full_name(LinphoneVcard *vCard, const char *name) {
}

const char* linphone_vcard_get_full_name(const LinphoneVcard *vCard) {
	return NULL;
}

void linphone_vcard_set_skip_validation(LinphoneVcard *vCard, bool_t skip) {
}

bool_t linphone_vcard_get_skip_validation(const LinphoneVcard *vCard) {
	return FALSE;
}

void linphone_vcard_set_family_name(LinphoneVcard *vCard, const char *name) {
}

const char* linphone_vcard_get_family_name(const LinphoneVcard *vCard) {
	return NULL;
}

void linphone_vcard_set_given_name(LinphoneVcard *vCard, const char *name) {
}

const char* linphone_vcard_get_given_name(const LinphoneVcard *vCard) {
	return NULL;
}

void linphone_vcard_add_sip_address(LinphoneVcard *vCard, const char *sip_address) {
}

void linphone_vcard_remove_sip_address(LinphoneVcard *vCard, const char *sip_address) {
}

void linphone_vcard_edit_main_sip_address(LinphoneVcard *vCard, const char *sip_address) {
}

const bctbx_list_t* linphone_vcard_get_sip_addresses(LinphoneVcard *vCard) {
	return NULL;
}

void linphone_vcard_add_phone_number(LinphoneVcard *vCard, const char *phone) {
}

void linphone_vcard_remove_phone_number(LinphoneVcard *vCard, const char *phone) {
}

bctbx_list_t* linphone_vcard_get_phone_numbers(const LinphoneVcard *vCard) {
	return NULL;
}

void linphone_vcard_set_organization(LinphoneVcard *vCard, const char *organization) {
}

const char* linphone_vcard_get_organization(const LinphoneVcard *vCard) {
	return NULL;
}

bool_t linphone_vcard_generate_unique_id(LinphoneVcard *vCard) {
	return FALSE;
}

void linphone_vcard_set_uid(LinphoneVcard *vCard, const char *uid) {
}

const char* linphone_vcard_get_uid(const LinphoneVcard *vCard) {
	return NULL;
}
 
void linphone_vcard_set_etag(LinphoneVcard *vCard, const char * etag) {
}

const char* linphone_vcard_get_etag(const LinphoneVcard *vCard) {
	return NULL;
}

void linphone_vcard_set_url(LinphoneVcard *vCard, const char * url) {
}

const char* linphone_vcard_get_url(const LinphoneVcard *vCard) {
	return NULL;
}

void linphone_vcard_compute_md5_hash(LinphoneVcard *vCard) {
}

bool_t linphone_vcard_compare_md5_hash(LinphoneVcard *vCard) {
	return FALSE;
}

bool_t linphone_core_vcard_supported(void) {
	return FALSE;
}

void linphone_vcard_clean_cache(LinphoneVcard *vCard) {
}
