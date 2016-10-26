 /*
 tester - liblinphone test suite
 Copyright (C) 2013  Belledonne Communications SARL

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "liblinphone_tester.h"
#include "private.h"

struct _Account{
	LinphoneAddress *identity;
	LinphoneAddress *modified_identity;
	char *password;
	int registered;
	int done;
	int created;
	char *phone_alias;
};

typedef struct _Account Account;

static Account *account_new(LinphoneAddress *identity, const char *unique_id){
	char *modified_username;
	Account *obj=ms_new0(Account,1);

	/* we need to inhibit leak detector because the two LinphoneAddress will remain behond the scope of the test being run */
	belle_sip_object_inhibit_leak_detector(TRUE);
	obj->identity=linphone_address_clone(identity);
	obj->password=sal_get_random_token(8);
	obj->phone_alias = NULL;
	obj->modified_identity=linphone_address_clone(identity);
	modified_username=ms_strdup_printf("%s_%s",linphone_address_get_username(identity), unique_id);
	linphone_address_set_username(obj->modified_identity, modified_username);
	ms_free(modified_username);
	belle_sip_object_inhibit_leak_detector(FALSE);
	return obj;
};

void account_destroy(Account *obj){
	linphone_address_unref(obj->identity);
	linphone_address_unref(obj->modified_identity);
	ms_free(obj->password);
	ms_free(obj);
}

struct _AccountManager{
	char *unique_id;
	bctbx_list_t *accounts;
};

typedef struct _AccountManager AccountManager;

static AccountManager *the_am=NULL;

AccountManager *account_manager_get(void){
	if (the_am==NULL){
		the_am=ms_new0(AccountManager,1);
		the_am->unique_id=sal_get_random_token(6);
	}
	return the_am;
}

void account_manager_destroy(void){
	if (the_am){
		ms_free(the_am->unique_id);
		bctbx_list_free_with_data(the_am->accounts,(void(*)(void*))account_destroy);
		ms_free(the_am);
	}
	the_am=NULL;
	ms_message("Test account manager destroyed.");
}

Account *account_manager_get_account(AccountManager *m, const LinphoneAddress *identity){
	bctbx_list_t *it;

	for(it=m->accounts;it!=NULL;it=it->next){
		Account *a=(Account*)it->data;
		if (linphone_address_weak_equal(a->identity,identity)){
			return a;
		}
	}
	return NULL;
}

static void account_created_on_server_cb(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const char *info){
	Account *account=(Account*)linphone_core_get_user_data(lc);
	switch(state){
		case LinphoneRegistrationOk: {
			char * phrase = sal_op_get_error_info((SalOp*)cfg->op)->full_string;
			if (phrase && strcasecmp("Test account created", phrase) == 0) {
				account->created=1;
			} else {
				account->registered=1;
			}
			break;
		}
		case LinphoneRegistrationCleared:
			account->done=1;
		break;
		default:
		break;
	}
}

// TEMPORARY CODE: remove function below when flexisip is updated, this is not needed anymore!
// The new flexisip now answer "200 Test account created" when creating a test account, and do not
// challenge authentication anymore! so this code is not used for newer version
static void account_created_auth_requested_cb(LinphoneCore *lc, const char *username, const char *realm, const char *domain){
	Account *account=(Account*)linphone_core_get_user_data(lc);
	account->created=1;
}
// TEMPORARY CODE: remove line above when flexisip is updated, this is not needed anymore!

