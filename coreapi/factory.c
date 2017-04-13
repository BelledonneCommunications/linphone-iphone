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

#include "linphone/factory.h"
#include "private.h"

#ifndef PACKAGE_SOUND_DIR
#define PACKAGE_SOUND_DIR "."
#endif
#ifndef PACKAGE_RING_DIR
#define PACKAGE_RING_DIR "."
#endif

#ifndef PACKAGE_DATA_DIR
#define PACKAGE_DATA_DIR "."
#endif

extern LinphoneCore *_linphone_core_new_with_config(LinphoneCoreCbs *cbs, struct _LpConfig *config, void *userdata);
extern LinphoneAddress *_linphone_address_new(const char *addr);

typedef belle_sip_object_t_vptr_t LinphoneFactory_vptr_t;

struct _LinphoneFactory {
	belle_sip_object_t base;
	/*these are the directories set by the application*/
	char *top_resources_dir;
	char *data_resources_dir;
	char *sound_resources_dir;
	char *ring_resources_dir;
	char *image_resources_dir;
	char *msplugins_dir;
	
	/*these are the cached result computed from directories set by the application*/
	char *cached_data_resources_dir;
	char *cached_sound_resources_dir;
	char *cached_ring_resources_dir;
	char *cached_image_resources_dir;
	char *cached_msplugins_dir;
	LinphoneErrorInfo* ei;
};

static void linphone_factory_uninit(LinphoneFactory *obj){
	STRING_RESET(obj->top_resources_dir);
	STRING_RESET(obj->data_resources_dir);
	STRING_RESET(obj->sound_resources_dir);
	STRING_RESET(obj->ring_resources_dir);
	STRING_RESET(obj->image_resources_dir);
	STRING_RESET(obj->msplugins_dir);
	
	STRING_RESET(obj->cached_data_resources_dir);
	STRING_RESET(obj->cached_sound_resources_dir);
	STRING_RESET(obj->cached_ring_resources_dir);
	STRING_RESET(obj->cached_image_resources_dir);
	STRING_RESET(obj->cached_msplugins_dir);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneFactory);
BELLE_SIP_INSTANCIATE_VPTR(LinphoneFactory, belle_sip_object_t,
	linphone_factory_uninit, // destroy
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

static LinphoneFactory *linphone_factory_new(void){
	LinphoneFactory *factory = belle_sip_object_new(LinphoneFactory);
	factory->top_resources_dir = bctbx_strdup(PACKAGE_DATA_DIR);
	return factory;
}


LinphoneFactory *linphone_factory_get(void) {
	if (_factory == NULL) {
		_factory = linphone_factory_new();
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

LinphoneCallCbs * linphone_factory_create_call_cbs(const LinphoneFactory *factory) {
	return _linphone_call_cbs_new();
}

LinphoneVcard *linphone_factory_create_vcard(LinphoneFactory *factory) {
	return _linphone_vcard_new();
}

const char * linphone_factory_get_top_resources_dir(const LinphoneFactory *factory) {
	return factory->top_resources_dir;
}

void linphone_factory_set_top_resources_dir(LinphoneFactory *factory, const char *path) {
	STRING_SET(factory->top_resources_dir, path);
}

const char * linphone_factory_get_data_resources_dir(LinphoneFactory *factory) {
	if (factory->data_resources_dir) return factory->data_resources_dir;
	if (factory->top_resources_dir){
		STRING_TRANSFER(factory->cached_data_resources_dir, bctbx_strdup_printf("%s/linphone", factory->top_resources_dir));
	}else{
		STRING_TRANSFER(factory->cached_data_resources_dir, bctbx_strdup_printf("%s/linphone", PACKAGE_DATA_DIR));
	}
	return factory->cached_data_resources_dir;
}

void linphone_factory_set_data_resources_dir(LinphoneFactory *factory, const char *path) {
	STRING_SET(factory->data_resources_dir, path);
}

const char * linphone_factory_get_sound_resources_dir(LinphoneFactory *factory) {
	if (factory->sound_resources_dir) return factory->sound_resources_dir;
	if (factory->top_resources_dir){
		STRING_TRANSFER(factory->cached_sound_resources_dir, bctbx_strdup_printf("%s/sounds/linphone", factory->top_resources_dir));
		return factory->cached_sound_resources_dir;
	}
	return PACKAGE_SOUND_DIR;
}

void linphone_factory_set_sound_resources_dir(LinphoneFactory *factory, const char *path) {
	STRING_SET(factory->sound_resources_dir, path);
}

const char * linphone_factory_get_ring_resources_dir(LinphoneFactory *factory) {
	if (factory->ring_resources_dir) return factory->ring_resources_dir;
	if (factory->sound_resources_dir){
		STRING_TRANSFER(factory->cached_sound_resources_dir, bctbx_strdup_printf("%s/rings", factory->sound_resources_dir));
		return factory->cached_sound_resources_dir;
	}
	return PACKAGE_RING_DIR;
}

void linphone_factory_set_ring_resources_dir(LinphoneFactory *factory, const char *path) {
	STRING_SET(factory->ring_resources_dir, path);
}

const char * linphone_factory_get_image_resources_dir(LinphoneFactory *factory) {
	if (factory->image_resources_dir) return factory->image_resources_dir;
	if (factory->top_resources_dir) {
		STRING_TRANSFER(factory->cached_image_resources_dir, bctbx_strdup_printf("%s/images", factory->top_resources_dir));
	}else{
		STRING_TRANSFER(factory->cached_image_resources_dir, bctbx_strdup_printf("%s/images", PACKAGE_DATA_DIR));
	}
	return factory->cached_image_resources_dir;
}

void linphone_factory_set_image_resources_dir(LinphoneFactory *factory, const char *path) {
	STRING_SET(factory->image_resources_dir, path);
}

const char * linphone_factory_get_msplugins_dir(LinphoneFactory *factory) {
	return factory->msplugins_dir;
}

void linphone_factory_set_msplugins_dir(LinphoneFactory *factory, const char *path) {
	STRING_SET(factory->msplugins_dir, path);
}

LinphoneErrorInfo *linphone_factory_create_error_info(LinphoneFactory *factory){
	
	return linphone_error_info_new();
	
}
