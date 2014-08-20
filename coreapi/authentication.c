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
#include "lpconfig.h"

/**
 * @addtogroup authentication
 * @{
**/

LinphoneAuthInfo *linphone_auth_info_new(const char *username, const char *userid, const char *passwd, const char *ha1, const char *realm, const char *domain){
	LinphoneAuthInfo *obj=ms_new0(LinphoneAuthInfo,1);
	if (username!=NULL && (strlen(username)>0) ) obj->username=ms_strdup(username);
	if (userid!=NULL && (strlen(userid)>0)) obj->userid=ms_strdup(userid);
	if (passwd!=NULL && (strlen(passwd)>0)) obj->passwd=ms_strdup(passwd);
	if (ha1!=NULL && (strlen(ha1)>0)) obj->ha1=ms_strdup(ha1);
	if (realm!=NULL && (strlen(realm)>0)) obj->realm=ms_strdup(realm);
	if (domain!=NULL && (strlen(domain)>0)) obj->domain=ms_strdup(domain);
	return obj;
}

LinphoneAuthInfo *linphone_auth_info_clone(const LinphoneAuthInfo *ai){
	LinphoneAuthInfo *obj=ms_new0(LinphoneAuthInfo,1);
	if (ai->username) obj->username=ms_strdup(ai->username);
	if (ai->userid) obj->userid=ms_strdup(ai->userid);
	if (ai->passwd) obj->passwd=ms_strdup(ai->passwd);
	if (ai->ha1)	obj->ha1=ms_strdup(ai->ha1);
	if (ai->realm)	obj->realm=ms_strdup(ai->realm);
	if (ai->domain)	obj->domain=ms_strdup(ai->domain);
	return obj;
}

const char *linphone_auth_info_get_username(const LinphoneAuthInfo *i){
	return i->username;
}

const char *linphone_auth_info_get_passwd(const LinphoneAuthInfo *i){
	return i->passwd;
}

const char *linphone_auth_info_get_userid(const LinphoneAuthInfo *i){
	return i->userid;
}

const char *linphone_auth_info_get_realm(const LinphoneAuthInfo *i){
	return i->realm;
}

const char *linphone_auth_info_get_domain(const LinphoneAuthInfo *i){
	return i->domain;
}

const char *linphone_auth_info_get_ha1(const LinphoneAuthInfo *i){
	return i->ha1;
}

void linphone_auth_info_set_passwd(LinphoneAuthInfo *info, const char *passwd){
	if (info->passwd!=NULL) {
		ms_free(info->passwd);
		info->passwd=NULL;
	}
	if (passwd!=NULL && (strlen(passwd)>0)) info->passwd=ms_strdup(passwd);
}

void linphone_auth_info_set_username(LinphoneAuthInfo *info, const char *username){
	if (info->username){
		ms_free(info->username);
		info->username=NULL;
	}
	if (username && strlen(username)>0) info->username=ms_strdup(username);
}

void linphone_auth_info_set_userid(LinphoneAuthInfo *info, const char *userid){
	if (info->userid){
		ms_free(info->userid);
		info->userid=NULL;
	}
	if (userid && strlen(userid)>0) info->userid=ms_strdup(userid);
}

void linphone_auth_info_set_realm(LinphoneAuthInfo *info, const char *realm){
	if (info->realm){
		ms_free(info->realm);
		info->realm=NULL;
	}
	if (realm && strlen(realm)>0) info->realm=ms_strdup(realm);
}

void linphone_auth_info_set_domain(LinphoneAuthInfo *info, const char *domain){
	if (info->domain){
		ms_free(info->domain);
		info->domain=NULL;
	}
	if (domain && strlen(domain)>0) info->domain=ms_strdup(domain);
}

void linphone_auth_info_set_ha1(LinphoneAuthInfo *info, const char *ha1){
	if (info->ha1){
		ms_free(info->ha1);
		info->ha1=NULL;
	}
	if (ha1 && strlen(ha1)>0) info->ha1=ms_strdup(ha1);
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
	if (obj->domain!=NULL) ms_free(obj->domain);
	ms_free(obj);
}

