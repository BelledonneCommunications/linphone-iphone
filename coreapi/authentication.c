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

#include "linphone/core.h"
#include "private.h"
#include "linphone/lpconfig.h"

static void _linphone_auth_info_uninit(LinphoneAuthInfo *obj);
static void _linphone_auth_info_copy(LinphoneAuthInfo *dst, const LinphoneAuthInfo *src);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneAuthInfo);
BELLE_SIP_DECLARE_VPTR(LinphoneAuthInfo);
BELLE_SIP_INSTANCIATE_VPTR(
	LinphoneAuthInfo,
	belle_sip_object_t,
	_linphone_auth_info_uninit, // destroy
	_linphone_auth_info_copy, // clone
	NULL, // marshal
	FALSE
);

LinphoneAuthInfo *linphone_auth_info_new(const char *username, const char *userid, const char *passwd, const char *ha1, const char *realm, const char *domain){
	LinphoneAuthInfo *obj=belle_sip_object_new(LinphoneAuthInfo);
	if (username!=NULL && (strlen(username)>0) ) obj->username=ms_strdup(username);
	if (userid!=NULL && (strlen(userid)>0)) obj->userid=ms_strdup(userid);
	if (passwd!=NULL && (strlen(passwd)>0)) obj->passwd=ms_strdup(passwd);
	if (ha1!=NULL && (strlen(ha1)>0)) obj->ha1=ms_strdup(ha1);
	if (realm!=NULL && (strlen(realm)>0)) obj->realm=ms_strdup(realm);
	if (domain!=NULL && (strlen(domain)>0)) obj->domain=ms_strdup(domain);
	return obj;
}

static void _linphone_auth_info_copy(LinphoneAuthInfo *dst, const LinphoneAuthInfo *src) {
	if (src->username)      dst->username = ms_strdup(src->username);
	if (src->userid)        dst->userid = ms_strdup(src->userid);
	if (src->passwd)        dst->passwd = ms_strdup(src->passwd);
	if (src->ha1)           dst->ha1 = ms_strdup(src->ha1);
	if (src->realm)         dst->realm = ms_strdup(src->realm);
	if (src->domain)        dst->domain = ms_strdup(src->domain);
	if (src->tls_cert)      dst->tls_cert = ms_strdup(src->tls_cert);
	if (src->tls_key)       dst->tls_key = ms_strdup(src->tls_key);
	if (src->tls_cert_path) dst->tls_cert_path = ms_strdup(src->tls_cert_path);
	if (src->tls_key_path)  dst->tls_key_path = ms_strdup(src->tls_key_path);
}

LinphoneAuthInfo *linphone_auth_info_clone(const LinphoneAuthInfo *ai){
	return LINPHONE_AUTH_INFO(belle_sip_object_clone(BELLE_SIP_OBJECT(ai)));
}

LinphoneAuthInfo *linphone_auth_info_ref(LinphoneAuthInfo *obj) {
	return LINPHONE_AUTH_INFO(belle_sip_object_ref(obj));
}

void linphone_auth_info_unref(LinphoneAuthInfo *obj) {
	belle_sip_object_unref(obj);
}

const char *linphone_auth_info_get_username(const LinphoneAuthInfo *i) {
	return i->username;
}

const char *linphone_auth_info_get_passwd(const LinphoneAuthInfo *i) {
	return i->passwd;
}

const char *linphone_auth_info_get_userid(const LinphoneAuthInfo *i) {
	return i->userid;
}

const char *linphone_auth_info_get_realm(const LinphoneAuthInfo *i) {
	return i->realm;
}

const char *linphone_auth_info_get_domain(const LinphoneAuthInfo *i) {
	return i->domain;
}

const char *linphone_auth_info_get_ha1(const LinphoneAuthInfo *i) {
	return i->ha1;
}

const char *linphone_auth_info_get_tls_cert(const LinphoneAuthInfo *i) {
	return i->tls_cert;
}

