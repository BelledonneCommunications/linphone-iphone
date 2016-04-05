/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)
*/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "linphonecore.h"
#include "sipsetup.h"
#include "lpconfig.h"
#include "private.h"
#include "mediastreamer2/mediastream.h"
#include "enum.h"

#include <ctype.h>

/*store current config related to server location*/
static void linphone_proxy_config_store_server_config(LinphoneProxyConfig* cfg) {
	if (cfg->saved_identity) linphone_address_destroy(cfg->saved_identity);
	if (cfg->identity_address)
		cfg->saved_identity = linphone_address_clone(cfg->identity_address);
	else
		cfg->saved_identity = NULL;

	if (cfg->saved_proxy) linphone_address_destroy(cfg->saved_proxy);
	if (cfg->reg_proxy)
		cfg->saved_proxy = linphone_address_new(cfg->reg_proxy);
	else
		cfg->saved_proxy = NULL;
}

LinphoneProxyConfigAddressComparisonResult linphone_proxy_config_address_equal(const LinphoneAddress *a, const LinphoneAddress *b) {
	if (a == NULL && b == NULL)
		return LinphoneProxyConfigAddressEqual;
	else if (!a || !b)
		return LinphoneProxyConfigAddressDifferent;

	if (linphone_address_equal(a,b))
		return LinphoneProxyConfigAddressEqual;
	if (linphone_address_weak_equal(a,b)) {
		/*also check both transport and uri */
		if (linphone_address_is_secure(a) == linphone_address_is_secure(b) && linphone_address_get_transport(a) == linphone_address_get_transport(b))
			return LinphoneProxyConfigAddressWeakEqual;
		else
			return LinphoneProxyConfigAddressDifferent;
	}
	return LinphoneProxyConfigAddressDifferent; /*either username, domain or port ar not equals*/
}

LinphoneProxyConfigAddressComparisonResult linphone_proxy_config_is_server_config_changed(const LinphoneProxyConfig* cfg) {
	LinphoneAddress *current_proxy=cfg->reg_proxy?linphone_address_new(cfg->reg_proxy):NULL;
	LinphoneProxyConfigAddressComparisonResult result_identity;
	LinphoneProxyConfigAddressComparisonResult result;

	result = linphone_proxy_config_address_equal(cfg->saved_identity,cfg->identity_address);
	if (result == LinphoneProxyConfigAddressDifferent) goto end;
	result_identity = result;

	result = linphone_proxy_config_address_equal(cfg->saved_proxy,current_proxy);
	if (result == LinphoneProxyConfigAddressDifferent) goto end;
	/** If the proxies are equal use the result of the difference between the identities,
	  * otherwise the result is weak-equal and so weak-equal must be returned even if the
	  * identities were equal.
	  */
	if (result == LinphoneProxyConfigAddressEqual) result = result_identity;

	end:
	if (current_proxy) linphone_address_destroy(current_proxy);
	return result;
}

void linphone_proxy_config_write_all_to_config_file(LinphoneCore *lc){
	MSList *elem;
	int i;
	if (!linphone_core_ready(lc)) return;

	for(elem=lc->sip_conf.proxies,i=0;elem!=NULL;elem=ms_list_next(elem),i++){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		linphone_proxy_config_write_to_config_file(lc->config,cfg,i);
	}
	/*to ensure removed configs are erased:*/
	linphone_proxy_config_write_to_config_file(lc->config,NULL,i);
	lp_config_set_int(lc->config,"sip","default_proxy",linphone_core_get_default_proxy_config_index(lc));
}

static void linphone_proxy_config_init(LinphoneCore* lc, LinphoneProxyConfig *cfg) {
	const char *dial_prefix = lc ? lp_config_get_default_string(lc->config,"proxy","dial_prefix",NULL) : NULL;
	const char *identity = lc ? lp_config_get_default_string(lc->config, "proxy", "reg_identity", NULL) : NULL;
	const char *proxy = lc ? lp_config_get_default_string(lc->config, "proxy", "reg_proxy", NULL) : NULL;
	const char *route = lc ? lp_config_get_default_string(lc->config, "proxy", "reg_route", NULL) : NULL;
	const char *realm = lc ? lp_config_get_default_string(lc->config, "proxy", "realm", NULL) : NULL;
	const char *quality_reporting_collector = lc ? lp_config_get_default_string(lc->config, "proxy", "quality_reporting_collector", NULL) : NULL;
	const char *contact_params = lc ? lp_config_get_default_string(lc->config, "proxy", "contact_parameters", NULL) : NULL;
	const char *contact_uri_params = lc ? lp_config_get_default_string(lc->config, "proxy", "contact_uri_parameters", NULL) : NULL;
	const char *refkey = lc ? lp_config_get_default_string(lc->config, "proxy", "refkey", NULL) : NULL;
	cfg->expires = lc ? lp_config_get_default_int(lc->config, "proxy", "reg_expires", 3600) : 3600;
	cfg->reg_sendregister = lc ? lp_config_get_default_int(lc->config, "proxy", "reg_sendregister", 1) : 1;
	cfg->dial_prefix = dial_prefix ? ms_strdup(dial_prefix) : NULL;
	cfg->dial_escape_plus = lc ? lp_config_get_default_int(lc->config, "proxy", "dial_escape_plus", 0) : 0;
	cfg->privacy = lc ? lp_config_get_default_int(lc->config, "proxy", "privacy", LinphonePrivacyDefault) : LinphonePrivacyDefault;
	cfg->identity_address = identity ? linphone_address_new(identity) : NULL;
	cfg->reg_identity = cfg->identity_address ? linphone_address_as_string(cfg->identity_address) : NULL;
	cfg->reg_proxy = proxy ? ms_strdup(proxy) : NULL;
	cfg->reg_route = route ? ms_strdup(route) : NULL;
	cfg->realm = realm ? ms_strdup(realm) : NULL;
	cfg->quality_reporting_enabled = lc ? lp_config_get_default_int(lc->config, "proxy", "quality_reporting_enabled", 0) : 0;
	cfg->quality_reporting_collector = quality_reporting_collector ? ms_strdup(quality_reporting_collector) : NULL;
	cfg->quality_reporting_interval = lc ? lp_config_get_default_int(lc->config, "proxy", "quality_reporting_interval", 0) : 0;
	cfg->contact_params = contact_params ? ms_strdup(contact_params) : NULL;
	cfg->contact_uri_params = contact_uri_params ? ms_strdup(contact_uri_params) : NULL;
	cfg->avpf_mode = lc ? lp_config_get_default_int(lc->config, "proxy", "avpf", LinphoneAVPFDefault) : LinphoneAVPFDefault;
	cfg->avpf_rr_interval = lc ? lp_config_get_default_int(lc->config, "proxy", "avpf_rr_interval", 5) : 5;
	cfg->publish_expires=-1;
	cfg->refkey = refkey ? ms_strdup(refkey) : NULL;
}

LinphoneProxyConfig *linphone_proxy_config_new() {
	return linphone_core_create_proxy_config(NULL);
}

static char * append_linphone_address(LinphoneAddress *addr,char *out) {
	char *res = out;
	if (addr) {
		char *tmp;
		tmp = linphone_address_as_string(addr);
		res = ms_strcat_printf(out, "%s",tmp);
		ms_free(tmp);
	}
	return res;
};
static char * append_string(const char * string,char *out) {
	char *res = out;
	if (string) {
		res = ms_strcat_printf(out, "%s",string);
	}
	return res;
}
/*
 * return true if computed value has changed
 */
bool_t linphone_proxy_config_compute_publish_params_hash(LinphoneProxyConfig * cfg) {
	char * source = NULL;
	char hash[33];
	char saved;
	unsigned long long previous_hash[2];
	previous_hash[0] = cfg->previous_publish_config_hash[0];
	previous_hash[1] = cfg->previous_publish_config_hash[1];

	source = ms_strcat_printf(source, "%i",cfg->privacy);
	source=append_linphone_address(cfg->identity_address, source);
	source=append_string(cfg->reg_proxy,source);
	source=append_string(cfg->reg_route,source);
	source=append_string(cfg->realm,source);
	source = ms_strcat_printf(source, "%i",cfg->publish_expires);
	source = ms_strcat_printf(source, "%i",cfg->publish);
	belle_sip_auth_helper_compute_ha1(source, "dummy", "dummy", hash);
	ms_free(source);
	saved = hash[16];
	hash[16] = '\0';
	cfg->previous_publish_config_hash[0] = strtoull(hash, (char **)NULL, 16);
	hash[16] = saved;
	cfg->previous_publish_config_hash[1] = strtoull(&hash[16], (char **)NULL, 16);
	return previous_hash[0] != cfg->previous_publish_config_hash[0] || previous_hash[1] != cfg->previous_publish_config_hash[1];
}
static void _linphone_proxy_config_destroy(LinphoneProxyConfig *cfg);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneProxyConfig);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneProxyConfig, belle_sip_object_t,
	(belle_sip_object_destroy_t)_linphone_proxy_config_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);

LinphoneProxyConfig * linphone_core_create_proxy_config(LinphoneCore *lc) {
	LinphoneProxyConfig *cfg = belle_sip_object_new(LinphoneProxyConfig);
	linphone_proxy_config_init(lc,cfg);
	return cfg;
}

void _linphone_proxy_config_release_ops(LinphoneProxyConfig *cfg){
	if (cfg->op) {
		sal_op_release(cfg->op);
		cfg->op=NULL;
	}
	if (cfg->long_term_event){
		linphone_event_unref(cfg->long_term_event);
		cfg->long_term_event=NULL;
	}
}

