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
	return obj;
}

static LinphoneAuthInfo *linphone_auth_info_clone(const LinphoneAuthInfo *ai){
	LinphoneAuthInfo *obj=ms_new0(LinphoneAuthInfo,1);
	if (ai->username) obj->username=ms_strdup(ai->username);
	if (ai->userid) obj->userid=ms_strdup(ai->userid);
	if (ai->passwd) obj->passwd=ms_strdup(ai->passwd);
	if (ai->ha1)	obj->ha1=ms_strdup(ai->ha1);
	if (ai->realm)	obj->realm=ms_strdup(ai->realm);
	obj->works=FALSE;
	obj->usecount=0;
	return obj;
}

/**
 * Returns username.
**/
const char *linphone_auth_info_get_username(const LinphoneAuthInfo *i){
	return i->username;
}

/**
 * Returns password.
**/
const char *linphone_auth_info_get_passwd(const LinphoneAuthInfo *i){
	return i->passwd;
}

const char *linphone_auth_info_get_userid(const LinphoneAuthInfo *i){
	return i->userid;
}

const char *linphone_auth_info_get_realm(const LinphoneAuthInfo *i){
	return i->realm;
}
const char *linphone_auth_info_get_ha1(const LinphoneAuthInfo *i){
	return i->ha1;
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
 * Sets realm.
**/
void linphone_auth_info_set_realm(LinphoneAuthInfo *info, const char *realm){
	if (info->realm){
		ms_free(info->realm);
		info->realm=NULL;
	}
	if (realm && strlen(realm)>0) info->realm=ms_strdup(realm);
}
/**
 * Sets ha1.
**/
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
const LinphoneAuthInfo *linphone_core_find_auth_info(LinphoneCore *lc, const char *realm, const char *username)
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

/**
 * Adds authentication information to the LinphoneCore.
 * 
 * This information will be used during all SIP transacations that require authentication.
**/
void linphone_core_add_auth_info(LinphoneCore *lc, const LinphoneAuthInfo *info)
{
	LinphoneAuthInfo *ai;
	MSList *elem;
	MSList *l;
	
	/* find if we are attempting to modify an existing auth info */
	ai=(LinphoneAuthInfo*)linphone_core_find_auth_info(lc,info->realm,info->username);
	if (ai!=NULL){
		lc->auth_info=ms_list_remove(lc->auth_info,ai);
		linphone_auth_info_destroy(ai);
	}
	lc->auth_info=ms_list_append(lc->auth_info,linphone_auth_info_clone(info));
	/* retry pending authentication operations */
	for(l=elem=sal_get_pending_auths(lc->sal);elem!=NULL;elem=elem->next){
		const char *username,*realm;
		SalOp *op=(SalOp*)elem->data;
		LinphoneAuthInfo *ai;
		sal_op_get_auth_requested(op,&realm,&username);
		ai=(LinphoneAuthInfo*)linphone_core_find_auth_info(lc,realm,username);
		if (ai){
			SalAuthInfo sai;
			sai.username=ai->username;
			sai.userid=ai->userid;
			sai.realm=ai->realm;
			sai.password=ai->passwd;
			sal_op_authenticate(op,&sai);
			ai->usecount++;
		}
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
	r=(LinphoneAuthInfo*)linphone_core_find_auth_info(lc,info->realm,info->username);
	if (r){
		lc->auth_info=ms_list_remove(lc->auth_info,r);
		/*printf("len=%i newlen=%i\n",len,newlen);*/
		linphone_auth_info_destroy(r);
		write_auth_infos(lc);
	}
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
