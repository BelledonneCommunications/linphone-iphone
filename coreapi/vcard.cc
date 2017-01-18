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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "vcard_private.h"
#include "belcard/belcard.hpp"
#include "belcard/belcard_parser.hpp"
#include "sal/sal.h"
#include <bctoolbox/crypto.h>
#include "linphone/core.h"
#include "private.h"

#define VCARD_MD5_HASH_SIZE 16


struct _LinphoneVcardContext {
	shared_ptr<belcard::BelCardParser> parser;
	void *user_data;
};

extern "C" {

LinphoneVcardContext* linphone_vcard_context_new(void) {
	LinphoneVcardContext* context = ms_new0(LinphoneVcardContext, 1);
	context->parser = belcard::BelCardParser::getInstance();
	context->user_data = NULL;
	return context;
}

void linphone_vcard_context_destroy(LinphoneVcardContext *context) {
	if (context) {
		context->parser = nullptr;
		ms_free(context);
	}
}

void* linphone_vcard_context_get_user_data(const LinphoneVcardContext *context) {
	return context ? context->user_data : NULL;
}


void linphone_vcard_context_set_user_data(LinphoneVcardContext *context, void *data) {
	if (context) context->user_data = data;
}

} // extern "C"


struct _LinphoneVcard {
	belle_sip_object_t base;
	shared_ptr<belcard::BelCard> belCard;
	char *etag;
	char *url;
	unsigned char md5[VCARD_MD5_HASH_SIZE];
	bctbx_list_t *sip_addresses_cache;
};

extern "C" {

static void _linphone_vcard_uninit(LinphoneVcard *vCard) {
	if (vCard->etag) ms_free(vCard->etag);
	if (vCard->url) ms_free(vCard->url);
	linphone_vcard_clean_cache(vCard);
	vCard->belCard.reset();
}

BELLE_SIP_DECLARE_VPTR(LinphoneVcard);
BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneVcard);
BELLE_SIP_INSTANCIATE_VPTR(LinphoneVcard, belle_sip_object_t,
	_linphone_vcard_uninit, // destroy
	NULL, // clone
	NULL, // Marshall
	FALSE
);

static LinphoneVcard* _linphone_vcard_new(void) {
	LinphoneVcard* vCard = belle_sip_object_new(LinphoneVcard);
	vCard->belCard = belcard::BelCardGeneric::create<belcard::BelCard>();
	return vCard;
}

LinphoneVcard *linphone_vcard_new(void) {
	return _linphone_vcard_new();
}

LinphoneVcard *linphone_factory_create_vcard(LinphoneFactory *factory) {
	return _linphone_vcard_new();
}

static LinphoneVcard* linphone_vcard_new_from_belcard(shared_ptr<belcard::BelCard> belcard) {
	LinphoneVcard* vCard = belle_sip_object_new(LinphoneVcard);
	vCard->belCard = belcard;
	return vCard;
}

void linphone_vcard_free(LinphoneVcard *vCard) {
	belle_sip_object_unref((belle_sip_object_t *)vCard);
}

LinphoneVcard *linphone_vcard_ref(LinphoneVcard *vCard) {
	return (LinphoneVcard *)belle_sip_object_ref((belle_sip_object_t *)vCard);
}

void linphone_vcard_unref(LinphoneVcard *vCard) {
	belle_sip_object_unref((belle_sip_object_t *)vCard);
}

bctbx_list_t* linphone_vcard_context_get_vcard_list_from_file(LinphoneVcardContext *context, const char *filename) {
	bctbx_list_t *result = NULL;
	if (context && filename) {
		if (!context->parser) {
			context->parser = belcard::BelCardParser::getInstance();
		}
		shared_ptr<belcard::BelCardList> belCards = context->parser->parseFile(filename);
		if (belCards) {
			for (auto it = belCards->getCards().begin(); it != belCards->getCards().end(); ++it) {
				shared_ptr<belcard::BelCard> belCard = (*it);
				LinphoneVcard *vCard = linphone_vcard_new_from_belcard(belCard);
				result = bctbx_list_append(result, vCard);
			}
		}
	}
	return result;
}

bctbx_list_t* linphone_vcard_context_get_vcard_list_from_buffer(LinphoneVcardContext *context, const char *buffer) {
	bctbx_list_t *result = NULL;
	if (context && buffer) {
		if (!context->parser) {
			context->parser = belcard::BelCardParser::getInstance();
		}
		shared_ptr<belcard::BelCardList> belCards = context->parser->parse(buffer);
		if (belCards) {
			for (auto it = belCards->getCards().begin(); it != belCards->getCards().end(); ++it) {
				shared_ptr<belcard::BelCard> belCard = (*it);
				LinphoneVcard *vCard = linphone_vcard_new_from_belcard(belCard);
				result = bctbx_list_append(result, vCard);
			}
		}
	}
	return result;
}