void _linphone_proxy_config_destroy(LinphoneProxyConfig *cfg){
	if (cfg->reg_proxy!=NULL) ms_free(cfg->reg_proxy);
	if (cfg->reg_identity!=NULL) ms_free(cfg->reg_identity);
	if (cfg->identity_address!=NULL) linphone_address_destroy(cfg->identity_address);
	if (cfg->reg_route!=NULL) ms_free(cfg->reg_route);
	if (cfg->quality_reporting_collector!=NULL) ms_free(cfg->quality_reporting_collector);
	if (cfg->ssctx!=NULL) sip_setup_context_free(cfg->ssctx);
	if (cfg->realm!=NULL) ms_free(cfg->realm);
	if (cfg->type!=NULL) ms_free(cfg->type);
	if (cfg->dial_prefix!=NULL) ms_free(cfg->dial_prefix);
	if (cfg->contact_params) ms_free(cfg->contact_params);
	if (cfg->contact_uri_params) ms_free(cfg->contact_uri_params);
	if (cfg->saved_proxy!=NULL) linphone_address_destroy(cfg->saved_proxy);
	if (cfg->saved_identity!=NULL) linphone_address_destroy(cfg->saved_identity);
	if (cfg->sent_headers!=NULL) sal_custom_header_free(cfg->sent_headers);
	if (cfg->pending_contact) linphone_address_unref(cfg->pending_contact);
	if (cfg->refkey) ms_free(cfg->refkey);
	_linphone_proxy_config_release_ops(cfg);
}

void linphone_proxy_config_destroy(LinphoneProxyConfig *cfg) {
	belle_sip_object_unref(cfg);
}

void _linphone_proxy_config_release(LinphoneProxyConfig *cfg) {
	_linphone_proxy_config_release_ops(cfg);
	belle_sip_object_unref(cfg);
}

LinphoneProxyConfig *linphone_proxy_config_ref(LinphoneProxyConfig *cfg) {
	belle_sip_object_ref(cfg);
	return cfg;
}

void linphone_proxy_config_unref(LinphoneProxyConfig *cfg) {
	belle_sip_object_unref(cfg);
}

bool_t linphone_proxy_config_is_registered(const LinphoneProxyConfig *cfg){
	return cfg->state == LinphoneRegistrationOk;
}

int linphone_proxy_config_set_server_addr(LinphoneProxyConfig *cfg, const char *server_addr){
	LinphoneAddress *addr=NULL;
	char *modified=NULL;

	if (cfg->reg_proxy!=NULL) ms_free(cfg->reg_proxy);
	cfg->reg_proxy=NULL;

	if (server_addr!=NULL && strlen(server_addr)>0){
		if (strstr(server_addr,"sip:")==NULL && strstr(server_addr,"sips:")==NULL){
			modified=ms_strdup_printf("sip:%s",server_addr);
			addr=linphone_address_new(modified);
			ms_free(modified);
		}
		if (addr==NULL)
			addr=linphone_address_new(server_addr);
		if (addr){
			cfg->reg_proxy=linphone_address_as_string(addr);
			linphone_address_destroy(addr);
		}else{
			ms_warning("Could not parse %s",server_addr);
			return -1;
		}
	}
	return 0;
}


int linphone_proxy_config_set_identity_address(LinphoneProxyConfig *cfg, const LinphoneAddress *addr){
	if (!addr || linphone_address_get_username(addr)==NULL){
		char* as_string = addr ? linphone_address_as_string(addr) : ms_strdup("NULL");
		ms_warning("Invalid sip identity: %s", as_string);
		ms_free(as_string);
		return -1;
	}
	if (cfg->identity_address != NULL) {
		linphone_address_destroy(cfg->identity_address);
	}
	cfg->identity_address=linphone_address_clone(addr);

	if (cfg->reg_identity!=NULL) {
		ms_free(cfg->reg_identity);
	}
	cfg->reg_identity= linphone_address_as_string(cfg->identity_address);
	return 0;
}

int linphone_proxy_config_set_identity(LinphoneProxyConfig *cfg, const char *identity){
	if (identity!=NULL && strlen(identity)>0){
		LinphoneAddress *addr=linphone_address_new(identity);
		int ret=linphone_proxy_config_set_identity_address(cfg, addr);
		if (addr) linphone_address_destroy(addr);
		return ret;
	}
	return -1;
}

const char *linphone_proxy_config_get_domain(const LinphoneProxyConfig *cfg){
	return cfg->identity_address ? linphone_address_get_domain(cfg->identity_address) : NULL;
}

int linphone_proxy_config_set_route(LinphoneProxyConfig *cfg, const char *route)
{
	if (cfg->reg_route!=NULL){
		ms_free(cfg->reg_route);
		cfg->reg_route=NULL;
	}
	if (route!=NULL && route[0] !='\0'){
		SalAddress *addr;
		char *tmp;
		/*try to prepend 'sip:' */
		if (strstr(route,"sip:")==NULL && strstr(route,"sips:")==NULL){
			tmp=ms_strdup_printf("sip:%s",route);
		}else tmp=ms_strdup(route);
		addr=sal_address_new(tmp);
		if (addr!=NULL){
			sal_address_destroy(addr);
			cfg->reg_route=tmp;
			return 0;
		}else{
			ms_free(tmp);
			return -1;
		}
	} else {
		return 0;
	}
}

bool_t linphone_proxy_config_check(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	if (cfg->reg_proxy==NULL){
		if (lc)
			linphone_core_notify_display_warning(lc,_("The sip proxy address you entered is invalid, it must start with \"sip:\""
						" followed by a hostname."));
		return FALSE;
	}
	if (cfg->identity_address==NULL){
		if (lc)
			linphone_core_notify_display_warning(lc,_("The sip identity you entered is invalid.\nIt should look like "
					"sip:username@proxydomain, such as sip:alice@example.net"));
		return FALSE;
	}
	return TRUE;
}

void linphone_proxy_config_enableregister(LinphoneProxyConfig *cfg, bool_t val){
	cfg->reg_sendregister=val;
}

void linphone_proxy_config_set_expires(LinphoneProxyConfig *cfg, int val){
	if (val<0) val=600;
	cfg->expires=val;
}

void linphone_proxy_config_enable_publish(LinphoneProxyConfig *cfg, bool_t val){
	cfg->publish=val;
}

void linphone_proxy_config_pause_register(LinphoneProxyConfig *cfg){
	if (cfg->op) sal_op_stop_refreshing(cfg->op);
}

void linphone_proxy_config_edit(LinphoneProxyConfig *cfg){
	/*store current config related to server location*/
	linphone_proxy_config_store_server_config(cfg);
	linphone_proxy_config_compute_publish_params_hash(cfg);

	if (cfg->publish && cfg->long_term_event){
		linphone_event_pause_publish(cfg->long_term_event);
	}
	/*stop refresher in any case*/
	linphone_proxy_config_pause_register(cfg);
}

void linphone_proxy_config_apply(LinphoneProxyConfig *cfg,LinphoneCore *lc){
	cfg->lc=lc;
	linphone_proxy_config_done(cfg);
}

void linphone_proxy_config_stop_refreshing(LinphoneProxyConfig * cfg){
	LinphoneAddress *contact_addr=NULL;
	if (	cfg->op
			&& cfg->state == LinphoneRegistrationOk
			&& (contact_addr = (LinphoneAddress*)sal_op_get_contact_address(cfg->op))
			&& linphone_address_get_transport(contact_addr) != LinphoneTransportUdp /*with udp, there is a risk of port reuse, so I prefer to not do anything for now*/) {
		/*need to save current contact in order to reset is later*/
		linphone_address_ref(contact_addr);
		if (cfg->pending_contact)
			linphone_address_unref(cfg->pending_contact);
		cfg->pending_contact=contact_addr;

	}
	if (cfg->long_term_event){ /*might probably do better*/
		linphone_event_terminate(cfg->long_term_event);
		linphone_event_unref(cfg->long_term_event);
		cfg->long_term_event=NULL;
	}
	if (cfg->op){
		sal_op_release(cfg->op);
		cfg->op=NULL;
	}
}

LinphoneAddress *guess_contact_for_register(LinphoneProxyConfig *cfg){
	LinphoneAddress *ret=NULL;
	LinphoneAddress *proxy=linphone_address_new(cfg->reg_proxy);
	const char *host;

	if (proxy==NULL) return NULL;
	host=linphone_address_get_domain(proxy);
	if (host!=NULL){
		int localport = -1;
		const char *localip = NULL;
		LinphoneAddress *contact=linphone_address_clone(cfg->identity_address);

		linphone_address_clean(contact);

		if (cfg->contact_params) {
			// We want to add a list of contacts params to the linphone address
			sal_address_set_params(contact,cfg->contact_params);
		}
		if (cfg->contact_uri_params){
			sal_address_set_uri_params(contact,cfg->contact_uri_params);
		}
#ifdef BUILD_UPNP
		if (cfg->lc->upnp != NULL && linphone_core_get_firewall_policy(cfg->lc)==LinphonePolicyUseUpnp &&
			linphone_upnp_context_get_state(cfg->lc->upnp) == LinphoneUpnpStateOk) {
			localip = linphone_upnp_context_get_external_ipaddress(cfg->lc->upnp);
			localport = linphone_upnp_context_get_external_port(cfg->lc->upnp);
		}
#endif //BUILD_UPNP
		linphone_address_set_port(contact,localport);
		linphone_address_set_domain(contact,localip);
		linphone_address_set_display_name(contact,NULL);

		ret=contact;
	}
	linphone_address_destroy(proxy);
	return ret;
}

void _linphone_proxy_config_unregister(LinphoneProxyConfig *obj) {
	if (obj->op && (obj->state == LinphoneRegistrationOk ||
					(obj->state == LinphoneRegistrationProgress && obj->expires != 0))) {
		sal_unregister(obj->op);
	}
}

