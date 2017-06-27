/*
linphone
Copyright (C) 2010-2017 Belledonne Communications SARL

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

#include "linphone/account_creator.h"
#include "linphone/core.h"
#include "linphone/lpconfig.h"
#include "private.h"
#if !_WIN32
#include "regex.h"
#endif

#include <bctoolbox/crypto.h>

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneAccountCreatorCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneAccountCreatorCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);

/************************** Start Misc **************************/
static const char* ha1_for_passwd(const char* username, const char* realm, const char* passwd) {
	static char ha1[33];
	sal_auth_compute_ha1(username, realm, passwd, ha1);
	return ha1;
}

static unsigned int validate_uri(const char* username, const char* domain, const char* display_name) {
	LinphoneAddress* addr;
	int status = 0;
	LinphoneProxyConfig* proxy = linphone_proxy_config_new();
	linphone_proxy_config_set_identity(proxy, "sip:?@domain.com");

	if (username) {
		addr = linphone_proxy_config_normalize_sip_uri(proxy, username);
	} else {
		addr = linphone_address_clone(linphone_proxy_config_get_identity_address(proxy));
	}

	if (addr == NULL) {
		status = 1;
		goto end;
	}

	if (domain && linphone_address_set_domain(addr, domain) != 0) {
		status = 1;
	}

	if (display_name && (!strlen(display_name) || linphone_address_set_display_name(addr, display_name) != 0)) {
		status = 1;
	}
	linphone_address_unref(addr);
end:
	linphone_proxy_config_destroy(proxy);
	return status;
}

static char* _get_identity(const LinphoneAccountCreator *creator) {
	char *identity = NULL;
	if ((creator->username || creator->phone_number)) {
		//we must escape username
		LinphoneProxyConfig* proxy = linphone_core_create_proxy_config(creator->core);
		LinphoneAddress* addr;

		addr = linphone_proxy_config_normalize_sip_uri(proxy, creator->username ? creator->username : creator->phone_number);
		if (addr == NULL) goto end;

		identity = linphone_address_as_string(addr);
		linphone_address_unref(addr);
		end:
		linphone_proxy_config_destroy(proxy);
	}
	return identity;
}

static bool_t is_matching_regex(const char *entry, const char* regex) {
#if _WIN32
	return TRUE;
#else
	regex_t regex_pattern;
	char err_msg[256];
	int res;
	res = regcomp(&regex_pattern, regex, REG_EXTENDED | REG_NOSUB);
	if(res != 0) {
		regerror(res, &regex_pattern, err_msg, sizeof(err_msg));
		ms_error("Could not compile regex '%s: %s", regex, err_msg);
		return FALSE;
	}
	res = regexec(&regex_pattern, entry, 0, NULL, 0);
	regfree(&regex_pattern);
	return (res != REG_NOMATCH);
#endif
}

LinphoneProxyConfig * linphone_account_creator_create_proxy_config(const LinphoneAccountCreator *creator) {
	LinphoneAuthInfo *info;
	LinphoneProxyConfig *cfg = linphone_core_create_proxy_config(creator->core);
	char *identity_str = _get_identity(creator);
	LinphoneAddress *identity = linphone_address_new(identity_str);

	ms_free(identity_str);
	if (creator->display_name) {
		linphone_address_set_display_name(identity, creator->display_name);
	}
	linphone_proxy_config_set_identity_address(cfg, identity);
	if (creator->phone_country_code) {
		linphone_proxy_config_set_dial_prefix(cfg, creator->phone_country_code);
	} else if (creator->phone_number) {
		int dial_prefix_number = linphone_dial_plan_lookup_ccc_from_e164(creator->phone_number);
		char buff[4];
		snprintf(buff, sizeof(buff), "%d", dial_prefix_number);
		linphone_proxy_config_set_dial_prefix(cfg, buff);
	}

	linphone_proxy_config_enable_register(cfg, TRUE);

	info = linphone_auth_info_new(linphone_address_get_username(identity), // username
								NULL, //user id
								creator->password, // passwd
								creator->password ? NULL : creator->ha1,  // ha1
								!creator->password && creator->ha1 ? linphone_address_get_domain(identity) : NULL,  // realm - assumed to be domain
								linphone_address_get_domain(identity) // domain
	);
	linphone_core_add_auth_info(creator->core, info);
	linphone_address_unref(identity);

	if (linphone_core_add_proxy_config(creator->core, cfg) != -1) {
		linphone_core_set_default_proxy(creator->core, cfg);
		return cfg;
	}

	linphone_core_remove_auth_info(creator->core, info);
	return NULL;
}

LinphoneProxyConfig * linphone_account_creator_configure(const LinphoneAccountCreator *creator) {
	return linphone_account_creator_create_proxy_config(creator);
}
/************************** End Misc **************************/

/************************** Start Account Creator Cbs **************************/

static LinphoneAccountCreatorCbs * linphone_account_creator_cbs_new(void) {
	return belle_sip_object_new(LinphoneAccountCreatorCbs);
}

LinphoneAccountCreatorCbs * linphone_account_creator_cbs_ref(LinphoneAccountCreatorCbs *cbs) {
	belle_sip_object_ref(cbs);
	return cbs;
}

