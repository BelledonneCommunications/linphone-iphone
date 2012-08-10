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


#include <ctype.h>


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
	lp_config_set_int(lc->config,"sip","default_proxy",linphone_core_get_default_proxy(lc,NULL));
}

void linphone_proxy_config_init(LinphoneProxyConfig *obj){
	memset(obj,0,sizeof(LinphoneProxyConfig));
	obj->magic=linphone_proxy_config_magic;
	obj->expires=3600;
}

/**
 * @addtogroup proxies
 * @{
**/

/**
 * Creates an empty proxy config.
**/
LinphoneProxyConfig *linphone_proxy_config_new(){
	LinphoneProxyConfig *obj=NULL;
	obj=ms_new(LinphoneProxyConfig,1);
	linphone_proxy_config_init(obj);
	return obj;
}

/**
 * Destroys a proxy config.
 * 
 * @note: LinphoneProxyConfig that have been removed from LinphoneCore with
 * linphone_core_remove_proxy_config() must not be freed.
**/
void linphone_proxy_config_destroy(LinphoneProxyConfig *obj){
	if (obj->reg_proxy!=NULL) ms_free(obj->reg_proxy);
	if (obj->reg_identity!=NULL) ms_free(obj->reg_identity);
	if (obj->reg_route!=NULL) ms_free(obj->reg_route);
	if (obj->ssctx!=NULL) sip_setup_context_free(obj->ssctx);
	if (obj->realm!=NULL) ms_free(obj->realm);
	if (obj->type!=NULL) ms_free(obj->type);
	if (obj->dial_prefix!=NULL) ms_free(obj->dial_prefix);
	if (obj->op) sal_op_release(obj->op);
	if (obj->publish_op) sal_op_release(obj->publish_op);
}

/**
 * Returns a boolean indicating that the user is sucessfully registered on the proxy.
**/
bool_t linphone_proxy_config_is_registered(const LinphoneProxyConfig *obj){
	return obj->state == LinphoneRegistrationOk;
}

/**
 * Sets the proxy address
 *
 * Examples of valid sip proxy address are:
 * - IP address: sip:87.98.157.38
 * - IP address with port: sip:87.98.157.38:5062
 * - hostnames : sip:sip.example.net
**/
int linphone_proxy_config_set_server_addr(LinphoneProxyConfig *obj, const char *server_addr){
	LinphoneAddress *addr=NULL;
	char *modified=NULL;
	
	if (obj->reg_proxy!=NULL) ms_free(obj->reg_proxy);
	obj->reg_proxy=NULL;
	
	if (server_addr!=NULL && strlen(server_addr)>0){
		if (strstr(server_addr,"sip:")==NULL){
			modified=ms_strdup_printf("sip:%s",server_addr);
			addr=linphone_address_new(modified);
			ms_free(modified);
		}
		if (addr==NULL)
			addr=linphone_address_new(server_addr);
		if (addr){
			obj->reg_proxy=linphone_address_as_string_uri_only(addr);
			linphone_address_destroy(addr);
		}else{
			ms_warning("Could not parse %s",server_addr);
			return -1;
		}
	}
	return 0;
}

/**
 * Sets the user identity as a SIP address.
 *
 * This identity is normally formed with display name, username and domain, such 
 * as:
 * Alice <sip:alice@example.net>
 * The REGISTER messages will have from and to set to this identity.
 *
**/
int linphone_proxy_config_set_identity(LinphoneProxyConfig *obj, const char *identity){
	LinphoneAddress *addr;
	if (identity!=NULL && strlen(identity)>0){
		addr=linphone_address_new(identity);
		if (!addr || linphone_address_get_username(addr)==NULL){
			ms_warning("Invalid sip identity: %s",identity);
			if (addr)
				linphone_address_destroy(addr);
			return -1;
		}else{
			if (obj->reg_identity!=NULL) {
				ms_free(obj->reg_identity);
				obj->reg_identity=NULL;
			}
			obj->reg_identity=ms_strdup(identity);
			if (obj->realm){
				ms_free(obj->realm);
			}
			obj->realm=ms_strdup(linphone_address_get_domain(addr));
			linphone_address_destroy(addr);
			return 0;
		}
	}
	return -1;
}

