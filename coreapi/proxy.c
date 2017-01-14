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

#include "linphone/core.h"
#include "linphone/core_utils.h"
#include "linphone/sipsetup.h"
#include "linphone/lpconfig.h"
#include "private.h"
#include "mediastreamer2/mediastream.h"
#include "enum.h"
#include <ctype.h>

/*store current config related to server location*/
static void linphone_proxy_config_store_server_config(LinphoneProxyConfig* cfg) {
	if (cfg->saved_identity) linphone_address_unref(cfg->saved_identity);
	if (cfg->identity_address)
		cfg->saved_identity = linphone_address_clone(cfg->identity_address);
	else
		cfg->saved_identity = NULL;

	if (cfg->saved_proxy) linphone_address_unref(cfg->saved_proxy);
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
		if (linphone_address_get_secure(a) == linphone_address_get_secure(b) && linphone_address_get_transport(a) == linphone_address_get_transport(b))
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
	if (current_proxy) linphone_address_unref(current_proxy);
	ms_message("linphone_proxy_config_is_server_config_changed : %i", result);
	return result;
}

void linphone_proxy_config_write_all_to_config_file(LinphoneCore *lc){
	bctbx_list_t *elem;
	int i;
	if (!linphone_core_ready(lc)) return;

	for(elem=lc->sip_conf.proxies,i=0;elem!=NULL;elem=bctbx_list_next(elem),i++){
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
	cfg->publish_expires= lc ? lp_config_get_default_int(lc->config, "proxy", "publish_expires", -1) : -1;
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
		linphone_event_terminate(cfg->long_term_event);
		linphone_event_unref(cfg->long_term_event);
		cfg->long_term_event=NULL;
	}
}

void _linphone_proxy_config_destroy(LinphoneProxyConfig *cfg){
	if (cfg->reg_proxy!=NULL) ms_free(cfg->reg_proxy);
	if (cfg->reg_identity!=NULL) ms_free(cfg->reg_identity);
	if (cfg->identity_address!=NULL) linphone_address_unref(cfg->identity_address);
	if (cfg->reg_route!=NULL) ms_free(cfg->reg_route);
	if (cfg->quality_reporting_collector!=NULL) ms_free(cfg->quality_reporting_collector);
	if (cfg->ssctx!=NULL) sip_setup_context_free(cfg->ssctx);
	if (cfg->realm!=NULL) ms_free(cfg->realm);
	if (cfg->type!=NULL) ms_free(cfg->type);
	if (cfg->dial_prefix!=NULL) ms_free(cfg->dial_prefix);
	if (cfg->contact_params) ms_free(cfg->contact_params);
	if (cfg->contact_uri_params) ms_free(cfg->contact_uri_params);
	if (cfg->saved_proxy!=NULL) linphone_address_unref(cfg->saved_proxy);
	if (cfg->saved_identity!=NULL) linphone_address_unref(cfg->saved_identity);
	if (cfg->sent_headers!=NULL) sal_custom_header_free(cfg->sent_headers);
	if (cfg->pending_contact) linphone_address_unref(cfg->pending_contact);
	if (cfg->refkey) ms_free(cfg->refkey);
	if (cfg->nat_policy != NULL) {
		linphone_nat_policy_unref(cfg->nat_policy);
	}
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
			linphone_address_unref(addr);
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
		linphone_address_unref(cfg->identity_address);
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
		if (addr) linphone_address_unref(addr);
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
	if (val != cfg->reg_sendregister) cfg->register_changed = TRUE;
	cfg->reg_sendregister=val;
}

void linphone_proxy_config_set_expires(LinphoneProxyConfig *cfg, int val){
	if (val<0) val=600;
	if (val != cfg->expires) cfg->register_changed = TRUE;
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
	/*Don't stop refresher*/
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
	linphone_address_unref(proxy);
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
		linphone_address_unref(proxy);
		if (cfg->op)
			sal_op_release(cfg->op);
		cfg->op=sal_op_new(cfg->lc->sal);

		linphone_configure_op(cfg->lc, cfg->op, cfg->identity_address, cfg->sent_headers, FALSE);

		if ((contact=guess_contact_for_register(cfg))) {
			sal_op_set_contact_address(cfg->op,contact);
			linphone_address_unref(contact);
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

void linphone_proxy_config_set_quality_reporting_interval(LinphoneProxyConfig *cfg, int interval) {
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
			linphone_address_unref(addr);
		}
	}
}