void linphone_account_creator_cbs_unref(LinphoneAccountCreatorCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void *linphone_account_creator_cbs_get_user_data(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->user_data;
}

void linphone_account_creator_cbs_set_user_data(LinphoneAccountCreatorCbs *cbs, void *ud) {
	cbs->user_data = ud;
}

LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_create_account(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->create_account_response_cb;
}

void linphone_account_creator_cbs_set_create_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb) {
	cbs->create_account_response_cb = cb;
}

LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_account_exist(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->is_account_exist_response_cb;
}

void linphone_account_creator_cbs_set_is_account_exist(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb) {
	cbs->is_account_exist_response_cb = cb;
}

LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_activate_account(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->activate_account_response_cb;
}

void linphone_account_creator_cbs_set_activate_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb) {
	cbs->activate_account_response_cb = cb;
}

LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_account_activated(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->is_account_activated_response_cb;
}

void linphone_account_creator_cbs_set_is_account_activated(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb) {
	cbs->is_account_activated_response_cb = cb;
}

LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_link_account(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->link_account_response_cb;
}

void linphone_account_creator_cbs_set_link_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb) {
	cbs->link_account_response_cb = cb;
}

LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_activate_alias(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->activate_alias_response_cb;
}

void linphone_account_creator_cbs_set_activate_alias(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb) {
	cbs->activate_alias_response_cb = cb;
}

LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_alias_used(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->is_alias_used_response_cb;
}

void linphone_account_creator_cbs_set_is_alias_used(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb) {
	cbs->is_alias_used_response_cb = cb;
}

LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_is_account_linked(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->is_account_linked_response_cb;
}

void linphone_account_creator_cbs_set_is_account_linked(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb) {
	cbs->is_account_linked_response_cb = cb;
}

LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_recover_account(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->recover_account_response_cb;
}

void linphone_account_creator_cbs_set_recover_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb) {
	cbs->recover_account_response_cb = cb;
}

LinphoneAccountCreatorCbsStatusCb linphone_account_creator_cbs_get_update_account(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->update_account_response_cb;
}

void linphone_account_creator_cbs_set_update_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsStatusCb cb) {
	cbs->update_account_response_cb = cb;
}
/************************** End Account Creator Cbs **************************/

/************************** Start Account Creator data **************************/
static void _linphone_account_creator_destroy(LinphoneAccountCreator *creator) {
	/*this will drop all pending requests if any*/
	if (creator->xmlrpc_session) linphone_xml_rpc_session_release(creator->xmlrpc_session);
	if (creator->service != NULL && linphone_account_creator_service_get_destructor_cb(creator->service) != NULL)
		linphone_account_creator_service_get_destructor_cb(creator->service)(creator);
	linphone_account_creator_cbs_unref(creator->cbs);
	linphone_proxy_config_unref(creator->proxy_cfg);
	linphone_account_creator_reset(creator);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneAccountCreator);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneAccountCreator, belle_sip_object_t,
	(belle_sip_object_destroy_t)_linphone_account_creator_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);

LinphoneAccountCreator * _linphone_account_creator_new(LinphoneCore *core, const char *xmlrpc_url) {
	LinphoneAccountCreator *creator;
	const char* domain = lp_config_get_string(core->config, "assistant", "domain", NULL);
	creator = belle_sip_object_new(LinphoneAccountCreator);
	creator->service = linphone_core_get_account_creator_service(core);
	creator->cbs = linphone_account_creator_cbs_new();
	creator->core = core;
	creator->transport = LinphoneTransportTcp;
	creator->xmlrpc_session = (xmlrpc_url) ? linphone_xml_rpc_session_new(core, xmlrpc_url) : NULL;
	if (domain) {
		linphone_account_creator_set_domain(creator, domain);
	}
	creator->proxy_cfg = linphone_core_create_proxy_config(core);
	if (creator->service != NULL && linphone_account_creator_service_get_constructor_cb(creator->service) != NULL)
		linphone_account_creator_service_get_constructor_cb(creator->service)(creator);
	return creator;
}

LinphoneAccountCreator * linphone_account_creator_new(LinphoneCore *core, const char *xmlrpc_url) {
	return _linphone_account_creator_new(core, xmlrpc_url);
}

#define _reset_field(field) \
	if (field) { \
		ms_free(field); \
		field = NULL; \
	}

void linphone_account_creator_reset(LinphoneAccountCreator *creator) {
	_reset_field(creator->username);
	_reset_field(creator->display_name);
	_reset_field(creator->password);
	_reset_field(creator->ha1);
	_reset_field(creator->phone_number);
	_reset_field(creator->phone_country_code);
	_reset_field(creator->email);
	_reset_field(creator->language);
	_reset_field(creator->activation_code);
	_reset_field(creator->domain);
	_reset_field(creator->route);
}

#undef _reset_field

LinphoneAccountCreator * linphone_core_create_account_creator(LinphoneCore *core, const char *xmlrpc_url) {
	return _linphone_account_creator_new(core, xmlrpc_url);
}

LinphoneAccountCreator * linphone_account_creator_ref(LinphoneAccountCreator *creator) {
	belle_sip_object_ref(creator);
	return creator;
}

void linphone_account_creator_unref(LinphoneAccountCreator *creator) {
	belle_sip_object_unref(creator);
}

void *linphone_account_creator_get_user_data(const LinphoneAccountCreator *creator) {
	return creator->user_data;
}

void linphone_account_creator_set_user_data(LinphoneAccountCreator *creator, void *ud) {
	creator->user_data = ud;
}