const char *linphone_proxy_config_get_domain(const LinphoneProxyConfig *cfg){
	return cfg->realm;
}

/**
 * Sets a SIP route.
 * When a route is set, all outgoing calls will go to the route's destination if this proxy
 * is the default one (see linphone_core_set_default_proxy() ).
**/
int linphone_proxy_config_set_route(LinphoneProxyConfig *obj, const char *route)
{
	if (obj->reg_route!=NULL){
		ms_free(obj->reg_route);
		obj->reg_route=NULL;
	}
	if (route!=NULL){
		SalAddress *addr;
		char *tmp;
		/*try to prepend 'sip:' */
		if (strstr(route,"sip:")==NULL){
			tmp=ms_strdup_printf("sip:%s",route);
		}else tmp=ms_strdup(route);
		addr=sal_address_new(tmp);
		if (addr!=NULL){
			sal_address_destroy(addr);
		}else{
			ms_free(tmp);
			tmp=NULL;
		}
		obj->reg_route=tmp;
	}
	return 0;
}

bool_t linphone_proxy_config_check(LinphoneCore *lc, LinphoneProxyConfig *obj){
	if (obj->reg_proxy==NULL){
		if (lc->vtable.display_warning)
			lc->vtable.display_warning(lc,_("The sip proxy address you entered is invalid, it must start with \"sip:\""
						" followed by a hostname."));
		return FALSE;
	}
	if (obj->reg_identity==NULL){
		if (lc->vtable.display_warning)
			lc->vtable.display_warning(lc,_("The sip identity you entered is invalid.\nIt should look like "
					"sip:username@proxydomain, such as sip:alice@example.net"));
		return FALSE;
	}
	return TRUE;
}

/**
 * Indicates whether a REGISTER request must be sent to the proxy.
**/
void linphone_proxy_config_enableregister(LinphoneProxyConfig *obj, bool_t val){
	obj->reg_sendregister=val;
}

/**
 * Sets the registration expiration time in seconds.
**/
void linphone_proxy_config_expires(LinphoneProxyConfig *obj, int val){
	if (val<0) val=600;
	obj->expires=val;
}

void linphone_proxy_config_enable_publish(LinphoneProxyConfig *obj, bool_t val){
	obj->publish=val;
}
/**
 * Starts editing a proxy configuration.
 *
 * Because proxy configuration must be consistent, applications MUST
 * call linphone_proxy_config_edit() before doing any attempts to modify
 * proxy configuration (such as identity, proxy address and so on).
 * Once the modifications are done, then the application must call
 * linphone_proxy_config_done() to commit the changes.
**/
void linphone_proxy_config_edit(LinphoneProxyConfig *obj){
	if (obj->reg_sendregister){
		/* unregister */
		if (obj->state != LinphoneRegistrationNone && obj->state != LinphoneRegistrationCleared) {
			sal_unregister(obj->op);
		}
	}
}

void linphone_proxy_config_apply(LinphoneProxyConfig *obj,LinphoneCore *lc)
{
	obj->lc=lc;
	linphone_proxy_config_done(obj);
}

static char *guess_contact_for_register(LinphoneProxyConfig *obj){
	LinphoneAddress *proxy=linphone_address_new(obj->reg_proxy);
	char *ret=NULL;
	const char *host;
	if (proxy==NULL) return NULL;
	host=linphone_address_get_domain (proxy);
	if (host!=NULL){
		char localip[LINPHONE_IPADDR_SIZE];
		char *tmp;
		LCSipTransports tr;
		LinphoneAddress *contact;
		
		linphone_core_get_local_ip(obj->lc,host,localip);
		contact=linphone_address_new(obj->reg_identity);
		linphone_address_set_domain (contact,localip);
		linphone_address_set_port_int(contact,linphone_core_get_sip_port(obj->lc));
		linphone_address_set_display_name(contact,NULL);
		
		linphone_core_get_sip_transports(obj->lc,&tr);
		if (tr.udp_port <= 0) {
			if (tr.tcp_port>0) {
				sal_address_set_param(contact,"transport","tcp");
			} else if (tr.tls_port>0) {
				sal_address_set_param(contact,"transport","tls");
			}
		}
		tmp=linphone_address_as_string_uri_only(contact);
		if (obj->contact_params)
			ret=ms_strdup_printf("<%s;%s>",tmp,obj->contact_params);
		else ret=ms_strdup_printf("<%s>",tmp);
		linphone_address_destroy(contact);
		ms_free(tmp);
	}
	linphone_address_destroy (proxy);
	return ret;
}