LinphoneVcard* linphone_vcard_context_get_vcard_from_buffer(LinphoneVcardContext *context, const char *buffer) {
	LinphoneVcard *vCard = NULL;
	if (context && buffer) {
		if (!context->parser) {
			context->parser = belcard::BelCardParser::getInstance();
		}
		shared_ptr<belcard::BelCard> belCard = context->parser->parseOne(buffer);
		if (belCard) {
			vCard = linphone_vcard_new_from_belcard(belCard);
		} else {
			ms_error("Couldn't parse buffer %s", buffer);
		}
	}
	return vCard;
}

const char * linphone_vcard_as_vcard4_string(LinphoneVcard *vCard) {
	if (!vCard) return NULL;

	return vCard->belCard->toFoldedString().c_str();
}

void *linphone_vcard_get_belcard(LinphoneVcard *vcard) {
	return &vcard->belCard;
}

void linphone_vcard_set_full_name(LinphoneVcard *vCard, const char *name) {
	if (!vCard || !name) return;

	if (vCard->belCard->getFullName()) {
		vCard->belCard->getFullName()->setValue(name);
	} else {
		shared_ptr<belcard::BelCardFullName> fn = belcard::BelCardGeneric::create<belcard::BelCardFullName>();
		fn->setValue(name);
		vCard->belCard->setFullName(fn);
	}
}

const char* linphone_vcard_get_full_name(const LinphoneVcard *vCard) {
	if (!vCard) return NULL;

	const char *result = vCard->belCard->getFullName() ? vCard->belCard->getFullName()->getValue().c_str() : NULL;
	return result;
}

void linphone_vcard_set_skip_validation(LinphoneVcard *vCard, bool_t skip) {
	if (!vCard || !vCard->belCard) return;

	vCard->belCard->setSkipFieldValidation((skip == TRUE) ? true : false);
}

bool_t linphone_vcard_get_skip_validation(const LinphoneVcard *vCard) {
	if (!vCard) return FALSE;

	bool_t result = vCard->belCard->getSkipFieldValidation();
	return result;
}

void linphone_vcard_set_family_name(LinphoneVcard *vCard, const char *name) {
	if (!vCard || !name) return;

	if (vCard->belCard->getName()) {
		vCard->belCard->getName()->setFamilyName(name);
	} else {
		shared_ptr<belcard::BelCardName> n = belcard::BelCardGeneric::create<belcard::BelCardName>();
		n->setFamilyName(name);
		vCard->belCard->setName(n);
	}
}

const char* linphone_vcard_get_family_name(const LinphoneVcard *vCard) {
	if (!vCard) return NULL;

	const char *result = vCard->belCard->getName() ? vCard->belCard->getName()->getFamilyName().c_str() : NULL;
	return result;
}

void linphone_vcard_set_given_name(LinphoneVcard *vCard, const char *name) {
	if (!vCard || !name) return;

	if (vCard->belCard->getName()) {
		vCard->belCard->getName()->setGivenName(name);
	} else {
		shared_ptr<belcard::BelCardName> n = belcard::BelCardGeneric::create<belcard::BelCardName>();
		n->setGivenName(name);
		vCard->belCard->setName(n);
	}
}

const char* linphone_vcard_get_given_name(const LinphoneVcard *vCard) {
	if (!vCard) return NULL;

	const char *result = vCard->belCard->getName() ? vCard->belCard->getName()->getGivenName().c_str() : NULL;
	return result;
}

void linphone_vcard_add_sip_address(LinphoneVcard *vCard, const char *sip_address) {
	if (!vCard || !sip_address) return;

	shared_ptr<belcard::BelCardImpp> impp = belcard::BelCardGeneric::create<belcard::BelCardImpp>();
	impp->setValue(sip_address);
	vCard->belCard->addImpp(impp);
}

void linphone_vcard_remove_sip_address(LinphoneVcard *vCard, const char *sip_address) {
	if (!vCard) return;

	shared_ptr<belcard::BelCardImpp> impp;
	for (auto it = vCard->belCard->getImpp().begin(); it != vCard->belCard->getImpp().end(); ++it) {
		const char *value = (*it)->getValue().c_str();
		if (strcmp(value, sip_address) == 0) {
			impp = *it;
			break;
		}
	}
	if (impp) {
		vCard->belCard->removeImpp(impp);
	}
}

