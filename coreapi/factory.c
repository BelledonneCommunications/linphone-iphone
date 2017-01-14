/*
linphone
Copyright (C) 2016 Belledonne Communications <info@belledonne-communications.com>

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

#include "private.h"

extern LinphoneCore *_linphone_core_new_with_config(LinphoneCoreCbs *cbs, struct _LpConfig *config, void *userdata);
extern LinphoneCoreCbs *_linphone_core_cbs_new(void);
extern LinphoneAddress *_linphone_address_new(const char *addr);

typedef belle_sip_object_t_vptr_t LinphoneFactory_vptr_t;

struct _LinphoneFactory {
	belle_sip_object_t base;
};

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneFactory);
BELLE_SIP_INSTANCIATE_VPTR(LinphoneFactory, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // Marshall
	FALSE
);

static LinphoneFactory *_factory = NULL;

static void _linphone_factory_destroying_cb(void) {
	if (_factory != NULL) {
		belle_sip_object_unref(_factory);
		_factory = NULL;
	}
}

LinphoneFactory *linphone_factory_get(void) {
	if (_factory == NULL) {
		_factory = belle_sip_object_new(LinphoneFactory);
		atexit(_linphone_factory_destroying_cb);
	}
	return _factory;
}

void linphone_factory_clean(void){
	if (_factory){
		belle_sip_object_unref(_factory);
		_factory = NULL;
	}
}

LinphoneCore *linphone_factory_create_core(const LinphoneFactory *factory, LinphoneCoreCbs *cbs,
		const char *config_path, const char *factory_config_path) {
	LpConfig *config = lp_config_new_with_factory(config_path, factory_config_path);
	LinphoneCore *lc = _linphone_core_new_with_config(cbs, config, NULL);
	lp_config_unref(config);
	return lc;
}

LinphoneCore *linphone_factory_create_core_with_config(const LinphoneFactory *factory, LinphoneCoreCbs *cbs, LinphoneConfig *config) {
	return _linphone_core_new_with_config(cbs, config, NULL);
}

LinphoneCoreCbs *linphone_factory_create_core_cbs(const LinphoneFactory *factory) {
	return _linphone_core_cbs_new();
}

LinphoneAddress *linphone_factory_create_address(const LinphoneFactory *factory, const char *addr) {
	return _linphone_address_new(addr);
}

LinphoneAuthInfo *linphone_factory_create_auth_info(const LinphoneFactory *factory, const char *username, const char *userid, const char *passwd, const char *ha1, const char *realm, const char *domain) {
	return linphone_auth_info_new(username, userid, passwd, ha1, realm, domain);
}
