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

#ifndef CONTACT_PROVIDERS_PRIV_H
#define CONTACT_PROVIDERS_PRIV_H

#include "private.h"
#include "linphone/core.h"

/* Base for contact search and contact provider */

struct _LinphoneContactSearch{
	belle_sip_object_t base;
	ContactSearchID id;
	char* predicate;
	ContactSearchCallback cb;
	void* data;
};

#define LINPHONE_CONTACT_SEARCH(obj) BELLE_SIP_CAST(obj,LinphoneContactSearch)
BELLE_SIP_DECLARE_VPTR(LinphoneContactSearch)


struct _LinphoneContactProvider {
	belle_sip_object_t base;
	LinphoneCore* lc;
};

#define LINPHONE_CONTACT_PROVIDER(obj) BELLE_SIP_CAST(obj,LinphoneContactProvider)

typedef LinphoneContactSearch* (*LinphoneContactProviderStartSearchMethod)( LinphoneContactProvider* thiz, const char* predicate, ContactSearchCallback cb, void* data );
typedef unsigned int           (*LinphoneContactProviderCancelSearchMethod)( LinphoneContactProvider* thiz, LinphoneContactSearch *request );

BELLE_SIP_DECLARE_CUSTOM_VPTR_BEGIN(LinphoneContactProvider,belle_sip_object_t)
	const char* name; /*!< Name of the contact provider (LDAP, Google, ...) */

	/* pure virtual methods: inheriting objects must implement these */
	LinphoneContactProviderStartSearchMethod  begin_search;
	LinphoneContactProviderCancelSearchMethod cancel_search;
BELLE_SIP_DECLARE_CUSTOM_VPTR_END

/* LDAP search and contact providers */


#define LINPHONE_LDAP_CONTACT_SEARCH(obj) BELLE_SIP_CAST(obj,LinphoneLDAPContactSearch)
BELLE_SIP_DECLARE_VPTR(LinphoneLDAPContactSearch)

#define LINPHONE_LDAP_CONTACT_PROVIDER(obj) BELLE_SIP_CAST(obj,LinphoneLDAPContactProvider)

BELLE_SIP_DECLARE_CUSTOM_VPTR_BEGIN(LinphoneLDAPContactProvider,LinphoneContactProvider)
BELLE_SIP_DECLARE_CUSTOM_VPTR_END


#endif // CONTACT_PROVIDERS_PRIV_H
