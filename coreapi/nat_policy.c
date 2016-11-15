/*
linphone
Copyright (C) 2010-2016 Belledonne Communications SARL

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

#include "linphone/core.h"
#include "private.h"


static LinphoneNatPolicy * _linphone_nat_policy_new_with_ref(LinphoneCore *lc, const char *ref) {
	LinphoneNatPolicy *policy = belle_sip_object_new(LinphoneNatPolicy);
	policy->lc = lc;
	policy->ref = belle_sip_strdup(ref);
	return policy;
}

static LinphoneNatPolicy * linphone_nat_policy_new(LinphoneCore *lc) {
	char ref[17] = { 0 };
	belle_sip_random_token(ref, 16);
	return _linphone_nat_policy_new_with_ref(lc, ref);
}

static void linphone_nat_policy_destroy(LinphoneNatPolicy *policy) {
	if (policy->ref) belle_sip_free(policy->ref);
	if (policy->stun_server) belle_sip_free(policy->stun_server);
	if (policy->stun_server_username) belle_sip_free(policy->stun_server_username);
	if (policy->stun_addrinfo) bctbx_freeaddrinfo(policy->stun_addrinfo);
	if (policy->stun_resolver_context) {
		sal_resolve_cancel(policy->stun_resolver_context);
		sal_resolver_context_unref(policy->stun_resolver_context);
	}
}

static bool_t linphone_nat_policy_stun_server_activated(LinphoneNatPolicy *policy) {
	const char *server = linphone_nat_policy_get_stun_server(policy);
	return (server != NULL) && (server[0] != '\0')
		&& ((linphone_nat_policy_stun_enabled(policy) == TRUE) || (linphone_nat_policy_turn_enabled(policy) == TRUE));
}



BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneNatPolicy);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneNatPolicy, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_nat_policy_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);


static void _linphone_nat_policy_save_to_config(const LinphoneNatPolicy *policy, LpConfig *config, int index) {
	char *section;
	bctbx_list_t *l = NULL;

	section = belle_sip_strdup_printf("nat_policy_%i", index);
	lp_config_set_string(config, section, "ref", policy->ref);
	lp_config_set_string(config, section, "stun_server", policy->stun_server);
	lp_config_set_string(config, section, "stun_server_username", policy->stun_server_username);
	if (linphone_nat_policy_upnp_enabled(policy)) {
		l = bctbx_list_append(l, "upnp");
	} else {
		if (linphone_nat_policy_stun_enabled(policy)) l = bctbx_list_append(l, "stun");
		if (linphone_nat_policy_turn_enabled(policy)) l = bctbx_list_append(l, "turn");
		if (linphone_nat_policy_ice_enabled(policy)) l = bctbx_list_append(l, "ice");
	}
	lp_config_set_string_list(config, section, "protocols", l);
	belle_sip_free(section);
	bctbx_list_free(l);
}

void linphone_nat_policy_save_to_config(const LinphoneNatPolicy *policy) {
	LpConfig *config = policy->lc->config;
	char *section;
	int index;
	bool_t finished = FALSE;

	for (index = 0; finished != TRUE; index++) {
		section = belle_sip_strdup_printf("nat_policy_%i", index);
		if (lp_config_has_section(config, section)) {
			const char *config_ref = lp_config_get_string(config, section, "ref", NULL);
			if ((config_ref != NULL) && (strcmp(config_ref, policy->ref) == 0)) {
				_linphone_nat_policy_save_to_config(policy, config, index);
				finished = TRUE;
			}
		} else {
			_linphone_nat_policy_save_to_config(policy, config, index);
			finished = TRUE;
		}
		belle_sip_free(section);
	}
}

LinphoneNatPolicy * linphone_nat_policy_ref(LinphoneNatPolicy *policy) {
	belle_sip_object_ref(policy);
	return policy;
}

void linphone_nat_policy_unref(LinphoneNatPolicy *policy) {
	belle_sip_object_unref(policy);
}

void *linphone_nat_policy_get_user_data(const LinphoneNatPolicy *policy) {
	return policy->user_data;
}

void linphone_nat_policy_set_user_data(LinphoneNatPolicy *policy, void *ud) {
	policy->user_data = ud;
}


void linphone_nat_policy_clear(LinphoneNatPolicy *policy) {
	linphone_nat_policy_enable_stun(policy, FALSE);
	linphone_nat_policy_enable_turn(policy, FALSE);
	linphone_nat_policy_enable_ice(policy, FALSE);
	linphone_nat_policy_enable_upnp(policy, FALSE);
	linphone_nat_policy_set_stun_server(policy, NULL);
	linphone_nat_policy_set_stun_server_username(policy, NULL);
}

bool_t linphone_nat_policy_stun_enabled(const LinphoneNatPolicy *policy) {
	return policy->stun_enabled;
}

void linphone_nat_policy_enable_stun(LinphoneNatPolicy *policy, bool_t enable) {
	policy->stun_enabled = enable;
}

bool_t linphone_nat_policy_turn_enabled(const LinphoneNatPolicy *policy) {
	return policy->turn_enabled;
}

void linphone_nat_policy_enable_turn(LinphoneNatPolicy *policy, bool_t enable) {
	policy->turn_enabled = enable;
}

bool_t linphone_nat_policy_ice_enabled(const LinphoneNatPolicy *policy) {
	return policy->ice_enabled;
}

void linphone_nat_policy_enable_ice(LinphoneNatPolicy *policy, bool_t enable) {
	policy->ice_enabled = enable;
}

bool_t linphone_nat_policy_upnp_enabled(const LinphoneNatPolicy *policy) {
	return policy->upnp_enabled;
}

void linphone_nat_policy_enable_upnp(LinphoneNatPolicy *policy, bool_t enable) {
	policy->upnp_enabled = enable;
	if (enable) {
#ifdef BUILD_UPNP
		policy->stun_enabled = policy->turn_enabled = policy->ice_enabled = FALSE;
		ms_warning("Enabling uPnP NAT policy has disabled any other previously enabled policies");
#else
		ms_warning("Cannot enable the uPnP NAT policy because the uPnP support is not compiled in");
#endif
	}
}

const char * linphone_nat_policy_get_stun_server(const LinphoneNatPolicy *policy) {
	return policy->stun_server;
}

void linphone_nat_policy_set_stun_server(LinphoneNatPolicy *policy, const char *stun_server) {
	char *new_stun_server = NULL;

	if (stun_server != NULL) new_stun_server = belle_sip_strdup(stun_server);
	if (policy->stun_server != NULL) {
		belle_sip_free(policy->stun_server);
		policy->stun_server = NULL;
	}
	if (new_stun_server != NULL) {
		policy->stun_server = new_stun_server;
	}
}

const char * linphone_nat_policy_get_stun_server_username(const LinphoneNatPolicy *policy) {
	return policy->stun_server_username;
}

void linphone_nat_policy_set_stun_server_username(LinphoneNatPolicy *policy, const char *username) {
	char *new_username = NULL;

	if (username != NULL) new_username = belle_sip_strdup(username);
	if (policy->stun_server_username != NULL) {
		belle_sip_free(policy->stun_server_username);
		policy->stun_server_username = NULL;
	}
	if (new_username != NULL) policy->stun_server_username = new_username;
}

static void stun_server_resolved(LinphoneNatPolicy *policy, const char *name, struct addrinfo *addrinfo) {
	if (policy->stun_addrinfo) {
		bctbx_freeaddrinfo(policy->stun_addrinfo);
		policy->stun_addrinfo = NULL;
	}
	if (addrinfo) {
		ms_message("Stun server resolution successful.");
	} else {
		ms_warning("Stun server resolution failed.");
	}
	policy->stun_addrinfo = addrinfo;
	if (policy->stun_resolver_context){
		sal_resolver_context_unref(policy->stun_resolver_context);
		policy->stun_resolver_context = NULL;
	}
}

void linphone_nat_policy_resolve_stun_server(LinphoneNatPolicy *policy) {
	const char *service = NULL;

	if (linphone_nat_policy_stun_server_activated(policy) && (policy->lc->sal != NULL) && !policy->stun_resolver_context) {
		char host[NI_MAXHOST];
		int port = 3478;
		linphone_parse_host_port(policy->stun_server, host, sizeof(host), &port);
		if (linphone_nat_policy_turn_enabled(policy)) service = "turn";
		else if (linphone_nat_policy_stun_enabled(policy)) service = "stun";
		if (service != NULL) {
			int family = AF_INET;
			if (linphone_core_ipv6_enabled(policy->lc) == TRUE) family = AF_INET6;
			policy->stun_resolver_context = sal_resolve(policy->lc->sal, service, "udp", host, port, family, (SalResolverCallback)stun_server_resolved, policy);
			if (policy->stun_resolver_context) sal_resolver_context_ref(policy->stun_resolver_context);
		}
	}
}

const struct addrinfo * linphone_nat_policy_get_stun_server_addrinfo(LinphoneNatPolicy *policy) {
	/*
	 * It is critical not to block for a long time if it can't be resolved, otherwise this stucks the main thread when making a call.
	 * On the contrary, a fully asynchronous call initiation is complex to develop.
	 * The compromise is then:
	 *  - have a cache of the stun server addrinfo
	 *  - this cached value is returned when it is non-null
	 *  - an asynchronous resolution is asked each time this function is called to ensure frequent refreshes of the cached value.
	 *  - if no cached value exists, block for a short time; this case must be unprobable because the resolution will be asked each
	 *    time the stun server value is changed.
	 */
	if (linphone_nat_policy_stun_server_activated(policy) && (policy->stun_addrinfo == NULL)) {
		int wait_ms = 0;
		int wait_limit = 1000;
		linphone_nat_policy_resolve_stun_server(policy);
		while ((policy->stun_addrinfo == NULL) && (policy->stun_resolver_context != NULL) && (wait_ms < wait_limit)) {
			sal_iterate(policy->lc->sal);
			ms_usleep(50000);
			wait_ms += 50;
		}
	}
	return policy->stun_addrinfo;
}

