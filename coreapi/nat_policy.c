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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "linphonecore.h"
#include "private.h"


static void linphone_nat_policy_destroy(LinphoneNatPolicy *policy) {
	if (policy->ref) belle_sip_free(policy->ref);
	if (policy->stun_server) belle_sip_free(policy->stun_server);
}


BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneNatPolicy);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneNatPolicy, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_nat_policy_destroy,
	NULL, // clone
	NULL, // marshal
	TRUE
);


static LinphoneNatPolicy * _linphone_nat_policy_new_with_ref(const char *ref) {
	LinphoneNatPolicy *policy = belle_sip_object_new(LinphoneNatPolicy);
	belle_sip_object_ref(policy);
	policy->ref = belle_sip_strdup(ref);
	return policy;
}

LinphoneNatPolicy * linphone_nat_policy_new(void) {
	char ref[17] = { 0 };
	belle_sip_random_token(ref, 16);
	return _linphone_nat_policy_new_with_ref(ref);
}

LinphoneNatPolicy * linphone_nat_policy_new_from_config(LpConfig *config, const char *ref) {
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
				MSList *l = lp_config_get_string_list(config, section, "protocols", NULL);
				policy = _linphone_nat_policy_new_with_ref(ref);
				if (server != NULL) linphone_nat_policy_set_stun_server(policy, server);
				if (l != NULL) {
					bool_t upnp_enabled = FALSE;
					MSList *elem;
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

static void _linphone_nat_policy_save_to_config(const LinphoneNatPolicy *policy, LpConfig *config, int index) {
	char *section;
	MSList *l = NULL;

	section = belle_sip_strdup_printf("nat_policy_%i", index);
	lp_config_set_string(config, section, "ref", policy->ref);
	lp_config_set_string(config, section, "stun_server", policy->stun_server);
	if (linphone_nat_policy_upnp_enabled(policy)) {
		l = ms_list_append(l, "upnp");
	} else {
		if (linphone_nat_policy_stun_enabled(policy)) l = ms_list_append(l, "stun");
		if (linphone_nat_policy_turn_enabled(policy)) l = ms_list_append(l, "turn");
		if (linphone_nat_policy_ice_enabled(policy)) l = ms_list_append(l, "ice");
	}
	lp_config_set_string_list(config, section, "protocols", l);
	belle_sip_free(section);
}

void linphone_nat_policy_save_to_config(const LinphoneNatPolicy *policy, LpConfig *config) {
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
	if (policy->stun_server != NULL) {
		belle_sip_free(policy->stun_server);
		policy->stun_server = NULL;
	}
	if (stun_server != NULL) {
		policy->stun_server = belle_sip_strdup(stun_server);
	}
}
