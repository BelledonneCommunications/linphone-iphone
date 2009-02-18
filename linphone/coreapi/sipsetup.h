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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#ifndef sipsetup_h
#define sipsetup_h
#include "linphonecore.h"

struct _SipSetup;

struct _BuddyInfo;

struct _SipSetupContext{
	struct _SipSetup *funcs;
	char domain[128];
	char username[128];
	void *data;
};

typedef struct _SipSetupContext SipSetupContext;

struct _SipSetup{
	char *name;
	bool_t (*init)(void);
	int (*create_account)( const char *uri, const char *passwd);
	int (*login_account)(SipSetupContext *ctx, const char *uri, const char *passwd);
	int (*get_proxy)(SipSetupContext *ctx, const char *domain, char *proxy, size_t sz);
	int (*get_stun_servers)(SipSetupContext *ctx, char *stun1, char *stun2, size_t size);
	int (*get_relay)(SipSetupContext *ctx, char *relay, size_t size);
	int (*lookup_buddy)(SipSetupContext *ctx, const char *key, struct _BuddyInfo *info);
	void (*exit)(void);
	bool_t initialized;
};

typedef struct _SipSetup SipSetup;

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
	BuddyAddress address;
}BuddyInfo;

void sip_setup_register_all(void);
SipSetup *sip_setup_lookup(const char *type_name);
void sip_setup_unregister_all(void);

int sip_setup_new_account(SipSetup *s, const char *uri, const char *passwd);
SipSetupContext * sip_setup_context_new(SipSetup *s);
int sip_setup_context_login_account(SipSetupContext * ctx, const char *uri, const char *passwd);
int sip_setup_context_get_proxy(SipSetupContext *ctx, const char *domain, char *proxy, size_t sz);
int sip_setup_context_get_stun_servers(SipSetupContext *ctx, char *stun1, char *stun2, size_t size);
int sip_setup_context_get_relay(SipSetupContext *ctx,char *relay, size_t size);
int sip_setup_context_lookup_buddy(SipSetupContext *ctx, const char *key, BuddyInfo *binfo);
void sip_setup_context_free(SipSetupContext *ctx);
#endif

