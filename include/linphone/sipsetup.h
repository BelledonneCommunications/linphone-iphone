/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef sipsetup_h
#define sipsetup_h

#include "mediastreamer2/mscommon.h"

struct _SipSetup;

struct _BuddyInfo;

struct _LinphoneXmlRpcSession;


struct _SipSetupContext{
	struct _SipSetup *funcs;
	struct _LinphoneProxyConfig *cfg;
	struct _LinphoneXmlRpcSession *xmlrpc_session;
	char domain[128];
	char username[128];
	void *data;
};

typedef struct _SipSetupContext SipSetupContext;

#define SIP_SETUP_CAP_PROXY_PROVIDER	(1)
#define SIP_SETUP_CAP_STUN_PROVIDER	(1<<1)
#define SIP_SETUP_CAP_RELAY_PROVIDER	(1<<2)
#define SIP_SETUP_CAP_BUDDY_LOOKUP	(1<<3)
#define SIP_SETUP_CAP_ACCOUNT_MANAGER	(1<<4)  /*can create accounts*/
#define SIP_SETUP_CAP_LOGIN		(1<<5)  /*can login to any account for a given proxy */

typedef enum _BuddyLookupStatus{
	BuddyLookupNone,
	BuddyLookupConnecting,
	BuddyLookupConnected,
	BuddyLookupReceivingResponse,
	BuddyLookupDone,
	BuddyLookupFailure
}BuddyLookupStatus;

typedef struct _BuddyAddress{
	char street[64];
	char zip[64];
	char town[64];
	char country[64];
} BuddyAddress;

typedef struct _BuddyInfo{
	char firstname[64];
	char lastname[64];
	char displayname[64];
	char sip_uri[128];
	char email[128];
	BuddyAddress address;
	char image_type[32];
	uint8_t *image_data;
	int image_length;
}BuddyInfo;

typedef struct _BuddyLookupRequest {
	char *key;
	int max_results;
	BuddyLookupStatus status;
	MSList *results; /*of BuddyInfo */
}BuddyLookupRequest;


typedef struct _BuddyLookupFuncs{
	BuddyLookupRequest * (*request_create)(SipSetupContext *ctx);
	int (*request_submit)(SipSetupContext *ctx, BuddyLookupRequest *req);
	int (*request_free)(SipSetupContext *ctx, BuddyLookupRequest *req);
}BuddyLookupFuncs;


struct _SipSetup{
	const char *name;
	unsigned int capabilities;
	int initialized;
	bool_t (*init)(void);
	void (*exit)(void);
	void (*init_instance)(SipSetupContext *ctx);
	void (*uninit_instance)(SipSetupContext *ctx);
	int (*account_exists)(SipSetupContext *ctx, const char *uri);
	int (*create_account)(SipSetupContext *ctx, const char *uri, const char *passwd, const char *email, int suscribe);
	int (*login_account)(SipSetupContext *ctx, const char *uri, const char *passwd, const char *userid);
	int (*get_proxy)(SipSetupContext *ctx, const char *domain, char *proxy, size_t sz);
	int (*get_stun_servers)(SipSetupContext *ctx, char *stun1, char *stun2, size_t size);
	int (*get_relay)(SipSetupContext *ctx, char *relay, size_t size);
	const char * (*get_notice)(SipSetupContext *ctx);
	const char ** (*get_domains)(SipSetupContext *ctx);
	int (*logout_account)(SipSetupContext *ctx);
	BuddyLookupFuncs *buddy_lookup_funcs;
	int (*account_validated)(SipSetupContext *ctx, const char *uri);
};

typedef struct _SipSetup SipSetup;



#ifdef __cplusplus
extern "C"{
#endif

BuddyInfo *buddy_info_new(void);
void buddy_info_free(BuddyInfo *info);

LINPHONE_PUBLIC void buddy_lookup_request_set_key(BuddyLookupRequest *req, const char *key);
void buddy_lookup_request_set_max_results(BuddyLookupRequest *req, int ncount);


void sip_setup_register(SipSetup *ss);
void sip_setup_register_all(MSFactory* factory);
SipSetup *sip_setup_lookup(const char *type_name);
void sip_setup_unregister_all(void);
LINPHONE_PUBLIC unsigned int sip_setup_get_capabilities(SipSetup *s);

SipSetupContext * sip_setup_context_new(SipSetup *s, struct _LinphoneProxyConfig *cfg);
int sip_setup_context_account_exists(SipSetupContext *ctx, const char *uri);
int sip_setup_context_account_validated(SipSetupContext *ctx, const char *uri);
LinphoneStatus sip_setup_context_create_account(SipSetupContext *ctx, const char *uri, const char *passwd, const char *email, int suscribe);
LINPHONE_PUBLIC int sip_setup_context_get_capabilities(SipSetupContext *ctx);
LINPHONE_PUBLIC LinphoneStatus sip_setup_context_login_account(SipSetupContext * ctx, const char *uri, const char *passwd, const char *userid);
LinphoneStatus sip_setup_context_get_proxy(SipSetupContext *ctx, const char *domain, char *proxy, size_t sz);
LinphoneStatus sip_setup_context_get_stun_servers(SipSetupContext *ctx, char *stun1, char *stun2, size_t size);
LinphoneStatus sip_setup_context_get_relay(SipSetupContext *ctx, char *relay, size_t size);

LINPHONE_PUBLIC BuddyLookupRequest *sip_setup_context_create_buddy_lookup_request(SipSetupContext *ctx);
LINPHONE_PUBLIC LinphoneStatus sip_setup_context_buddy_lookup_submit(SipSetupContext *ctx , BuddyLookupRequest *req);
LINPHONE_PUBLIC LinphoneStatus sip_setup_context_buddy_lookup_free(SipSetupContext *ctx , BuddyLookupRequest *req);

const char * sip_setup_context_get_notice(SipSetupContext *ctx);
const char ** sip_setup_context_get_domains(SipSetupContext *ctx);

void sip_setup_context_free(SipSetupContext *ctx);

LINPHONE_PUBLIC LinphoneStatus sip_setup_context_logout(SipSetupContext *ctx);

/*internal methods for use WITHIN plugins: do not use elsewhere*/
struct _LinphoneProxyConfig *sip_setup_context_get_proxy_config(const SipSetupContext *ctx);
void buddy_lookup_request_free(BuddyLookupRequest *req);

#ifdef __cplusplus
}
#endif


#endif