void linphone_auth_info_write_config(LpConfig *config, LinphoneAuthInfo *obj, int pos)
{
	char key[50];
	sprintf(key,"auth_info_%i",pos);
	lp_config_clean_section(config,key);

	if (obj==NULL || lp_config_get_int(config, "sip", "store_auth_info", 1) == 0){
		return;
	}
	if (!obj->ha1 && obj->realm && obj->passwd && (obj->username||obj->userid) && lp_config_get_int(config, "sip", "store_ha1_passwd", 1) == 1) {
		/*compute ha1 to avoid storing clear text password*/
		obj->ha1=ms_malloc(33);
		sal_auth_compute_ha1(obj->userid?obj->userid:obj->username,obj->realm,obj->passwd,obj->ha1);
	}
	if (obj->username!=NULL){
		lp_config_set_string(config,key,"username",obj->username);
	}
	if (obj->userid!=NULL){
		lp_config_set_string(config,key,"userid",obj->userid);
	}
	if (obj->ha1!=NULL){
		lp_config_set_string(config,key,"ha1",obj->ha1);
	} else if (obj->passwd!=NULL){ /*only write passwd if no ha1*/
		lp_config_set_string(config,key,"passwd",obj->passwd);
	}
	if (obj->realm!=NULL){
		lp_config_set_string(config,key,"realm",obj->realm);
	}
	if (obj->domain!=NULL){
		lp_config_set_string(config,key,"domain",obj->domain);
	}
}

