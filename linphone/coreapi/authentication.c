/***************************************************************************
 *            authentication.c
 *
 *  Fri Jul 16 12:08:34 2004
 *  Copyright  2004-2009  Simon MORLAT
 *  simon.morlat@linphone.org
 ****************************************************************************/

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
#include "private.h"
#include <eXosip2/eXosip.h>
#include <osipparser2/osip_message.h>
#include "lpconfig.h"

extern LinphoneProxyConfig *linphone_core_get_proxy_config_from_rid(LinphoneCore *lc, int rid);

/**
 * @addtogroup authentication
 * @{
**/

/**
 * Create a LinphoneAuthInfo object with supplied information.
 *
 * The object can be created empty, that is with all arguments set to NULL.
 * Username, userid, password and realm can be set later using specific methods.
**/
LinphoneAuthInfo *linphone_auth_info_new(const char *username, const char *userid,
				   										const char *passwd, const char *ha1,const char *realm)
{
	LinphoneAuthInfo *obj=ms_new0(LinphoneAuthInfo,1);
	if (username!=NULL && (strlen(username)>0) ) obj->username=ms_strdup(username);
	if (userid!=NULL && (strlen(userid)>0)) obj->userid=ms_strdup(userid);
	if (passwd!=NULL && (strlen(passwd)>0)) obj->passwd=ms_strdup(passwd);
	if (ha1!=NULL && (strlen(ha1)>0)) obj->ha1=ms_strdup(ha1);
	if (realm!=NULL && (strlen(realm)>0)) obj->realm=ms_strdup(realm);
	obj->works=FALSE;
	obj->first_time=TRUE;
	return obj;
}

/**
 * Sets the password.
**/
void linphone_auth_info_set_passwd(LinphoneAuthInfo *info, const char *passwd){
	if (info->passwd!=NULL) {
		ms_free(info->passwd);
		info->passwd=NULL;
	}
	if (passwd!=NULL && (strlen(passwd)>0)) info->passwd=ms_strdup(passwd);
}

/**
 * Sets the username.
**/
void linphone_auth_info_set_username(LinphoneAuthInfo *info, const char *username){
	if (info->username){
		ms_free(info->username);
		info->username=NULL;
	}
	if (username && strlen(username)>0) info->username=ms_strdup(username);
}

/**
 * Sets userid.
**/
void linphone_auth_info_set_userid(LinphoneAuthInfo *info, const char *userid){
	if (info->userid){
		ms_free(info->userid);
		info->userid=NULL;
	}
	if (userid && strlen(userid)>0) info->userid=ms_strdup(userid);
}

/**
 * Destroys a LinphoneAuthInfo object.
**/
void linphone_auth_info_destroy(LinphoneAuthInfo *obj){
	if (obj->username!=NULL) ms_free(obj->username);
	if (obj->userid!=NULL) ms_free(obj->userid);
	if (obj->passwd!=NULL) ms_free(obj->passwd);
	if (obj->ha1!=NULL) ms_free(obj->ha1);
	if (obj->realm!=NULL) ms_free(obj->realm);
	ms_free(obj);
}

void linphone_auth_info_write_config(LpConfig *config, LinphoneAuthInfo *obj, int pos)
{
	char key[50];
	sprintf(key,"auth_info_%i",pos);
	lp_config_clean_section(config,key);
	
	if (obj==NULL){
		return;
	}		
	if (obj->username!=NULL){
		lp_config_set_string(config,key,"username",obj->username);
	}
	if (obj->userid!=NULL){
		lp_config_set_string(config,key,"userid",obj->userid);
	}
	if (obj->passwd!=NULL){
		lp_config_set_string(config,key,"passwd",obj->passwd);
	}
	if (obj->ha1!=NULL){
		lp_config_set_string(config,key,"ha1",obj->ha1);
	}
	if (obj->realm!=NULL){
		lp_config_set_string(config,key,"realm",obj->realm);
	}
}

LinphoneAuthInfo *linphone_auth_info_new_from_config_file(LpConfig * config, int pos)
{
	char key[50];
	const char *username,*userid,*passwd,*ha1,*realm;
	
	sprintf(key,"auth_info_%i",pos);
	if (!lp_config_has_section(config,key)){
		return NULL;
	}
	
	username=lp_config_get_string(config,key,"username",NULL);
	userid=lp_config_get_string(config,key,"userid",NULL);
	passwd=lp_config_get_string(config,key,"passwd",NULL);
	ha1=lp_config_get_string(config,key,"ha1",NULL);
	realm=lp_config_get_string(config,key,"realm",NULL);
	return linphone_auth_info_new(username,userid,passwd,ha1,realm);
}

