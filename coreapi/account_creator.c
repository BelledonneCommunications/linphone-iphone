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
#include "private.h"
#if !_WIN32
#include "regex.h"
#endif

#include <bctoolbox/crypto.h>

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneAccountCreatorResponseCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneAccountCreatorResponseCbs, belle_sip_object_t,
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

static unsigned int validate_uri(LinphoneCore *lc, LinphoneProxyConfig *proxy, const char* username, const char* display_name) {
	LinphoneAddress* addr;

	if (username) {
		addr = linphone_proxy_config_normalize_sip_uri(proxy, username);
	} else {
		addr = linphone_address_clone(linphone_proxy_config_get_identity_address(proxy));
	}

	if (addr == NULL)
		return 1;

	if (display_name && (!strlen(display_name) || linphone_address_set_display_name(addr, display_name) != 0)) {
		linphone_address_unref(addr);
		return 1;
	}

	linphone_address_unref(addr);
	return 0;
}

static char* _get_identity(const LinphoneAccountCreator *creator) {
	char *identity = NULL;
	if ((creator->username || creator->phone_number)) {
		//we must escape username
		LinphoneProxyConfig* proxy = creator->proxy_cfg;
		LinphoneAddress* addr;

		addr = linphone_proxy_config_normalize_sip_uri(proxy, creator->username ? creator->username : creator->phone_number);

		identity = linphone_address_as_string(addr);
		linphone_address_unref(addr);
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

LinphoneProxyConfig * linphone_account_creator_configure(const LinphoneAccountCreator *creator) {
	LinphoneAuthInfo *info;
	LinphoneProxyConfig *cfg = creator->proxy_cfg;
	char *identity_str = _get_identity(creator);
	LinphoneAddress *identity = linphone_address_new(identity_str);
	// char *route = NULL;
	// char *domain = NULL;
	ms_free(identity_str);
	if (creator->display_name) {
		linphone_address_set_display_name(identity, creator->display_name);
	}
	// if (creator->route) {
	// 	route = ms_strdup_printf("%s;transport=%s", creator->route, linphone_transport_to_string(creator->transport));
	// }
	// if (creator->domain) {
	// 	domain = ms_strdup_printf("%s;transport=%s", creator->domain, linphone_transport_to_string(creator->transport));
	// }
	linphone_proxy_config_set_identity_address(cfg, identity);
	if (creator->phone_country_code) {
		linphone_proxy_config_set_dial_prefix(cfg, creator->phone_country_code);
	} else if (creator->phone_number) {
		int dial_prefix_number = linphone_dial_plan_lookup_ccc_from_e164(creator->phone_number);
		char buff[4];
		snprintf(buff, sizeof(buff), "%d", dial_prefix_number);
		linphone_proxy_config_set_dial_prefix(cfg, buff);
	}
	// if (linphone_proxy_config_get_server_addr(cfg) == NULL)
	// 	linphone_proxy_config_set_server_addr(cfg, domain);
	// if (linphone_proxy_config_get_route(cfg) == NULL)
	// 	linphone_proxy_config_set_route(cfg, route);
	linphone_proxy_config_enable_publish(cfg, FALSE);
	linphone_proxy_config_enable_register(cfg, TRUE);

	if (strcmp(linphone_proxy_config_get_realm(creator->proxy_cfg), "sip.linphone.org") == 0) {
		linphone_proxy_config_enable_avpf(cfg, TRUE);
		// If account created on sip.linphone.org, we configure linphone to use TLS by default
		if (linphone_core_sip_transport_supported(creator->core, LinphoneTransportTls)) {
			LinphoneAddress *addr = linphone_address_new(linphone_proxy_config_get_server_addr(cfg));
			char *tmp;
			linphone_address_set_transport(addr, LinphoneTransportTls);
			tmp = linphone_address_as_string(addr);
			linphone_proxy_config_set_server_addr(cfg, tmp);
			linphone_proxy_config_set_route(cfg, tmp);
			ms_free(tmp);
			linphone_address_unref(addr);
		}
		linphone_core_set_stun_server(creator->core, "stun.linphone.org");
		linphone_core_set_firewall_policy(creator->core, LinphonePolicyUseIce);
	}

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
	linphone_proxy_config_unref(cfg);
	return NULL;
}
/************************** End Misc **************************/

/************************** Start Account Creator Cbs **************************/

static LinphoneAccountCreatorResponseCbs * linphone_account_creator_reponses_cbs_new(void) {
	return belle_sip_object_new(LinphoneAccountCreatorResponseCbs);
}

LinphoneAccountCreatorResponseCbs * linphone_account_creator_responses_cbs_ref(LinphoneAccountCreatorResponseCbs *responses_cbs) {
	belle_sip_object_ref(responses_cbs);
	return responses_cbs;
}

void linphone_account_creator_responses_cbs_unref(LinphoneAccountCreatorResponseCbs *responses_cbs) {
	belle_sip_object_unref(responses_cbs);
}

void *linphone_account_creator_responses_cbs_get_user_data(const LinphoneAccountCreatorResponseCbs *responses_cbs) {
	return responses_cbs->user_data;
}

void linphone_account_creator_responses_cbs_set_user_data(LinphoneAccountCreatorResponseCbs *responses_cbs, void *ud) {
	responses_cbs->user_data = ud;
}

LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_create_account_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs) {
	return responses_cbs->create_account_response_cb;
}

void linphone_account_creator_responses_cbs_set_create_account_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb) {
	responses_cbs->create_account_response_cb = cb;
}

LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_is_account_exist_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs) {
	return responses_cbs->is_account_exist_response_cb;
}

void linphone_account_creator_responses_cbs_set_is_account_exist_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb) {
	responses_cbs->is_account_exist_response_cb = cb;
}

LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_activate_account_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs) {
	return responses_cbs->activate_account_response_cb;
}

void linphone_account_creator_responses_cbs_set_activate_account_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb) {
	responses_cbs->activate_account_response_cb = cb;
}

LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_is_account_activated_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs) {
	return responses_cbs->is_account_activated_response_cb;
}

void linphone_account_creator_responses_cbs_set_is_account_activated_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb) {
	responses_cbs->is_account_activated_response_cb = cb;
}

LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_link_account_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs) {
	return responses_cbs->link_account_response_cb;
}

void linphone_account_creator_responses_cbs_set_link_account_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb) {
	responses_cbs->link_account_response_cb = cb;
}

LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_activate_alias_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs) {
	return responses_cbs->activate_alias_response_cb;
}

void linphone_account_creator_responses_cbs_set_activate_alias_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb) {
	responses_cbs->activate_alias_response_cb = cb;
}

LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_is_alias_used_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs) {
	return responses_cbs->is_alias_used_response_cb;
}

void linphone_account_creator_responses_cbs_set_is_alias_used_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb) {
	responses_cbs->is_alias_used_response_cb = cb;
}

LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_is_account_linked_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs) {
	return responses_cbs->is_account_linked_response_cb;
}

void linphone_account_creator_responses_cbs_set_is_account_linked_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb) {
	responses_cbs->is_account_linked_response_cb = cb;
}

LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_recover_account_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs) {
	return responses_cbs->recover_account_response_cb;
}

void linphone_account_creator_responses_cbs_set_recover_account_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb) {
	responses_cbs->recover_account_response_cb = cb;
}

LinphoneAccountCreatorResponseFunc linphone_account_creator_responses_cbs_get_update_account_cb(const LinphoneAccountCreatorResponseCbs *responses_cbs) {
	return responses_cbs->update_account_response_cb;
}

void linphone_account_creator_responses_cbs_set_update_account_cb(LinphoneAccountCreatorResponseCbs *responses_cbs, LinphoneAccountCreatorResponseFunc cb) {
	responses_cbs->update_account_response_cb = cb;
}
/************************** End Account Creator Cbs **************************/

/************************** Start Account Creator data **************************/
static void _linphone_account_creator_destroy(LinphoneAccountCreator *creator) {
	/*this will drop all pending requests if any*/
	if (creator->xmlrpc_session) linphone_xml_rpc_session_release(creator->xmlrpc_session);
	if (creator->requests_cbs != NULL && linphone_account_creator_requests_cbs_get_destructor_cb(creator->requests_cbs) != NULL)
		linphone_account_creator_requests_cbs_get_destructor_cb(creator->requests_cbs)(creator);
	linphone_account_creator_responses_cbs_unref(creator->responses_cbs);
	linphone_proxy_config_destroy(creator->proxy_cfg);
	if (creator->username) ms_free(creator->username);
	if (creator->display_name) ms_free(creator->display_name);
	if (creator->password) ms_free(creator->password);
	if (creator->ha1) ms_free(creator->ha1);
	if (creator->phone_number) ms_free(creator->phone_number);
	if (creator->phone_country_code) ms_free(creator->phone_country_code);
	if (creator->email) ms_free(creator->email);
	if (creator->language) ms_free(creator->language);
	if (creator->activation_code) ms_free(creator->activation_code);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneAccountCreator);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneAccountCreator, belle_sip_object_t,
	(belle_sip_object_destroy_t)_linphone_account_creator_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);