static void linphone_proxy_config_register(LinphoneProxyConfig *cfg){
	if (cfg->reg_sendregister){
		LinphoneAddress* proxy=linphone_address_new(cfg->reg_proxy);
		char* proxy_string;
		char * from = linphone_address_as_string(cfg->identity_address);
		LinphoneAddress *contact;
		ms_message("LinphoneProxyConfig [%p] about to register (LinphoneCore version: %s)",cfg,linphone_core_get_version());
		proxy_string=linphone_address_as_string_uri_only(proxy);
		linphone_address_destroy(proxy);
		if (cfg->op)
			sal_op_release(cfg->op);
		cfg->op=sal_op_new(cfg->lc->sal);

		linphone_configure_op(cfg->lc, cfg->op, cfg->identity_address, cfg->sent_headers, FALSE);

		if ((contact=guess_contact_for_register(cfg))) {
			sal_op_set_contact_address(cfg->op,contact);
			linphone_address_destroy(contact);
		}

		sal_op_set_user_pointer(cfg->op,cfg);


		if (sal_register(cfg->op,proxy_string, cfg->reg_identity, cfg->expires, cfg->pending_contact)==0) {
			if (cfg->pending_contact) {
				linphone_address_unref(cfg->pending_contact);
				cfg->pending_contact=NULL;
			}
			linphone_proxy_config_set_state(cfg,LinphoneRegistrationProgress,"Registration in progress");
		} else {
			linphone_proxy_config_set_state(cfg,LinphoneRegistrationFailed,"Registration failed");
		}
		ms_free(proxy_string);
		ms_free(from);
	} else {
		/* unregister if registered*/
		if (cfg->state == LinphoneRegistrationProgress) {
			linphone_proxy_config_set_state(cfg,LinphoneRegistrationCleared,"Registration cleared");
		}
		_linphone_proxy_config_unregister(cfg);
	}
}

void linphone_proxy_config_refresh_register(LinphoneProxyConfig *cfg){
	if (cfg->reg_sendregister && cfg->op && cfg->state!=LinphoneRegistrationProgress){
		if (sal_register_refresh(cfg->op,cfg->expires) == 0) {
			linphone_proxy_config_set_state(cfg,LinphoneRegistrationProgress, "Refresh registration");
		}
	}
}


void linphone_proxy_config_set_dial_prefix(LinphoneProxyConfig *cfg, const char *prefix){
	if (cfg->dial_prefix!=NULL){
		ms_free(cfg->dial_prefix);
		cfg->dial_prefix=NULL;
	}
	if (prefix && prefix[0]!='\0') cfg->dial_prefix=ms_strdup(prefix);
}

const char *linphone_proxy_config_get_dial_prefix(const LinphoneProxyConfig *cfg){
	return cfg->dial_prefix;
}

void linphone_proxy_config_set_dial_escape_plus(LinphoneProxyConfig *cfg, bool_t val){
	cfg->dial_escape_plus=val;
}

bool_t linphone_proxy_config_get_dial_escape_plus(const LinphoneProxyConfig *cfg){
	return cfg->dial_escape_plus;
}

void linphone_proxy_config_enable_quality_reporting(LinphoneProxyConfig *cfg, bool_t val){
	cfg->quality_reporting_enabled = val;
}

bool_t linphone_proxy_config_quality_reporting_enabled(LinphoneProxyConfig *cfg){
	return cfg->quality_reporting_enabled;
}

void linphone_proxy_config_set_quality_reporting_interval(LinphoneProxyConfig *cfg, uint8_t interval) {
	cfg->quality_reporting_interval = interval;
}

int linphone_proxy_config_get_quality_reporting_interval(LinphoneProxyConfig *cfg) {
	return cfg->quality_reporting_interval;
}

void linphone_proxy_config_set_quality_reporting_collector(LinphoneProxyConfig *cfg, const char *collector){
	if (collector!=NULL && strlen(collector)>0){
		LinphoneAddress *addr=linphone_address_new(collector);
		if (!addr){
			ms_error("Invalid SIP collector URI: %s. Quality reporting will be DISABLED.",collector);
		} else {
			if (cfg->quality_reporting_collector != NULL){
				ms_free(cfg->quality_reporting_collector);
			}
			cfg->quality_reporting_collector = ms_strdup(collector);
		}

		if (addr){
			linphone_address_destroy(addr);
		}
	}
}

const char *linphone_proxy_config_get_quality_reporting_collector(const LinphoneProxyConfig *cfg){
	return cfg->quality_reporting_collector;
}


/*
 * http://en.wikipedia.org/wiki/Telephone_numbering_plan
 * http://en.wikipedia.org/wiki/Telephone_numbers_in_Europe
 */
typedef struct dial_plan{
	const char *country;
	const char* iso_country_code; /* ISO 3166-1 alpha-2 code, ex: FR for France*/
	char  ccc[8]; /*country calling code*/
	int nnl; /*maximum national number length*/
	const char * icp; /*international call prefix, ex: 00 in europe*/

}dial_plan_t;

