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

#include "linphonecore.h"
#include "private.h"


struct _LinphoneAccountCreator {
	LinphoneXmlRpcSession *xmlrpc_session;
	LinphoneAccountCreatorCb existence_test_cb;
	LinphoneAccountCreatorCb validation_test_cb;
	LinphoneAccountCreatorCb validate_cb;
	LinphoneCore *core;
	void *existence_test_cb_ud;
	void *validation_test_cb_ud;
	void *validate_cb_ud;
	char *xmlrpc_url;
	char *username;
	char *password;
	char *domain;
	char *route;
	char *email;
	bool_t subscribe_to_newsletter;
};

LinphoneAccountCreator * linphone_account_creator_new(LinphoneCore *core, const char *xmlrpc_url) {
	LinphoneAccountCreator *creator;
	creator = ms_new0(LinphoneAccountCreator, 1);
	creator->core = core;
	creator->xmlrpc_session = linphone_xml_rpc_session_new(core, xmlrpc_url);
	return creator;
}

void linphone_account_creator_set_username(LinphoneAccountCreator *creator, const char *username) {
	set_string(&creator->username, username);
}

const char * linphone_account_creator_get_username(const LinphoneAccountCreator *creator) {
	return creator->username;
}

void linphone_account_creator_set_password(LinphoneAccountCreator *creator, const char *password){
	set_string(&creator->password, password);
}

const char * linphone_account_creator_get_password(const LinphoneAccountCreator *creator) {
	return creator->password;
}

void linphone_account_creator_set_domain(LinphoneAccountCreator *creator, const char *domain){
	set_string(&creator->domain, domain);
}

const char * linphone_account_creator_get_domain(const LinphoneAccountCreator *creator) {
	return creator->domain;
}

void linphone_account_creator_set_route(LinphoneAccountCreator *creator, const char *route) {
	set_string(&creator->route, route);
}

const char * linphone_account_creator_get_route(const LinphoneAccountCreator *creator) {
	return creator->route;
}

void linphone_account_creator_set_email(LinphoneAccountCreator *creator, const char *email) {
	set_string(&creator->email, email);
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

void linphone_account_creator_set_test_existence_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorCb cb, void *user_data) {
	creator->existence_test_cb = cb;
	creator->existence_test_cb_ud = user_data;
}

void linphone_account_creator_set_test_validation_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorCb cb, void *user_data) {
	creator->validation_test_cb = cb;
	creator->validation_test_cb_ud = user_data;
}

void linphone_account_creator_set_validate_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorCb cb, void *user_data) {
	creator->validate_cb = cb;
	creator->validate_cb_ud = user_data;
}

static void _test_existence_cb(LinphoneXmlRpcRequest *request, void *user_data) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)user_data;
	if (creator->existence_test_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorFailed;
		if ((linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk)
			&& (linphone_xml_rpc_request_get_int_response(request) == 0)) {
			status = LinphoneAccountCreatorOk;
		}
		creator->existence_test_cb(creator, status, creator->existence_test_cb_ud);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_test_existence(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity;

	if (!creator->username || !creator->domain) return LinphoneAccountCreatorFailed;

	identity = ms_strdup_printf("%s@%s", creator->username, creator->domain);
	request = linphone_xml_rpc_request_new_with_args("check_account", LinphoneXmlRpcArgInt,
		_test_existence_cb, creator,
		LinphoneXmlRpcArgString, identity,
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneAccountCreatorOk;
}

static void _test_validation_cb(LinphoneXmlRpcRequest *request, void *user_data) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)user_data;
	if (creator->validation_test_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorFailed;
		if ((linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk)
			&& (linphone_xml_rpc_request_get_int_response(request) == 1)) {
			status = LinphoneAccountCreatorOk;
		}
		creator->validation_test_cb(creator, status, creator->validation_test_cb_ud);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_test_validation(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity;

	if (!creator->username || !creator->domain) return LinphoneAccountCreatorFailed;

	identity = ms_strdup_printf("%s@%s", creator->username, creator->domain);
	request = linphone_xml_rpc_request_new_with_args("check_account_validated", LinphoneXmlRpcArgInt,
		_test_validation_cb, creator,
		LinphoneXmlRpcArgString, identity,
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneAccountCreatorOk;
}

static void _validate_cb(LinphoneXmlRpcRequest *request, void *user_data) {
	LinphoneAccountCreator *creator = (LinphoneAccountCreator *)user_data;
	if (creator->validate_cb != NULL) {
		LinphoneAccountCreatorStatus status = LinphoneAccountCreatorFailed;
		if ((linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk)
			&& (linphone_xml_rpc_request_get_int_response(request) == 0)) {
			status = LinphoneAccountCreatorOk;
		}
		creator->validate_cb(creator, status, creator->validate_cb_ud);
	}
}

LinphoneAccountCreatorStatus linphone_account_creator_validate(LinphoneAccountCreator *creator) {
	LinphoneXmlRpcRequest *request;
	char *identity;

	if (!creator->username || !creator->domain) return LinphoneAccountCreatorFailed;

	identity = ms_strdup_printf("%s@%s", creator->username, creator->domain);
	request = linphone_xml_rpc_request_new_with_args("create_account", LinphoneXmlRpcArgInt,
		_validate_cb, creator,
		LinphoneXmlRpcArgString, identity,
		LinphoneXmlRpcArgString, creator->password,
		LinphoneXmlRpcArgString, creator->email,
		LinphoneXmlRpcArgInt, (creator->subscribe_to_newsletter == TRUE) ? 1 : 0,
		LinphoneXmlRpcArgNone);
	linphone_xml_rpc_session_send_request(creator->xmlrpc_session, request);
	linphone_xml_rpc_request_unref(request);
	ms_free(identity);
	return LinphoneAccountCreatorOk;
}

LinphoneProxyConfig * linphone_account_creator_configure(const LinphoneAccountCreator *creator) {
	LinphoneAddress *identity;
	LinphoneAuthInfo *info;
	LinphoneProxyConfig *cfg = linphone_core_create_proxy_config(creator->core);
	char *identity_str = ms_strdup_printf("sip:%s@%s", creator->username, creator->domain);

	linphone_proxy_config_set_identity(cfg, identity_str);
	linphone_proxy_config_set_server_addr(cfg, creator->domain);
	linphone_proxy_config_set_route(cfg, creator->route);
	linphone_proxy_config_enable_publish(cfg, FALSE);
	linphone_proxy_config_enable_register(cfg, TRUE);
	ms_free(identity_str);

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

	identity = linphone_address_new(linphone_proxy_config_get_identity(cfg));
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

void linphone_account_creator_destroy(LinphoneAccountCreator *creator){
	linphone_xml_rpc_session_unref(creator->xmlrpc_session);
	if (creator->username) ms_free(creator->username);
	if (creator->password) ms_free(creator->password);
	if (creator->domain) ms_free(creator->domain);
	if (creator->route) ms_free(creator->route);
	if (creator->email) ms_free(creator->email);
	ms_free(creator);
}
