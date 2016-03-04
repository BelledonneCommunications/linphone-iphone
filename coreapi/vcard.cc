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
#include "sal/sal.h"
#include <bctoolbox/crypto.h>

struct _LinphoneVCard {
	shared_ptr<belcard::BelCard> belCard;
	char *etag;
	char *url;
	unsigned char *md5;
};

#ifdef __cplusplus
extern "C" {
#endif

LinphoneVCard* linphone_vcard_new(void) {
	LinphoneVCard* vCard = (LinphoneVCard*) ms_new0(LinphoneVCard, 1);
	vCard->belCard = belcard::BelCardGeneric::create<belcard::BelCard>();
	return vCard;
}

void linphone_vcard_free(LinphoneVCard *vCard) {
	if (!vCard) return;
	
	vCard->belCard.reset();
	ms_free(vCard);
}

MSList* linphone_vcard_list_from_vcard4_file(const char *filename) {
	MSList *result = NULL;
	if (filename && ortp_file_exist(filename) == 0) {
		belcard::BelCardParser parser = belcard::BelCardParser::getInstance();
		shared_ptr<belcard::BelCardList> belCards = parser.parseFile(filename);
		if (belCards) {
			for (auto it = belCards->getCards().begin(); it != belCards->getCards().end(); ++it) {
				shared_ptr<belcard::BelCard> belcard = (*it);
				LinphoneVCard *vCard = linphone_vcard_new();
				vCard->belCard = belcard;
				result = ms_list_append(result, vCard);
			}
		}
	}
	return result;
}

MSList* linphone_vcard_list_from_vcard4_buffer(const char *buffer) {
	MSList *result = NULL;
	if (buffer) {
		belcard::BelCardParser parser = belcard::BelCardParser::getInstance();
		shared_ptr<belcard::BelCardList> belCards = parser.parse(buffer);
		if (belCards) {
			for (auto it = belCards->getCards().begin(); it != belCards->getCards().end(); ++it) {
				shared_ptr<belcard::BelCard> belCard = (*it);
				LinphoneVCard *vCard = linphone_vcard_new();
				vCard->belCard = belCard;
				result = ms_list_append(result, vCard);
			}
		}
	}
	return result;
}

LinphoneVCard* linphone_vcard_new_from_vcard4_buffer(const char *buffer) {
	LinphoneVCard *vCard = NULL;
	if (buffer) {
		belcard::BelCardParser parser = belcard::BelCardParser::getInstance();
		shared_ptr<belcard::BelCard> belCard = parser.parseOne(buffer);
		if (belCard) {
			vCard = linphone_vcard_new();
			vCard->belCard = belCard;
		} else {
			ms_error("Couldn't parse buffer %s", buffer);
		}
	}
	return vCard;
}

const char * linphone_vcard_as_vcard4_string(LinphoneVCard *vCard) {
	if (!vCard) return NULL;
	
	return vCard->belCard->toFoldedString().c_str();
}

void linphone_vcard_set_full_name(LinphoneVCard *vCard, const char *name) {
	if (!vCard || !name) return;
	
	shared_ptr<belcard::BelCardFullName> fn = belcard::BelCardGeneric::create<belcard::BelCardFullName>();
	fn->setValue(name);
	vCard->belCard->setFullName(fn);
}

const char* linphone_vcard_get_full_name(const LinphoneVCard *vCard) {
	if (!vCard) return NULL;
	
	const char *result = vCard->belCard->getFullName() ? vCard->belCard->getFullName()->getValue().c_str() : NULL;
	return result;
}

void linphone_vcard_add_sip_address(LinphoneVCard *vCard, const char *sip_address) {
	if (!vCard || !sip_address) return;
	
	shared_ptr<belcard::BelCardImpp> impp = belcard::BelCardGeneric::create<belcard::BelCardImpp>();
	impp->setValue(sip_address);
	vCard->belCard->addImpp(impp);
}

void linphone_vcard_remove_sip_address(LinphoneVCard *vCard, const char *sip_address) {
	if (!vCard) return;
	
	for (auto it = vCard->belCard->getImpp().begin(); it != vCard->belCard->getImpp().end(); ++it) {
		const char *value = (*it)->getValue().c_str();
		if (strcmp(value, sip_address) == 0) {
			vCard->belCard->removeImpp(*it);
			break;
		}
	}
}

void linphone_vcard_edit_main_sip_address(LinphoneVCard *vCard, const char *sip_address) {
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

MSList* linphone_vcard_get_sip_addresses(const LinphoneVCard *vCard) {
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

bool_t linphone_vcard_generate_unique_id(LinphoneVCard *vCard) {
	char uuid[64];
	
	if (vCard) {
		if (linphone_vcard_get_uid(vCard)) {
			return FALSE;
		}
		if (sal_generate_uuid(uuid, sizeof(uuid)) == 0) {
			char vcard_uuid[sizeof(uuid)+4];
			snprintf(vcard_uuid, sizeof(vcard_uuid), "urn:%s", uuid);
			linphone_vcard_set_uid(vCard, ms_strdup(vcard_uuid));
			return TRUE;
		}
	}
	return FALSE;
}

void linphone_vcard_set_uid(LinphoneVCard *vCard, const char *uid) {
	if (!vCard || !uid) return;
	
	shared_ptr<belcard::BelCardUniqueId> uniqueId = belcard::BelCardGeneric::create<belcard::BelCardUniqueId>();
	uniqueId->setValue(uid);
	vCard->belCard->setUniqueId(uniqueId);
}

const char* linphone_vcard_get_uid(const LinphoneVCard *vCard) {
	if (vCard && vCard->belCard->getUniqueId()) {
		return vCard->belCard->getUniqueId()->getValue().c_str();
	}
	return NULL;
}

void linphone_vcard_set_etag(LinphoneVCard *vCard, const char * etag) {
	if (!vCard) {
		return;
	}
	if (vCard->etag) {
		ms_free(vCard->etag);
	}
	vCard->etag = ms_strdup(etag);
}

const char* linphone_vcard_get_etag(const LinphoneVCard *vCard) {
	if (!vCard) return NULL;
	return vCard->etag;
}

void linphone_vcard_set_url(LinphoneVCard *vCard, const char * url) {
	if (!vCard) {
		return;
	}
	if (vCard->url) {
		ms_free(vCard->url);
	}
	vCard->url = ms_strdup(url);
}

const char* linphone_vcard_get_url(const LinphoneVCard *vCard) {
	if (!vCard) return NULL;
	return vCard->url;
}

#define VCARD_MD5_HASH_SIZE 16

void linphone_vcard_compute_md5_hash(LinphoneVCard *vCard) {
	unsigned char digest[VCARD_MD5_HASH_SIZE];
	const char *text = NULL;
	if (!vCard) {
		return;
	}
	text = linphone_vcard_as_vcard4_string(vCard);
	bctoolbox_md5((unsigned char *)text, strlen(text), digest);
	vCard->md5 = (unsigned char *)ms_malloc(sizeof(digest));
	memcpy(vCard->md5, digest, sizeof(digest));
}

bool_t linphone_vcard_compare_md5_hash(LinphoneVCard *vCard) {
	unsigned char *previous_md5 = vCard->md5;
	unsigned char *new_md5 = NULL;
	int result = -1;
	
	if (!previous_md5) {
		return result;
	}
	
	linphone_vcard_compute_md5_hash(vCard);
	new_md5 = vCard->md5;
	result = memcmp(new_md5, previous_md5, VCARD_MD5_HASH_SIZE);
	
	ms_free(previous_md5);
	ms_free(new_md5);
	return result;
}

#ifdef __cplusplus
}
#endif