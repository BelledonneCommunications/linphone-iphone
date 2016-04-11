/*
linphone
Copyright (C) 2010-2015 Belledonne Communications SARL

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

#include "account_creator.h"
#include "private.h"
#if !_WIN32
#include "regex.h"
#endif

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneAccountCreatorCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneAccountCreatorCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);

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

LinphoneAccountCreatorCbsExistenceTestedCb linphone_account_creator_cbs_get_existence_tested(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->existence_tested;
}

void linphone_account_creator_cbs_set_existence_tested(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsExistenceTestedCb cb) {
	cbs->existence_tested = cb;
}

LinphoneAccountCreatorCbsValidationTestedCb linphone_account_creator_cbs_get_validation_tested(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->validation_tested;
}

void linphone_account_creator_cbs_set_validation_tested(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsValidationTestedCb cb) {
	cbs->validation_tested = cb;
}

LinphoneAccountCreatorCbsCreateAccountCb linphone_account_creator_cbs_get_create_account(const LinphoneAccountCreatorCbs *cbs) {
	return cbs->create_account;
}

void linphone_account_creator_cbs_set_create_account(LinphoneAccountCreatorCbs *cbs, LinphoneAccountCreatorCbsCreateAccountCb cb) {
	cbs->create_account = cb;
}


static void _linphone_account_creator_destroy(LinphoneAccountCreator *creator) {
	linphone_xml_rpc_session_unref(creator->xmlrpc_session);
	linphone_account_creator_cbs_unref(creator->callbacks);
	if (creator->username) ms_free(creator->username);
	if (creator->password) ms_free(creator->password);
	if (creator->domain) ms_free(creator->domain);
	if (creator->route) ms_free(creator->route);
	if (creator->email) ms_free(creator->email);
	if (creator->display_name) ms_free(creator->display_name);
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
	const char* domain = lp_config_get_string(core->config, "assistant", "domain", NULL);
	creator = belle_sip_object_new(LinphoneAccountCreator);
	creator->callbacks = linphone_account_creator_cbs_new();
	creator->core = core;
	creator->xmlrpc_session = linphone_xml_rpc_session_new(core, xmlrpc_url);
	if (domain) {
		linphone_account_creator_set_domain(creator, domain);
	}
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

static LinphoneAccountCreatorStatus validate_uri(const char* username, const char* domain, const char* route, const char* display_name) {
	LinphoneAddress* addr;
	LinphoneAccountCreatorStatus status = LinphoneAccountCreatorOK;
	LinphoneProxyConfig* proxy = linphone_proxy_config_new();
	linphone_proxy_config_set_identity(proxy, "sip:userame@domain.com");

	if (route && linphone_proxy_config_set_route(proxy, route) != 0) {
		status = LinphoneAccountCreatorRouteInvalid;
		goto end;
	}

	if (username) {
		addr = linphone_proxy_config_normalize_sip_uri(proxy, username);
	} else {
		addr = linphone_address_clone(linphone_proxy_config_get_identity_address(proxy));
	}

	if (addr == NULL) {
		status = LinphoneAccountCreatorUsernameInvalid;
		goto end;
	}

	if (domain && linphone_address_set_domain(addr, domain) != 0) {
		status = LinphoneAccountCreatorDomainInvalid;
	}

	if (display_name && (!strlen(display_name) || linphone_address_set_display_name(addr, display_name) != 0)) {
		status = LinphoneAccountCreatorDisplayNameInvalid;
	}
	linphone_address_unref(addr);
end:
	linphone_proxy_config_destroy(proxy);
	return status;
}

static char* _get_identity(const LinphoneAccountCreator *creator) {
	char *identity = NULL;
	if (creator->username && creator->domain) {
		//we must escape username
		LinphoneProxyConfig* proxy = linphone_proxy_config_new();
		LinphoneAddress* addr;
		// creator->domain may contain some port or some transport (eg. toto.org:443;transport=tcp),
		// we will accept that
		char * tmpidentity = ms_strdup_printf("sip:username@%s", creator->domain);
		linphone_proxy_config_set_identity(proxy, tmpidentity);
		ms_free(tmpidentity);
		addr = linphone_proxy_config_normalize_sip_uri(proxy, creator->username);

		identity = linphone_address_as_string(addr);
		linphone_address_destroy(addr);
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

LinphoneAccountCreatorStatus linphone_account_creator_set_username(LinphoneAccountCreator *creator, const char *username) {
	int min_length = lp_config_get_int(creator->core->config, "assistant", "username_min_length", -1);
	int fixed_length = lp_config_get_int(creator->core->config, "assistant", "username_length", -1);
	int max_length = lp_config_get_int(creator->core->config, "assistant", "username_max_length", -1);
	bool_t use_phone_number = lp_config_get_int(creator->core->config, "assistant", "use_phone_number", 0);
	const char* regex = lp_config_get_string(creator->core->config, "assistant", "username_regex", 0);
	LinphoneAccountCreatorStatus status;
	if (min_length > 0 && strlen(username) < min_length) {
		return LinphoneAccountCreatorUsernameTooShort;
	} else if (max_length > 0 && strlen(username) > max_length) {
		return LinphoneAccountCreatorUsernameTooLong;
	} else if (fixed_length > 0 && strlen(username) != fixed_length) {
		return LinphoneAccountCreatorUsernameInvalidSize;
	} else if (use_phone_number && !linphone_proxy_config_is_phone_number(NULL, username)) {
		return LinphoneAccountCreatorUsernameInvalid;
	} else if (regex && !is_matching_regex(username, regex)) {
		return LinphoneAccountCreatorUsernameInvalid;
	} else if ((status = validate_uri(username, NULL, NULL, NULL)) != LinphoneAccountCreatorOK) {
		return status;
	}

	set_string(&creator->username, username, TRUE);

	return LinphoneAccountCreatorOK;
}

const char * linphone_account_creator_get_username(const LinphoneAccountCreator *creator) {
	return creator->username;
}

LinphoneAccountCreatorStatus linphone_account_creator_set_password(LinphoneAccountCreator *creator, const char *password){
	int min_length = lp_config_get_int(creator->core->config, "assistant", "password_min_length", -1);
	int max_length = lp_config_get_int(creator->core->config, "assistant", "password_max_length", -1);
	if (min_length > 0 && strlen(password) < min_length) {
		return LinphoneAccountCreatorPasswordTooShort;
	} else if (max_length > 0 && strlen(password) > max_length) {
		return LinphoneAccountCreatorPasswordTooLong;
	}
	set_string(&creator->password, password, FALSE);
	return LinphoneAccountCreatorOK;
}

const char * linphone_account_creator_get_password(const LinphoneAccountCreator *creator) {
	return creator->password;
}

LinphoneAccountCreatorStatus linphone_account_creator_set_transport(LinphoneAccountCreator *creator, LinphoneTransportType transport){
	if (!linphone_core_sip_transport_supported(creator->core, transport)) {
		return LinphoneAccountCreatorTransportNotSupported;
	}
	creator->transport = transport;
	return LinphoneAccountCreatorOK;
}

LinphoneTransportType linphone_account_creator_get_transport(const LinphoneAccountCreator *creator) {
	return creator->transport;
}

LinphoneAccountCreatorStatus linphone_account_creator_set_domain(LinphoneAccountCreator *creator, const char *domain){
	if (validate_uri(NULL, domain, NULL, NULL) != 0) {
		return LinphoneAccountCreatorDomainInvalid;
	}
	set_string(&creator->domain, domain, TRUE);
	return LinphoneAccountCreatorOK;
}

const char * linphone_account_creator_get_domain(const LinphoneAccountCreator *creator) {
	return creator->domain;
}

LinphoneAccountCreatorStatus linphone_account_creator_set_route(LinphoneAccountCreator *creator, const char *route) {
	if (validate_uri(NULL, NULL, route, NULL) != 0) {
		return LinphoneAccountCreatorRouteInvalid;
	}
	set_string(&creator->route, route, TRUE);
	return LinphoneAccountCreatorOK;
}

const char * linphone_account_creator_get_route(const LinphoneAccountCreator *creator) {
	return creator->route;
}

LinphoneAccountCreatorStatus linphone_account_creator_set_display_name(LinphoneAccountCreator *creator, const char *display_name) {
	if (validate_uri(NULL, NULL, NULL, display_name) != 0) {
		return LinphoneAccountCreatorDisplayNameInvalid;
	}
	set_string(&creator->display_name, display_name, FALSE);
	return LinphoneAccountCreatorOK;
}

const char * linphone_account_creator_get_display_name(const LinphoneAccountCreator *creator) {
	return creator->display_name;
}

LinphoneAccountCreatorStatus linphone_account_creator_set_email(LinphoneAccountCreator *creator, const char *email) {
	if (!is_matching_regex(email, "^.+@.+\\.[A-Za-z]{2}[A-Za-z]*$")) {
		return LinphoneAccountCreatorEmailInvalid;
	}
	set_string(&creator->email, email, TRUE);
	return LinphoneAccountCreatorOK;
}

const char * linphone_account_creator_get_email(const LinphoneAccountCreator *creator) {
	return creator->email;
}

void linphone_account_creator_enable_newsletter_subscription(LinphoneAccountCreator *creator, bool_t subscribe) {
	creator->subscribe_to_newsletter = subscribe;
}

bool_t linphone_account_creator_newsletter_subscription_enabled(const LinphoneAccountCreator *creator) {
	return creator->subscribe_to_newsletter;
}

LinphoneAccountCreatorCbs * linphone_account_creator_get_callbacks(const LinphoneAccountCreator *creator) {
	return creator->callbacks;
}

static void _test_existence_cb(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->callbacks->existence_tested != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorReqFailed;
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			int resp = linphone_xml_rpc_request_get_int_response(request);
			status = (resp == 0) ? LinphoneAccountCreatorAccountNotExist : LinphoneAccountCreatorAccountExist;
		}
		creator->callbacks->existence_tested(creator, status);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_test_existence(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	if (!identity) {
		if (creator->callbacks->existence_tested != NULL) {
			creator->callbacks->existence_tested(creator, LinphoneAccountCreatorReqFailed);
		}
		return LinphoneAccountCreatorReqFailed;
	}

	request = linphone_xml_rpc_request_new_with_args("check_account", LinphoneXmlRpcArgInt,
		LinphoneXmlRpcArgString, identity,
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _test_existence_cb);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneAccountCreatorOK;
}

static void _test_validation_cb(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->callbacks->validation_tested != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorReqFailed;
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			int resp = linphone_xml_rpc_request_get_int_response(request);
			status = (resp == 0) ? LinphoneAccountCreatorAccountNotValidated : LinphoneAccountCreatorAccountValidated;
		}
		creator->callbacks->validation_tested(creator, status);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_test_validation(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	if (!identity) {
		if (creator->callbacks->validation_tested != NULL) {
			creator->callbacks->validation_tested(creator, LinphoneAccountCreatorReqFailed);
		}
		return LinphoneAccountCreatorReqFailed;
	}
	request = linphone_xml_rpc_request_new_with_args("check_account_validated", LinphoneXmlRpcArgInt,
		LinphoneXmlRpcArgString, identity,
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _test_validation_cb);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneAccountCreatorOK;
}

static void _create_account_cb(LinphoneXmlRpcRequest *request) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)linphone_xml_rpc_request_get_user_data(request);
	if (creator->callbacks->create_account != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorReqFailed;
		if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
			int resp = linphone_xml_rpc_request_get_int_response(request);
			status = (resp == 0) ? LinphoneAccountCreatorAccountCreated : LinphoneAccountCreatorAccountNotCreated;
		}
		creator->callbacks->create_account(creator, status);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_create_account(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity = _get_identity(creator);
	if (!identity || !creator->email) {
		if (creator->callbacks->create_account != NULL) {
			creator->callbacks->create_account(creator, LinphoneAccountCreatorReqFailed);
		}
		if (identity) ms_free(identity);
		return LinphoneAccountCreatorReqFailed;
	}

	request = linphone_xml_rpc_request_new_with_args("create_account", LinphoneXmlRpcArgInt,
		LinphoneXmlRpcArgString, identity,
		LinphoneXmlRpcArgString, creator->password,
		LinphoneXmlRpcArgString, creator->email,
		LinphoneXmlRpcArgInt, (creator->subscribe_to_newsletter == TRUE) ? 1 : 0,
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_request_set_user_data(request, creator);
	linphone_xml_rpc_request_cbs_set_response(linphone_xml_rpc_request_get_callbacks(request), _create_account_cb);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneAccountCreatorOK;
}

LinphoneProxyConfig * linphone_account_creator_configure(const LinphoneAccountCreator *creator) {
	LinphoneAuthInfo *info;
	LinphoneProxyConfig *cfg = linphone_core_create_proxy_config(creator->core);
	char *identity_str = _get_identity(creator);
 	LinphoneAddress *identity = linphone_address_new(identity_str);
	char *route = NULL;
	char *domain = NULL;
	ms_free(identity_str);
	if (creator->display_name) {
		linphone_address_set_display_name(identity, creator->display_name);
	}
	if (creator->route) {
		route = ms_strdup_printf("%s;transport=%s", creator->route, linphone_transport_to_string(creator->transport));
	}
	if (creator->domain) {
		domain = ms_strdup_printf("%s;transport=%s", creator->domain, linphone_transport_to_string(creator->transport));
	}
	linphone_proxy_config_set_identity_address(cfg, identity);
	linphone_proxy_config_set_server_addr(cfg, domain);
	linphone_proxy_config_set_route(cfg, route);
	linphone_proxy_config_enable_publish(cfg, FALSE);
	linphone_proxy_config_enable_register(cfg, TRUE);

	if (strcmp(creator->domain, "sip.linphone.org") == 0) {
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
			linphone_address_destroy(addr);
		}
		linphone_core_set_stun_server(creator->core, "stun.linphone.org");
		linphone_core_set_firewall_policy(creator->core, LinphonePolicyUseIce);
	}

	info = linphone_auth_info_new(linphone_address_get_username(identity), NULL, creator->password, NULL, NULL, linphone_address_get_domain(identity));
	linphone_core_add_auth_info(creator->core, info);
	linphone_address_destroy(identity);

	if (linphone_core_add_proxy_config(creator->core, cfg) != -1) {
		linphone_core_set_default_proxy(creator->core, cfg);
		return cfg;
	}

	linphone_core_remove_auth_info(creator->core, info);
	linphone_proxy_config_unref(cfg);
	return NULL;
}