static bool_t key_match(const char *tmp1, const char *tmp2){
	if (tmp1==NULL && tmp2==NULL) return TRUE;
	if (tmp1!=NULL && tmp2!=NULL && strcmp(tmp1,tmp2)==0) return TRUE;
	return FALSE;
	
}

static char * remove_quotes(char * input){
	char *tmp;
	if (*input=='"') input++;
	tmp=strchr(input,'"');
	if (tmp) *tmp='\0';
	return input;
}

static int realm_match(const char *realm1, const char *realm2){
	if (realm1==NULL && realm2==NULL) return TRUE;
	if (realm1!=NULL && realm2!=NULL){
		if (strcmp(realm1,realm2)==0) return TRUE;
		else{
			char tmp1[128];
			char tmp2[128];
			char *p1,*p2;
			strncpy(tmp1,realm1,sizeof(tmp1)-1);
			strncpy(tmp2,realm2,sizeof(tmp2)-1);
			p1=remove_quotes(tmp1);
			p2=remove_quotes(tmp2);
			return strcmp(p1,p2)==0;
		}
	}
	return FALSE;
}

/**
 * Retrieves a LinphoneAuthInfo previously entered into the LinphoneCore.
**/
LinphoneAuthInfo *linphone_core_find_auth_info(LinphoneCore *lc, const char *realm, const char *username)
{
	MSList *elem;
	LinphoneAuthInfo *ret=NULL,*candidate=NULL;
	for (elem=lc->auth_info;elem!=NULL;elem=elem->next){
		LinphoneAuthInfo *pinfo=(LinphoneAuthInfo*)elem->data;
		if (realm==NULL){
			/*return the authinfo for any realm provided that there is only one for that username*/
			if (key_match(pinfo->username,username)){
				if (ret!=NULL){
					ms_warning("There are several auth info for username '%s'",username);
					return NULL;
				}
				ret=pinfo;
			}
		}else{
			/*return the exact authinfo, or an authinfo for which realm was not supplied yet*/
			if (pinfo->realm!=NULL){
				if (realm_match(pinfo->realm,realm) 
					&& key_match(pinfo->username,username))
					ret=pinfo;
			}else{
				if (key_match(pinfo->username,username))
					candidate=pinfo;
			}
		}
	}
	if (ret==NULL && candidate!=NULL)
		ret=candidate;
	return ret;
}

static void refresh_exosip_auth_info(LinphoneCore *lc){
	MSList *elem;
	eXosip_lock();
	eXosip_clear_authentication_info();
	for (elem=lc->auth_info;elem!=NULL;elem=ms_list_next(elem)){
		LinphoneAuthInfo *info=(LinphoneAuthInfo*)elem->data;
		char *userid;
		if (info->userid==NULL || info->userid[0]=='\0') userid=info->username;
		else userid=info->userid;
		eXosip_add_authentication_info(info->username,userid,
				info->passwd,info->ha1,info->realm);
	}
	eXosip_unlock();
}

/**
 * Adds authentication information to the LinphoneCore.
 * 
 * This information will be used during all SIP transacations that require authentication.
**/
void linphone_core_add_auth_info(LinphoneCore *lc, LinphoneAuthInfo *info)
{
	MSList *elem;
	LinphoneAuthInfo *ai;
	
	/* find if we are attempting to modify an existing auth info */
	ai=linphone_core_find_auth_info(lc,info->realm,info->username);
	if (ai!=NULL){
		elem=ms_list_find(lc->auth_info,ai);
		if (elem==NULL){
			ms_error("AuthInfo list corruption ?");
			return;
		}
		linphone_auth_info_destroy((LinphoneAuthInfo*)elem->data);
		elem->data=(void *)info;
	}else {
		lc->auth_info=ms_list_append(lc->auth_info,(void *)info);
	}
	refresh_exosip_auth_info(lc);
	/* if the user was prompted, re-allow automatic_action */
	if (lc->automatic_action>0) lc->automatic_action--;
}


/**
 * This method is used to abort a user authentication request initiated by LinphoneCore
 * from the auth_info_requested callback of LinphoneCoreVTable.
**/
void linphone_core_abort_authentication(LinphoneCore *lc,  LinphoneAuthInfo *info){
	if (lc->automatic_action>0) lc->automatic_action--;
}