const char *linphone_auth_info_get_tls_key(const LinphoneAuthInfo *i) {
	return i->tls_key;
}

const char *linphone_auth_info_get_tls_cert_path(const LinphoneAuthInfo *i) {
	return i->tls_cert_path;
}

const char *linphone_auth_info_get_tls_key_path(const LinphoneAuthInfo *i) {
	return i->tls_key_path;
}


void linphone_auth_info_set_passwd(LinphoneAuthInfo *info, const char *passwd) {
	if (info->passwd) {
		ms_free(info->passwd);
		info->passwd = NULL;
	}
	if (passwd && strlen(passwd) > 0) info->passwd = ms_strdup(passwd);
}

void linphone_auth_info_set_username(LinphoneAuthInfo *info, const char *username) {
	if (info->username) {
		ms_free(info->username);
		info->username = NULL;
	}
	if (username && strlen(username) > 0) info->username = ms_strdup(username);
}

void linphone_auth_info_set_userid(LinphoneAuthInfo *info, const char *userid) {
	if (info->userid) {
		ms_free(info->userid);
		info->userid = NULL;
	}
	if (userid && strlen(userid) > 0) info->userid = ms_strdup(userid);
}

void linphone_auth_info_set_realm(LinphoneAuthInfo *info, const char *realm) {
	if (info->realm) {
		ms_free(info->realm);
		info->realm = NULL;
	}
	if (realm && strlen(realm) > 0) info->realm = ms_strdup(realm);
}

void linphone_auth_info_set_domain(LinphoneAuthInfo *info, const char *domain) {
	if (info->domain) {
		ms_free(info->domain);
		info->domain = NULL;
	}
	if (domain && strlen(domain) > 0) info->domain = ms_strdup(domain);
}

void linphone_auth_info_set_ha1(LinphoneAuthInfo *info, const char *ha1) {
	if (info->ha1) {
		ms_free(info->ha1);
		info->ha1 = NULL;
	}
	if (ha1 && strlen(ha1) > 0) info->ha1 = ms_strdup(ha1);
}

void linphone_auth_info_set_tls_cert(LinphoneAuthInfo *info, const char *tls_cert) {
	if (info->tls_cert) {
		ms_free(info->tls_cert);
		info->tls_cert = NULL;
	}
	if (tls_cert && strlen(tls_cert) > 0) info->tls_cert = ms_strdup(tls_cert);
}

void linphone_auth_info_set_tls_key(LinphoneAuthInfo *info, const char *tls_key) {
	if (info->tls_key) {
		ms_free(info->tls_key);
		info->tls_key = NULL;
	}
	if (tls_key && strlen(tls_key) > 0) info->tls_key = ms_strdup(tls_key);
}

void linphone_auth_info_set_tls_cert_path(LinphoneAuthInfo *info, const char *tls_cert_path) {
	if (info->tls_cert_path) {
		ms_free(info->tls_cert_path);
		info->tls_cert_path = NULL;
	}
	if (tls_cert_path && strlen(tls_cert_path) > 0) info->tls_cert_path = ms_strdup(tls_cert_path);
}

void linphone_auth_info_set_tls_key_path(LinphoneAuthInfo *info, const char *tls_key_path) {
	if (info->tls_key_path) {
		ms_free(info->tls_key_path);
		info->tls_key_path = NULL;
	}
	if (tls_key_path && strlen(tls_key_path) > 0) info->tls_key_path = ms_strdup(tls_key_path);
}

static void _linphone_auth_info_uninit(LinphoneAuthInfo *obj) {
	if (obj->username != NULL) ms_free(obj->username);
	if (obj->userid != NULL) ms_free(obj->userid);
	if (obj->passwd != NULL) ms_free(obj->passwd);
	if (obj->ha1 != NULL) ms_free(obj->ha1);
	if (obj->realm != NULL) ms_free(obj->realm);
	if (obj->domain != NULL) ms_free(obj->domain);
	if (obj->tls_cert != NULL) ms_free(obj->tls_cert);
	if (obj->tls_key != NULL) ms_free(obj->tls_key);
	if (obj->tls_cert_path != NULL) ms_free(obj->tls_cert_path);
	if (obj->tls_key_path != NULL) ms_free(obj->tls_key_path);
}