const char *linphone_proxy_config_get_quality_reporting_collector(const LinphoneProxyConfig *cfg){
	return cfg->quality_reporting_collector;
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

/*static char* replace_plus_with_icp(char *phone, const char* icp){
	return (icp && phone[0]=='+') ? ms_strdup_printf("%s%s", icp, phone+1) : ms_strdup(phone);
}*/

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
	LinphoneDialPlan dialplan = {0};
	char * nationnal_significant_number = NULL;
	int ccc = -1;
	
	if (linphone_proxy_config_is_phone_number(tmpproxy, username)){
		char * flatten=flatten_number(username);
		ms_debug("Flattened number is '%s' for '%s'",flatten, username);

		ccc = linphone_dial_plan_lookup_ccc_from_e164(flatten);
		if (ccc>-1) { /*e164 like phone number*/
			dialplan = *linphone_dial_plan_by_ccc_as_int(ccc);
			nationnal_significant_number = strstr(flatten, dialplan.ccc);
			if (nationnal_significant_number) {
				nationnal_significant_number +=strlen(dialplan.ccc);
			}
		} else if (flatten[0] =='+') {
			ms_message ("Unknown ccc for e164 like number [%s]", flatten);
			goto end;
		} else {
			dialplan = *linphone_dial_plan_by_ccc(tmpproxy->dial_prefix); //copy dial plan;
			if (tmpproxy->dial_prefix){
				if (strcmp(tmpproxy->dial_prefix,dialplan.ccc) != 0){
					//probably generic dialplan, preserving proxy dial prefix
					strncpy(dialplan.ccc,tmpproxy->dial_prefix,sizeof(dialplan.ccc));
				}
			}
			if (strstr(flatten,dialplan.icp)==flatten) {
				char *e164 = replace_icp_with_plus(flatten,dialplan.icp);
				result = linphone_proxy_config_normalize_phone_number(tmpproxy,e164);
				ms_free(e164);
				goto end;
			}
			nationnal_significant_number=flatten;
		}
		ms_debug("Using dial plan '%s'",dialplan.country);

		/*if proxy has a dial prefix, modify phonenumber accordingly*/
		if (dialplan.ccc[0]!='\0') {
			/* the number already starts with + or international prefix*/
			/*0. keep at most national number significant digits */
			char* nationnal_significant_number_start = nationnal_significant_number
														+ MAX(0, (int)strlen(nationnal_significant_number)
														- (int)dialplan.nnl);
			ms_debug("Prefix not present. Keeping at most %d digits: %s", dialplan.nnl, nationnal_significant_number_start);

			/*1. First prepend international calling prefix or +*/
			/*2. Second add prefix*/
			/*3. Finally add user digits */
			result = ms_strdup_printf("%s%s%s"
										, tmpproxy->dial_escape_plus ? dialplan.icp : "+"
										, dialplan.ccc
										, nationnal_significant_number_start);
			ms_debug("Prepended prefix resulted in %s", result);
		}

	end:
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
		linphone_address_unref(addr);
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

	/*check if server address has changed*/
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
		cfg->commit = TRUE;
	}
	if (cfg->register_changed){
		cfg->commit = TRUE;
		cfg->register_changed = FALSE;
	}
	if (cfg->commit){
		linphone_proxy_config_pause_register(cfg);
	}
	
	if (linphone_proxy_config_compute_publish_params_hash(cfg)) {
		ms_message("Publish params have changed on proxy config [%p]",cfg);
		if (cfg->long_term_event) {
			if (cfg->publish) {
				const char * sip_etag = linphone_event_get_custom_header(cfg->long_term_event, "SIP-ETag");
				if (sip_etag) {
					if (cfg->sip_etag) ms_free(cfg->sip_etag);
					cfg->sip_etag = ms_strdup(sip_etag);
				}
			}
			/*publish is terminated*/
			linphone_event_terminate(cfg->long_term_event);
			linphone_event_unref(cfg->long_term_event);
			cfg->long_term_event = NULL;
		}
		if (cfg->publish) cfg->send_publish=TRUE;
	} else {
		ms_message("Publish params have not changed on proxy config [%p]",cfg);
	}
	
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
		if (proxy->sip_etag) {
			linphone_event_add_custom_header(proxy->long_term_event, "SIP-If-Match", proxy->sip_etag);
			ms_free(proxy->sip_etag);
			proxy->sip_etag=NULL;
		}
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
	if (obj->sip_etag) {
		ms_free(obj->sip_etag);
		obj->sip_etag=NULL;
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
	cfg->register_changed = TRUE;
}

void linphone_proxy_config_set_contact_uri_parameters(LinphoneProxyConfig *cfg, const char *contact_uri_params){
	if (cfg->contact_uri_params) {
		ms_free(cfg->contact_uri_params);
		cfg->contact_uri_params=NULL;
	}
	if (contact_uri_params){
		cfg->contact_uri_params=ms_strdup(contact_uri_params);
	}
	cfg->register_changed = TRUE;
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
	cfg->register_changed = TRUE;
}

int linphone_core_add_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	if (!linphone_proxy_config_check(lc,cfg)) {
		return -1;
	}
	if (bctbx_list_find(lc->sip_conf.proxies,cfg)!=NULL){
		ms_warning("ProxyConfig already entered, ignored.");
		return 0;
	}
	lc->sip_conf.proxies=bctbx_list_append(lc->sip_conf.proxies,(void *)linphone_proxy_config_ref(cfg));
	linphone_proxy_config_apply(cfg,lc);
	return 0;
}