static void linphone_proxy_config_register(LinphoneProxyConfig *obj){
	if (obj->reg_sendregister){
		char *contact;
		if (obj->op)
			sal_op_release(obj->op);
		obj->op=sal_op_new(obj->lc->sal);
		contact=guess_contact_for_register(obj);
		sal_op_set_contact(obj->op,contact);
		ms_free(contact);
		sal_op_set_user_pointer(obj->op,obj);
		if (sal_register(obj->op,obj->reg_proxy,obj->reg_identity,obj->expires)==0) {
			linphone_proxy_config_set_state(obj,LinphoneRegistrationProgress,"Registration in progress");
		} else {
			linphone_proxy_config_set_state(obj,LinphoneRegistrationFailed,"Registration failed");
		}
	}
}

/**
 * Refresh a proxy registration.
 * This is useful if for example you resuming from suspend, thus IP address may have changed.
**/
void linphone_proxy_config_refresh_register(LinphoneProxyConfig *obj){
	if (obj->reg_sendregister && obj->op){
		if (sal_register_refresh(obj->op,obj->expires) == 0) {
			linphone_proxy_config_set_state(obj,LinphoneRegistrationProgress, "Refresh registration");
		}
	}
}


/**
 * Sets a dialing prefix to be automatically prepended when inviting a number with 
 * #linphone_core_invite.
 *
**/
void linphone_proxy_config_set_dial_prefix(LinphoneProxyConfig *cfg, const char *prefix){
	if (cfg->dial_prefix!=NULL){
		ms_free(cfg->dial_prefix);
		cfg->dial_prefix=NULL;
	}
	if (prefix && prefix[0]!='\0') cfg->dial_prefix=ms_strdup(prefix);
}

/**
 * Returns dialing prefix.
 *
 * 
**/
const char *linphone_proxy_config_get_dial_prefix(const LinphoneProxyConfig *cfg){
	return cfg->dial_prefix;
}

/**
 * Sets whether liblinphone should replace "+" by "00" in dialed numbers (passed to
 * #linphone_core_invite ).
 *
**/
void linphone_proxy_config_set_dial_escape_plus(LinphoneProxyConfig *cfg, bool_t val){
	cfg->dial_escape_plus=val;
}

/**
 * Returns whether liblinphone should replace "+" by "00" in dialed numbers (passed to
 * #linphone_core_invite ).
 *
**/
bool_t linphone_proxy_config_get_dial_escape_plus(const LinphoneProxyConfig *cfg){
	return cfg->dial_escape_plus;
}


static bool_t is_a_phone_number(const char *username){
	const char *p;
	for(p=username;*p!='\0';++p){
		if (isdigit(*p) || 
		    *p==' ' ||
		    *p=='-' ||
		    *p==')' ||
			*p=='(' ||
			*p=='/' ||
			*p=='+') continue;
		else return FALSE;
	}
	return TRUE;
}

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

static void copy_result(const char *src, char *dest, size_t destlen, bool_t escape_plus){
	int i=0;
	
	if (escape_plus && src[0]=='+' && destlen>2){
		dest[0]='0';
		dest[1]='0';
		src++;
		i=2;
	}
	
	for(;(i<destlen-1) && *src!='\0';++i){
		dest[i]=*src;
		src++;
	}
	dest[i]='\0';
}


static char *append_prefix(const char *number, const char *prefix){
	char *res=ms_malloc(strlen(number)+strlen(prefix)+1);
	strcpy(res,prefix);
	return strcat(res,number);
}