/**
 * Destroys a LinphoneAuthInfo object.
**/
void linphone_auth_info_destroy(LinphoneAuthInfo *obj){
	belle_sip_object_unref(obj);
}

void linphone_auth_info_write_config(LpConfig *config, LinphoneAuthInfo *obj, int pos) {
	char key[50];
	bool_t store_ha1_passwd = lp_config_get_int(config, "sip", "store_ha1_passwd", 1);

	sprintf(key, "auth_info_%i", pos);
	lp_config_clean_section(config, key);

	if (obj == NULL || lp_config_get_int(config, "sip", "store_auth_info", 1) == 0) {
		return;
	}
	if (!obj->ha1 && obj->realm && obj->passwd && (obj->username || obj->userid) && store_ha1_passwd) {
		/*compute ha1 to avoid storing clear text password*/
		obj->ha1 = ms_malloc(33);
		sal_auth_compute_ha1(obj->userid ? obj->userid : obj->username, obj->realm, obj->passwd, obj->ha1);
	}
	if (obj->username != NULL) {
		lp_config_set_string(config, key, "username", obj->username);
	}
	if (obj->userid != NULL) {
		lp_config_set_string(config, key, "userid", obj->userid);
	}
	if (obj->ha1 != NULL) {
		lp_config_set_string(config, key, "ha1", obj->ha1);
	}
	if (obj->passwd != NULL) {
		if (store_ha1_passwd && obj->ha1) {
			/*if we have our ha1 and store_ha1_passwd set to TRUE, then drop the clear text password for security*/
			linphone_auth_info_set_passwd(obj, NULL);
		} else {
			/*we store clear text password only if store_ha1_passwd is FALSE AND we have an ha1 to store. Otherwise, passwd would simply be removed, which might bring major auth issue*/
			lp_config_set_string(config, key, "passwd", obj->passwd);
		}
	}
	if (obj->realm != NULL) {
		lp_config_set_string(config, key, "realm", obj->realm);
	}
	if (obj->domain != NULL) {
		lp_config_set_string(config, key, "domain", obj->domain);
	}
	if (obj->tls_cert_path != NULL) {
		lp_config_set_string(config, key, "client_cert_chain", obj->tls_cert_path);
	}
	if (obj->tls_key_path != NULL) {
		lp_config_set_string(config, key, "client_cert_key", obj->tls_key_path);
	}
}

LinphoneAuthInfo *linphone_auth_info_new_from_config_file(LpConfig * config, int pos)
{
	char key[50];
	const char *username,*userid,*passwd,*ha1,*realm,*domain,*tls_cert_path,*tls_key_path;
	LinphoneAuthInfo *ret;

	sprintf(key, "auth_info_%i", pos);
	if (!lp_config_has_section(config, key)) {
		return NULL;
	}

	username = lp_config_get_string(config, key, "username", NULL);
	userid = lp_config_get_string(config, key, "userid", NULL);
	passwd = lp_config_get_string(config, key, "passwd", NULL);
	ha1 = lp_config_get_string(config, key, "ha1", NULL);
	realm = lp_config_get_string(config, key, "realm", NULL);
	domain = lp_config_get_string(config, key, "domain", NULL);
	tls_cert_path = lp_config_get_string(config, key, "client_cert_chain", NULL);
	tls_key_path = lp_config_get_string(config, key, "client_cert_key", NULL);
	ret = linphone_auth_info_new(username, userid, passwd, ha1, realm, domain);
	linphone_auth_info_set_tls_cert_path(ret, tls_cert_path);
	linphone_auth_info_set_tls_key_path(ret, tls_key_path);
	return ret;
}

