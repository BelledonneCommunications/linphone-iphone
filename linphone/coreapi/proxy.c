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
#include <eXosip2/eXosip.h>
#include <osipparser2/osip_message.h>
#include "lpconfig.h"
#include "private.h"

void linphone_proxy_config_write_all_to_config_file(LinphoneCore *lc){
	MSList *elem;
	int i;
	for(elem=lc->sip_conf.proxies,i=0;elem!=NULL;elem=ms_list_next(elem),i++){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		linphone_proxy_config_write_to_config_file(lc->config,cfg,i);
	}
}

void linphone_proxy_config_init(LinphoneProxyConfig *obj){
	memset(obj,0,sizeof(LinphoneProxyConfig));
	obj->rid=-1;
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
	if (obj->contact_addr!=NULL) ms_free(obj->contact_addr);
}

/**
 * Returns a boolean indicating that the user is sucessfully registered on the proxy.
**/
bool_t linphone_proxy_config_is_registered(const LinphoneProxyConfig *obj){
	return obj->registered;
}

void linphone_proxy_config_get_contact(LinphoneProxyConfig *cfg, const char **ip, int *port){
	if (cfg->registered){
		*ip=cfg->contact_addr;
		*port=cfg->contact_port;
	}else{
		*ip=NULL;
		*port=0;
	}
}

static void update_contact(LinphoneProxyConfig *cfg, const char *ip, const char *port){
	if (cfg->contact_addr){
		ms_free(cfg->contact_addr);
	}
	cfg->contact_addr=ms_strdup(ip);
	if (port!=NULL)
		cfg->contact_port=atoi(port);
	else cfg->contact_port=5060;
}

