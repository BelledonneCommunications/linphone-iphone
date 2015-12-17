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
	shared_ptr<belcard::BelCard> belCard;
};

extern "C" LinphoneVCard* linphone_vcard_new(void) {
	LinphoneVCard* vCard = (LinphoneVCard*) ms_new0(LinphoneVCard, 1);
	vCard->belCard = belcard::BelCardGeneric::create<belcard::BelCard>();
	return vCard;
}

extern "C" void linphone_vcard_free(LinphoneVCard *vCard) {
	if (!vCard) return;
	
	vCard->belCard.reset();
	ms_free(vCard);
}

extern "C" MSList* linphone_vcard_list_from_vcard4_file(const char *filename) {
	MSList *result = NULL;
	if (filename && ortp_file_exist(filename) == 0) {
		belcard::BelCardParser *parser = new belcard::BelCardParser();
		shared_ptr<belcard::BelCardList> belCards = parser->parseFile(filename);
		if (belCards) {
			for (auto it = belCards->getCards().begin(); it != belCards->getCards().end(); ++it) {
				shared_ptr<belcard::BelCard> belcard = (*it);
				LinphoneVCard *vCard = linphone_vcard_new();
				vCard->belCard = belcard;
				result = ms_list_append(result, vCard);
			}
		}
		delete parser;
	}
	return result;
}

extern "C" MSList* linphone_vcard_list_from_vcard4_buffer(const char *buffer) {
	MSList *result = NULL;
	if (buffer) {
		belcard::BelCardParser *parser = new belcard::BelCardParser();
		shared_ptr<belcard::BelCardList> belCards = parser->parse(buffer);
		if (belCards) {
			for (auto it = belCards->getCards().begin(); it != belCards->getCards().end(); ++it) {
				shared_ptr<belcard::BelCard> belCard = (*it);
				LinphoneVCard *vCard = linphone_vcard_new();
				vCard->belCard = belCard;
				result = ms_list_append(result, vCard);
			}
		}
		delete parser;
	}
	return result;
}

extern "C" LinphoneVCard* linphone_vcard_new_from_vcard4_buffer(const char *buffer) {
	LinphoneVCard *vCard = NULL;
	if (buffer) {
		belcard::BelCardParser *parser = new belcard::BelCardParser();
		shared_ptr<belcard::BelCard> belCard = parser->parseOne(buffer);
		if (belCard) {
			vCard = linphone_vcard_new();
			vCard->belCard = belCard;
		}
		delete parser;
	}
	return vCard;
}

extern "C" const char * linphone_vcard_as_vcard4_string(LinphoneVCard *vCard) {
	if (!vCard) return NULL;
	
	return vCard->belCard->toFoldedString().c_str();
}

extern "C" void linphone_vcard_set_full_name(LinphoneVCard *vCard, const char *name) {
	if (!vCard || !name) return;
	
	shared_ptr<belcard::BelCardFullName> fn = belcard::BelCardGeneric::create<belcard::BelCardFullName>();
	fn->setValue(name);
	vCard->belCard->setFullName(fn);
}

extern "C" const char* linphone_vcard_get_full_name(const LinphoneVCard *vCard) {
	if (!vCard) return NULL;
	
	const char *result = vCard->belCard->getFullName() ? vCard->belCard->getFullName()->getValue().c_str() : NULL;
	return result;
}

extern "C" void linphone_vcard_add_sip_address(LinphoneVCard *vCard, const char *sip_address) {
	if (!vCard || !sip_address) return;
	
	shared_ptr<belcard::BelCardImpp> impp = belcard::BelCardGeneric::create<belcard::BelCardImpp>();
	impp->setValue(sip_address);
	vCard->belCard->addImpp(impp);
}

extern "C" void linphone_vcard_remove_sip_address(LinphoneVCard *vCard, const char *sip_address) {
	if (!vCard) return;
	
	for (auto it = vCard->belCard->getImpp().begin(); it != vCard->belCard->getImpp().end(); ++it) {
		const char *value = (*it)->getValue().c_str();
		if (strcmp(value, sip_address) == 0) {
			vCard->belCard->removeImpp(*it);
			break;
		}
	}
}

extern "C" void linphone_vcard_edit_main_sip_address(LinphoneVCard *vCard, const char *sip_address) {
	if (!vCard || !sip_address) return;
	
	if (vCard->belCard->getImpp().size() > 0) {
		const shared_ptr<belcard::BelCardImpp> impp = vCard->belCard->getImpp().front();
		impp->setValue(sip_address);
	} else {
		shared_ptr<belcard::BelCardImpp> impp = belcard::BelCardGeneric::create<belcard::BelCardImpp>();
		impp->setValue(sip_address);
		vCard->belCard->addImpp(impp);
	}
}

extern "C" MSList* linphone_vcard_get_sip_addresses(const LinphoneVCard *vCard) {
	MSList *result = NULL;
	if (!vCard) return NULL;
	
	for (auto it = vCard->belCard->getImpp().begin(); it != vCard->belCard->getImpp().end(); ++it) {
		const char *value = (*it)->getValue().c_str();
		if (strncmp(value, "sip:", 4) == 0) {
			result = ms_list_append(result, (char *)value);
		}
	}
	return result;
}