void account_create_on_server(Account *account, const LinphoneProxyConfig *refcfg, const char* phone_alias){
	LinphoneCoreVTable vtable={0};
	LinphoneCore *lc;
	LinphoneAddress *tmp_identity=linphone_address_clone(account->modified_identity);
	LinphoneProxyConfig *cfg;
	LinphoneAuthInfo *ai;
	char *tmp;
	LinphoneAddress *server_addr;
	LCSipTransports tr;
	char *chatdb;

	vtable.registration_state_changed=account_created_on_server_cb;
	// TEMPORARY CODE: remove line below when flexisip is updated, this is not needed anymore!
	vtable.auth_info_requested=account_created_auth_requested_cb;
	lc=configure_lc_from(&vtable,bc_tester_get_resource_dir_prefix(),NULL,account);
	chatdb = ms_strdup(linphone_core_get_chat_database_path(lc));
	tr.udp_port=LC_SIP_TRANSPORT_RANDOM;
	tr.tcp_port=LC_SIP_TRANSPORT_RANDOM;
	tr.tls_port=LC_SIP_TRANSPORT_RANDOM;
	linphone_core_set_sip_transports(lc,&tr);

	cfg=linphone_core_create_proxy_config(lc);
	linphone_address_set_secure(tmp_identity, FALSE);
	linphone_address_set_password(tmp_identity,account->password);
	linphone_address_set_header(tmp_identity,"X-Create-Account","yes");
	if (phone_alias) linphone_address_set_header(tmp_identity, "X-Phone-Alias", phone_alias);
	tmp=linphone_address_as_string(tmp_identity);
	linphone_proxy_config_set_identity(cfg,tmp);
	ms_free(tmp);
	linphone_address_unref(tmp_identity);

	server_addr=linphone_address_new(linphone_proxy_config_get_server_addr(refcfg));
	linphone_address_set_secure(server_addr, FALSE);
	linphone_address_set_transport(server_addr,LinphoneTransportTcp); /*use tcp for account creation, we may not have certificates configured at this stage*/
	linphone_address_set_port(server_addr,0);
	tmp=linphone_address_as_string(server_addr);
	linphone_proxy_config_set_server_addr(cfg,tmp);
	ms_free(tmp);
	linphone_address_unref(server_addr);
	linphone_proxy_config_set_expires(cfg,3*3600); //accounts are valid 3 hours

	linphone_core_add_proxy_config(lc,cfg);
	/*wait 25 seconds, since the DNS SRV resolution may take a while - and
	especially if router does NOT support DNS SRV and we have to wait its timeout*/
	if (wait_for_until(lc,NULL,&account->created,1,25000)==FALSE){
		ms_fatal("Account for %s could not be created on server.", linphone_proxy_config_get_identity(refcfg));
	}
	linphone_proxy_config_edit(cfg);
	tmp_identity=linphone_address_clone(account->modified_identity);
	linphone_address_set_secure(tmp_identity, FALSE);
	tmp=linphone_address_as_string(tmp_identity);
	linphone_proxy_config_set_identity(cfg,tmp); /*remove the X-Create-Account header*/
	linphone_address_unref(tmp_identity);
	ms_free(tmp);
	linphone_proxy_config_done(cfg);

	ai=linphone_auth_info_new(linphone_address_get_username(account->modified_identity),
				NULL,
				account->password,NULL,NULL,linphone_address_get_domain(account->modified_identity));
	linphone_core_add_auth_info(lc,ai);
	linphone_auth_info_destroy(ai);

	if (wait_for_until(lc,NULL,&account->registered,1,3000)==FALSE){
		ms_fatal("Account for %s is not working on server.", linphone_proxy_config_get_identity(refcfg));
	}
	linphone_core_remove_proxy_config(lc,cfg);
	linphone_proxy_config_unref(cfg);
	if (wait_for_until(lc,NULL,&account->done,1,3000)==FALSE){
		ms_error("Account creation could not clean the registration context.");
	}
	linphone_core_destroy(lc);
	unlink(chatdb);
	ms_free(chatdb);
}

static LinphoneAddress *account_manager_check_account(AccountManager *m, LinphoneProxyConfig *cfg,const char* phone_alias){
	LinphoneCore *lc=linphone_proxy_config_get_core(cfg);
	const char *identity=linphone_proxy_config_get_identity(cfg);
	LinphoneAddress *id_addr=linphone_address_new(identity);
	Account *account=account_manager_get_account(m,id_addr);
	LinphoneAuthInfo *ai;
	char *tmp;
	bool_t create_account=FALSE;
	const LinphoneAuthInfo *original_ai = linphone_core_find_auth_info(lc
																		,NULL
																		, linphone_address_get_username(id_addr)
																		, linphone_address_get_domain(id_addr));

	if (!account||(phone_alias&&(!account->phone_alias||strcmp(phone_alias,account->phone_alias)!=0))){
		if (account) {
			m->accounts=bctbx_list_remove(m->accounts,account);
			account_destroy(account);
		}
		account=account_new(id_addr,m->unique_id);
		account->phone_alias=ms_strdup(phone_alias);
		ms_message("No account for %s exists, going to create one.",identity);
		create_account=TRUE;
		m->accounts=bctbx_list_append(m->accounts,account);
	}
	/*modify the username of the identity of the proxy config*/
	linphone_address_set_username(id_addr, linphone_address_get_username(account->modified_identity));
	tmp=linphone_address_as_string(id_addr);
	linphone_proxy_config_set_identity(cfg,tmp);
	ms_free(tmp);

	if (create_account){
		account_create_on_server(account,cfg,phone_alias);
	}

	/*remove previous auth info to avoid mismatching*/
	if (original_ai)
		linphone_core_remove_auth_info(lc,original_ai);

	ai=linphone_auth_info_new(linphone_address_get_username(account->modified_identity),
				NULL,
				account->password,NULL,NULL,linphone_address_get_domain(account->modified_identity));
	linphone_core_add_auth_info(lc,ai);
	linphone_auth_info_destroy(ai);

	linphone_address_unref(id_addr);
	return account->modified_identity;
}

void linphone_core_manager_check_accounts(LinphoneCoreManager *m){
	const bctbx_list_t *it;
	AccountManager *am=account_manager_get();
	int logmask = ortp_get_log_level_mask(NULL);
	
	if (!liblinphonetester_show_account_manager_logs) linphone_core_set_log_level_mask(ORTP_ERROR|ORTP_FATAL);
	for(it=linphone_core_get_proxy_config_list(m->lc);it!=NULL;it=it->next){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig *)it->data;
		account_manager_check_account(am,cfg,m->phone_alias);
	}
	if (!liblinphonetester_show_account_manager_logs) linphone_core_set_log_level_mask(logmask);
}