LinphoneAccountCreatorUsernameStatus linphone_account_creator_set_username(LinphoneAccountCreator *creator, const char *username) {
	int min_length = lp_config_get_int(creator->core->config, "assistant", "username_min_length", -1);
	int max_length = lp_config_get_int(creator->core->config, "assistant", "username_max_length", -1);
	bool_t use_phone_number = lp_config_get_int(creator->core->config, "assistant", "use_phone_number", 0);
	const char* regex = lp_config_get_string(creator->core->config, "assistant", "username_regex", 0);
	if (!username) {
		creator->username = NULL;
		return LinphoneAccountCreatorUsernameStatusOk;
	} else if (min_length > 0 && strlen(username) < (size_t)min_length) {
		return LinphoneAccountCreatorUsernameStatusTooShort;
	} else if (max_length > 0 && strlen(username) > (size_t)max_length) {
		return LinphoneAccountCreatorUsernameStatusTooLong;
	} else if (use_phone_number && !linphone_proxy_config_is_phone_number(NULL, username)) {
		return LinphoneAccountCreatorUsernameStatusInvalid;
	} else if (regex && !is_matching_regex(username, regex)) {
		return LinphoneAccountCreatorUsernameStatusInvalidCharacters;
	} else if (validate_uri(username, NULL, NULL) != 0) {
		return LinphoneAccountCreatorUsernameStatusInvalid;
	}

	set_string(&creator->username, username, TRUE);
	return LinphoneAccountCreatorUsernameStatusOk;
}

const char * linphone_account_creator_get_username(const LinphoneAccountCreator *creator) {
	return creator->username;
}

LinphoneAccountCreatorPhoneNumberStatusMask linphone_account_creator_set_phone_number(LinphoneAccountCreator *creator, const char *phone_number, const char *country_code) {
	char *normalized_phone_number;
	LinphoneAccountCreatorPhoneNumberStatusMask return_status = 0;
	if (!phone_number || !country_code) {
		if (!phone_number && !country_code) {
			creator->phone_number = NULL;
			creator->phone_country_code = NULL;
			return (LinphoneAccountCreatorPhoneNumberStatusMask)LinphoneAccountCreatorPhoneNumberStatusOk;
		} else {
			return (LinphoneAccountCreatorPhoneNumberStatusMask)LinphoneAccountCreatorPhoneNumberStatusInvalid;
		}
	} else {
		LinphoneProxyConfig *numCfg = creator->proxy_cfg;
		creator->phone_country_code = ms_strdup(country_code[0] == '+' ? &country_code[1] : country_code);
		linphone_proxy_config_set_dial_prefix(numCfg, creator->phone_country_code);
		normalized_phone_number = linphone_proxy_config_normalize_phone_number(numCfg, phone_number);
		if (!normalized_phone_number) {
			return LinphoneAccountCreatorPhoneNumberStatusInvalid;
		}

		// if phone is valid, we lastly want to check that length is OK
		{
			const LinphoneDialPlan* plan = linphone_dial_plan_by_ccc(creator->phone_country_code);
			int size = (int)strlen(phone_number);
			if (linphone_dial_plan_is_generic(plan)) {
				return_status = LinphoneAccountCreatorPhoneNumberStatusInvalidCountryCode;
			}
			if (size < plan->nnl - 1) {
				return_status += LinphoneAccountCreatorPhoneNumberStatusTooShort;
				goto end;
			} else if (size > plan->nnl + 1) {
				return_status += LinphoneAccountCreatorPhoneNumberStatusTooLong;
				goto end;
			} else if (return_status & LinphoneAccountCreatorPhoneNumberStatusInvalidCountryCode) {
				goto end;
			}
		}
	}
	set_string(&creator->phone_number, normalized_phone_number, TRUE);
	return_status = LinphoneAccountCreatorPhoneNumberStatusOk;
end:
	ms_free(normalized_phone_number);
	return return_status;
}

const char * linphone_account_creator_get_phone_number(const LinphoneAccountCreator *creator) {
	return creator->phone_number;
}

LinphoneAccountCreatorPasswordStatus linphone_account_creator_set_password(LinphoneAccountCreator *creator, const char *password) {
	int min_length = lp_config_get_int(creator->core->config, "assistant", "password_min_length", -1);
	int max_length = lp_config_get_int(creator->core->config, "assistant", "password_max_length", -1);
	if (!password) {
		creator->password = NULL;
		return LinphoneAccountCreatorPasswordStatusTooShort;
	}
	if (min_length > 0 && strlen(password) < (size_t)min_length) {
		return LinphoneAccountCreatorPasswordStatusTooShort;
	} else if (max_length > 0 && strlen(password) > (size_t)max_length) {
		return LinphoneAccountCreatorPasswordStatusTooLong;
	}
	set_string(&creator->password, password, FALSE);
	return LinphoneAccountCreatorPasswordStatusOk;
}

const char * linphone_account_creator_get_password(const LinphoneAccountCreator *creator) {
	return creator->password;
}

LinphoneAccountCreatorPasswordStatus linphone_account_creator_set_ha1(LinphoneAccountCreator *creator, const char *ha1){
	set_string(&creator->ha1, ha1, FALSE);
	return LinphoneAccountCreatorPasswordStatusOk;
}

const char * linphone_account_creator_get_ha1(const LinphoneAccountCreator *creator) {
	return creator->ha1;
}