LinphoneAuthInfo *linphone_auth_info_new_from_config_file(LpConfig * config, int pos)
{
	char key[50];
	const char *username,*userid,*passwd,*ha1,*realm,*domain;
	LinphoneAuthInfo *ret;

	sprintf(key,"auth_info_%i",pos);
	if (!lp_config_has_section(config,key)){
		return NULL;
	}

	username=lp_config_get_string(config,key,"username",NULL);
	userid=lp_config_get_string(config,key,"userid",NULL);
	passwd=lp_config_get_string(config,key,"passwd",NULL);
	ha1=lp_config_get_string(config,key,"ha1",NULL);
	realm=lp_config_get_string(config,key,"realm",NULL);
	domain=lp_config_get_string(config,key,"domain",NULL);
	ret=linphone_auth_info_new(username,userid,passwd,ha1,realm,domain);
	return ret;
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

static const LinphoneAuthInfo *find_auth_info(LinphoneCore *lc, const char *username, const char *realm, const char *domain){
	MSList *elem;
	const LinphoneAuthInfo *ret=NULL;

	for (elem=lc->auth_info;elem!=NULL;elem=elem->next) {
		LinphoneAuthInfo *pinfo = (LinphoneAuthInfo*)elem->data;
		if (username && pinfo->username && strcmp(username,pinfo->username)==0) {
			if (realm && domain){
				if (pinfo->realm && strcmp(realm,pinfo->realm)==0
					&& pinfo->domain && strcmp(domain,pinfo->domain)==0) {
					return pinfo;
				}
			} else if (realm) {
				if (pinfo->realm && realm_match(realm,pinfo->realm)) {
					if (ret!=NULL) {
						ms_warning("Non unique realm found for %s",username);
						return NULL;
					}
					ret=pinfo;
				}
			} else if (domain && pinfo->domain && strcmp(domain,pinfo->domain)==0) {
				return pinfo;
			} else if (!domain) {
				return pinfo;
			}
		}
	}
	return ret;
}

/**
 * Find authentication info matching realm, username, domain criteria.
 * First of all, (realm,username) pair are searched. If multiple results (which should not happen because realm are supposed to be unique), then domain is added to the search.
 * @param lc the LinphoneCore
 * @param realm the authentication 'realm' (optional)
 * @param username the SIP username to be authenticated (mandatory)
 * @param domain the SIP domain name (optional)
 * @return a #LinphoneAuthInfo
**/
const LinphoneAuthInfo *linphone_core_find_auth_info(LinphoneCore *lc, const char *realm, const char *username, const char *domain){
	const LinphoneAuthInfo *ai=NULL;
	if (realm){
		ai=find_auth_info(lc,username,realm,NULL);
		if (ai==NULL && domain){
			ai=find_auth_info(lc,username,realm,domain);
		}
	}
	if (ai == NULL && domain != NULL) {
		ai=find_auth_info(lc,username,NULL,domain);
	}
	if (ai==NULL){
		ai=find_auth_info(lc,username,NULL,NULL);
	}
	return ai;
}

static void write_auth_infos(LinphoneCore *lc){
	MSList *elem;
	int i;

	if (!linphone_core_ready(lc)) return;
	for(elem=lc->auth_info,i=0;elem!=NULL;elem=ms_list_next(elem),i++){
		LinphoneAuthInfo *ai=(LinphoneAuthInfo*)(elem->data);
		linphone_auth_info_write_config(lc->config,ai,i);
	}
	linphone_auth_info_write_config(lc->config,NULL,i); /* mark the end */
}

LinphoneAuthInfo * linphone_core_create_auth_info(LinphoneCore *lc, const char *username, const char *userid, const char *passwd, const char *ha1, const char *realm, const char *domain) {
	return linphone_auth_info_new(username, userid, passwd, ha1, realm, domain);
}

/**
 * Adds authentication information to the LinphoneCore.
 *
 * This information will be used during all SIP transactions that require authentication.
**/
void linphone_core_add_auth_info(LinphoneCore *lc, const LinphoneAuthInfo *info){
	LinphoneAuthInfo *ai;
	MSList *elem;
	MSList *l;
	int restarted_op_count=0;
	bool_t updating=FALSE;

	if (info->ha1==NULL && info->passwd==NULL){
		ms_error("linphone_core_add_auth_info(): info supplied with empty password or ha1.");
		return;
	}
	/* find if we are attempting to modify an existing auth info */
	ai=(LinphoneAuthInfo*)linphone_core_find_auth_info(lc,info->realm,info->username,info->domain);
	if (ai!=NULL && ai->domain && info->domain && strcmp(ai->domain, info->domain)==0){
		lc->auth_info=ms_list_remove(lc->auth_info,ai);
		linphone_auth_info_destroy(ai);
		updating=TRUE;
	}
	lc->auth_info=ms_list_append(lc->auth_info,linphone_auth_info_clone(info));

	/* retry pending authentication operations */
	for(l=elem=sal_get_pending_auths(lc->sal);elem!=NULL;elem=elem->next){
		SalOp *op=(SalOp*)elem->data;
		LinphoneAuthInfo *ai;
		const SalAuthInfo *req_sai=sal_op_get_auth_requested(op);
		ai=(LinphoneAuthInfo*)linphone_core_find_auth_info(lc,req_sai->realm,req_sai->username,req_sai->domain);
		if (ai){
			SalAuthInfo sai;
			MSList* proxy;
			sai.username=ai->username;
			sai.userid=ai->userid;
			sai.realm=ai->realm;
			sai.password=ai->passwd;
			sai.ha1=ai->ha1;
			/*proxy case*/
			for (proxy=(MSList*)linphone_core_get_proxy_config_list(lc);proxy!=NULL;proxy=proxy->next) {
				if (proxy->data == sal_op_get_user_pointer(op)) {
					linphone_proxy_config_set_state((LinphoneProxyConfig*)(proxy->data),LinphoneRegistrationProgress,"Authentication...");
					break;
				}
			}
			sal_op_authenticate(op,&sai);
			restarted_op_count++;
		}
	}
	if (l){
		ms_message("linphone_core_add_auth_info(): restarted [%i] operation(s) after %s auth info for\n"
			"\tusername: [%s]\n"
			"\trealm [%s]\n"
			"\tdomain [%s]\n",
			restarted_op_count,
			updating ? "updating" : "adding",
			info->username ? info->username : "",
			info->realm ? info->realm : "",
			info->domain ? info->domain : "");
	}
	ms_list_free(l);
	write_auth_infos(lc);
}


/**
 * This method is used to abort a user authentication request initiated by LinphoneCore
 * from the auth_info_requested callback of LinphoneCoreVTable.
**/
void linphone_core_abort_authentication(LinphoneCore *lc,  LinphoneAuthInfo *info){
}

/**
 * Removes an authentication information object.
**/
void linphone_core_remove_auth_info(LinphoneCore *lc, const LinphoneAuthInfo *info){
	LinphoneAuthInfo *r;
	r=(LinphoneAuthInfo*)linphone_core_find_auth_info(lc,info->realm,info->username,info->domain);
	if (r){
		lc->auth_info=ms_list_remove(lc->auth_info,r);
		linphone_auth_info_destroy(r);
		write_auth_infos(lc);
	}
}

/**
 * Returns an unmodifiable list of currently entered LinphoneAuthInfo.
 * @param[in] lc The LinphoneCore object
 * @return \mslist{LinphoneAuthInfo}
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
	for(i=0,elem=lc->auth_info;elem!=NULL;elem=ms_list_next(elem),i++){
		LinphoneAuthInfo *info=(LinphoneAuthInfo*)elem->data;
		linphone_auth_info_destroy(info);
		linphone_auth_info_write_config(lc->config,NULL,i);
	}
	ms_list_free(lc->auth_info);
	lc->auth_info=NULL;
}

/**
 * @}
**/