bool_t linphone_proxy_config_register_again_with_updated_contact(LinphoneProxyConfig *obj, osip_message_t *orig_request, osip_message_t *last_answer){
	osip_message_t *msg;
	const char *rport,*received;
	osip_via_t *via=NULL;
	osip_generic_param_t *param=NULL;
	osip_contact_t *ctt=NULL;
	osip_message_get_via(last_answer,0,&via);
	if (!via) return FALSE;
	osip_via_param_get_byname(via,"rport",&param);
	if (param) rport=param->gvalue;
	else return FALSE;
	param=NULL;
	osip_via_param_get_byname(via,"received",&param);
	if (param) received=param->gvalue;
	else return FALSE;
	osip_message_get_contact(orig_request,0,&ctt);
	if (strcmp(ctt->url->host,received)==0){
		/*ip address matches, check ports*/
		const char *contact_port=ctt->url->port;
		const char *via_rport=rport;
		if (via_rport==NULL || strlen(via_rport)>0)
			via_rport="5060";
		if (contact_port==NULL || strlen(contact_port)>0)
			contact_port="5060";
		if (strcmp(contact_port,via_rport)==0){
			ms_message("Register has up to date contact, doing nothing.");
			return FALSE;
		}else ms_message("ports do not match, need to update the register (%s <> %s)", contact_port,via_rport);
	}
	eXosip_lock();
	msg=NULL;
	eXosip_register_build_register(obj->rid,obj->expires,&msg);
	if (msg==NULL){
		eXosip_unlock();
		ms_warning("Fail to create a contact updated register.");
		return FALSE;
	}
	osip_message_get_contact(msg,0,&ctt);
	if (ctt->url->host!=NULL){
		osip_free(ctt->url->host);
	}
	ctt->url->host=osip_strdup(received);
	if (ctt->url->port!=NULL){
		osip_free(ctt->url->port);
	}
	ctt->url->port=osip_strdup(rport);
	eXosip_register_send_register(obj->rid,msg);
	eXosip_unlock();
	update_contact(obj,received,rport);
	ms_message("Resending new register with updated contact %s:%s",received,rport);
	return TRUE;
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
	int err;
	osip_from_t *url;
	if (obj->reg_proxy!=NULL) ms_free(obj->reg_proxy);
	obj->reg_proxy=NULL;
	if (server_addr!=NULL && strlen(server_addr)>0){
		osip_from_init(&url);
		err=osip_from_parse(url,server_addr);
		if (err==0 && url->url->host!=NULL){
			obj->reg_proxy=ms_strdup(server_addr);
		}else{
			ms_warning("Could not parse %s",server_addr);
		}
		osip_from_free(url);
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
void linphone_proxy_config_set_identity(LinphoneProxyConfig *obj, const char *identity){
	int err=0;
	osip_from_t *url=NULL;
	if (identity!=NULL && strlen(identity)>0){
		osip_from_init(&url);
		err=osip_from_parse(url,identity);
		if (err<0 || url->url->host==NULL || url->url->username==NULL){
			ms_warning("Could not parse %s",identity);
			osip_from_free(url);
			return;
		}
	} else err=-2;
	if (obj->reg_identity!=NULL) {
		ms_free(obj->reg_identity);
		obj->reg_identity=NULL;
	}
	if (err==-2) obj->reg_identity=NULL;
	else {
		obj->reg_identity=ms_strdup(identity);
		if (obj->realm)
			ms_free(obj->realm);
		obj->realm=ms_strdup(url->url->host);
	}
	if (url) osip_from_free(url);
}

const char *linphone_proxy_config_get_domain(const LinphoneProxyConfig *cfg){
	return cfg->realm;
}

/**
 * Sets a SIP route.
 * When a route is set, all outgoing calls will go to the route's destination if this proxy
 * is the default one (see linphone_core_set_default_proxy() ).
**/
void linphone_proxy_config_set_route(LinphoneProxyConfig *obj, const char *route)
{
	int err;
	osip_uri_param_t *lr_param=NULL;
	osip_route_t *rt=NULL;
	char *tmproute=NULL;
	if (route!=NULL && strlen(route)>0){
		osip_route_init(&rt);
		err=osip_route_parse(rt,route);
		if (err<0){
			ms_warning("Could not parse %s",route);
			osip_route_free(rt);
			return ;
		}
		if (obj->reg_route!=NULL) {
			ms_free(obj->reg_route);
			obj->reg_route=NULL;
		}
			
		/* check if the lr parameter is set , if not add it */
		osip_uri_uparam_get_byname(rt->url, "lr", &lr_param);
	  	if (lr_param==NULL){
			osip_uri_uparam_add(rt->url,osip_strdup("lr"),NULL);
			osip_route_to_str(rt,&tmproute);
			obj->reg_route=ms_strdup(tmproute);
			osip_free(tmproute);
		}else obj->reg_route=ms_strdup(route);
	}else{
		if (obj->reg_route!=NULL) ms_free(obj->reg_route);
		obj->reg_route=NULL;
	}
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
	if (val<=0) val=600;
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
	obj->auth_failures=0;
	if (obj->reg_sendregister){
		/* unregister */
		if (obj->registered) {
			osip_message_t *msg;
			eXosip_lock();
			eXosip_register_build_register(obj->rid,0,&msg);
			eXosip_register_send_register(obj->rid,msg);
			eXosip_unlock();
			obj->registered=FALSE;
		}
	}
}

void linphone_proxy_config_apply(LinphoneProxyConfig *obj,LinphoneCore *lc)
{
	obj->lc=lc;
	linphone_proxy_config_done(obj);
}

static void linphone_proxy_config_register(LinphoneProxyConfig *obj){
	const char *id_str;
	if (obj->reg_identity!=NULL) id_str=obj->reg_identity;
	else id_str=linphone_core_get_primary_contact(obj->lc);
	if (obj->reg_sendregister){
		char *ct=NULL;
		osip_message_t *msg=NULL;
		eXosip_lock();
		obj->rid=eXosip_register_build_initial_register(id_str,obj->reg_proxy,NULL,obj->expires,&msg);
		eXosip_register_send_register(obj->rid,msg);
		eXosip_unlock();
		if (ct!=NULL) osip_free(ct);
	}
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
			       LinphoneOnlineStatus presence_mode)
{
  osip_message_t *pub;
  int i;
  const char *from=NULL;
  char buf[5000];

  if (proxy->publish==FALSE) return 0;
	
  if (proxy!=NULL) {
    from=linphone_proxy_config_get_identity(proxy);
  }
  if (from==NULL) from=linphone_core_get_primary_contact(proxy->lc);

  if (presence_mode==LINPHONE_STATUS_ONLINE)
    {
      snprintf(buf, 5000, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>online</note>\n\
</tuple>\n\
</presence>",
	       from, from);
    }
  else if (presence_mode==LINPHONE_STATUS_BUSY
	   ||presence_mode==LINPHONE_STATUS_NOT_DISTURB)
    {
      snprintf(buf, 5000, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>busy</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>busy</note>\n\
</tuple>\n\
</presence>",
	      from, from);
    }
  else if (presence_mode==LINPHONE_STATUS_BERIGHTBACK)
    {
      snprintf(buf, 5000, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>in-transit</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>be right back</note>\n\
</tuple>\n\
</presence>",
	      from,from);
    }
  else if (presence_mode==LINPHONE_STATUS_AWAY
	   ||presence_mode==LINPHONE_STATUS_MOVED
	   ||presence_mode==LINPHONE_STATUS_ALT_SERVICE)
    {
      snprintf(buf, 5000, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>away</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>away</note>\n\
</tuple>\n\
</presence>",
	      from, from);
    }
  else if (presence_mode==LINPHONE_STATUS_ONTHEPHONE)
    {
      snprintf(buf, 5000, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>on-the-phone</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>on the phone</note>\n\
</tuple>\n\
</presence>",
	      from, from);
    }
  else if (presence_mode==LINPHONE_STATUS_OUTTOLUNCH)
    {
      snprintf(buf, 5000, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
          xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
          entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>open</basic>\n\
<es:activities>\n\
  <es:activity>meal</es:activity>\n\
</es:activities>\n\
</status>\n\
<contact priority=\"0.8\">%s</contact>\n\
<note>out to lunch</note>\n\
</tuple>\n\
</presence>",
	      from, from);
    }
  else if (presence_mode==LINPHONE_STATUS_OFFLINE)
    {
      /* */
      snprintf(buf, 5000, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\n\
xmlns:es=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\n\
entity=\"%s\">\n%s",
	      from,
"<tuple id=\"sg89ae\">\n\
<status>\n\
<basic>closed</basic>\n\
<es:activities>\n\
  <es:activity>permanent-absence</e:activity>\n\
</es:activities>\n\
</status>\n\
</tuple>\n\
\n</presence>\n");
    }

  i = eXosip_build_publish(&pub, (char *)from, (char *)from, NULL, "presence", "1800", "application/pidf+xml", buf);

  if (i<0)
    {
      ms_message("Failed to build publish request.");
      return -1;
    }

  eXosip_lock();
  i = eXosip_publish(pub, from); /* should update the sip-if-match parameter
				    from sip-etag  from last 200ok of PUBLISH */
  eXosip_unlock();
  if (i<0)
    {
      ms_message("Failed to send publish request.");
      return -1;
    }
  return 0;
}


/**
 * Add a proxy configuration.
 * This will start registration on the proxy, if registration is enabled.
**/
int linphone_core_add_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	if (!linphone_proxy_config_check(lc,cfg)) return -1;
	lc->sip_conf.proxies=ms_list_append(lc->sip_conf.proxies,(void *)cfg);
	linphone_proxy_config_apply(cfg,lc);
	return 0;
}

extern void linphone_friend_check_for_removed_proxy(LinphoneFriend *lf, LinphoneProxyConfig *cfg);

/**
 * Removes a proxy configuration.
 *
 * LinphoneCore will then automatically unregister and place the proxy configuration
 * on a deleted list. For that reason, a removed proxy does NOT need to be freed.
**/
void linphone_core_remove_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	MSList *elem;
	lc->sip_conf.proxies=ms_list_remove(lc->sip_conf.proxies,(void *)cfg);
	/* add to the list of destroyed proxies, so that the possible unREGISTER request can succeed authentication */
	lc->sip_conf.deleted_proxies=ms_list_append(lc->sip_conf.deleted_proxies,(void *)cfg);
	/* this will unREGISTER */
	linphone_proxy_config_edit(cfg);
	if (lc->default_proxy==cfg){
		lc->default_proxy=NULL;
	}
	/* invalidate all references to this proxy in our friend list */
	for (elem=lc->friends;elem!=NULL;elem=ms_list_next(elem)){
		linphone_friend_check_for_removed_proxy((LinphoneFriend*)elem->data,cfg);
	}
	
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

static int rid_compare(const void *pcfg,const void *prid){
	const LinphoneProxyConfig *cfg=(const LinphoneProxyConfig*)pcfg;
	const int *rid=(const int*)prid;
	ms_message("cfg= %s, cfg->rid=%i, rid=%i",cfg->reg_proxy, cfg->rid, *rid);
	return cfg->rid-(*rid);
}

LinphoneProxyConfig *linphone_core_get_proxy_config_from_rid(LinphoneCore *lc, int rid){
	MSList *elem=ms_list_find_custom(lc->sip_conf.proxies,rid_compare, &rid);
	if (elem==NULL){
		ms_message("linphone_core_get_proxy_config_from_rid: searching in deleted proxies...");
		elem=ms_list_find_custom(lc->sip_conf.deleted_proxies,rid_compare, &rid);
	}
	if (elem==NULL) return NULL;
	else return (LinphoneProxyConfig*)elem->data;
}

/**
 * Returns an unmodifiable list of entered proxy configurations.
**/
const MSList *linphone_core_get_proxy_config_list(const LinphoneCore *lc){
	return lc->sip_conf.proxies;
}


void linphone_proxy_config_process_authentication_failure(LinphoneCore *lc, eXosip_event_t *ev){
	LinphoneProxyConfig *cfg=linphone_core_get_proxy_config_from_rid(lc, ev->rid);
	if (cfg){
		cfg->auth_failures++;
		if (strcmp(ev->request->sip_method,"REGISTER")==0) {
			gstate_new_state(lc, GSTATE_REG_FAILED, "Authentication failed.");
		}
		/*restart a new register so that the user gets a chance to be prompted for a password*/
		if (cfg->auth_failures==1){
			linphone_proxy_config_register(cfg);
		}
	}
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
	if (cfg->commit){
		if (cfg->type && cfg->ssctx==NULL){
			linphone_proxy_config_activate_sip_setup(cfg);
		}
		linphone_proxy_config_register(cfg);
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
		ms_error("%s cannot manage accounts.");
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

LinphoneProxyConfig * linphone_account_creator_validate(LinphoneAccountCreator *obj){
	SipSetupContext *ssctx=obj->ssctx;
	char *uri=ms_strdup_printf("%s@%s",obj->username,obj->domain);
	int err=sip_setup_context_create_account(ssctx,uri,obj->password);
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