LinphoneNatPolicy * linphone_core_create_nat_policy(LinphoneCore *lc) {
	return linphone_nat_policy_new(lc);
}

LinphoneNatPolicy * linphone_core_create_nat_policy_from_config(LinphoneCore *lc, const char *ref) {
	LpConfig *config = lc->config;
	LinphoneNatPolicy *policy = NULL;
	char *section;
	int index;
	bool_t finished = FALSE;

	for (index = 0; finished != TRUE; index++) {
		section = belle_sip_strdup_printf("nat_policy_%i", index);
		if (lp_config_has_section(config, section)) {
			const char *config_ref = lp_config_get_string(config, section, "ref", NULL);
			if ((config_ref != NULL) && (strcmp(config_ref, ref) == 0)) {
				const char *server = lp_config_get_string(config, section, "stun_server", NULL);
				const char *username = lp_config_get_string(config, section, "stun_server_username", NULL);
				bctbx_list_t *l = lp_config_get_string_list(config, section, "protocols", NULL);
				policy = _linphone_nat_policy_new_with_ref(lc, ref);
				if (server != NULL) linphone_nat_policy_set_stun_server(policy, server);
				if (username != NULL) linphone_nat_policy_set_stun_server_username(policy, username);
				if (l != NULL) {
					bool_t upnp_enabled = FALSE;
					bctbx_list_t *elem;
					for (elem = l; elem != NULL; elem = elem->next) {
						const char *value = (const char *)elem->data;
						if (strcmp(value, "stun") == 0) linphone_nat_policy_enable_stun(policy, TRUE);
						else if (strcmp(value, "turn") == 0) linphone_nat_policy_enable_turn(policy, TRUE);
						else if (strcmp(value, "ice") == 0) linphone_nat_policy_enable_ice(policy, TRUE);
						else if (strcmp(value, "upnp") == 0) upnp_enabled = TRUE;
					}
					if (upnp_enabled) linphone_nat_policy_enable_upnp(policy, TRUE);
				}
				finished = TRUE;
			}
		} else finished = TRUE;
		belle_sip_free(section);
	}
	return policy;
}