static dial_plan_t const dial_plans[]={
	//Country					, iso country code, e164 country calling code, number length, international usual prefix
	{"Afghanistan"                  ,"AF"		, "93"      , 9		, "00"  },
	{"Albania"                      ,"AL"		, "355"     , 9		, "00"  },
	{"Algeria"                      ,"DZ"		, "213"     , 9		, "00"  },
	{"American Samoa"               ,"AS"		, "1"       , 10	, "011"	},
	{"Andorra"                      ,"AD"		, "376"     , 6		, "00"  },
	{"Angola"                       ,"AO"		, "244"     , 9		, "00"  },
	{"Anguilla"                     ,"AI"		, "1"       , 10	, "011" },
	{"Antigua and Barbuda"          ,"AG"		, "1"       , 10	, "011"	},
	{"Argentina"                    ,"AR"		, "54"      , 10	, "00"  },
	{"Armenia"                      ,"AM"		, "374"     , 8		, "00"  },
	{"Aruba"                        ,"AW"		, "297"     , 7		, "011"	},
	{"Australia"                    ,"AU"		, "61"      , 9	    , "0011"},
	{"Austria"                      ,"AT"		, "43"      , 10	, "00"  },
	{"Azerbaijan"                   ,"AZ"       , "994"     , 9		, "00"  },
	{"Bahamas"                      ,"BS"		, "1"       , 10    , "011"	},
	{"Bahrain"                      ,"BH"		, "973"     , 8     , "00"  },
	{"Bangladesh"                   ,"BD"		, "880"     , 10    , "00"  },
	{"Barbados"                     ,"BB"		, "1"       , 10    , "011"	},
	{"Belarus"                      ,"BY"		, "375"     , 9     , "00"  },
	{"Belgium"                      ,"BE"		, "32"      , 9     , "00"  },
	{"Belize"                       ,"BZ"		, "501"     , 7     , "00"  },
	{"Benin"                        ,"BJ"		, "229"     , 8     , "00"	},
	{"Bermuda"                      ,"BM"		, "1"       , 10    , "011" },
	{"Bhutan"                       ,"BT"		, "975"     , 8     , "00"  },
	{"Bolivia"                      ,"BO"		, "591"     , 8     , "00"	},
	{"Bosnia and Herzegovina"       ,"BA"		, "387"     , 8     , "00"  },
	{"Botswana"                     ,"BW"		, "267"     , 8     , "00"  },
	{"Brazil"                       ,"BR"		, "55"      , 10	, "00"  },
	{"Brunei Darussalam"            ,"BN"		, "673"     , 7		, "00"	},
	{"Bulgaria"                     ,"BG"		, "359"     , 9		, "00"  },
	{"Burkina Faso"                 ,"BF"		, "226"     , 8		, "00"  },
	{"Burundi"                      ,"BI"		, "257"     , 8     , "011" },
	{"Cambodia"                     ,"KH"		, "855"     , 9		, "00"  },
	{"Cameroon"                     ,"CM"		, "237"     , 8		, "00"  },
	{"Canada"                       ,"CA"		, "1"       , 10	, "011" },
	{"Cape Verde"                   ,"CV"		, "238"     , 7		, "00"	},
	{"Cayman Islands"               ,"KY"		, "1"       , 10	, "011" },
	{"Central African Republic"     ,"CF"		, "236"     , 8     , "00"  },
	{"Chad"                         ,"TD"		, "235"     , 8		, "00"	},
	{"Chile"                        ,"CL"		, "56"      , 9	    , "00"  },
	{"China"                        ,"CN"		, "86"      , 11	, "00"  },
	{"Colombia"                     ,"CO"       , "57"      , 10	, "00"  },
	{"Comoros"                      ,"KM"		, "269"     , 7     , "00"	},
	{"Congo"                        ,"CG"		, "242"     , 9		, "00"	},
	{"Congo Democratic Republic"	,"CD"		, "243"     , 9		, "00"  },
	{"Cook Islands"                 ,"CK"		, "682"     , 5		, "00"  },
	{"Costa Rica"                   ,"CR"		, "506"     , 8     , "00"	},
	{"Cote d'Ivoire"	            ,"AD"		, "225"     , 8     , "00"  },
	{"Croatia"                      ,"HR"		, "385"     , 9		, "00"  },
	{"Cuba"                         ,"CU"		, "53"      , 8     , "119" },
	{"Cyprus"                       ,"CY"		, "357"     , 8     , "00"	},
	{"Czech Republic"               ,"CZ"		, "420"     , 9     , "00"  },
	{"Denmark"                      ,"DK"		, "45"      , 8		, "00"  },
	{"Djibouti"                     ,"DJ"		, "253"     , 8		, "00"	},
	{"Dominica"                     ,"DM"		, "1"       , 10	, "011" },
	{"Dominican Republic"	        ,"DO"		, "1"       , 10	, "011" },
	{"Ecuador"                      ,"EC"       , "593"     , 9		, "00"  },
	{"Egypt"                        ,"EG"		, "20"      , 10	, "00"	},
	{"El Salvador"                  ,"SV"		, "503"     , 8		, "00"	},
	{"Equatorial Guinea"            ,"GQ"		, "240"     , 9		, "00"  },
	{"Eritrea"                      ,"ER"		, "291"     , 7		, "00"  },
	{"Estonia"                      ,"EE"		, "372"     , 8     , "00"	},
	{"Ethiopia"                     ,"ET"		, "251"     , 9     , "00"  },
	{"Falkland Islands"	            ,"FK"		, "500"     , 5		, "00"  },
	{"Faroe Islands"	            ,"FO"		, "298"     , 6     , "00"  },
	{"Fiji"                         ,"FJ"		, "679"     , 7     , "00"	},
	{"Finland"                      ,"FI"		, "358"     , 9     , "00"  },
	{"France"                       ,"FR"		, "33"      , 9		, "00"	},
	{"French Guiana"				,"GF"		, "594"     , 9		, "00"	},
	{"French Polynesia"             ,"PF"		, "689"     , 6	    , "00"  },
	{"Gabon"                        ,"GA"		, "241"     , 8     , "00"  },
	{"Gambia"                       ,"GM"       , "220"     , 7		, "00"  },
	{"Georgia"                      ,"GE"		, "995"     , 9     , "00"	},
	{"Germany"                      ,"DE"		, "49"      , 11	, "00"	},
	{"Ghana"                        ,"GH"		, "233"     , 9		, "00"  },
	{"Gibraltar"                    ,"GI"		, "350"     , 8		, "00"  },
	{"Greece"                       ,"GR"		, "30"      ,10     , "00"	},
	{"Greenland"                    ,"GL"		, "299"     , 6		, "00"  },
	{"Grenada"                      ,"GD"		, "1"       , 10	, "011" },
	{"Guadeloupe"                   ,"GP"		, "590"     , 9     , "00"  },
	{"Guam"                         ,"GU"		, "1"       , 10	, "011"	},
	{"Guatemala"                    ,"GT"		, "502"     , 8     , "00"  },
	{"Guinea"                       ,"GN"		, "224"     , 8		, "00"  },
	{"Guinea-Bissau"				,"GW"		, "245"     , 7		, "00"	},
	{"Guyana"                       ,"GY"		, "592"     , 7	    , "001" },
	{"Haiti"                        ,"HT"		, "509"     , 8     , "00"  },
	{"Honduras"                     ,"HN"       , "504"     , 8		, "00"  },
	{"Hong Kong"                    ,"HK"		, "852"     , 8     , "001"	},
	{"Hungary"                      ,"HU"		, "36"      , 9     , "00"  },
	{"Iceland"                      ,"IS"		, "354"     , 9     , "00"  },
	{"India"                        ,"IN"		, "91"      , 10    , "00"  },
	{"Indonesia"                    ,"ID"		, "62"      , 10	, "001"	},
	{"Iran"                         ,"IR"		, "98"      , 10	, "00"	},
	{"Iraq"                         ,"IQ"		, "964"     , 10	, "00"  },
	{"Ireland"                      ,"IE"		, "353"     , 9		, "00"  },
	{"Israel"                       ,"IL"		, "972"     , 9     , "00"	},
	{"Italy"                        ,"IT"		, "39"      , 10	, "00"  },
/*	{"Jersey"                       ,"JE"		, "44"      , 10	, "00"	},*/
	{"Jamaica"                      ,"JM"		, "1"       , 10	, "011" },
	{"Japan"                        ,"JP"		, "81"      , 10	, "010" },
	{"Jordan"                       ,"JO"		, "962"     , 9     , "00"	},
	{"Kazakhstan"                   ,"KZ"		, "7"       , 10    , "00"  },
	{"Kenya"                        ,"KE"		, "254"     , 9		, "000" },
	{"Kiribati"                     ,"KI"		, "686"     , 5		, "00"	},
	{"Korea, North"                 ,"KP"		, "850"     , 12	, "99"  },
	{"Korea, South"                 ,"KR"       , "82"      , 12	, "001" },
	{"Kuwait"                       ,"KW"		, "965"     , 8     , "00"	},
	{"Kyrgyzstan"                   ,"KG"		, "996"     , 9     , "00"  },
	{"Laos"                         ,"LA"		, "856"     , 10    , "00"  },
	{"Latvia"                       ,"LV"		, "371"     , 8     , "00"	},
	{"Lebanon"                      ,"LB"		, "961"     , 7     , "00"	},
	{"Lesotho"                      ,"LS"		, "266"     , 8		, "00"	},
	{"Liberia"                      ,"LR"		, "231"     , 8		, "00"  },
	{"Libya"                        ,"LY"		, "218"     , 8		, "00"  },
	{"Liechtenstein"                ,"LI"		, "423"     , 7     , "00"	},
	{"Lithuania"                    ,"LT"		, "370"     , 8		, "00"  },
	{"Luxembourg"                   ,"LU"		, "352"     , 9		, "00"  },
	{"Macau"                        ,"MO"		, "853"     , 8     , "00"  },
	{"Macedonia"                    ,"MK"		, "389"     , 8     , "00"	},
	{"Madagascar"                   ,"MG"		, "261"     , 9     , "00"  },
	{"Malawi"                       ,"MW"		, "265"     , 9		, "00"  },
	{"Malaysia"                     ,"MY"		, "60"      , 9		, "00"	},
	{"Maldives"                     ,"MV"		, "960"     , 7	    , "00"  },
	{"Mali"                         ,"ML"		, "223"     , 8     , "00"  },
	{"Malta"                        ,"MT"       , "356"     , 8		, "00"  },
	{"Marshall Islands"				,"MH"		, "692"     , 7     , "011"	},
	{"Martinique"                   ,"MQ"		, "596"     , 9     , "00"  },
	{"Mauritania"                   ,"MR"		, "222"     , 8     , "00"  },
	{"Mauritius"                    ,"MU"		, "230"     , 7     , "00"	},
	{"Mayotte Island"               ,"YT"		, "262"     , 9     , "00"	},
	{"Mexico"                       ,"MX"		, "52"      , 10	, "00"	},
	{"Micronesia"                   ,"FM"		, "691"     , 7		, "011" },
	{"Moldova"                      ,"MD"		, "373"     , 8		, "00"  },
	{"Monaco"                       ,"MC"		, "377"     , 8     , "00"	},
	{"Mongolia"                     ,"MN"		, "976"     , 8     , "001" },
	{"Montenegro"                   ,"ME"		, "382"     , 8		, "00"  },
	{"Montserrat"                   ,"MS"		, "664"     , 10	, "011" },
	{"Morocco"                      ,"MA"		, "212"     , 9     , "00"	},
	{"Mozambique"                   ,"MZ"		, "258"     , 9     , "00"  },
	{"Myanmar"                      ,"MM"		, "95"      , 8		, "00"  },
	{"Namibia"                      ,"NA"		, "264"     , 9		, "00"	},
	{"Nauru"                        ,"NR"		, "674"     , 7	    , "00"  },
	{"Nepal"                        ,"NP"		, "43"      , 10	, "00"  },
	{"Netherlands"                  ,"NL"       , "31"      , 9		, "00"  },
	{"New Caledonia"				,"NC"		, "687"     , 6     , "00"	},
	{"New Zealand"                  ,"NZ"		, "64"      , 10	, "00"  },
	{"Nicaragua"                    ,"NI"		, "505"     , 8     , "00"  },
	{"Niger"                        ,"NE"		, "227"     , 8     , "00"	},
	{"Nigeria"                      ,"NG"		, "234"     , 10	, "009"	},
	{"Niue"                         ,"NU"		, "683"     , 4		, "00"	},
	{"Norfolk Island"	            ,"NF"		, "672"     , 5		, "00"  },
	{"Northern Mariana Islands"	    ,"MP"		, "1"       , 10	, "011" },
	{"Norway"                       ,"NO"		, "47"      , 8     , "00"	},
	{"Oman"                         ,"OM"		, "968"     , 8		, "00"  },
	{"Pakistan"                     ,"PK"		, "92"      , 10	, "00"  },
	{"Palau"                        ,"PW"		, "680"     , 7     , "011" },
	{"Palestine"                    ,"PS"		, "970"     , 9     , "00"	},
	{"Panama"                       ,"PA"		, "507"     , 8     , "00"  },
	{"Papua New Guinea"	            ,"PG"		, "675"     , 8		, "00"  },
	{"Paraguay"                     ,"PY"		, "595"     , 9		, "00"	},
	{"Peru"                         ,"PE"		, "51"      , 9	    , "00"  },
	{"Philippines"                  ,"PH"		, "63"      , 10	, "00"  },
	{"Poland"                       ,"PL"       , "48"      , 9		, "00"  },
	{"Portugal"                     ,"PT"		, "351"     , 9     , "00"	},
	{"Puerto Rico"                  ,"PR"		, "1"       , 10	, "011" },
	{"Qatar"                        ,"QA"		, "974"     , 8     , "00"  },
	{"R�union Island"				,"RE"		, "262"     , 9     , "011"	},
	{"Romania"                      ,"RO"		, "40"      , 9     , "00"	},
	{"Russian Federation"           ,"RU"		, "7"       , 10	, "8"	},
	{"Rwanda"                       ,"RW"		, "250"     , 9		, "00"  },
	{"Saint Helena"                 ,"SH"		, "290"     , 4		, "00"  },
	{"Saint Kitts and Nevis"		,"KN"		, "1"       , 10	, "011"	},
	{"Saint Lucia"                  ,"LC"		, "1"       , 10	, "011" },
	{"Saint Pierre and Miquelon"    ,"PM"		, "508"     , 6		, "00"  },
	{"Saint Vincent and the Grenadines","VC"	, "1"       , 10	, "011" },
	{"Samoa"                        ,"WS"		, "685"     , 7     , "0"	},
	{"San Marino"                   ,"SM"		, "378"     , 10	, "00"  },
	{"Sao Tome and Principe"        ,"ST"		, "239"     , 7		, "00"  },
	{"Saudi Arabia"                 ,"SA"		, "966"     , 9		, "00"	},
	{"Senegal"                      ,"SN"		, "221"     , 9	    , "00"  },
	{"Serbia"                       ,"RS"		, "381"     , 9     , "00"  },
	{"Seychelles"                   ,"SC"       , "248"     , 7		, "00"  },
	{"Sierra Leone"                 ,"SL"		, "232"     , 8     , "00"	},
	{"Singapore"                    ,"SG"		, "65"      , 8     , "001" },
	{"Slovakia"                     ,"SK"		, "421"     , 9     , "00"  },
	{"Slovenia"                     ,"SI"		, "386"     , 8     , "00"	},
	{"Solomon Islands"              ,"SB"		, "677"     , 7     , "00"	},
	{"Somalia"                      ,"SO"		, "252"     , 8		, "00"	},
	{"South Africa"                 ,"ZA"		, "27"      , 9		, "00"  },
	{"Spain"                        ,"ES"		, "34"      , 9		, "00"  },
	{"Sri Lanka"                    ,"LK"		, "94"      , 9     , "00"	},
	{"Sudan"                        ,"SD"		, "249"     , 9		, "00"  },
	{"Suriname"                     ,"SR"		, "597"     , 7		, "00"  },
	{"Swaziland"                    ,"SZ"		, "268"     , 8     , "00"  },
	{"Sweden"                       ,"SE"		, "1"       , 9     , "00"	},
	{"Switzerland"                  ,"XK"		, "41"      , 9		, "00"	},
	{"Syria"                        ,"SY"		, "963"     , 9		, "00"  },
	{"Taiwan"                       ,"TW"		, "886"     , 9		, "810"	},
	{"Tajikistan"                   ,"TJ"		, "992"     , 9	    , "002" },
	{"Tanzania"                     ,"TZ"		, "255"     , 9     , "000" },
	{"Thailand"                     ,"TH"       , "66"      , 9		, "001" },
	{"Togo"                         ,"TG"		, "228"     , 8     , "00"	},
	{"Tokelau"                      ,"TK"		, "690"     , 4     , "00"  },
	{"Tonga"                        ,"TO"		, "676"     , 5     , "00"  },
	{"Trinidad and Tobago"			,"TT"		, "1"       , 10    , "011"	},
	{"Tunisia"                      ,"TN"		, "216"     , 8     , "00"	},
	{"Turkey"                       ,"TR"		, "90"      , 10	, "00"	},
	{"Turkmenistan"                 ,"TM"		, "993"     , 8		, "00"  },
	{"Turks and Caicos Islands"	    ,"TC"		, "1"       , 7		, "0"   },
	{"Tuvalu"                       ,"TV"		, "688"     , 5     , "00"	},
	{"Uganda"                       ,"UG"		, "256"     , 9     , "000" },
	{"Ukraine"                      ,"UA"		, "380"     , 9		, "00"  },
	{"United Arab Emirates"	        ,"AE"		, "971"     , 9     , "00"  },
	{"United Kingdom"               ,"GB"		, "44"      , 10	, "00"	},
/*	{"United Kingdom"               ,"UK"		, "44"      , 10	, "00"	},*/
	{"United States"                ,"US"		, "1"       , 10	, "011" },
	{"Uruguay"                      ,"UY"		, "598"     , 8		, "00"  },
	{"Uzbekistan"                   ,"UZ"		, "998"     , 9		, "8"	},
	{"Vanuatu"                      ,"VU"		, "678"     , 7	    , "00"  },
	{"Venezuela"                    ,"VE"		, "58"      , 10	, "00"  },
	{"Vietnam"                      ,"VN"		, "84"      , 9     , "00"  },
	{"Wallis and Futuna"	        ,"WF"		, "681"     , 5		, "00"  },
	{"Yemen"                        ,"YE"		, "967"     , 9     , "00"  },
	{"Zambia"                       ,"ZM"		, "260"     , 9     , "00"	},
	{"Zimbabwe"                     ,"ZW"		, "263"     , 9     , "00"  },
	{NULL                           ,NULL       ,  ""       , 0     , NULL	}
};
static dial_plan_t most_common_dialplan={ "generic" ,"", "", 10, "00"};