void linphone_vcard_edit_main_sip_address(LinphoneVcard *vCard, const char *sip_address) {
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

const bctbx_list_t* linphone_vcard_get_sip_addresses(LinphoneVcard *vCard) {
	if (!vCard) return NULL;
	if (!vCard->sip_addresses_cache) {
		for (auto it = vCard->belCard->getImpp().begin(); it != vCard->belCard->getImpp().end(); ++it) {
			LinphoneAddress* addr = linphone_address_new((*it)->getValue().c_str());
			if (addr) {
				vCard->sip_addresses_cache = bctbx_list_append(vCard->sip_addresses_cache, addr);
			}
		}
	}
	return vCard->sip_addresses_cache;
}

void linphone_vcard_add_phone_number(LinphoneVcard *vCard, const char *phone) {
	if (!vCard || !phone) return;

	shared_ptr<belcard::BelCardPhoneNumber> phone_number = belcard::BelCardGeneric::create<belcard::BelCardPhoneNumber>();
	phone_number->setValue(phone);
	vCard->belCard->addPhoneNumber(phone_number);
}

void linphone_vcard_remove_phone_number(LinphoneVcard *vCard, const char *phone) {
	if (!vCard) return;

	shared_ptr<belcard::BelCardPhoneNumber> tel;
	for (auto it = vCard->belCard->getPhoneNumbers().begin(); it != vCard->belCard->getPhoneNumbers().end(); ++it) {
		const char *value = (*it)->getValue().c_str();
		if (strcmp(value, phone) == 0) {
			tel = *it;
			break;
		}
	}
	if (tel) {
		vCard->belCard->removePhoneNumber(tel);
	}
}

bctbx_list_t* linphone_vcard_get_phone_numbers(const LinphoneVcard *vCard) {
	bctbx_list_t *result = NULL;
	if (!vCard) return NULL;

	for (auto it = vCard->belCard->getPhoneNumbers().begin(); it != vCard->belCard->getPhoneNumbers().end(); ++it) {
		const char *value = (*it)->getValue().c_str();
		result = bctbx_list_append(result, (char *)value);
	}
	return result;
}

void linphone_vcard_set_organization(LinphoneVcard *vCard, const char *organization) {
	if (!vCard) return;

	if (vCard->belCard->getOrganizations().size() > 0) {
		const shared_ptr<belcard::BelCardOrganization> org = vCard->belCard->getOrganizations().front();
		org->setValue(organization);
	} else {
		shared_ptr<belcard::BelCardOrganization> org = belcard::BelCardGeneric::create<belcard::BelCardOrganization>();
		org->setValue(organization);
		vCard->belCard->addOrganization(org);
	}
}

const char* linphone_vcard_get_organization(const LinphoneVcard *vCard) {
	if (vCard && vCard->belCard->getOrganizations().size() > 0) {
		const shared_ptr<belcard::BelCardOrganization> org = vCard->belCard->getOrganizations().front();
		return org->getValue().c_str();
	}

	return NULL;
}

bool_t linphone_vcard_generate_unique_id(LinphoneVcard *vCard) {
	char uuid[64];

	if (vCard) {
		if (linphone_vcard_get_uid(vCard)) {
			return FALSE;
		}
		if (sal_generate_uuid(uuid, sizeof(uuid)) == 0) {
			char vcard_uuid[sizeof(uuid)+4];
			snprintf(vcard_uuid, sizeof(vcard_uuid), "urn:%s", uuid);
			linphone_vcard_set_uid(vCard, vcard_uuid);
			return TRUE;
		}
	}
	return FALSE;
}

void linphone_vcard_set_uid(LinphoneVcard *vCard, const char *uid) {
	if (!vCard || !uid) return;

	shared_ptr<belcard::BelCardUniqueId> uniqueId = belcard::BelCardGeneric::create<belcard::BelCardUniqueId>();
	uniqueId->setValue(uid);
	vCard->belCard->setUniqueId(uniqueId);
}

const char* linphone_vcard_get_uid(const LinphoneVcard *vCard) {
	if (vCard && vCard->belCard->getUniqueId()) {
		return vCard->belCard->getUniqueId()->getValue().c_str();
	}
	return NULL;
}

void linphone_vcard_set_etag(LinphoneVcard *vCard, const char * etag) {
	if (!vCard) {
		return;
	}
	if (vCard->etag) {
		ms_free(vCard->etag);
		vCard->etag = NULL;
	}
	vCard->etag = ms_strdup(etag);
}

const char* linphone_vcard_get_etag(const LinphoneVcard *vCard) {
	if (!vCard) return NULL;
	return vCard->etag;
}

void linphone_vcard_set_url(LinphoneVcard *vCard, const char * url) {
	if (!vCard) {
		return;
	}
	if (vCard->url) {
		ms_free(vCard->url);
		vCard->url = NULL;
	}
	vCard->url = ms_strdup(url);
}

const char* linphone_vcard_get_url(const LinphoneVcard *vCard) {
	if (!vCard) return NULL;
	return vCard->url;
}

void linphone_vcard_compute_md5_hash(LinphoneVcard *vCard) {
	const char *text = NULL;
	if (!vCard) return;
	text = linphone_vcard_as_vcard4_string(vCard);
	bctbx_md5((unsigned char *)text, strlen(text), vCard->md5);
}

bool_t linphone_vcard_compare_md5_hash(LinphoneVcard *vCard) {
	unsigned char previous_md5[VCARD_MD5_HASH_SIZE];
	memcpy(previous_md5, vCard->md5, VCARD_MD5_HASH_SIZE);
	linphone_vcard_compute_md5_hash(vCard);
	return memcmp(vCard->md5, previous_md5, VCARD_MD5_HASH_SIZE);
}

bool_t linphone_core_vcard_supported(void) {
	return TRUE;
}

void linphone_vcard_clean_cache(LinphoneVcard *vCard) {
	if (vCard->sip_addresses_cache) bctbx_list_free_with_data(vCard->sip_addresses_cache, (void (*)(void*))linphone_address_unref);
	vCard->sip_addresses_cache = NULL;
}

} // extern "C"