LinphoneAccountCreatorActivationCodeStatus linphone_account_creator_set_activation_code(LinphoneAccountCreator *creator, const char *activation_code){
	set_string(&creator->activation_code, activation_code, FALSE);
	return LinphoneAccountCreatorActivationCodeStatusOk;
}

const char * linphone_account_creator_get_activation_code(const LinphoneAccountCreator *creator) {
	return creator->activation_code;
}

LinphoneAccountCreatorLanguageStatus linphone_account_creator_set_language(LinphoneAccountCreator *creator, const char *lang) {
	set_string(&creator->language, lang, FALSE);
	return LinphoneAccountCreatorLanguageStatusOk;
}

const char * linphone_account_creator_get_language(const LinphoneAccountCreator *creator) {
	return creator->language;
}

LinphoneAccountCreatorUsernameStatus linphone_account_creator_set_display_name(LinphoneAccountCreator *creator, const char *display_name) {
	if (validate_uri(NULL, display_name, NULL) != 0) {
		return LinphoneAccountCreatorUsernameStatusInvalid;
	}
	set_string(&creator->display_name, display_name, FALSE);
	return LinphoneAccountCreatorUsernameStatusOk;
}

const char * linphone_account_creator_get_display_name(const LinphoneAccountCreator *creator) {
	return creator->display_name;
}

LinphoneAccountCreatorEmailStatus linphone_account_creator_set_email(LinphoneAccountCreator *creator, const char *email) {
	if (!email || !is_matching_regex(email, "^.+@.+\\..*$")) {
		return LinphoneAccountCreatorEmailStatusMalformed;
	}
	if (!is_matching_regex(email, "^.+@.+\\.[A-Za-z]{2}[A-Za-z]*$")) {
		return LinphoneAccountCreatorEmailStatusInvalidCharacters;
	}
	set_string(&creator->email, email, TRUE);
	return LinphoneAccountCreatorEmailStatusOk;
}

const char * linphone_account_creator_get_email(const LinphoneAccountCreator *creator) {
	return creator->email;
}

LinphoneAccountCreatorDomainStatus linphone_account_creator_set_domain(LinphoneAccountCreator *creator, const char *domain) {
	if (domain && validate_uri(NULL, domain, NULL) != 0) {
		return LinphoneAccountCreatorDomainInvalid;
	}

	set_string(&creator->domain, domain, TRUE);
	return LinphoneAccountCreatorDomainOk;
 }

const char * linphone_account_creator_get_domain(const LinphoneAccountCreator *creator) {
	return creator->domain;
}

LinphoneAccountCreatorTransportStatus linphone_account_creator_set_transport(LinphoneAccountCreator *creator, LinphoneTransportType transport) {
	if (!linphone_core_sip_transport_supported(creator->core, transport)) {
		return LinphoneAccountCreatorTransportUnsupported;
	}
	creator->transport = transport;
	return LinphoneAccountCreatorTransportOk;
}

LinphoneTransportType linphone_account_creator_get_transport(const LinphoneAccountCreator *creator) {
	return creator->transport;
}

LinphoneAccountCreatorStatus linphone_account_creator_set_route(LinphoneAccountCreator *creator, const char *route) {
	if (!route || linphone_proxy_config_set_route(creator->proxy_cfg, route) != 0)
		return LinphoneAccountCreatorStatusRequestFailed;

	set_string(&creator->route, route, TRUE);
	return LinphoneAccountCreatorStatusRequestOk;
}

const char * linphone_account_creator_get_route(const LinphoneAccountCreator *creator) {
	return creator->route;
}

LinphoneAccountCreatorCbs * linphone_account_creator_get_callbacks(const LinphoneAccountCreator *creator) {
	return creator->cbs;
}

LinphoneAccountCreatorService * linphone_account_creator_get_service(const LinphoneAccountCreator *creator) {
	return creator->service;
}

LinphoneAccountCreatorStatus linphone_account_creator_is_account_exist(LinphoneAccountCreator *creator) {
	if (creator->service->is_account_exist_request_cb == NULL
		|| creator->cbs->is_account_exist_response_cb == NULL) {
		return LinphoneAccountCreatorStatusMissingCallbacks;
	}

	return creator->service->is_account_exist_request_cb(creator);
}

LinphoneAccountCreatorStatus linphone_account_creator_create_account(LinphoneAccountCreator *creator) {
	if (creator->service->create_account_request_cb == NULL
		|| creator->cbs->create_account_response_cb == NULL) {
		return LinphoneAccountCreatorStatusMissingCallbacks;
	}

	return creator->service->create_account_request_cb(creator);
}

LinphoneAccountCreatorStatus linphone_account_creator_is_account_activated(LinphoneAccountCreator *creator) {
	if (creator->service->is_account_activated_request_cb == NULL
		|| creator->cbs->is_account_activated_response_cb == NULL) {
		return LinphoneAccountCreatorStatusMissingCallbacks;
	}

	return creator->service->is_account_activated_request_cb(creator);
}

LinphoneAccountCreatorStatus linphone_account_creator_activate_account(LinphoneAccountCreator *creator) {
	if (creator->service->activate_account_request_cb == NULL
		|| creator->cbs->activate_account_response_cb == NULL) {
		return LinphoneAccountCreatorStatusMissingCallbacks;
	}

	return creator->service->activate_account_request_cb(creator);
}

