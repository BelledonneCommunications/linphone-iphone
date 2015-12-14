/*
vcard.cc
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
#include "belcard/belcard.hpp"
#include "belcard/belcard_parser.hpp"

struct _LinphoneVCard {
	shared_ptr<belcard::BelCard> belcard;
};

extern "C" LinphoneVCard* linphone_vcard_new(void) {
	LinphoneVCard* vcard = (LinphoneVCard*) ms_new0(LinphoneVCard, 1);
	vcard->belcard = belcard::BelCardGeneric::create<belcard::BelCard>();
	return vcard;
}

extern "C" void linphone_vcard_free(LinphoneVCard *vcard) {
	vcard->belcard.reset();
	ms_free(vcard);
}

extern "C" MSList* linphone_vcard_new_from_vcard4_file(const char *filename) {
	MSList *result = NULL;
	if (filename && ortp_file_exist(filename) == 0) {
		belcard::BelCardParser *parser = new belcard::BelCardParser();
		shared_ptr<belcard::BelCardList> belCards = parser->parseFile(filename);
		if (belCards) {
			for (auto it = belCards->getCards().begin(); it != belCards->getCards().end(); ++it) {
				shared_ptr<belcard::BelCard> belcard = (*it);
				LinphoneVCard *vcard = linphone_vcard_new();
				vcard->belcard = belcard;
				result = ms_list_append(result, vcard);
			}
		}
	}
	return result;
}

extern "C" const char * linphone_vcard_as_vcard4_string(LinphoneVCard *vcard) {
	return vcard->belcard->toFoldedString().c_str();
}

extern "C" void linphone_vcard_set_full_name(LinphoneVCard *vcard, const char *name) {
	shared_ptr<belcard::BelCardFullName> fn = belcard::BelCardGeneric::create<belcard::BelCardFullName>();
	fn->setValue(name);
	vcard->belcard->setFullName(fn);
}

extern "C" const char* linphone_vcard_get_full_name(const LinphoneVCard *vcard) {
	const char *result = vcard->belcard->getFullName() ? vcard->belcard->getFullName()->getValue().c_str() : NULL;
	return result;
}

extern "C" MSList* linphone_vcard_get_sip_addresses(const LinphoneVCard *vcard) {
	MSList *result = NULL;
	for (auto it = vcard->belcard->getImpp().begin(); it != vcard->belcard->getImpp().end(); ++it) {
		const char *value = (*it)->getValue().c_str();
		if (strncmp(value, "sip:", 4) == 0) {
			result = ms_list_append(result, (char *)value);
		}
	}
	return result;
}