LinphoneAccountCreator * linphone_account_creator_new(LinphoneCore *core, const char *xmlrpc_url) {
	LinphoneAccountCreator *creator;
	creator = belle_sip_object_new(LinphoneAccountCreator);
	creator->requests_cbs = linphone_core_get_account_creator_request_engine_cbs(core);
	creator->responses_cbs = linphone_account_creator_reponses_cbs_new();
	creator->core = core;
	creator->xmlrpc_session = (xmlrpc_url) ? linphone_xml_rpc_session_new(core, xmlrpc_url) : NULL;
	creator->proxy_cfg = linphone_core_create_proxy_config(core);
	if (creator->requests_cbs != NULL && linphone_account_creator_requests_cbs_get_constructor_cb(creator->requests_cbs) != NULL)
		linphone_account_creator_requests_cbs_get_constructor_cb(creator->requests_cbs)(creator);
	return creator;
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

LinphoneUsernameCheck linphone_account_creator_set_username(LinphoneAccountCreator *creator, const char *username) {
	int min_length = lp_config_get_int(creator->core->config, "assistant", "username_min_length", -1);
	int max_length = lp_config_get_int(creator->core->config, "assistant", "username_max_length", -1);
	bool_t use_phone_number = lp_config_get_int(creator->core->config, "assistant", "use_phone_number", 0);
	const char* regex = lp_config_get_string(creator->core->config, "assistant", "username_regex", 0);
	if (!username) {
		creator->username = NULL;
		return LinphoneUsernameOk;
	} else if (min_length > 0 && strlen(username) < (size_t)min_length) {
		return LinphoneUsernameTooShort;
	} else if (max_length > 0 && strlen(username) > (size_t)max_length) {
		return LinphoneUsernameTooLong;
	} else if (use_phone_number && !linphone_proxy_config_is_phone_number(NULL, username)) {
		return LinphoneUsernameInvalid;
	} else if (regex && !is_matching_regex(username, regex)) {
		return LinphoneUsernameInvalidCharacters;
	} else if (validate_uri(creator->core, creator->proxy_cfg, username, NULL) != 0) {
		return LinphoneUsernameInvalid;
	}

	set_string(&creator->username, username, TRUE);
	return LinphoneUsernameOk;
}

const char * linphone_account_creator_get_username(const LinphoneAccountCreator *creator) {
	return creator->username;
}

LinphonePhoneNumberMask linphone_account_creator_set_phone_number(LinphoneAccountCreator *creator, const char *phone_number, const char *country_code) {
	char *normalized_phone_number;
	LinphonePhoneNumberMask return_status = 0;
	if (!phone_number || !country_code) {
		if (!phone_number && !country_code) {
			creator->phone_number = NULL;
			creator->phone_country_code = NULL;
			return (LinphonePhoneNumberMask)LinphonePhoneNumberOk;
		} else {
			return (LinphonePhoneNumberMask)LinphonePhoneNumberInvalid;
		}
	} else {
		LinphoneProxyConfig *numCfg = creator->proxy_cfg;
		creator->phone_country_code = ms_strdup(country_code[0] == '+' ? &country_code[1] : country_code);
		linphone_proxy_config_set_dial_prefix(numCfg, creator->phone_country_code);
		normalized_phone_number = linphone_proxy_config_normalize_phone_number(numCfg, phone_number);
		if (!normalized_phone_number) {
			return LinphonePhoneNumberInvalid;
		}

		// if phone is valid, we lastly want to check that length is OK
		{
			const LinphoneDialPlan* plan = linphone_dial_plan_by_ccc(creator->phone_country_code);
			int size = (int)strlen(phone_number);
			if (linphone_dial_plan_is_generic(plan)) {
				return_status = LinphonePhoneNumberCountryCodeInvalid;
			}
			if (size < plan->nnl - 1) {
				return_status += LinphonePhoneNumberTooShort;
				goto end;
			} else if (size > plan->nnl + 1) {
				return_status += LinphonePhoneNumberTooLong;
				goto end;
			} else if (return_status & LinphonePhoneNumberCountryCodeInvalid) {
				goto end;
			}
		}
	}
	set_string(&creator->phone_number, normalized_phone_number, TRUE);
	return_status = LinphonePhoneNumberOk;
end:
	ms_free(normalized_phone_number);
	return return_status;
}

const char * linphone_account_creator_get_phone_number(const LinphoneAccountCreator *creator) {
	return creator->phone_number;
}

LinphonePasswordCheck linphone_account_creator_set_password(LinphoneAccountCreator *creator, const char *password) {
	int min_length = lp_config_get_int(creator->core->config, "assistant", "password_min_length", -1);
	int max_length = lp_config_get_int(creator->core->config, "assistant", "password_max_length", -1);
	if (!password) {
		creator->password = NULL;
		return LinphonePasswordTooShort;
	}
	if (min_length > 0 && strlen(password) < (size_t)min_length) {
		return LinphonePasswordTooShort;
	} else if (max_length > 0 && strlen(password) > (size_t)max_length) {
		return LinphonePasswordTooLong;
	}
	set_string(&creator->password, password, FALSE);
	return LinphonePasswordOk;
}

const char * linphone_account_creator_get_password(const LinphoneAccountCreator *creator) {
	return creator->password;
}

LinphonePasswordCheck linphone_account_creator_set_ha1(LinphoneAccountCreator *creator, const char *ha1){
	set_string(&creator->ha1, ha1, FALSE);
	return LinphonePasswordOk;
}

const char * linphone_account_creator_get_ha1(const LinphoneAccountCreator *creator) {
	return creator->ha1;
}

LinphoneActivationCodeCheck linphone_account_creator_set_activation_code(LinphoneAccountCreator *creator, const char *activation_code){
	set_string(&creator->activation_code, activation_code, FALSE);
	return LinphoneActivationCodeOk;
}

const char * linphone_account_creator_get_activation_code(const LinphoneAccountCreator *creator) {
	return creator->activation_code;
}

LinphoneLanguageCheck linphone_account_creator_set_language(LinphoneAccountCreator *creator, const char *lang) {
	set_string(&creator->language, lang, FALSE);
	return LinphoneLanguageOk;
}

const char * linphone_account_creator_get_language(const LinphoneAccountCreator *creator) {
	return creator->language;
}

LinphoneUsernameCheck linphone_account_creator_set_display_name(LinphoneAccountCreator *creator, const char *display_name) {
	if (validate_uri(creator->core, creator->proxy_cfg, NULL, display_name) != 0) {
		return LinphoneUsernameInvalid;
	}
	set_string(&creator->display_name, display_name, FALSE);
	return LinphoneUsernameOk;
}

const char * linphone_account_creator_get_display_name(const LinphoneAccountCreator *creator) {
	return creator->display_name;
}

LinphoneEmailCheck linphone_account_creator_set_email(LinphoneAccountCreator *creator, const char *email) {
	if (!is_matching_regex(email, "^.+@.+\\..*$")) {
		return LinphoneEmailMalformed;
	}
	if (!is_matching_regex(email, "^.+@.+\\.[A-Za-z]{2}[A-Za-z]*$")) {
		return LinphoneEmailInvalidCharacters;
	}
	set_string(&creator->email, email, TRUE);
	return LinphoneEmailOk;
}

const char * linphone_account_creator_get_email(const LinphoneAccountCreator *creator) {
	return creator->email;
}

LinphoneAccountCreatorResponseCbs * linphone_account_creator_get_responses_cbs(const LinphoneAccountCreator *creator) {
	return creator->responses_cbs;
}

LinphoneAccountCreatorRequestCbs * linphone_account_creator_get_requests_cbs(const LinphoneAccountCreator *creator) {
	return creator->requests_cbs;
}

LinphoneRequestStatus linphone_account_creator_is_account_exist(LinphoneAccountCreator *creator) {
	if (creator->requests_cbs->is_account_exist_request_cb == NULL
		|| creator->responses_cbs->is_account_exist_response_cb == NULL) {
		return LinphoneRequestMissingCallbacks;
	}

	return creator->requests_cbs->is_account_exist_request_cb(creator);
}

LinphoneRequestStatus linphone_account_creator_create_account(LinphoneAccountCreator *creator) {
	if (creator->requests_cbs->create_account_request_cb == NULL
		|| creator->responses_cbs->create_account_response_cb == NULL) {
		return LinphoneRequestMissingCallbacks;
	}

	return creator->requests_cbs->create_account_request_cb(creator);
}

LinphoneRequestStatus linphone_account_creator_is_account_activated(LinphoneAccountCreator *creator) {
	if (creator->requests_cbs->is_account_activated_request_cb == NULL
		|| creator->responses_cbs->is_account_activated_response_cb == NULL) {
		return LinphoneRequestMissingCallbacks;
	}

	return creator->requests_cbs->is_account_activated_request_cb(creator);
}

LinphoneRequestStatus linphone_account_creator_activate_account(LinphoneAccountCreator *creator) {
	if (creator->requests_cbs->activate_account_request_cb == NULL
		|| creator->responses_cbs->activate_account_response_cb == NULL) {
		return LinphoneRequestMissingCallbacks;
	}

	return creator->requests_cbs->activate_account_request_cb(creator);
}

LinphoneRequestStatus linphone_account_creator_link_account(LinphoneAccountCreator *creator) {
	if (creator->requests_cbs->link_account_request_cb == NULL
		|| creator->responses_cbs->link_account_response_cb == NULL) {
		return LinphoneRequestMissingCallbacks;
	}

	return creator->requests_cbs->link_account_request_cb(creator);
}

LinphoneRequestStatus linphone_account_creator_activate_alias(LinphoneAccountCreator *creator) {
	if (creator->requests_cbs->activate_alias_request_cb == NULL
		|| creator->responses_cbs->activate_alias_response_cb == NULL) {
		return LinphoneRequestMissingCallbacks;
	}

	return creator->requests_cbs->activate_alias_request_cb(creator);
}

LinphoneRequestStatus linphone_account_creator_is_alias_used(LinphoneAccountCreator *creator) {
	if (creator->requests_cbs->is_alias_used_request_cb == NULL
		|| creator->responses_cbs->is_alias_used_response_cb == NULL) {
		return LinphoneRequestMissingCallbacks;
	}

	return creator->requests_cbs->is_alias_used_request_cb(creator);
}

LinphoneRequestStatus linphone_account_creator_is_account_linked(LinphoneAccountCreator *creator) {
	if (creator->requests_cbs->is_account_linked_request_cb == NULL
		|| creator->responses_cbs->is_account_linked_response_cb == NULL) {
		return LinphoneRequestMissingCallbacks;
	}

	return creator->requests_cbs->is_account_linked_request_cb(creator);
}

LinphoneRequestStatus linphone_account_creator_recover_account(LinphoneAccountCreator *creator) {
	if (creator->requests_cbs->recover_account_request_cb == NULL
		|| creator->responses_cbs->recover_account_response_cb == NULL) {
		return LinphoneRequestMissingCallbacks;
	}

	return creator->requests_cbs->recover_account_request_cb(creator);
}

LinphoneRequestStatus linphone_account_creator_update_account(LinphoneAccountCreator *creator) {
	if (creator->requests_cbs->update_account_request_cb == NULL
		|| creator->responses_cbs->update_account_response_cb == NULL) {
		return LinphoneRequestMissingCallbacks;
	}

	return creator->requests_cbs->update_account_request_cb(creator);
}
/************************** End Account Creator data **************************/

/************************** Start Account Creator Linphone **************************/

/****************** START OF ACCOUNT USED SECTION *****************************/
static void _is_account_exist_response_cb(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->responses_cbs->is_account_exist_response_cb != NULL) {
		LinphoneRequestStatus status = LinphoneRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "ERROR_ACCOUNT_DOESNT_EXIST") == 0) ? LinphoneRequestAccountNotExist : (
							(strcmp(resp, "ERROR_ALIAS_DOESNT_EXIST") == 0) ? LinphoneRequestAccountExist :
														LinphoneRequestAccountExistWithAlias);
			if (status == LinphoneRequestAccountExistWithAlias) {
				set_string(&creator->phone_number, resp, FALSE);
			}
		}
		creator->responses_cbs->is_account_exist_response_cb(creator, status, resp);
	}
}