int linphone_proxy_config_normalize_number(LinphoneProxyConfig *proxy, const char *username, char *result, size_t result_len){
	char *flatten;
	int numlen;
	if (is_a_phone_number(username)){
		flatten=flatten_number(username);
		ms_message("Flattened number is '%s'",flatten);
		numlen=strlen(flatten);
		if (numlen>10 || flatten[0]=='+' || proxy->dial_prefix==NULL || proxy->dial_prefix[0]=='\0'){
			ms_message("No need to add a prefix");
			/* prefix is already there */
			copy_result(flatten,result,result_len,proxy->dial_escape_plus);
			ms_free(flatten);
			return 0;
		}else if (proxy->dial_prefix && proxy->dial_prefix[0]!='\0'){
			char *prefixed;
			int skipped=0;
			ms_message("Need to prefix with %s",proxy->dial_prefix);
			if (numlen==10){
				/*remove initial number before prepending prefix*/
				skipped=1;
			}
			prefixed=append_prefix(flatten+skipped,proxy->dial_prefix);
			ms_free(flatten);
			copy_result(prefixed,result,result_len,proxy->dial_escape_plus);
			ms_free(prefixed);
		}
	}else strncpy(result,username,result_len);
	return 0;
}

/**
 * Commits modification made to the proxy configuration.
**/
int linphone_proxy_config_done(LinphoneProxyConfig *obj)
{
	if (!linphone_proxy_config_check(obj->lc,obj)) return -1;
	obj->commit=TRUE;
	linphone_proxy_config_write_all_to_config_file(obj->lc);
	return 0;
}

void linphone_proxy_config_set_realm(LinphoneProxyConfig *cfg, const char *realm)
{
	if (cfg->realm!=NULL) {
		ms_free(cfg->realm);
		cfg->realm=NULL;
	}
	if (realm!=NULL) cfg->realm=ms_strdup(realm);
}

int linphone_proxy_config_send_publish(LinphoneProxyConfig *proxy,
			       LinphoneOnlineStatus presence_mode){
	int err;
	SalOp *op=sal_op_new(proxy->lc->sal);
	err=sal_publish(op,linphone_proxy_config_get_identity(proxy),
	    linphone_proxy_config_get_identity(proxy),linphone_online_status_to_sal(presence_mode));
	if (proxy->publish_op!=NULL)
		sal_op_release(proxy->publish_op);
	proxy->publish_op=op;
	return err;
}

/**
 * Returns the route set for this proxy configuration.
**/
const char *linphone_proxy_config_get_route(const LinphoneProxyConfig *obj){
	return obj->reg_route;
}

/**
 * Returns the SIP identity that belongs to this proxy configuration.
 *
 * The SIP identity is a SIP address (Display Name <sip:username@@domain> )
**/
const char *linphone_proxy_config_get_identity(const LinphoneProxyConfig *obj){
	return obj->reg_identity;
}

/**
 * Returns TRUE if PUBLISH request is enabled for this proxy.
**/
bool_t linphone_proxy_config_publish_enabled(const LinphoneProxyConfig *obj){
	return obj->publish;
}

/**
 * Returns the proxy's SIP address.
**/
const char *linphone_proxy_config_get_addr(const LinphoneProxyConfig *obj){
	return obj->reg_proxy;
}

/**
 * Returns the duration of registration.
**/
int linphone_proxy_config_get_expires(const LinphoneProxyConfig *obj){
	return obj->expires;
}

/**
 * Returns TRUE if registration to the proxy is enabled.
**/
bool_t linphone_proxy_config_register_enabled(const LinphoneProxyConfig *obj){
	return obj->reg_sendregister;
}

/**
 * Set optional contact parameters that will be added to the contact information sent in the registration.
 * @param obj the proxy config object
 * @param contact_params a string contaning the additional parameters in text form, like "myparam=something;myparam2=something_else"
 *
 * The main use case for this function is provide the proxy additional information regarding the user agent, like for example unique identifier or apple push id.
 * As an example, the contact address in the SIP register sent will look like <sip:joe@15.128.128.93:50421;apple-push-id=43143-DFE23F-2323-FA2232>.
**/
void linphone_proxy_config_set_contact_parameters(LinphoneProxyConfig *obj, const char *contact_params){
	if (obj->contact_params) {
		ms_free(obj->contact_params);
		obj->contact_params=NULL;
	}
	if (contact_params){
		obj->contact_params=ms_strdup(contact_params);
	}
}

