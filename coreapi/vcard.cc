#include "vcard.h"
#include "belcard/belcard.hpp"

struct _LinphoneVCard {
	shared_ptr<belcard::BelCard> belcard;
};

extern "C" LinphoneVCard* linphone_vcard_new(void) {
	LinphoneVCard* vcard = (LinphoneVCard*) malloc(sizeof(LinphoneVCard));
	vcard->belcard = belcard::BelCardGeneric::create<belcard::BelCard>();
	return vcard;
}

extern "C" void linphone_vcard_free(LinphoneVCard *vcard) {
	vcard->belcard.reset();
	free(vcard);
}

extern "C" void linphone_vcard_set_full_name(LinphoneVCard *vcard, const char *name) {
	shared_ptr<belcard::BelCardFullName> fn = belcard::BelCardGeneric::create<belcard::BelCardFullName>();
	fn->setValue(name);
	vcard->belcard->setFullName(fn);
}

extern "C" const char* linphone_vcard_get_full_name(LinphoneVCard *vcard) {
	return vcard->belcard->getFullName() ? vcard->belcard->getFullName()->getValue().c_str() : NULL;
}