LinphoneRequestStatus linphone_account_creator_is_account_exist_custom(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->username && !creator->phone_number) {
		if (creator->responses_cbs->is_account_exist_response_cb != NULL) {
			creator->responses_cbs->is_account_exist_response_cb(creator, LinphoneRequestMissingArguments, "Missing required parameters");
		}
		return LinphoneRequestMissingArguments;
	}

	request = linphone_xml_rpc_request_new_with_args("get_phone_number_for_account", LinphoneXmlRpcArgString,
		LinphoneXmlRpcArgString, creator->username ? creator->username : creator->phone_number,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);

	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _is_account_exist_response_cb);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);

	return LinphoneRequestOk;
}
/****************** END OF CREATE ACCOUNT USED SECTION ************************/

/****************** START OF CREATE ACCOUNT SECTION ***************************/
static void _create_account_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->responses_cbs->create_account_response_cb != NULL) {
		LinphoneRequestStatus status = LinphoneRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "OK") == 0) ? LinphoneRequestAccountCreated
			: (strcmp(resp, "ERROR_CANNOT_SEND_SMS") == 0) ? LinphoneRequestErrorServer
			: (strcmp(resp, "ERROR_ACCOUNT_ALREADY_IN_USE") == 0) ? LinphoneRequestAccountExist
			: (strcmp(resp, "ERROR_ALIAS_ALREADY_IN_USE") == 0) ? LinphoneRequestAccountExistWithAlias
			:LinphoneRequestAccountNotCreated;
		}
		creator->responses_cbs->create_account_response_cb(creator, status, resp);
	}
}