void linphone_core_remove_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	/* check this proxy config is in the list before doing more*/
	if (bctbx_list_find(lc->sip_conf.proxies,cfg)==NULL){
		ms_error("linphone_core_remove_proxy_config: LinphoneProxyConfig [%p] is not known by LinphoneCore (programming error?)",cfg);
		return;
	}
	lc->sip_conf.proxies=bctbx_list_remove(lc->sip_conf.proxies,cfg);
	/* add to the list of destroyed proxies, so that the possible unREGISTER request can succeed authentication */
	lc->sip_conf.deleted_proxies=bctbx_list_append(lc->sip_conf.deleted_proxies,cfg);

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
	bctbx_list_t* list=bctbx_list_copy(linphone_core_get_proxy_config_list((const LinphoneCore*)lc));
	bctbx_list_t* copy=list;
	for(;list!=NULL;list=list->next){
		linphone_core_remove_proxy_config(lc,(LinphoneProxyConfig *)list->data);
	}
	bctbx_list_free(copy);
	linphone_proxy_config_write_all_to_config_file(lc);
}

int linphone_core_get_default_proxy_config_index(LinphoneCore *lc) {
	int pos = -1;
	if (lc->default_proxy != NULL) {
		pos = bctbx_list_position(lc->sip_conf.proxies, bctbx_list_find(lc->sip_conf.proxies, (void *)lc->default_proxy));
	}
	return pos;
}

void linphone_core_set_default_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *config){
	/* check if this proxy is in our list */
	if (config!=NULL){
		if (bctbx_list_find(lc->sip_conf.proxies,config)==NULL){
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
	else linphone_core_set_default_proxy(lc,bctbx_list_nth_data(lc->sip_conf.proxies,index));
}

int linphone_core_get_default_proxy(LinphoneCore *lc, LinphoneProxyConfig **config){
	if (config!=NULL) *config=lc->default_proxy;
	return linphone_core_get_default_proxy_config_index(lc);
}

LinphoneProxyConfig * linphone_core_get_default_proxy_config(LinphoneCore *lc) {
	return lc->default_proxy;
}

const bctbx_list_t *linphone_core_get_proxy_config_list(const LinphoneCore *lc){
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
	lp_config_set_int(config, key, "publish_expires", cfg->publish_expires);

	if (cfg->nat_policy != NULL) {
		lp_config_set_string(config, key, "nat_policy_ref", cfg->nat_policy->ref);
		linphone_nat_policy_save_to_config(cfg->nat_policy);
	}
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
	const char *nat_policy_ref;

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
	CONFIGURE_INT_VALUE(cfg,config,key,publish_expires,"publish_expires")

	nat_policy_ref = lp_config_get_string(config, key, "nat_policy_ref", NULL);
	if (nat_policy_ref != NULL) {
		cfg->nat_policy = linphone_core_create_nat_policy_from_config(lc, nat_policy_ref);
	}

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
			linphone_core_update_friends_subscriptions(lc);
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

LinphoneNatPolicy * linphone_proxy_config_get_nat_policy(const LinphoneProxyConfig *cfg) {
	return cfg->nat_policy;
}

void linphone_proxy_config_set_nat_policy(LinphoneProxyConfig *cfg, LinphoneNatPolicy *policy) {
	if (policy != NULL) policy = linphone_nat_policy_ref(policy); /* Prevent object destruction if the same policy is used */
	if (cfg->nat_policy != NULL) linphone_nat_policy_unref(cfg->nat_policy);
	cfg->nat_policy = policy;
}