/**
 * Removes an authentication information object.
**/
void linphone_core_remove_auth_info(LinphoneCore *lc, LinphoneAuthInfo *info){
	int len=ms_list_size(lc->auth_info);
	int newlen;
	int i;
	MSList *elem;
	lc->auth_info=ms_list_remove(lc->auth_info,info);
	newlen=ms_list_size(lc->auth_info);
	/*printf("len=%i newlen=%i\n",len,newlen);*/
	linphone_auth_info_destroy(info);
	for (i=0;i<len;i++){
		linphone_auth_info_write_config(lc->config,NULL,i);
	}
	for (elem=lc->auth_info,i=0;elem!=NULL;elem=ms_list_next(elem),i++){
		linphone_auth_info_write_config(lc->config,(LinphoneAuthInfo*)elem->data,i);
	}
	refresh_exosip_auth_info(lc);
	
}

/**
 * Returns an unmodifiable list of currently entered LinphoneAuthInfo.
**/
const MSList *linphone_core_get_auth_info_list(const LinphoneCore *lc){
	return lc->auth_info;
}

/**
 * Clear all authentication information.
**/
void linphone_core_clear_all_auth_info(LinphoneCore *lc){
	MSList *elem;
	int i;
	eXosip_lock();
	eXosip_clear_authentication_info();
	eXosip_unlock();
	for(i=0,elem=lc->auth_info;elem!=NULL;elem=ms_list_next(elem),i++){
		LinphoneAuthInfo *info=(LinphoneAuthInfo*)elem->data;
		linphone_auth_info_destroy(info);
		linphone_auth_info_write_config(lc->config,NULL,i);
	}
	ms_list_free(lc->auth_info);
	lc->auth_info=NULL;
}

void linphone_authentication_ok(LinphoneCore *lc, eXosip_event_t *ev){
	char *prx_realm=NULL,*www_realm=NULL;
	osip_proxy_authorization_t *prx_auth;
	osip_authorization_t *www_auth;
	osip_message_t *msg=ev->request;
	char *username;
	LinphoneAuthInfo *as=NULL;

	username=osip_uri_get_username(msg->from->url);
	osip_message_get_proxy_authorization(msg,0,&prx_auth);
	osip_message_get_authorization(msg,0,&www_auth);
	if (prx_auth!=NULL)
		prx_realm=osip_proxy_authorization_get_realm(prx_auth);
	if (www_auth!=NULL)
		www_realm=osip_authorization_get_realm(www_auth);
	
	if (prx_realm==NULL && www_realm==NULL){
		ms_message("No authentication info in the request, ignoring");
		return;
	}
	/* see if we already have this auth information , not to ask it everytime to the user */
	if (prx_realm!=NULL)
		as=linphone_core_find_auth_info(lc,prx_realm,username);
	if (www_realm!=NULL) 
		as=linphone_core_find_auth_info(lc,www_realm,username);
	if (as){
		ms_message("Authentication for user=%s realm=%s is working.",username,prx_realm ? prx_realm : www_realm);
		as->works=TRUE;
	}
}


void linphone_core_find_or_ask_for_auth_info(LinphoneCore *lc,const char *username,const char* realm, int tid)
{
	LinphoneAuthInfo *as=linphone_core_find_auth_info(lc,realm,username);
	if ( as==NULL || (as!=NULL && as->works==FALSE && as->first_time==FALSE)){
		if (lc->vtable.auth_info_requested!=NULL){
			lc->vtable.auth_info_requested(lc,realm,username);
			lc->automatic_action++;/*suspends eXosip_automatic_action until the user supplies a password */
		}
	}
	if (as) as->first_time=FALSE;
}

void linphone_process_authentication(LinphoneCore *lc, eXosip_event_t *ev)
{
	char *prx_realm=NULL,*www_realm=NULL;
	osip_proxy_authenticate_t *prx_auth;
	osip_www_authenticate_t *www_auth;
	osip_message_t *resp=ev->response;
	char *username;
	
	if (strcmp(ev->request->sip_method,"REGISTER")==0) {
		gstate_new_state(lc, GSTATE_REG_FAILED, "Authentication required");
	}

	username=osip_uri_get_username(resp->from->url);
	prx_auth=(osip_proxy_authenticate_t*)osip_list_get(&resp->proxy_authenticates,0);
	www_auth=(osip_proxy_authenticate_t*)osip_list_get(&resp->www_authenticates,0);
	if (prx_auth!=NULL)
		prx_realm=osip_proxy_authenticate_get_realm(prx_auth);
	if (www_auth!=NULL)
		www_realm=osip_www_authenticate_get_realm(www_auth);
	
	if (prx_realm==NULL && www_realm==NULL){
		ms_warning("No realm in the server response.");
		return;
	}
	/* see if we already have this auth information , not to ask it everytime to the user */
	if (prx_realm!=NULL) 
		linphone_core_find_or_ask_for_auth_info(lc,username,prx_realm,ev->tid);
	if (www_realm!=NULL) 
		linphone_core_find_or_ask_for_auth_info(lc,username,www_realm,ev->tid);
}


/**
 * @}
**/