static LinphoneXmlRpcRequest * _create_account_with_phone_custom(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->phone_number) {
		return NULL;
	}
	request = linphone_xml_rpc_request_new_with_args("create_phone_account", LinphoneXmlRpcArgString,
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
	request = linphone_xml_rpc_request_new_with_args("create_email_account", LinphoneXmlRpcArgString,
		LinphoneXmlRpcArgString, creator->username,
		LinphoneXmlRpcArgString, creator->email,
		LinphoneXmlRpcArgString, ha1_for_passwd(creator->username ? creator->username : creator->phone_number, linphone_proxy_config_get_domain(creator->proxy_cfg), creator->password),
		LinphoneXmlRpcArgString, linphone_core_get_user_agent(creator->core),
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);
	return request;
}

LinphoneRequestStatus linphone_account_creator_create_account_custom(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	if (!identity || (!(request = _create_account_with_phone_custom(creator))
		&& !(request = _create_account_with_email_custom(creator)))) {
		if (creator->responses_cbs->create_account_response_cb != NULL) {
			creator->responses_cbs->create_account_response_cb(creator, LinphoneRequestMissingArguments, "Missing required parameters");
		}
		if (identity) ms_free(identity);
		return LinphoneRequestMissingArguments;
	}

	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _create_account_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneRequestOk;
}
/****************** END OF CREATE ACCOUNT SECTION *****************************/