int linphone_dial_plan_lookup_ccc_from_e164(const char* e164) {
	dial_plan_t* dial_plan;
	dial_plan_t* elected_dial_plan=NULL;
	unsigned int found;
	unsigned int i=0;
	if (e164[1]=='1') {
		/*USA case*/
		return 1;
	}
	do {
		found=0;
		i++;
		for (dial_plan=(dial_plan_t*)dial_plans; dial_plan->country!=NULL; dial_plan++) {
			if (strncmp(dial_plan->ccc,&e164[1],i) == 0) {
				elected_dial_plan=dial_plan;
				found++;
			}
		}
	} while ((found>1 || found==0) && i < sizeof(dial_plan->ccc));
	if (found==1) {
		return atoi(elected_dial_plan->ccc);
	} else {
		return -1; /*not found */
	}

}
int linphone_dial_plan_lookup_ccc_from_iso(const char* iso) {
	dial_plan_t* dial_plan;
	for (dial_plan=(dial_plan_t*)dial_plans; dial_plan->country!=NULL; dial_plan++) {
		if (strcmp(iso, dial_plan->iso_country_code)==0) {
			return atoi(dial_plan->ccc);
		}
	}
	return -1;
}

static bool_t lookup_dial_plan_by_ccc(const char *ccc, dial_plan_t *plan){
	int i;
	for(i=0;dial_plans[i].country!=NULL;++i){
		if (strcmp(ccc,dial_plans[i].ccc)==0){
			*plan=dial_plans[i];
			return TRUE;
		}
	}
	/*else return a generic "most common" dial plan*/
	*plan=most_common_dialplan;
	strcpy(plan->ccc,ccc);
	return FALSE;
}

bool_t linphone_proxy_config_is_phone_number(LinphoneProxyConfig *proxy, const char *username){
	const char *p;
	if (!username) return FALSE;
	for(p=username;*p!='\0';++p){
		if (isdigit(*p) ||
			*p==' ' ||
			*p=='.' ||
			*p=='-' ||
			*p==')' ||
			*p=='(' ||
			*p=='/' ||
			*p=='+' ||
			(unsigned char)*p==0xca || (unsigned char)*p==0xc2 || (unsigned char)*p==0xa0 // non-breakable space (iOS uses it to format contacts phone number)
			) {
			continue;
		}
		return FALSE;
	}
	return TRUE;
}

//remove anything but [0-9] and +
static char *flatten_number(const char *number){
	char *result=ms_malloc0(strlen(number)+1);
	char *w=result;
	const char *r;
	for(r=number;*r!='\0';++r){
		if (*r=='+' || isdigit(*r)){
			*w++=*r;
		}
	}
	*w++='\0';
	return result;
}

static char* replace_plus_with_icp(char *phone, const char* icp){
	return (icp && phone[0]=='+') ? ms_strdup_printf("%s%s", icp, phone+1) : ms_strdup(phone);
}

static char* replace_icp_with_plus(char *phone, const char *icp){
	return (strstr(phone, icp) == phone) ?  ms_strdup_printf("+%s", phone+strlen(icp)) : ms_strdup(phone);
}

bool_t linphone_proxy_config_normalize_number(LinphoneProxyConfig *proxy, const char *username, char *result, size_t result_len){
	char * normalized_phone = linphone_proxy_config_normalize_phone_number(proxy, username);
	const char * output = normalized_phone ? normalized_phone : username;
	memset(result, 0, result_len);
	memcpy(result, output, MIN(strlen(output) + 1, result_len));
	ms_free(normalized_phone);
	return output != username;
}

char* linphone_proxy_config_normalize_phone_number(LinphoneProxyConfig *proxy, const char *username) {
	LinphoneProxyConfig *tmpproxy = proxy ? proxy : linphone_proxy_config_new();
	char* result = NULL;
	if (linphone_proxy_config_is_phone_number(tmpproxy, username)){
		dial_plan_t dialplan = {0};
		char * flatten=flatten_number(username);
		ms_debug("Flattened number is '%s' for '%s'",flatten, username);

		/*if proxy has a dial prefix, modify phonenumber accordingly*/
		if (tmpproxy->dial_prefix!=NULL && tmpproxy->dial_prefix[0]!='\0'){
			lookup_dial_plan_by_ccc(tmpproxy->dial_prefix,&dialplan);
			ms_debug("Using dial plan '%s'",dialplan.country);
			/* the number already starts with + or international prefix*/
			if (flatten[0]=='+'||strstr(flatten,dialplan.icp)==flatten){
				ms_debug("Prefix already present.");
				if (tmpproxy->dial_escape_plus) {
					result = replace_plus_with_icp(flatten,dialplan.icp);
				} else {
					result = replace_icp_with_plus(flatten,dialplan.icp);
				}
			}else{
				/*0. keep at most national number significant digits */
				char* flatten_start = flatten + MAX(0, (int)strlen(flatten) - (int)dialplan.nnl);
				ms_debug("Prefix not present. Keeping at most %d digits: %s", dialplan.nnl, flatten_start);

				/*1. First prepend international calling prefix or +*/
				/*2. Second add prefix*/
				/*3. Finally add user digits */
				result = ms_strdup_printf("%s%s%s"
											, tmpproxy->dial_escape_plus ? dialplan.icp : "+"
											, dialplan.ccc
											, flatten_start);
				ms_debug("Prepended prefix resulted in %s", result);
			}
		}else if (tmpproxy->dial_escape_plus){
			/* user did not provide dial prefix, so we'll take the most generic one */
			result = replace_plus_with_icp(flatten,most_common_dialplan.icp);
		}
		if (result==NULL) {
			result = flatten;
		} else {
			ms_free(flatten);
		}
	}
	if (proxy==NULL) linphone_proxy_config_destroy(tmpproxy);
	return result;
}