LinphoneAccountCreatorStatus linphone_account_creator_link_account(LinphoneAccountCreator *creator) {
	if (creator->service->link_account_request_cb == NULL
		|| creator->cbs->link_account_response_cb == NULL) {
		return LinphoneAccountCreatorStatusMissingCallbacks;
	}

	return creator->service->link_account_request_cb(creator);
}

LinphoneAccountCreatorStatus linphone_account_creator_activate_alias(LinphoneAccountCreator *creator) {
	if (creator->service->activate_alias_request_cb == NULL
		|| creator->cbs->activate_alias_response_cb == NULL) {
		return LinphoneAccountCreatorStatusMissingCallbacks;
	}

	return creator->service->activate_alias_request_cb(creator);
}

LinphoneAccountCreatorStatus linphone_account_creator_is_alias_used(LinphoneAccountCreator *creator) {
	if (creator->service->is_alias_used_request_cb == NULL
		|| creator->cbs->is_alias_used_response_cb == NULL) {
		return LinphoneAccountCreatorStatusMissingCallbacks;
	}

	return creator->service->is_alias_used_request_cb(creator);
}

LinphoneAccountCreatorStatus linphone_account_creator_is_account_linked(LinphoneAccountCreator *creator) {
	if (creator->service->is_account_linked_request_cb == NULL
		|| creator->cbs->is_account_linked_response_cb == NULL) {
		return LinphoneAccountCreatorStatusMissingCallbacks;
	}

	return creator->service->is_account_linked_request_cb(creator);
}

LinphoneAccountCreatorStatus linphone_account_creator_recover_account(LinphoneAccountCreator *creator) {
	if (creator->service->recover_account_request_cb == NULL
		|| creator->cbs->recover_account_response_cb == NULL) {
		return LinphoneAccountCreatorStatusMissingCallbacks;
	}

	return creator->service->recover_account_request_cb(creator);
}

LinphoneAccountCreatorStatus linphone_account_creator_update_account(LinphoneAccountCreator *creator) {
	if (creator->service->update_account_request_cb == NULL
		|| creator->cbs->update_account_response_cb == NULL) {
		return LinphoneAccountCreatorStatusMissingCallbacks;
	}

	return creator->service->update_account_request_cb(creator);
}
/************************** End Account Creator data **************************/

/************************** Start Account Creator Linphone **************************/

LinphoneAccountCreatorStatus linphone_account_creator_constructor_linphone(LinphoneAccountCreator *creator) {
	LinphoneAddress *addr;
	const char *identity = lp_config_get_default_string(creator->core->config, "proxy", "reg_identity", NULL);
	const char *proxy = lp_config_get_default_string(creator->core->config, "proxy", "reg_proxy", NULL);
	const char *route = lp_config_get_default_string(creator->core->config, "proxy", "reg_route", NULL);
	const char *realm = lp_config_get_default_string(creator->core->config, "proxy", "realm", NULL);
	linphone_proxy_config_set_realm(creator->proxy_cfg, realm ? realm : "sip.linphone.org");
	linphone_proxy_config_set_route(creator->proxy_cfg, route ? route : "sip.linphone.org");
	linphone_proxy_config_set_server_addr(creator->proxy_cfg, proxy ? proxy : "sip.linphone.org");
	addr = linphone_address_new(identity ? identity : "sip:username@sip.linphone.org");
	linphone_proxy_config_set_identity_address(creator->proxy_cfg, addr);
	linphone_address_unref(addr);
	return LinphoneAccountCreatorStatusRequestOk;
}

/****************** START OF ACCOUNT USED SECTION *****************************/
static void _is_account_exist_response_cb(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->cbs->is_account_exist_response_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "ERROR_ACCOUNT_DOESNT_EXIST") == 0) ? LinphoneAccountCreatorStatusAccountNotExist : (
							(strcmp(resp, "ERROR_ALIAS_DOESNT_EXIST") == 0) ? LinphoneAccountCreatorStatusAccountExist :
														LinphoneAccountCreatorStatusAccountExistWithAlias);
			if (status == LinphoneAccountCreatorStatusAccountExistWithAlias) {
				set_string(&creator->phone_number, resp, FALSE);
			}
		}
		creator->cbs->is_account_exist_response_cb(creator, status, resp);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_is_account_exist_linphone(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->username && !creator->phone_number) {
		if (creator->cbs->is_account_exist_response_cb != NULL) {
			creator->cbs->is_account_exist_response_cb(creator, LinphoneAccountCreatorStatusMissingArguments, "Missing required parameters");
		}
		return LinphoneAccountCreatorStatusMissingArguments;
	}

	ms_debug("Account creator: is_account_exist (%s=%s, domain=%s)",
		(creator->username) ? "username" : "phone number",
		(creator->username) ? creator->username : creator->phone_number,
		linphone_proxy_config_get_domain(creator->proxy_cfg));
	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "get_phone_number_for_account",
		LinphoneXmlRpcArgString, creator->username ? creator->username : creator->phone_number,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);

	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _is_account_exist_response_cb);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);

	return LinphoneAccountCreatorStatusRequestOk;
}
/****************** END OF CREATE ACCOUNT USED SECTION ************************/