/**
 * Returns previously set contact parameters.
**/
const char *linphone_proxy_config_get_contact_parameters(const LinphoneProxyConfig *obj){
	return obj->contact_params;
}

struct _LinphoneCore * linphone_proxy_config_get_core(const LinphoneProxyConfig *obj){
	return obj->lc;
}

/**
 * Add a proxy configuration.
 * This will start registration on the proxy, if registration is enabled.
**/
int linphone_core_add_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	if (!linphone_proxy_config_check(lc,cfg)) {
		return -1;
	}
	if (ms_list_find(lc->sip_conf.proxies,cfg)!=NULL){
		ms_warning("ProxyConfig already entered, ignored.");
		return 0;
	}
	lc->sip_conf.proxies=ms_list_append(lc->sip_conf.proxies,(void *)cfg);
	linphone_proxy_config_apply(cfg,lc);
	return 0;
}

/**
 * Removes a proxy configuration.
 *
 * LinphoneCore will then automatically unregister and place the proxy configuration
 * on a deleted list. For that reason, a removed proxy does NOT need to be freed.
**/
void linphone_core_remove_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	/* check this proxy config is in the list before doing more*/
	if (ms_list_find(lc->sip_conf.proxies,cfg)==NULL){
		ms_error("linphone_core_remove_proxy_config: LinphoneProxyConfig %p is not known by LinphoneCore (programming error?)",cfg);
		return;
	}
	lc->sip_conf.proxies=ms_list_remove(lc->sip_conf.proxies,(void *)cfg);
	/* add to the list of destroyed proxies, so that the possible unREGISTER request can succeed authentication */
	lc->sip_conf.deleted_proxies=ms_list_append(lc->sip_conf.deleted_proxies,(void *)cfg);
	cfg->deletion_date=ms_time(NULL);
	if (cfg->state==LinphoneRegistrationOk){
		/* this will unREGISTER */
		linphone_proxy_config_edit(cfg);
	}
	if (lc->default_proxy==cfg){
		lc->default_proxy=NULL;
	}
	linphone_proxy_config_write_all_to_config_file(lc);
}
/**
 * Erase all proxies from config.
 *
 * @ingroup proxy
**/
void linphone_core_clear_proxy_config(LinphoneCore *lc){
	MSList* list=ms_list_copy(linphone_core_get_proxy_config_list((const LinphoneCore*)lc));
	MSList* copy=list;
	for(;list!=NULL;list=list->next){
		linphone_core_remove_proxy_config(lc,(LinphoneProxyConfig *)list->data);
	}
	ms_list_free(copy);
	linphone_proxy_config_write_all_to_config_file(lc);
}
/**
 * Sets the default proxy.
 *
 * This default proxy must be part of the list of already entered LinphoneProxyConfig.
 * Toggling it as default will make LinphoneCore use the identity associated with
 * the proxy configuration in all incoming and outgoing calls.
**/
void linphone_core_set_default_proxy(LinphoneCore *lc, LinphoneProxyConfig *config){
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
		lp_config_set_int(lc->config,"sip","default_proxy",linphone_core_get_default_proxy(lc,NULL));
}	

void linphone_core_set_default_proxy_index(LinphoneCore *lc, int index){
	if (index<0) linphone_core_set_default_proxy(lc,NULL);
	else linphone_core_set_default_proxy(lc,ms_list_nth_data(lc->sip_conf.proxies,index));
}

/**
 * Returns the default proxy configuration, that is the one used to determine the current identity.
**/
int linphone_core_get_default_proxy(LinphoneCore *lc, LinphoneProxyConfig **config){
	int pos=-1;
	if (config!=NULL) *config=lc->default_proxy;
	if (lc->default_proxy!=NULL){
		pos=ms_list_position(lc->sip_conf.proxies,ms_list_find(lc->sip_conf.proxies,(void *)lc->default_proxy));
	}
	return pos;
}