static LinphoneAddress* _linphone_core_destroy_addr_if_not_sip( LinphoneAddress* addr ){
	if( linphone_address_is_sip(addr) ) {
		return addr;
	} else {
		linphone_address_destroy(addr);
		return NULL;
	}
}

LinphoneAddress* linphone_proxy_config_normalize_sip_uri(LinphoneProxyConfig *proxy, const char *username) {
	enum_lookup_res_t *enumres=NULL;
	char *enum_domain=NULL;
	char *tmpurl;
	LinphoneAddress *uri;

	if (!username || *username=='\0') return NULL;

	if (is_enum(username,&enum_domain)){
		if (proxy) {
			linphone_core_notify_display_status(proxy->lc,_("Looking for telephone number destination..."));
		}
		if (enum_lookup(enum_domain,&enumres)<0){
			if (proxy) {
				linphone_core_notify_display_status(proxy->lc,_("Could not resolve this number."));
			}
			ms_free(enum_domain);
			return NULL;
		}
		ms_free(enum_domain);
		tmpurl=enumres->sip_address[0];
		uri=linphone_address_new(tmpurl);
		enum_lookup_res_free(enumres);
		return _linphone_core_destroy_addr_if_not_sip(uri);
	}
	/* check if we have a "sip:" or a "sips:" */
	if ( (strstr(username,"sip:")==NULL) && (strstr(username,"sips:")==NULL) ){
		/* this doesn't look like a true sip uri */
		if (strchr(username,'@')!=NULL){
			/* seems like sip: is missing !*/
			tmpurl=ms_strdup_printf("sip:%s",username);
			uri=linphone_address_new(tmpurl);
			ms_free(tmpurl);
			if (uri){
				return _linphone_core_destroy_addr_if_not_sip(uri);
			}
		}

		if (proxy!=NULL){
			/* append the proxy domain suffix but remove any custom parameters/headers */
			LinphoneAddress *uri=linphone_address_clone(linphone_proxy_config_get_identity_address(proxy));
			linphone_address_clean(uri);
			if (uri==NULL){
				return NULL;
			} else {
				linphone_address_set_display_name(uri,NULL);
				linphone_address_set_username(uri,username);
				return _linphone_core_destroy_addr_if_not_sip(uri);
			}
		} else {
			return NULL;
		}
	}
	uri=linphone_address_new(username);
	if (uri!=NULL){
		return _linphone_core_destroy_addr_if_not_sip(uri);
	}

	return NULL;
}

/**
 * Commits modification made to the proxy configuration.
**/
int linphone_proxy_config_done(LinphoneProxyConfig *cfg)
{
	LinphoneProxyConfigAddressComparisonResult res;

	if (!linphone_proxy_config_check(cfg->lc,cfg))
		return -1;

	/*check if server address as changed*/
	res = linphone_proxy_config_is_server_config_changed(cfg);
	if (res != LinphoneProxyConfigAddressEqual) {
		/* server config has changed, need to unregister from previous first*/
		if (cfg->op) {
			if (res == LinphoneProxyConfigAddressDifferent) {
				_linphone_proxy_config_unregister(cfg);
			}
			sal_op_set_user_pointer(cfg->op,NULL); /*we don't want to receive status for this un register*/
			sal_op_unref(cfg->op); /*but we keep refresher to handle authentication if needed*/
			cfg->op=NULL;
		}
		if (cfg->long_term_event) {
			if (res == LinphoneProxyConfigAddressDifferent) {
				_linphone_proxy_config_unpublish(cfg);
			}

		}
	}
	if (linphone_proxy_config_compute_publish_params_hash(cfg)) {
		ms_message("Publish params have changed on proxy config [%p]",cfg);
		if (cfg->long_term_event) {
			if (!cfg->publish) {
				/*publish is terminated*/
				linphone_event_terminate(cfg->long_term_event);
			}
			linphone_event_unref(cfg->long_term_event);
			cfg->long_term_event = NULL;
		}
		if (cfg->publish) cfg->send_publish=TRUE;
	} else {
		ms_message("Publish params have not changed on proxy config [%p]",cfg);
	}
	cfg->commit=TRUE;
	linphone_proxy_config_write_all_to_config_file(cfg->lc);
	return 0;
}

const char* linphone_proxy_config_get_realm(const LinphoneProxyConfig *cfg)
{
	return cfg?cfg->realm:NULL;
}
void linphone_proxy_config_set_realm(LinphoneProxyConfig *cfg, const char *realm)
{
	if (cfg->realm!=NULL) {
		ms_free(cfg->realm);
	}
	cfg->realm=ms_strdup(realm);
}

int linphone_proxy_config_send_publish(LinphoneProxyConfig *proxy, LinphonePresenceModel *presence){
	int err=0;

	if (proxy->state==LinphoneRegistrationOk || proxy->state==LinphoneRegistrationCleared){
		LinphoneContent *content;
		char *presence_body;
		if (proxy->long_term_event==NULL){
			proxy->long_term_event = linphone_core_create_publish(proxy->lc
										 , linphone_proxy_config_get_identity_address(proxy)
										 , "presence"
										 , linphone_proxy_config_get_publish_expires(proxy));
			linphone_event_ref(proxy->long_term_event);
		}
		proxy->long_term_event->internal = TRUE;

		if (linphone_presence_model_get_presentity(presence) == NULL) {
			ms_message("No presentity set for model [%p], using identity from proxy config [%p]", presence, proxy);
			linphone_presence_model_set_presentity(presence,linphone_proxy_config_get_identity_address(proxy));
		}

		if (!(presence_body = linphone_presence_model_to_xml(presence))) {
			ms_error("Cannot publish presence model [%p] for proxy config [%p] because of xml serilization error",presence,proxy);
			return -1;
		}

		content = linphone_content_new();
		linphone_content_set_buffer(content,presence_body,strlen(presence_body));
		linphone_content_set_type(content, "application");
		linphone_content_set_subtype(content,"pidf+xml");
		err = linphone_event_send_publish(proxy->long_term_event, content);
		linphone_content_unref(content);
		ms_free(presence_body);
	}else proxy->send_publish=TRUE; /*otherwise do not send publish if registration is in progress, this will be done later*/
	return err;
}

void _linphone_proxy_config_unpublish(LinphoneProxyConfig *obj) {
	if (obj->long_term_event
		&& (linphone_event_get_publish_state(obj->long_term_event) == LinphonePublishOk ||
					(linphone_event_get_publish_state(obj->long_term_event)  == LinphonePublishProgress && obj->publish_expires != 0))) {
		linphone_event_unpublish(obj->long_term_event);
	}
}

const char *linphone_proxy_config_get_route(const LinphoneProxyConfig *cfg){
	return cfg->reg_route;
}

const LinphoneAddress *linphone_proxy_config_get_identity_address(const LinphoneProxyConfig *cfg){
	return cfg->identity_address;
}

const char *linphone_proxy_config_get_identity(const LinphoneProxyConfig *cfg){
	return cfg->reg_identity;
}

bool_t linphone_proxy_config_publish_enabled(const LinphoneProxyConfig *cfg){
	return cfg->publish;
}

const char *linphone_proxy_config_get_server_addr(const LinphoneProxyConfig *cfg){
	return cfg->reg_proxy;
}

/**
 * @return the duration of registration.
**/
int linphone_proxy_config_get_expires(const LinphoneProxyConfig *cfg){
	return cfg->expires;
}

bool_t linphone_proxy_config_register_enabled(const LinphoneProxyConfig *cfg){
	return cfg->reg_sendregister;
}

void linphone_proxy_config_set_contact_parameters(LinphoneProxyConfig *cfg, const char *contact_params){
	if (cfg->contact_params) {
		ms_free(cfg->contact_params);
		cfg->contact_params=NULL;
	}
	if (contact_params){
		cfg->contact_params=ms_strdup(contact_params);
	}
}

void linphone_proxy_config_set_contact_uri_parameters(LinphoneProxyConfig *cfg, const char *contact_uri_params){
	if (cfg->contact_uri_params) {
		ms_free(cfg->contact_uri_params);
		cfg->contact_uri_params=NULL;
	}
	if (contact_uri_params){
		cfg->contact_uri_params=ms_strdup(contact_uri_params);
	}
}

const char *linphone_proxy_config_get_contact_parameters(const LinphoneProxyConfig *cfg){
	return cfg->contact_params;
}

const char *linphone_proxy_config_get_contact_uri_parameters(const LinphoneProxyConfig *cfg){
	return cfg->contact_uri_params;
}

struct _LinphoneCore * linphone_proxy_config_get_core(const LinphoneProxyConfig *cfg){
	return cfg->lc;
}

const char *linphone_proxy_config_get_custom_header(LinphoneProxyConfig *cfg, const char *header_name){
	const SalCustomHeader *ch;
	if (!cfg->op) return NULL;
	ch = sal_op_get_recv_custom_header(cfg->op);
	return sal_custom_header_find(ch, header_name);
}

void linphone_proxy_config_set_custom_header(LinphoneProxyConfig *cfg, const char *header_name, const char *header_value){
	cfg->sent_headers=sal_custom_header_append(cfg->sent_headers, header_name, header_value);
}

int linphone_core_add_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	if (!linphone_proxy_config_check(lc,cfg)) {
		return -1;
	}
	if (ms_list_find(lc->sip_conf.proxies,cfg)!=NULL){
		ms_warning("ProxyConfig already entered, ignored.");
		return 0;
	}
	lc->sip_conf.proxies=ms_list_append(lc->sip_conf.proxies,(void *)linphone_proxy_config_ref(cfg));
	linphone_proxy_config_apply(cfg,lc);
	return 0;
}