/****************** START OF CREATE ACCOUNT SECTION ***************************/
static void _create_account_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->cbs->create_account_response_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "OK") == 0) ? LinphoneAccountCreatorStatusAccountCreated
			: (strcmp(resp, "ERROR_CANNOT_SEND_SMS") == 0) ? LinphoneAccountCreatorStatusServerError
			: (strcmp(resp, "ERROR_ACCOUNT_ALREADY_IN_USE") == 0) ? LinphoneAccountCreatorStatusAccountExist
			: (strcmp(resp, "ERROR_ALIAS_ALREADY_IN_USE") == 0) ? LinphoneAccountCreatorStatusAccountExistWithAlias
			:LinphoneAccountCreatorStatusAccountNotCreated;
		}
		creator->cbs->create_account_response_cb(creator, status, resp);
	}
}

static LinphoneXmlRpcRequest * _create_account_with_phone_custom(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->phone_number) {
		return NULL;
	}
	ms_debug("Account creator: create_account_with_phone (phone number=%s, username=%s, domain=%s, language=%s)",
		creator->phone_number,
		(creator->username) ? creator->username : creator->phone_number,
		linphone_proxy_config_get_domain(creator->proxy_cfg),
		creator->language);

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "create_phone_account",
		LinphoneXmlRpcArgString, creator->phone_number,
		LinphoneXmlRpcArgString, creator->username ? creator->username : creator->phone_number,
		LinphoneXmlRpcArgString, creator->password ? ha1_for_passwd(creator->username ? creator->username : creator->phone_number, linphone_proxy_config_get_domain(creator->proxy_cfg), creator->password) : "",
		LinphoneXmlRpcArgString, linphone_core_get_user_agent(creator->core),
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgString, creator->language,
		LinphoneXmlRpcArgNone);
	return request;
}

static LinphoneXmlRpcRequest * _create_account_with_email_custom(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->username || !creator->email || !creator->password) {
		return NULL;
	}
	ms_debug("Account creator: create_account_with_email (username=%s, email=%s, domain=%s)",
		creator->username,
		creator->email,
		linphone_proxy_config_get_domain(creator->proxy_cfg));

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "create_email_account",
		LinphoneXmlRpcArgString, creator->username,
		LinphoneXmlRpcArgString, creator->email,
		LinphoneXmlRpcArgString, ha1_for_passwd(creator->username ? creator->username : creator->phone_number, linphone_proxy_config_get_domain(creator->proxy_cfg), creator->password),
		LinphoneXmlRpcArgString, linphone_core_get_user_agent(creator->core),
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);
	return request;
}

LinphoneAccountCreatorStatus linphone_account_creator_create_account_linphone(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	if (!identity || (!(request = _create_account_with_phone_custom(creator))
		&& !(request = _create_account_with_email_custom(creator)))) {
		if (creator->cbs->create_account_response_cb != NULL) {
			creator->cbs->create_account_response_cb(creator, LinphoneAccountCreatorStatusMissingArguments, "Missing required parameters");
		}
		if (identity) ms_free(identity);
		return LinphoneAccountCreatorStatusMissingArguments;
	}

	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _create_account_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneAccountCreatorStatusRequestOk;
}
/****************** END OF CREATE ACCOUNT SECTION *****************************/

/****************** START OF VALIDATE ACCOUNT SECTION *************************/
static void _activate_account_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->cbs->activate_account_response_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			if (strcmp(resp, "ERROR_ACCOUNT_ALREADY_ACTIVATED") == 0) {
				status = LinphoneAccountCreatorStatusAccountAlreadyActivated;
			} else if (strstr(resp, "ERROR_") == resp) {
				status = LinphoneAccountCreatorStatusAccountNotActivated;
			} else {
				status = LinphoneAccountCreatorStatusAccountActivated;
				set_string(&creator->ha1, resp, FALSE);
			}
		}
		creator->cbs->activate_account_response_cb(creator, status, resp);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_activate_account_linphone(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->phone_number || !creator->activation_code) {
		if (creator->cbs->is_account_activated_response_cb != NULL) {
			creator->cbs->is_account_activated_response_cb(creator, LinphoneAccountCreatorStatusMissingArguments, "Missing required parameters");
		}
		return LinphoneAccountCreatorStatusMissingArguments;
	}
	ms_debug("Account creator: activate_account_phone (phone number=%s, username=%s, activation code=%s, domain=%s)",
		creator->phone_number,
		creator->username ? creator->username : creator->phone_number,
		creator->activation_code,
		linphone_proxy_config_get_domain(creator->proxy_cfg));

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "activate_phone_account",
		LinphoneXmlRpcArgString, creator->phone_number,
		LinphoneXmlRpcArgString, creator->username ? creator->username : creator->phone_number,
		LinphoneXmlRpcArgString, creator->activation_code,
		linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);

	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _activate_account_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	return LinphoneAccountCreatorStatusRequestOk;
}

LinphoneAccountCreatorStatus linphone_account_creator_activate_email_account_linphone(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->activation_code || !creator->username) {
		if (creator->cbs->is_account_activated_response_cb != NULL) {
			creator->cbs->is_account_activated_response_cb(creator, LinphoneAccountCreatorStatusMissingArguments, "Missing required parameters");
		}
		return LinphoneAccountCreatorStatusMissingArguments;
	}

	ms_debug("Account creator: activate_account_email (username=%s, activation code=%s, domain=%s)",
		creator->username,
		creator->activation_code,
		linphone_proxy_config_get_domain(creator->proxy_cfg));

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "activate_email_account",
		LinphoneXmlRpcArgString, creator->username,
		LinphoneXmlRpcArgString, creator->activation_code,
		linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);

	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _activate_account_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	return LinphoneAccountCreatorStatusRequestOk;
}
/****************** END OF CREATE VALIDATE ACCOUNT SECTION ********************/

