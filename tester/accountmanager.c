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
	int created;
	int done;
	int auth_requested;
};

typedef struct _Account Account;

Account *account_new(LinphoneAddress *identity, const char *unique_id){
	char *modified_username;
	Account *obj=ms_new0(Account,1);
	
	/* we need to inhibit leak detector because the two LinphoneAddress will remain behond the scope of the test being run */
	belle_sip_object_inhibit_leak_detector(TRUE);
	obj->identity=linphone_address_clone(identity);
	obj->password=sal_get_random_token(8);
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
	MSList *accounts;
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
		ms_list_free_with_data(the_am->accounts,(void(*)(void*))account_destroy);
		ms_free(the_am);
	}
	the_am=NULL;
}

Account *account_manager_get_account(AccountManager *m, const LinphoneAddress *identity){
	MSList *it;
	
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
		case LinphoneRegistrationOk:
			account->created=1;
		break;
		case LinphoneRegistrationCleared:
			account->done=1;
		break;
		default:
		break;
	}
}

static void account_created_auth_requested_cb(LinphoneCore *lc, const char *username, const char *realm, const char *domain){
	Account *account=(Account*)linphone_core_get_user_data(lc);
	account->auth_requested=1;
}

void account_create_on_server(Account *account, const LinphoneProxyConfig *refcfg){
	LinphoneCoreVTable vtable={0};
	LinphoneCore *lc;
	LinphoneAddress *tmp_identity=linphone_address_clone(account->modified_identity);
	LinphoneProxyConfig *cfg;
	LinphoneAuthInfo *ai;
	char *tmp;
	LinphoneAddress *server_addr;
	LCSipTransports tr;
	
	vtable.registration_state_changed=account_created_on_server_cb;
	vtable.auth_info_requested=account_created_auth_requested_cb;
	lc=configure_lc_from(&vtable,liblinphone_tester_file_prefix,NULL,account);
	tr.udp_port=LC_SIP_TRANSPORT_RANDOM;
	tr.tcp_port=LC_SIP_TRANSPORT_RANDOM;
	tr.tls_port=LC_SIP_TRANSPORT_RANDOM;
	linphone_core_set_sip_transports(lc,&tr);
	
	cfg=linphone_core_create_proxy_config(lc);
	linphone_address_set_password(tmp_identity,account->password);
	linphone_address_set_header(tmp_identity,"X-Create-Account","yes");
	tmp=linphone_address_as_string(tmp_identity);
	linphone_proxy_config_set_identity(cfg,tmp);
	ms_free(tmp);
	linphone_address_unref(tmp_identity);
	
	server_addr=linphone_address_new(linphone_proxy_config_get_server_addr(refcfg));
	linphone_address_set_transport(server_addr,LinphoneTransportTcp); /*use tcp for account creation*/
	linphone_address_set_port(server_addr,0);
	tmp=linphone_address_as_string(server_addr);
	linphone_proxy_config_set_server_addr(cfg,tmp);
	ms_free(tmp);
	linphone_address_unref(server_addr);
	linphone_proxy_config_set_expires(cfg,3600);
	
	linphone_core_add_proxy_config(lc,cfg);
	
	if (wait_for_until(lc,NULL,&account->auth_requested,1,10000)==FALSE){
		ms_fatal("Account for %s could not be created on server.", linphone_proxy_config_get_identity(refcfg));
	}
	linphone_proxy_config_edit(cfg);
	tmp=linphone_address_as_string(account->modified_identity);
	linphone_proxy_config_set_identity(cfg,tmp); /*remove the X-Create-Account header*/
	ms_free(tmp);
	linphone_proxy_config_done(cfg);
	
	ai=linphone_auth_info_new(linphone_address_get_username(account->modified_identity),
				NULL,
				account->password,NULL,NULL,linphone_address_get_domain(account->modified_identity));
	linphone_core_add_auth_info(lc,ai);
	linphone_auth_info_destroy(ai);
	
	if (wait_for_until(lc,NULL,&account->created,1,3000)==FALSE){
		ms_fatal("Account for %s is not working on server.", linphone_proxy_config_get_identity(refcfg));
	}
	linphone_core_remove_proxy_config(lc,cfg);
	linphone_proxy_config_unref(cfg);
	if (wait_for_until(lc,NULL,&account->done,1,3000)==FALSE){
		ms_error("Account creation could not clean the registration context.");
	}
	linphone_core_destroy(lc);
}

LinphoneAddress *account_manager_check_account(AccountManager *m, LinphoneProxyConfig *cfg){
	LinphoneCore *lc=linphone_proxy_config_get_core(cfg);
	const char *identity=linphone_proxy_config_get_identity(cfg);
	LinphoneAddress *id_addr=linphone_address_new(identity);
	Account *account=account_manager_get_account(m,id_addr);
	LinphoneAuthInfo *ai;
	char *tmp;
	bool_t create_account=FALSE;
	
	if (!account){
		account=account_new(id_addr,m->unique_id);
		ms_message("No account for %s exists, going to create one.",identity);
		create_account=TRUE;
		m->accounts=ms_list_append(m->accounts,account);
	}
	tmp=linphone_address_as_string(account->modified_identity);
	linphone_proxy_config_set_identity(cfg,tmp);
	ms_free(tmp);
	
	if (create_account){
		account_create_on_server(account,cfg);
	}
	ai=linphone_auth_info_new(linphone_address_get_username(account->modified_identity),
				NULL,
				account->password,NULL,NULL,linphone_address_get_domain(account->modified_identity));
	linphone_core_add_auth_info(lc,ai);
	linphone_auth_info_destroy(ai);
	
	linphone_address_unref(id_addr);
	return account->modified_identity;
}

void linphone_core_manager_check_accounts(LinphoneCoreManager *m){
	const MSList *it;
	AccountManager *am=account_manager_get();

	for(it=linphone_core_get_proxy_config_list(m->lc);it!=NULL;it=it->next){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig *)it->data;
		LinphoneAddress *modified_identity=account_manager_check_account(am,cfg);
		if (m->identity){
			linphone_address_unref(m->identity);
		}
		m->identity=linphone_address_ref(modified_identity);
	}
}