/****************** START OF VALIDATE ACCOUNT SECTION *************************/
static void _activate_account_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->responses_cbs->activate_account_response_cb != NULL) {
		LinphoneRequestStatus status = LinphoneRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			if (strcmp(resp, "ERROR_ACCOUNT_ALREADY_ACTIVATED") == 0) {
				status = LinphoneRequestAccountAlreadyActivated;
			} else if (strstr(resp, "ERROR_") == resp) {
				status = LinphoneRequestAccountNotActivated;
			} else {
				status = LinphoneRequestAccountActivated;
				set_string(&creator->ha1, resp, FALSE);
			}
		}
		creator->responses_cbs->activate_account_response_cb(creator, status, resp);
	}
}

LinphoneRequestStatus linphone_account_creator_activate_account_custom(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	if (!identity || !creator->activation_code) {
		if (creator->responses_cbs->is_account_activated_response_cb != NULL) {
			creator->responses_cbs->is_account_activated_response_cb(creator, LinphoneRequestMissingArguments, "Missing required parameters");
		}
		return LinphoneRequestMissingArguments;
	}

	if (creator->phone_number) {
		request = linphone_xml_rpc_request_new_with_args("activate_phone_account", LinphoneXmlRpcArgString,
			LinphoneXmlRpcArgString, creator->phone_number,
			LinphoneXmlRpcArgString, creator->username ? creator->username : creator->phone_number,
			LinphoneXmlRpcArgString, creator->activation_code,
			linphone_proxy_config_get_domain(creator->proxy_cfg),
			LinphoneXmlRpcArgNone);
	} else {
		request = linphone_xml_rpc_request_new_with_args("activate_email_account", LinphoneXmlRpcArgString,
			LinphoneXmlRpcArgString, creator->username,
			LinphoneXmlRpcArgString, creator->activation_code,
			linphone_proxy_config_get_domain(creator->proxy_cfg),
			LinphoneXmlRpcArgNone);
	}
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _activate_account_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneRequestOk;
}
/****************** END OF CREATE VALIDATE ACCOUNT SECTION ********************/

/****************** START OF ACCOUNT VALIDATED SECTION ************************/
static void _is_account_activated_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->responses_cbs->is_account_activated_response_cb != NULL) {
		LinphoneRequestStatus status = LinphoneRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "OK") == 0) ? LinphoneRequestAccountActivated : LinphoneRequestAccountNotActivated;
		}
		creator->responses_cbs->is_account_activated_response_cb(creator, status, resp);
	}
}