/****************** START OF ACCOUNT VALIDATED SECTION ************************/
static void _is_account_activated_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->cbs->is_account_activated_response_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "OK") == 0) ? LinphoneAccountCreatorStatusAccountActivated : LinphoneAccountCreatorStatusAccountNotActivated;
		}
		creator->cbs->is_account_activated_response_cb(creator, status, resp);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_is_account_activated_linphone(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	if (!identity) {
		if (creator->cbs->is_account_activated_response_cb != NULL) {
			creator->cbs->is_account_activated_response_cb(creator, LinphoneAccountCreatorStatusMissingArguments, "Missing required parameters");
		}
		return LinphoneAccountCreatorStatusMissingArguments;
	}
	ms_debug("Account creator: is_account_activated (username=%s, domain=%s)",
		creator->username ? creator->username : creator->phone_number,
		linphone_proxy_config_get_domain(creator->proxy_cfg));

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "is_account_activated",
		LinphoneXmlRpcArgString, creator->username ? creator->username : creator->phone_number,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _is_account_activated_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneAccountCreatorStatusRequestOk;
}
/****************** END OF CREATE ACCOUNT VALIDATED SECTION********************/

/****************** START OF PHONE NUMBER VALIDATED SECTION *******************/

static void _is_phone_number_used_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->cbs->is_alias_used_response_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "OK_ACCOUNT") == 0) ? LinphoneAccountCreatorStatusAliasIsAccount
			: (strcmp(resp, "OK_ALIAS") == 0) ? LinphoneAccountCreatorStatusAliasExist
			: LinphoneAccountCreatorStatusAliasNotExist;
		}
		creator->cbs->is_alias_used_response_cb(creator, status, resp);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_is_phone_number_used_linphone(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	if (!identity) {
		if (creator->cbs->is_alias_used_response_cb != NULL) {
			creator->cbs->is_alias_used_response_cb(creator, LinphoneAccountCreatorStatusMissingArguments, "Missing required parameters");
		}
		return LinphoneAccountCreatorStatusMissingArguments;
	}

	ms_debug("Account creator: is_phone_number_used (phone number=%s, domain=%s)",
		creator->phone_number,
		linphone_proxy_config_get_domain(creator->proxy_cfg));

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "is_phone_number_used",
		LinphoneXmlRpcArgString, creator->phone_number,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _is_phone_number_used_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneAccountCreatorStatusRequestOk;
}

/****************** END OF PHONE NUMBER VALIDATED SECTION *********************/

/****************** START OF LINK PHONE NUMBER WITH ACCOUNT SECTION ***********/
static void _link_phone_number_with_account_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->cbs->link_account_response_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "OK") == 0) ? LinphoneAccountCreatorStatusRequestOk : LinphoneAccountCreatorStatusAccountNotLinked;
		}
		creator->cbs->link_account_response_cb(creator, status, resp);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_link_phone_number_with_account_linphone(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->phone_number || !creator->username) {
		if (creator->cbs->link_account_response_cb != NULL) {
			creator->cbs->link_account_response_cb(creator, LinphoneAccountCreatorStatusMissingArguments, "Missing required parameters");
		}
		return LinphoneAccountCreatorStatusMissingArguments;
	}

	ms_debug("Account creator: link_phone_number_with_account (phone number=%s, username=%s, domain=%s, language=%s)",
		creator->phone_number,
		creator->username,
		linphone_proxy_config_get_domain(creator->proxy_cfg),
		creator->language);

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "link_phone_number_with_account",
		LinphoneXmlRpcArgString, creator->phone_number,
		LinphoneXmlRpcArgString, creator->username,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgString, creator->language,
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _link_phone_number_with_account_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	return LinphoneAccountCreatorStatusRequestOk;
}

static void _get_phone_number_for_account_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->cbs->is_account_linked_response_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "ERROR_USERNAME_PARAMETER_NOT_FOUND") == 0
				|| strcmp(resp, "ERROR_ACCOUNT_DOESNT_EXIST") == 0
				|| strcmp(resp, "ERROR_ALIAS_DOESNT_EXIST") == 0) ? LinphoneAccountCreatorStatusAccountNotLinked : LinphoneAccountCreatorStatusAccountLinked;
		}
		creator->cbs->is_account_linked_response_cb(creator, status, resp);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_is_account_linked_linphone(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->username || !linphone_proxy_config_get_domain(creator->proxy_cfg)) {
		return LinphoneAccountCreatorStatusMissingArguments;
	}

	ms_debug("Account creator: is_account_linked (username=%s, domain=%s)",
		creator->username,
		linphone_proxy_config_get_domain(creator->proxy_cfg));

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "get_phone_number_for_account",
		LinphoneXmlRpcArgString, creator->username,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _get_phone_number_for_account_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	return LinphoneAccountCreatorStatusRequestOk;
}
/****************** END OF LINK PHONE NUMBER WITH ACCOUNT SECTION *************/