/**
 * Returns an unmodifiable list of entered proxy configurations.
**/
const MSList *linphone_core_get_proxy_config_list(const LinphoneCore *lc){
	return lc->sip_conf.proxies;
}

void linphone_proxy_config_write_to_config_file(LpConfig *config, LinphoneProxyConfig *obj, int index)
{
	char key[50];

	sprintf(key,"proxy_%i",index);
	lp_config_clean_section(config,key);
	if (obj==NULL){
		return;
	}
	if (obj->type!=NULL){
		lp_config_set_string(config,key,"type",obj->type);
	}
	if (obj->reg_proxy!=NULL){
		lp_config_set_string(config,key,"reg_proxy",obj->reg_proxy);
	}
	if (obj->reg_route!=NULL){
		lp_config_set_string(config,key,"reg_route",obj->reg_route);
	}
	if (obj->reg_identity!=NULL){
		lp_config_set_string(config,key,"reg_identity",obj->reg_identity);
	}
	lp_config_set_int(config,key,"reg_expires",obj->expires);
	lp_config_set_int(config,key,"reg_sendregister",obj->reg_sendregister);
	lp_config_set_int(config,key,"publish",obj->publish);
	lp_config_set_int(config,key,"dial_escape_plus",obj->dial_escape_plus);
	lp_config_set_string(config,key,"dial_prefix",obj->dial_prefix);
}



