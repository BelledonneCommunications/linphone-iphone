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

#include <belle-sip/object.h>
#include "linphonecore.h"

/* LinphoneContactSearchRequest */

struct _LinphoneContactSearch{
	belle_sip_object_t base;
	ContactSearchID id;
	char* predicate;
	ContactSearchCallback cb;
	void* data;
};

#define LINPHONE_CONTACT_SEARCH(obj) BELLE_SIP_CAST(obj,LinphoneContactSearch)
BELLE_SIP_DECLARE_VPTR(LinphoneContactSearch)


void linphone_contact_search_init(LinphoneContactSearch* obj, const char* predicate, ContactSearchCallback cb, void* cb_data);
ContactSearchID linphone_contact_search_get_id(LinphoneContactSearch* obj);
const char* linphone_contact_search_get_predicate(LinphoneContactSearch* obj);
void linphone_contact_search_invoke_cb(LinphoneContactSearch* req, MSList* friends);


/* LinphoneContactProvider */

struct _LinphoneContactProvider {
	belle_sip_object_t base;
	LinphoneCore* lc;
};

typedef struct _LinphoneContactProvider LinphoneContactProvider;
typedef LinphoneContactSearch* (*LinphoneContactProviderStartSearchMethod)( LinphoneContactProvider* thiz, const char* predicate, ContactSearchCallback cb, void* data );
typedef unsigned int           (*LinphoneContactProviderCancelSearchMethod)( LinphoneContactProvider* thiz, LinphoneContactSearch *request );
#define LINPHONE_CONTACT_PROVIDER(obj) BELLE_SIP_CAST(obj,LinphoneContactProvider)

BELLE_SIP_DECLARE_CUSTOM_VPTR_BEGIN(LinphoneContactProvider,belle_sip_object_t)
	const char* name; /*!< Name of the contact provider (LDAP, Google, ...) */

	/* pure virtual methods: inheriting objects must implement these */
	LinphoneContactProviderStartSearchMethod  begin_search;
	LinphoneContactProviderCancelSearchMethod cancel_search;
BELLE_SIP_DECLARE_CUSTOM_VPTR_END


void          linphone_contact_provider_init(LinphoneContactProvider* obj, LinphoneCore* lc);
LinphoneCore* linphone_contact_provider_get_core(LinphoneContactProvider* obj);
const char*   linphone_contact_provider_get_name(LinphoneContactProvider* obj);