void linphone_core_remove_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	/* check this proxy config is in the list before doing more*/
	if (ms_list_find(lc->sip_conf.proxies,cfg)==NULL){
		ms_error("linphone_core_remove_proxy_config: LinphoneProxyConfig [%p] is not known by LinphoneCore (programming error?)",cfg);
		return;
	}
	lc->sip_conf.proxies=ms_list_remove(lc->sip_conf.proxies,cfg);
	/* add to the list of destroyed proxies, so that the possible unREGISTER request can succeed authentication */
	lc->sip_conf.deleted_proxies=ms_list_append(lc->sip_conf.deleted_proxies,cfg);

	if (lc->default_proxy==cfg){
		lc->default_proxy=NULL;
	}

	cfg->deletion_date=ms_time(NULL);
	if (cfg->state==LinphoneRegistrationOk){
		/* UNREGISTER */
		linphone_proxy_config_edit(cfg);
		linphone_proxy_config_enable_register(cfg,FALSE);
		linphone_proxy_config_done(cfg);
		linphone_proxy_config_update(cfg);
	} else if (cfg->state != LinphoneRegistrationNone) {
		linphone_proxy_config_set_state(cfg, LinphoneRegistrationNone,"Registration disabled");
	}
	linphone_proxy_config_write_all_to_config_file(lc);
}

void linphone_core_clear_proxy_config(LinphoneCore *lc){
	MSList* list=ms_list_copy(linphone_core_get_proxy_config_list((const LinphoneCore*)lc));
	MSList* copy=list;
	for(;list!=NULL;list=list->next){
		linphone_core_remove_proxy_config(lc,(LinphoneProxyConfig *)list->data);
	}
	ms_list_free(copy);
	linphone_proxy_config_write_all_to_config_file(lc);
}

int linphone_core_get_default_proxy_config_index(LinphoneCore *lc) {
	int pos = -1;
	if (lc->default_proxy != NULL) {
		pos = ms_list_position(lc->sip_conf.proxies, ms_list_find(lc->sip_conf.proxies, (void *)lc->default_proxy));
	}
	return pos;
}

void linphone_core_set_default_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *config){
	/* check if this proxy is in our list */
	if (config!=NULL){
		if (ms_list_find(lc->sip_conf.proxies,config)==NULL){
			ms_warning("Bad proxy address: it is not in the list !");
			lc->default_proxy=NULL;
			return ;
		}
	}
	lc->default_proxy=config;
	if (linphone_core_ready(lc))
		lp_config_set_int(lc->config,"sip","default_proxy",linphone_core_get_default_proxy_config_index(lc));
}

void linphone_core_set_default_proxy_index(LinphoneCore *lc, int index){
	if (index<0) linphone_core_set_default_proxy(lc,NULL);
	else linphone_core_set_default_proxy(lc,ms_list_nth_data(lc->sip_conf.proxies,index));
}

int linphone_core_get_default_proxy(LinphoneCore *lc, LinphoneProxyConfig **config){
	if (config!=NULL) *config=lc->default_proxy;
	return linphone_core_get_default_proxy_config_index(lc);
}

LinphoneProxyConfig * linphone_core_get_default_proxy_config(LinphoneCore *lc) {
	return lc->default_proxy;
}

const MSList *linphone_core_get_proxy_config_list(const LinphoneCore *lc){
	return lc->sip_conf.proxies;
}

void linphone_proxy_config_write_to_config_file(LpConfig *config, LinphoneProxyConfig *cfg, int index)
{
	char key[50];

	sprintf(key,"proxy_%i",index);
	lp_config_clean_section(config,key);
	if (cfg==NULL){
		return;
	}
	if (cfg->type!=NULL){
		lp_config_set_string(config,key,"type",cfg->type);
	}
	if (cfg->reg_proxy!=NULL){
		lp_config_set_string(config,key,"reg_proxy",cfg->reg_proxy);
	}
	if (cfg->reg_route!=NULL){
		lp_config_set_string(config,key,"reg_route",cfg->reg_route);
	}
	if (cfg->reg_identity!=NULL){
		lp_config_set_string(config,key,"reg_identity",cfg->reg_identity);
	}
	if (cfg->realm!=NULL){
		lp_config_set_string(config,key,"realm",cfg->realm);
	}
	if (cfg->contact_params!=NULL){
		lp_config_set_string(config,key,"contact_parameters",cfg->contact_params);
	}
	if (cfg->contact_uri_params!=NULL){
		lp_config_set_string(config,key,"contact_uri_parameters",cfg->contact_uri_params);
	}
	if (cfg->quality_reporting_collector!=NULL){
		lp_config_set_string(config,key,"quality_reporting_collector",cfg->quality_reporting_collector);
	}
	lp_config_set_int(config,key,"quality_reporting_enabled",cfg->quality_reporting_enabled);
	lp_config_set_int(config,key,"quality_reporting_interval",cfg->quality_reporting_interval);
	lp_config_set_int(config,key,"reg_expires",cfg->expires);
	lp_config_set_int(config,key,"reg_sendregister",cfg->reg_sendregister);
	lp_config_set_int(config,key,"publish",cfg->publish);
	lp_config_set_int(config, key, "avpf", cfg->avpf_mode);
	lp_config_set_int(config, key, "avpf_rr_interval", cfg->avpf_rr_interval);
	lp_config_set_int(config,key,"dial_escape_plus",cfg->dial_escape_plus);
	lp_config_set_string(config,key,"dial_prefix",cfg->dial_prefix);
	lp_config_set_int(config,key,"privacy",cfg->privacy);
	if (cfg->refkey) lp_config_set_string(config,key,"refkey",cfg->refkey);
}


#define CONFIGURE_STRING_VALUE(cfg,config,key,param,param_name) \
	{\
	char* default_value = linphone_proxy_config_get_##param(cfg)?ms_strdup(linphone_proxy_config_get_##param(cfg)):NULL;\
	linphone_proxy_config_set_##param(cfg,lp_config_get_string(config,key,param_name,default_value)); \
	if ( default_value) ms_free(default_value); \
	}

#define CONFIGURE_BOOL_VALUE(cfg,config,key,param,param_name) \
	linphone_proxy_config_enable_##param(cfg,lp_config_get_int(config,key,param_name,linphone_proxy_config_##param##_enabled(cfg)));

#define CONFIGURE_INT_VALUE(cfg,config,key,param,param_name) \
		linphone_proxy_config_set_##param(cfg,lp_config_get_int(config,key,param_name,linphone_proxy_config_get_##param(cfg)));

LinphoneProxyConfig *linphone_proxy_config_new_from_config_file(LinphoneCore* lc, int index)
{
	const char *tmp;
	LinphoneProxyConfig *cfg;
	char key[50];
	LpConfig *config=lc->config;

	sprintf(key,"proxy_%i",index);

	if (!lp_config_has_section(config,key)){
		return NULL;
	}

	cfg=linphone_core_create_proxy_config(lc);

	CONFIGURE_STRING_VALUE(cfg,config,key,identity,"reg_identity")
	CONFIGURE_STRING_VALUE(cfg,config,key,server_addr,"reg_proxy")
	CONFIGURE_STRING_VALUE(cfg,config,key,route,"reg_route")

	CONFIGURE_STRING_VALUE(cfg,config,key,realm,"realm")

	CONFIGURE_BOOL_VALUE(cfg,config,key,quality_reporting,"quality_reporting_enabled")
	CONFIGURE_STRING_VALUE(cfg,config,key,quality_reporting_collector,"quality_reporting_collector")
	CONFIGURE_INT_VALUE(cfg,config,key,quality_reporting_interval,"quality_reporting_interval")

	CONFIGURE_STRING_VALUE(cfg,config,key,contact_parameters,"contact_parameters")
	CONFIGURE_STRING_VALUE(cfg,config,key,contact_uri_parameters,"contact_uri_parameters")

	CONFIGURE_INT_VALUE(cfg,config,key,expires,"reg_expires")
	CONFIGURE_BOOL_VALUE(cfg,config,key,register,"reg_sendregister")
	CONFIGURE_BOOL_VALUE(cfg,config,key,publish,"publish")
	CONFIGURE_INT_VALUE(cfg,config,key,avpf_mode,"avpf")
	CONFIGURE_INT_VALUE(cfg,config,key,avpf_rr_interval,"avpf_rr_interval")
	CONFIGURE_INT_VALUE(cfg,config,key,dial_escape_plus,"dial_escape_plus")
	CONFIGURE_STRING_VALUE(cfg,config,key,dial_prefix,"dial_prefix")

	tmp=lp_config_get_string(config,key,"type",NULL);
	if (tmp!=NULL && strlen(tmp)>0)
		linphone_proxy_config_set_sip_setup(cfg,tmp);
	CONFIGURE_INT_VALUE(cfg,config,key,privacy,"privacy")

	CONFIGURE_STRING_VALUE(cfg,config,key,ref_key,"refkey")

	return cfg;
}

static void linphone_proxy_config_activate_sip_setup(LinphoneProxyConfig *cfg){
	SipSetupContext *ssc;
	SipSetup *ss=sip_setup_lookup(cfg->type);
	LinphoneCore *lc=linphone_proxy_config_get_core(cfg);
	unsigned int caps;
	if (!ss) return ;
	ssc=sip_setup_context_new(ss,cfg);
	cfg->ssctx=ssc;
	if (cfg->reg_identity==NULL){
		ms_error("Invalid identity for this proxy configuration.");
		return;
	}
	caps=sip_setup_context_get_capabilities(ssc);
	if (caps & SIP_SETUP_CAP_ACCOUNT_MANAGER){
		if (sip_setup_context_login_account(ssc,cfg->reg_identity,NULL,NULL)!=0){
			{
				char *tmp=ms_strdup_printf(_("Could not login as %s"),cfg->reg_identity);
				linphone_core_notify_display_warning(lc,tmp);
				ms_free(tmp);
			}
			return;
		}
	}
	if (caps & SIP_SETUP_CAP_PROXY_PROVIDER){
		char proxy[256];
		if (sip_setup_context_get_proxy(ssc,NULL,proxy,sizeof(proxy))==0){
			linphone_proxy_config_set_server_addr(cfg,proxy);
		}else{
			ms_error("Could not retrieve proxy uri !");
		}
	}

}