LinphoneProxyConfig *linphone_proxy_config_new_from_config_file(LpConfig *config, int index)
{
	const char *tmp;
	const char *identity;
	const char *proxy;
	LinphoneProxyConfig *cfg;
	char key[50];
	
	sprintf(key,"proxy_%i",index);

	if (!lp_config_has_section(config,key)){
		return NULL;
	}

	cfg=linphone_proxy_config_new();

	identity=lp_config_get_string(config,key,"reg_identity",NULL);	
	proxy=lp_config_get_string(config,key,"reg_proxy",NULL);
	
	linphone_proxy_config_set_identity(cfg,identity);
	linphone_proxy_config_set_server_addr(cfg,proxy);
	
	tmp=lp_config_get_string(config,key,"reg_route",NULL);
	if (tmp!=NULL) linphone_proxy_config_set_route(cfg,tmp);

	linphone_proxy_config_expires(cfg,lp_config_get_int(config,key,"reg_expires",600));
	linphone_proxy_config_enableregister(cfg,lp_config_get_int(config,key,"reg_sendregister",0));
	
	linphone_proxy_config_enable_publish(cfg,lp_config_get_int(config,key,"publish",0));

	linphone_proxy_config_set_dial_escape_plus(cfg,lp_config_get_int(config,key,"dial_escape_plus",0));
	linphone_proxy_config_set_dial_prefix(cfg,lp_config_get_string(config,key,"dial_prefix",NULL));
	
	tmp=lp_config_get_string(config,key,"type",NULL);
	if (tmp!=NULL && strlen(tmp)>0) 
		linphone_proxy_config_set_sip_setup(cfg,tmp);

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
		if (sip_setup_context_login_account(ssc,cfg->reg_identity,NULL)!=0){
			if (lc->vtable.display_warning){
				char *tmp=ms_strdup_printf(_("Could not login as %s"),cfg->reg_identity);
				lc->vtable.display_warning(lc,tmp);
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

void linphone_proxy_config_update(LinphoneProxyConfig *cfg){
	LinphoneCore *lc=cfg->lc;
	if (cfg->commit){
		if (cfg->type && cfg->ssctx==NULL){
			linphone_proxy_config_activate_sip_setup(cfg);
		}
		if (!lc->sip_conf.register_only_when_network_is_up || lc->network_reachable)
			linphone_proxy_config_register(cfg);
		if (cfg->publish && cfg->publish_op==NULL){
			linphone_proxy_config_send_publish(cfg,lc->presence_mode);
		}
		cfg->commit=FALSE;
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

/**
 * @}
**/

LinphoneAccountCreator *linphone_account_creator_new(struct _LinphoneCore *core, const char *type){
	LinphoneAccountCreator *obj;
	LinphoneProxyConfig *cfg;
	SipSetup *ss=sip_setup_lookup(type);
	SipSetupContext *ssctx;
	if (!ss){
		return NULL;
	}
	if (!(sip_setup_get_capabilities(ss) & SIP_SETUP_CAP_ACCOUNT_MANAGER)){
		ms_error("%s cannot manage accounts.",type);
		return NULL;
	}
	obj=ms_new0(LinphoneAccountCreator,1);
	cfg=linphone_proxy_config_new();
	ssctx=sip_setup_context_new(ss,cfg);
	obj->lc=core;
	obj->ssctx=ssctx;
	set_string(&obj->domain,sip_setup_context_get_domains(ssctx)[0]);
	cfg->lc=core;
	return obj;
}

void linphone_account_creator_set_username(LinphoneAccountCreator *obj, const char *username){
	set_string(&obj->username,username);
}

void linphone_account_creator_set_password(LinphoneAccountCreator *obj, const char *password){
	set_string(&obj->password,password);
}

void linphone_account_creator_set_domain(LinphoneAccountCreator *obj, const char *domain){
	set_string(&obj->domain,domain);
}

void linphone_account_creator_set_route(LinphoneAccountCreator *obj, const char *route) {
	set_string(&obj->route,route);
}

void linphone_account_creator_set_email(LinphoneAccountCreator *obj, const char *email) {
	set_string(&obj->email,email);
}

void linphone_account_creator_set_suscribe(LinphoneAccountCreator *obj, int suscribe) {
	obj->suscribe = suscribe;
}

const char * linphone_account_creator_get_username(LinphoneAccountCreator *obj){
	return obj->username;
}

const char * linphone_account_creator_get_domain(LinphoneAccountCreator *obj){
	return obj->domain;
}

int linphone_account_creator_test_existence(LinphoneAccountCreator *obj){
	SipSetupContext *ssctx=obj->ssctx;
	char *uri=ms_strdup_printf("%s@%s",obj->username,obj->domain);
	int err=sip_setup_context_account_exists(ssctx,uri);
	ms_free(uri);
	return err;
}

int linphone_account_creator_test_validation(LinphoneAccountCreator *obj) {
	SipSetupContext *ssctx=obj->ssctx;
	int err=sip_setup_context_account_validated(ssctx,obj->username);
	return err;
}

LinphoneProxyConfig * linphone_account_creator_validate(LinphoneAccountCreator *obj){
	SipSetupContext *ssctx=obj->ssctx;
	char *uri=ms_strdup_printf("%s@%s",obj->username,obj->domain);
	int err=sip_setup_context_create_account(ssctx, uri, obj->password, obj->email, obj->suscribe);
	ms_free(uri);
	if (err==0) {
		obj->succeeded=TRUE;
		return sip_setup_context_get_proxy_config(ssctx);
	}
	return NULL;
}

void linphone_account_creator_destroy(LinphoneAccountCreator *obj){
	if (obj->username)
		ms_free(obj->username);
	if (obj->password)
		ms_free(obj->password);
	if (obj->domain)
		ms_free(obj->domain);
	if (!obj->succeeded){
		linphone_proxy_config_destroy(sip_setup_context_get_proxy_config(obj->ssctx));
	}
}

void linphone_proxy_config_set_user_data(LinphoneProxyConfig *cr, void * ud) {
	cr->user_data=ud;
}

void * linphone_proxy_config_get_user_data(LinphoneProxyConfig *cr) {
	return cr->user_data;
}

void linphone_proxy_config_set_state(LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const char *message){
	LinphoneCore *lc=cfg->lc;
	cfg->state=state;
	if (lc && lc->vtable.registration_state_changed){
		lc->vtable.registration_state_changed(lc,cfg,state,message);
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
	return cfg->error;
}

void linphone_proxy_config_set_error(LinphoneProxyConfig *cfg,LinphoneReason error) {
	cfg->error = error;
}


