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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "vcard.h"

struct _LinphoneVCard {
	void *dummy;
};

LinphoneVCard* linphone_vcard_new(void) {
	return NULL;
}

void linphone_vcard_free(LinphoneVCard *vCard) {
	
}

MSList* linphone_vcard_list_from_vcard4_file(const char *filename) {
	return NULL;
}

MSList* linphone_vcard_list_from_vcard4_buffer(const char *buffer) {
	return NULL;
}

LinphoneVCard* linphone_vcard_new_from_vcard4_buffer(const char *buffer) {
	return NULL;
}

const char * linphone_vcard_as_vcard4_string(LinphoneVCard *vCard) {
	return NULL;
}

void linphone_vcard_set_full_name(LinphoneVCard *vCard, const char *name) {
	
}

const char* linphone_vcard_get_full_name(const LinphoneVCard *vCard) {
	return NULL;
}

void linphone_vcard_add_sip_address(LinphoneVCard *vCard, const char *sip_address) {
	
}

void linphone_vcard_remove_sip_address(LinphoneVCard *vCard, const char *sip_address) {
	
}

void linphone_vcard_edit_main_sip_address(LinphoneVCard *vCard, const char *sip_address) {
	
}

MSList* linphone_vcard_get_sip_addresses(const LinphoneVCard *vCard) {
	return NULL;
}

const char* linphone_vcard_get_uid(const LinphoneVCard *vCard) {
	return NULL;
}

void linphone_vcard_set_etag(LinphoneVCard *vCard, const char * etag) {
	
}

const char* linphone_vcard_get_etag(const LinphoneVCard *vCard) {
	return NULL;
}

void linphone_vcard_set_url(LinphoneVCard *vCard, const char * url) {
	
}

const char* linphone_vcard_get_url(const LinphoneVCard *vCard) {
	return NULL;
}