/****************** START OF ACTIVE PHONE NUMBER LINK **************************/
static void _activate_phone_number_link_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->cbs->activate_alias_response_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strstr(resp, "ERROR_") == resp) ? LinphoneAccountCreatorStatusAccountNotActivated : LinphoneAccountCreatorStatusAccountActivated;
		}
		creator->cbs->activate_alias_response_cb(creator, status, resp);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_activate_phone_number_link_linphone(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->phone_number || !creator->username || !creator->activation_code || (!creator->password && !creator->ha1) || !linphone_proxy_config_get_domain(creator->proxy_cfg)) {
		if (creator->cbs->activate_alias_response_cb != NULL) {
			creator->cbs->activate_alias_response_cb(creator, LinphoneAccountCreatorStatusMissingArguments, "Missing required parameters");
		}
		return LinphoneAccountCreatorStatusMissingArguments;
	}

	ms_debug("Account creator: activate_phone_number_link (phone number=%s, username=%s, activation code=%s, domain=%s)",
		creator->phone_number,
		creator->username,
		creator->activation_code,
		linphone_proxy_config_get_domain(creator->proxy_cfg));

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "activate_phone_number_link",
		LinphoneXmlRpcArgString, creator->phone_number,
		LinphoneXmlRpcArgString, creator->username,
		LinphoneXmlRpcArgString, creator->activation_code,
		LinphoneXmlRpcArgString, creator->ha1 ? creator->ha1 : ha1_for_passwd(creator->username, linphone_proxy_config_get_domain(creator->proxy_cfg), creator->password),
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _activate_phone_number_link_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	return LinphoneAccountCreatorStatusRequestOk;
}
/****************** END OF ACTIVE PHONE NUMBER LINK **************************/

/****************** START OF ACTIVE PHONE NUMBER LINK **************************/
static void _recover_phone_account_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->cbs->recover_account_response_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			if (strstr(resp, "ERROR_") == resp) {
				status = (strstr(resp, "ERROR_CANNOT_SEND_SMS") == resp) ? LinphoneAccountCreatorStatusServerError
					: (strstr(resp, "ERROR_ACCOUNT_DOESNT_EXIST") == resp) ? LinphoneAccountCreatorStatusAccountNotExist
					: LinphoneAccountCreatorStatusRequestFailed;
			} else {
				status = LinphoneAccountCreatorStatusRequestOk;
				set_string(&creator->username, resp, FALSE);
			}
		}
		creator->cbs->recover_account_response_cb(creator, status, resp);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_recover_phone_account_linphone(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->phone_number) {
		if (creator->cbs->recover_account_response_cb != NULL) {
			creator->cbs->recover_account_response_cb(creator, LinphoneAccountCreatorStatusMissingArguments, "Missing required parameters");
		}
		return LinphoneAccountCreatorStatusMissingArguments;
	}

	ms_debug("Account creator: recover_phone_account (phone number=%s, domain=%s, language=%s)",
		creator->phone_number,
		linphone_proxy_config_get_domain(creator->proxy_cfg),
		creator->language);

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "recover_phone_account",
		LinphoneXmlRpcArgString, creator->phone_number,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgString, creator->language,
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _recover_phone_account_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	return LinphoneAccountCreatorStatusRequestOk;
}
/****************** END OF ACTIVE PHONE NUMBER LINK **************************/

/****************** START OF UPDATE ACCOUNT **************************/
static void _password_updated_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->cbs->update_account_response_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorStatusRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			if (strcmp(resp, "OK") == 0) {
				status = LinphoneAccountCreatorStatusRequestOk;
			} else if (strcmp(resp, "ERROR_PASSWORD_DOESNT_MATCH") == 0) {
				status = LinphoneAccountCreatorStatusAccountNotExist;
			} else {
				status = LinphoneAccountCreatorStatusServerError;
			}
		}
		creator->cbs->update_account_response_cb(creator, status, resp);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_update_password_linphone(LinphoneAccountCreator *creator){
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	const char* new_pwd = (const char*)linphone_account_creator_get_user_data(creator);
	if (!identity ||
			((!creator->username && !creator->phone_number)
				|| !linphone_proxy_config_get_domain(creator->proxy_cfg)
				|| (!creator->password && !creator->ha1) || !new_pwd
			)
		) {
		if (creator->cbs->update_account_response_cb != NULL) {
			creator->cbs->update_account_response_cb(creator, LinphoneAccountCreatorStatusMissingArguments, "Missing required parameters");
		}
		return LinphoneAccountCreatorStatusMissingArguments;
	}

	const char * username = creator->username ? creator->username : creator->phone_number;
	const char * ha1 = ms_strdup(creator->ha1 ? creator->ha1 : ha1_for_passwd(username, linphone_proxy_config_get_domain(creator->proxy_cfg), creator->password) );
	const char * new_ha1 = ms_strdup(ha1_for_passwd(username, linphone_proxy_config_get_domain(creator->proxy_cfg), new_pwd));

	ms_debug("Account creator: update_password (username=%s, domain=%s)",
		creator->username,
		linphone_proxy_config_get_domain(creator->proxy_cfg));

	request = linphone_xml_rpc_request_new_with_args(LinphoneXmlRpcArgString, "update_hash",
		LinphoneXmlRpcArgString, username,
		LinphoneXmlRpcArgString, ha1,
		LinphoneXmlRpcArgString, new_ha1,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);

	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _password_updated_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);

	return LinphoneAccountCreatorStatusRequestOk;
}
/****************** END OF UPDATE ACCOUNT **************************/

/************************** End Account Creator Linphone **************************/