LinphoneRequestStatus linphone_account_creator_is_account_activated_custom(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	if (!identity) {
		if (creator->responses_cbs->is_account_activated_response_cb != NULL) {
			creator->responses_cbs->is_account_activated_response_cb(creator, LinphoneRequestMissingArguments, "Missing required parameters");
		}
		return LinphoneRequestMissingArguments;
	}
	request = linphone_xml_rpc_request_new_with_args("is_account_activated", LinphoneXmlRpcArgString,
		LinphoneXmlRpcArgString, creator->username ? creator->username : creator->phone_number,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _is_account_activated_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneRequestOk;
}
/****************** END OF CREATE ACCOUNT VALIDATED SECTION********************/

/****************** START OF PHONE NUMBER VALIDATED SECTION *******************/

static void _is_phone_number_used_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->responses_cbs->is_alias_used_response_cb != NULL) {
		LinphoneRequestStatus status = LinphoneRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "OK_ACCOUNT") == 0) ? LinphoneRequestAliasIsAccount
			: (strcmp(resp, "OK_ALIAS") == 0) ? LinphoneRequestAliasExist
			: LinphoneRequestAliasNotExist;
		}
		creator->responses_cbs->is_alias_used_response_cb(creator, status, resp);
	}
}

LinphoneRequestStatus linphone_account_creator_is_phone_number_used_custom(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	if (!identity) {
		if (creator->responses_cbs->is_alias_used_response_cb != NULL) {
			creator->responses_cbs->is_alias_used_response_cb(creator, LinphoneRequestMissingArguments, "Missing required parameters");
		}
		return LinphoneRequestMissingArguments;
	}
	request = linphone_xml_rpc_request_new_with_args("is_phone_number_used", LinphoneXmlRpcArgString,
		LinphoneXmlRpcArgString, creator->phone_number,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _is_phone_number_used_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneRequestOk;
}

/****************** END OF PHONE NUMBER VALIDATED SECTION *********************/

/****************** START OF LINK PHONE NUMBER WITH ACCOUNT SECTION ***********/
static void _link_phone_number_with_account_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->responses_cbs->link_account_response_cb != NULL) {
		LinphoneRequestStatus status = LinphoneRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "OK") == 0) ? LinphoneRequestOk : LinphoneRequestAccountNotLinked;
		}
		creator->responses_cbs->link_account_response_cb(creator, status, resp);
	}
}

LinphoneRequestStatus linphone_account_creator_link_phone_number_with_account_custom(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->phone_number || !creator->username) {
		if (creator->responses_cbs->link_account_response_cb != NULL) {
			creator->responses_cbs->link_account_response_cb(creator, LinphoneRequestMissingArguments, "Missing required parameters");
		}
		return LinphoneRequestMissingArguments;
	}
	request = linphone_xml_rpc_request_new_with_args("link_phone_number_with_account", LinphoneXmlRpcArgString,
		LinphoneXmlRpcArgString, creator->phone_number,
		LinphoneXmlRpcArgString, creator->username,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgString, creator->language,
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _link_phone_number_with_account_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	return LinphoneRequestOk;
}

static void _get_phone_number_for_account_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->responses_cbs->is_account_linked_response_cb != NULL) {
		LinphoneRequestStatus status = LinphoneRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strcmp(resp, "ERROR_USERNAME_PARAMETER_NOT_FOUND") == 0
				|| strcmp(resp, "ERROR_ACCOUNT_DOESNT_EXIST") == 0
				|| strcmp(resp, "ERROR_ALIAS_DOESNT_EXIST") == 0) ? LinphoneRequestAccountNotLinked : LinphoneRequestAccountLinked;
		}
		creator->responses_cbs->is_account_linked_response_cb(creator, status, resp);
	}
}

LinphoneRequestStatus linphone_account_creator_is_account_linked_custom(LinphoneAccountCreator *creator) {
    LinphoneXmlRpcRequest *request;
    if (!creator->username || !linphone_proxy_config_get_domain(creator->proxy_cfg)) {
		return LinphoneRequestMissingArguments;
    }
    request = linphone_xml_rpc_request_new_with_args("get_phone_number_for_account",LinphoneXmlRpcArgString,
		LinphoneXmlRpcArgString, creator->username,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);
    linphone_xml_rpc_request_set_user_data(request, creator);
    linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _get_phone_number_for_account_cb_custom);
    linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
    linphone_xml_rpc_request_unref(request);
    return LinphoneRequestOk;
}
/****************** END OF LINK PHONE NUMBER WITH ACCOUNT SECTION *************/

/****************** START OF ACTIVE PHONE NUMBER LINK **************************/
static void _activate_phone_number_link_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->responses_cbs->activate_alias_response_cb != NULL) {
		LinphoneRequestStatus status = LinphoneRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			status = (strstr(resp, "ERROR_") == resp) ? LinphoneRequestAccountNotActivated : LinphoneRequestAccountActivated;
		}
		creator->responses_cbs->activate_alias_response_cb(creator, status, resp);
	}
}