SipSetup *linphone_proxy_config_get_sip_setup(LinphoneProxyConfig *cfg){
	if (cfg->ssctx!=NULL) return cfg->ssctx->funcs;
	if (cfg->type!=NULL){
		return sip_setup_lookup(cfg->type);
	}
	return NULL;
}

static bool_t can_register(LinphoneProxyConfig *cfg){
	LinphoneCore *lc=cfg->lc;
#ifdef BUILD_UPNP
	if (linphone_core_get_firewall_policy(lc)==LinphonePolicyUseUpnp){
		if(lc->sip_conf.register_only_when_upnp_is_ok &&
			(lc->upnp == NULL || !linphone_upnp_context_is_ready_for_register(lc->upnp))) {
			return FALSE;
		}
	}
#endif //BUILD_UPNP
	if (lc->sip_conf.register_only_when_network_is_up){
		return lc->sip_network_reachable;
	}
	return TRUE;
}

void linphone_proxy_config_update(LinphoneProxyConfig *cfg){
	LinphoneCore *lc=cfg->lc;
	if (cfg->commit){
		if (cfg->type && cfg->ssctx==NULL){
			linphone_proxy_config_activate_sip_setup(cfg);
		}
		if (can_register(cfg)){
			linphone_proxy_config_register(cfg);
			cfg->commit=FALSE;
		}
	}
	if (cfg->send_publish && (cfg->state==LinphoneRegistrationOk || cfg->state==LinphoneRegistrationCleared)){
		linphone_proxy_config_send_publish(cfg,lc->presence_model);
		cfg->send_publish=FALSE;
	}
}

void linphone_proxy_config_set_sip_setup(LinphoneProxyConfig *cfg, const char *type){
	if (cfg->type)
		ms_free(cfg->type);
	cfg->type=ms_strdup(type);
	if (linphone_proxy_config_get_addr(cfg)==NULL){
		/*put a placeholder so that the sip setup gets saved into the config */
		linphone_proxy_config_set_server_addr(cfg,"sip:undefined");
	}
}

SipSetupContext *linphone_proxy_config_get_sip_setup_context(LinphoneProxyConfig *cfg){
	return cfg->ssctx;
}

void linphone_proxy_config_set_user_data(LinphoneProxyConfig *cfg, void *ud) {
	cfg->user_data = ud;
}

void * linphone_proxy_config_get_user_data(const LinphoneProxyConfig *cfg) {
	return cfg->user_data;
}

void linphone_proxy_config_set_state(LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const char *message){
	LinphoneCore *lc=cfg->lc;

	if (state==LinphoneRegistrationProgress) {
		char *msg=ortp_strdup_printf(_("Refreshing on %s..."), linphone_proxy_config_get_identity(cfg));
		linphone_core_notify_display_status(lc,msg);
		ms_free(msg);

	}

	if (cfg->state!=state || state==LinphoneRegistrationOk) { /*allow multiple notification of LinphoneRegistrationOk for refreshing*/
		ms_message("Proxy config [%p] for identity [%s] moving from state [%s] to [%s] on core [%p]"	,
					cfg,
					linphone_proxy_config_get_identity(cfg),
					linphone_registration_state_to_string(cfg->state),
					linphone_registration_state_to_string(state),
					cfg->lc);
		if (linphone_core_should_subscribe_friends_only_when_registered(lc) && cfg->state!=state && state == LinphoneRegistrationOk){
			ms_message("Updating friends for identity [%s] on core [%p]",linphone_proxy_config_get_identity(cfg),cfg->lc);
			/* state must be updated before calling linphone_core_update_friends_subscriptions*/
			cfg->state=state;
			linphone_core_update_friends_subscriptions(lc,cfg,TRUE);
		} else {
			/*at this point state must be updated*/
			cfg->state=state;
		}

		if (lc){
			linphone_core_notify_registration_state_changed(lc,cfg,state,message);
			linphone_core_repair_calls(lc);
		}
	} else {
		/*state already reported*/
	}
}

LinphoneRegistrationState linphone_proxy_config_get_state(const LinphoneProxyConfig *cfg){
	return cfg->state;
}

 const char *linphone_registration_state_to_string(LinphoneRegistrationState cs){
	 switch(cs){
		case LinphoneRegistrationCleared:
			 return "LinphoneRegistrationCleared";
		break;
		case LinphoneRegistrationNone:
			 return "LinphoneRegistrationNone";
		break;
		case LinphoneRegistrationProgress:
			return "LinphoneRegistrationProgress";
		break;
		case LinphoneRegistrationOk:
			 return "LinphoneRegistrationOk";
		break;
		case LinphoneRegistrationFailed:
			 return "LinphoneRegistrationFailed";
		break;
	 }
	 return NULL;
 }

LinphoneReason linphone_proxy_config_get_error(const LinphoneProxyConfig *cfg) {
	return linphone_error_info_get_reason(linphone_proxy_config_get_error_info(cfg));
}

const LinphoneErrorInfo *linphone_proxy_config_get_error_info(const LinphoneProxyConfig *cfg){
	return linphone_error_info_from_sal_op(cfg->op);
}

const LinphoneAddress* linphone_proxy_config_get_service_route(const LinphoneProxyConfig* cfg) {
	return cfg->op?(const LinphoneAddress*) sal_op_get_service_route(cfg->op):NULL;
}
const char* linphone_proxy_config_get_transport(const LinphoneProxyConfig *cfg) {
	const char* addr=NULL;
	const char* ret="udp"; /*default value*/
	SalAddress* route_addr=NULL;
	if (linphone_proxy_config_get_service_route(cfg)) {
		route_addr=(SalAddress*)linphone_proxy_config_get_service_route(cfg);
	} else if (linphone_proxy_config_get_route(cfg)) {
		addr=linphone_proxy_config_get_route(cfg);
	} else if(linphone_proxy_config_get_addr(cfg)) {
		addr=linphone_proxy_config_get_addr(cfg);
	} else {
		ms_error("Cannot guess transport for proxy with identity [%s]",linphone_proxy_config_get_identity(cfg));
		return NULL;
	}

	if (route_addr || (route_addr=sal_address_new(addr))) {
		ret=sal_transport_to_string(sal_address_get_transport(route_addr));
		if (!linphone_proxy_config_get_service_route(cfg)) {
			sal_address_destroy(route_addr);
		}
	}

	return ret;
}
void linphone_proxy_config_set_privacy(LinphoneProxyConfig *params, LinphonePrivacyMask privacy) {
	params->privacy=privacy;
}
LinphonePrivacyMask linphone_proxy_config_get_privacy(const LinphoneProxyConfig *params) {
	return params->privacy;
}
void linphone_proxy_config_set_publish_expires(LinphoneProxyConfig *cfg, int expires) {
	cfg->publish_expires=expires;
}
int linphone_proxy_config_get_publish_expires(const LinphoneProxyConfig *cfg) {
	if (cfg->publish_expires<0) {
		return cfg->expires; /*default value is same as register*/
	} else {
		return cfg->publish_expires;
	}
}

void linphone_proxy_config_enable_avpf(LinphoneProxyConfig *cfg, bool_t enable) {
	cfg->avpf_mode=enable ? LinphoneAVPFEnabled : LinphoneAVPFDisabled;
}

bool_t linphone_proxy_config_avpf_enabled(LinphoneProxyConfig *cfg) {
	if (cfg->avpf_mode==LinphoneAVPFDefault && cfg->lc){
		return linphone_core_get_avpf_mode(cfg->lc)==LinphoneAVPFEnabled;
	}
	return cfg->avpf_mode == LinphoneAVPFEnabled;
}

LinphoneAVPFMode linphone_proxy_config_get_avpf_mode(const LinphoneProxyConfig *cfg){
	return cfg->avpf_mode;
}

void linphone_proxy_config_set_avpf_mode(LinphoneProxyConfig *cfg, LinphoneAVPFMode mode){
	cfg->avpf_mode=mode;
}

void linphone_proxy_config_set_avpf_rr_interval(LinphoneProxyConfig *cfg, uint8_t interval) {
	if (interval > 5) interval = 5;
	cfg->avpf_rr_interval = interval;
}

uint8_t linphone_proxy_config_get_avpf_rr_interval(const LinphoneProxyConfig *cfg) {
	return cfg->avpf_rr_interval;
}

const LinphoneAddress* linphone_proxy_config_get_contact(const LinphoneProxyConfig *cfg) {
	return sal_op_get_contact_address(cfg->op);
}

const struct _LinphoneAuthInfo* linphone_proxy_config_find_auth_info(const LinphoneProxyConfig *cfg) {
	const char* username = cfg->identity_address ? linphone_address_get_username(cfg->identity_address) : NULL;
	const char* domain =  cfg->identity_address ? linphone_address_get_domain(cfg->identity_address) : NULL;
	return _linphone_core_find_auth_info(cfg->lc, cfg->realm, username, domain, TRUE);
}

const char * linphone_proxy_config_get_ref_key(const LinphoneProxyConfig *cfg) {
	return cfg->refkey;
}

void linphone_proxy_config_set_ref_key(LinphoneProxyConfig *cfg, const char *refkey) {
	if (cfg->refkey!=NULL){
		ms_free(cfg->refkey);
		cfg->refkey=NULL;
	}
	if (refkey) cfg->refkey=ms_strdup(refkey);
}