static char * remove_quotes(char * input){
	char *tmp;
	if (*input=='"') input++;
	tmp=strchr(input,'"');
	if (tmp) *tmp='\0';
	return input;
}

static bool_t realm_match(const char *realm1, const char *realm2){
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

static const LinphoneAuthInfo *find_auth_info(LinphoneCore *lc, const char *username, const char *realm, const char *domain, bool_t ignore_realm){
	bctbx_list_t *elem;
	const LinphoneAuthInfo *ret=NULL;

	for (elem=lc->auth_info;elem!=NULL;elem=elem->next) {
		LinphoneAuthInfo *pinfo = (LinphoneAuthInfo*)elem->data;
		if (username && pinfo->username && strcmp(username,pinfo->username)==0) {
			if (realm && domain){
				if (pinfo->realm && realm_match(realm,pinfo->realm)
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
			} else if (domain && pinfo->domain && strcmp(domain,pinfo->domain)==0 && (pinfo->ha1==NULL || ignore_realm)) {
				return pinfo;
			} else if (!domain && (pinfo->ha1==NULL || ignore_realm)) {
				return pinfo;
			}
		}
	}
	return ret;
}

const LinphoneAuthInfo *_linphone_core_find_tls_auth_info(LinphoneCore *lc) {
	bctbx_list_t *elem;
	for (elem=lc->auth_info;elem!=NULL;elem=elem->next) {
		LinphoneAuthInfo *pinfo = (LinphoneAuthInfo*)elem->data;
		if (pinfo->tls_cert && pinfo->tls_key) {
			return pinfo;
		} else if (pinfo->tls_cert_path && pinfo->tls_key_path) {
			return pinfo;
		}
	}
	return NULL;
}

const LinphoneAuthInfo *_linphone_core_find_auth_info(LinphoneCore *lc, const char *realm, const char *username, const char *domain, bool_t ignore_realm){
	const LinphoneAuthInfo *ai=NULL;
	if (realm){
		ai=find_auth_info(lc,username,realm,NULL, FALSE);
		if (ai==NULL && domain){
			ai=find_auth_info(lc,username,realm,domain, FALSE);
		}
	}
	if (ai == NULL && domain != NULL) {
		ai=find_auth_info(lc,username,NULL,domain, ignore_realm);
	}
	if (ai==NULL){
		ai=find_auth_info(lc,username,NULL,NULL, ignore_realm);
	}
	if (ai) ms_message("linphone_core_find_auth_info(): returning auth info username=%s, realm=%s", ai->username ? ai->username : "", ai->realm ? ai->realm : "");
	return ai;
}

const LinphoneAuthInfo *linphone_core_find_auth_info(LinphoneCore *lc, const char *realm, const char *username, const char *domain){
	return _linphone_core_find_auth_info(lc, realm, username, domain, TRUE);
}

/*the auth info is expected to be in the core's list*/
void linphone_core_write_auth_info(LinphoneCore *lc, LinphoneAuthInfo *ai){
	int i;
	bctbx_list_t *elem = lc->auth_info;

	if (!lc->sip_conf.save_auth_info) return;

	for (i=0; elem != NULL; elem = elem->next, i++){
		if (ai == elem->data){
			linphone_auth_info_write_config(lc->config, ai, i);
		}
	}
}

static void write_auth_infos(LinphoneCore *lc){
	bctbx_list_t *elem;
	int i;

	if (!linphone_core_ready(lc)) return;
	if (!lc->sip_conf.save_auth_info) return;
	for(elem=lc->auth_info,i=0;elem!=NULL;elem=bctbx_list_next(elem),i++){
		LinphoneAuthInfo *ai=(LinphoneAuthInfo*)(elem->data);
		linphone_auth_info_write_config(lc->config,ai,i);
	}
	linphone_auth_info_write_config(lc->config,NULL,i); /* mark the end */
}

LinphoneAuthInfo * linphone_core_create_auth_info(LinphoneCore *lc, const char *username, const char *userid, const char *passwd, const char *ha1, const char *realm, const char *domain) {
	return linphone_auth_info_new(username, userid, passwd, ha1, realm, domain);
}

void linphone_core_add_auth_info(LinphoneCore *lc, const LinphoneAuthInfo *info){
	LinphoneAuthInfo *ai;
	bctbx_list_t *elem;
	bctbx_list_t *l;
	int restarted_op_count=0;
	bool_t updating=FALSE;

	if (info->tls_key == NULL && info->tls_key_path == NULL
		&& info->ha1==NULL && info->passwd==NULL){
		ms_error("linphone_core_add_auth_info(): info supplied with empty password, ha1 or TLS client/key");
		return;
	}
	/* find if we are attempting to modify an existing auth info */
	ai=(LinphoneAuthInfo*)linphone_core_find_auth_info(lc,info->realm,info->username,info->domain);
	if (ai!=NULL && ai->domain && info->domain && strcmp(ai->domain, info->domain)==0){
		lc->auth_info=bctbx_list_remove(lc->auth_info,ai);
		linphone_auth_info_destroy(ai);
		updating=TRUE;
	}
	lc->auth_info=bctbx_list_append(lc->auth_info,linphone_auth_info_clone(info));

	/* retry pending authentication operations */
	for(l=elem=sal_get_pending_auths(lc->sal);elem!=NULL;elem=elem->next){
		SalOp *op=(SalOp*)elem->data;
		LinphoneAuthInfo *ai;
		const SalAuthInfo *req_sai=sal_op_get_auth_requested(op);
		ai=(LinphoneAuthInfo*)_linphone_core_find_auth_info(lc,req_sai->realm,req_sai->username,req_sai->domain, FALSE);
		if (ai){
			SalAuthInfo sai;
			bctbx_list_t* proxy;
			sai.username=ai->username;
			sai.userid=ai->userid;
			sai.realm=ai->realm;
			sai.password=ai->passwd;
			sai.ha1=ai->ha1;
			if (ai->tls_cert && ai->tls_key) {
				sal_certificates_chain_parse(&sai, ai->tls_cert, SAL_CERTIFICATE_RAW_FORMAT_PEM);
				sal_signing_key_parse(&sai, ai->tls_key, "");
			} else if (ai->tls_cert_path && ai->tls_key_path) {
				sal_certificates_chain_parse_file(&sai, ai->tls_cert_path, SAL_CERTIFICATE_RAW_FORMAT_PEM);
				sal_signing_key_parse_file(&sai, ai->tls_key_path, "");
			}
			/*proxy case*/
			for (proxy=(bctbx_list_t*)linphone_core_get_proxy_config_list(lc);proxy!=NULL;proxy=proxy->next) {
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
	bctbx_list_free(l);
	write_auth_infos(lc);
}

void linphone_core_abort_authentication(LinphoneCore *lc,  LinphoneAuthInfo *info){
}

void linphone_core_remove_auth_info(LinphoneCore *lc, const LinphoneAuthInfo *info){
	LinphoneAuthInfo *r;
	r=(LinphoneAuthInfo*)linphone_core_find_auth_info(lc,info->realm,info->username,info->domain);
	if (r){
		lc->auth_info=bctbx_list_remove(lc->auth_info,r);
		linphone_auth_info_destroy(r);
		write_auth_infos(lc);
	}
}

const bctbx_list_t *linphone_core_get_auth_info_list(const LinphoneCore *lc){
	return lc->auth_info;
}

void linphone_core_clear_all_auth_info(LinphoneCore *lc){
	bctbx_list_t *elem;
	int i;
	for(i=0,elem=lc->auth_info;elem!=NULL;elem=bctbx_list_next(elem),i++){
		LinphoneAuthInfo *info=(LinphoneAuthInfo*)elem->data;
		linphone_auth_info_destroy(info);
		linphone_auth_info_write_config(lc->config,NULL,i);
	}
	bctbx_list_free(lc->auth_info);
	lc->auth_info=NULL;
}