LinphoneRequestStatus linphone_account_creator_activate_phone_number_link_custom(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->phone_number || !creator->username || !creator->activation_code || (!creator->password && !creator->ha1) || !linphone_proxy_config_get_domain(creator->proxy_cfg)) {
		if (creator->responses_cbs->activate_alias_response_cb != NULL) {
			creator->responses_cbs->activate_alias_response_cb(creator, LinphoneRequestMissingArguments, "Missing required parameters");
		}
		return LinphoneRequestMissingArguments;
	}
	request = linphone_xml_rpc_request_new_with_args("activate_phone_number_link", LinphoneXmlRpcArgString,
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
	return LinphoneRequestOk;
}
/****************** END OF ACTIVE PHONE NUMBER LINK **************************/

/****************** START OF ACTIVE PHONE NUMBER LINK **************************/
static void _recover_phone_account_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->responses_cbs->recover_account_response_cb != NULL) {
		LinphoneRequestStatus status = LinphoneRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			if (strstr(resp, "ERROR_") == resp) {
				status = (strstr(resp, "ERROR_CANNOT_SEND_SMS") == resp) ? LinphoneRequestErrorServer
					: (strstr(resp, "ERROR_ACCOUNT_DOESNT_EXIST") == resp) ? LinphoneRequestAccountNotExist
					: LinphoneRequestFailed;
			} else {
				status = LinphoneRequestOk;
				set_string(&creator->username, resp, FALSE);
			}
		}
		creator->responses_cbs->recover_account_response_cb(creator, status, resp);
	}
}

LinphoneRequestStatus linphone_account_creator_recover_phone_account_custom(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	if (!creator->phone_number) {
		if (creator->responses_cbs->recover_account_response_cb != NULL) {
			creator->responses_cbs->recover_account_response_cb(creator, LinphoneRequestMissingArguments, "Missing required parameters");
		}
		return LinphoneRequestMissingArguments;
	}
	request = linphone_xml_rpc_request_new_with_args("recover_phone_account", LinphoneXmlRpcArgString,
		LinphoneXmlRpcArgString, creator->phone_number,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgString, creator->language,
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _recover_phone_account_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	return LinphoneRequestOk;
}
/****************** END OF ACTIVE PHONE NUMBER LINK **************************/

/****************** START OF UPDATE ACCOUNT **************************/
static void _password_updated_cb_custom(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->responses_cbs->update_account_response_cb != NULL) {
		LinphoneRequestStatus status = LinphoneRequestFailed;
		const char* resp = linphone_xml_rpc_request_get_string_response(request);
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			if (strcmp(resp, "OK") == 0) {
				status = LinphoneRequestOk;
			} else if (strcmp(resp, "ERROR_PASSWORD_DOESNT_MATCH") == 0) {
				status = LinphoneRequestAccountNotExist;
			} else {
				status = LinphoneRequestErrorServer;
			}
		}
		creator->responses_cbs->update_account_response_cb(creator, status, resp);
	}
}

LinphoneRequestStatus linphone_account_creator_update_password_custom(LinphoneAccountCreator *creator){
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	const char* new_pwd = (const char*)linphone_account_creator_get_user_data(creator);
	if (!identity ||
			(!creator->username || !creator->phone_number
				|| !linphone_proxy_config_get_domain(creator->proxy_cfg)
				|| (!creator->password && !creator->ha1) || !new_pwd
			)
		) {
		if (creator->responses_cbs->update_account_response_cb != NULL) {
			creator->responses_cbs->update_account_response_cb(creator, LinphoneRequestMissingArguments, "Missing required parameters");
		}
		return LinphoneRequestMissingArguments;
	}

	const char * username = creator->username ? creator->username : creator->phone_number;
	const char * ha1 = ms_strdup(creator->ha1 ? creator->ha1 : ha1_for_passwd(username, linphone_proxy_config_get_domain(creator->proxy_cfg), creator->password) );
	const char * new_ha1 = ms_strdup(ha1_for_passwd(username, linphone_proxy_config_get_domain(creator->proxy_cfg), new_pwd));

	request = linphone_xml_rpc_request_new_with_args("update_hash", LinphoneXmlRpcArgString,
		LinphoneXmlRpcArgString, username,
		LinphoneXmlRpcArgString, ha1,
		LinphoneXmlRpcArgString, new_ha1,
		LinphoneXmlRpcArgString, linphone_proxy_config_get_domain(creator->proxy_cfg),
		LinphoneXmlRpcArgNone);

	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _password_updated_cb_custom);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);

	return LinphoneRequestOk;
}
/****************** END OF UPDATE ACCOUNT **************************/

/************************** End Account Creator Linphone **************************/
