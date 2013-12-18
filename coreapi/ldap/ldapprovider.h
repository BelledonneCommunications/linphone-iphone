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

#include "contactprovider.h"

#include <ldap.h>

typedef struct _LinphoneLDAPContactProvider LinphoneLDAPContactProvider;

/* LinphoneLDAPContactSearch */
typedef struct _LinphoneLDAPContactSearch LinphoneLDAPContactSearch;

#define LINPHONE_LDAP_CONTACT_SEARCH(obj) BELLE_SIP_CAST(obj,LinphoneLDAPContactSearch)
BELLE_SIP_DECLARE_VPTR(LinphoneLDAPContactSearch)

LinphoneLDAPContactSearch* linphone_ldap_contact_search_create(LinphoneLDAPContactProvider* ld,
															   const char* predicate,
															   ContactSearchCallback cb,
															   void* cb_data);

unsigned int linphone_ldap_contact_search_result_count(LinphoneLDAPContactSearch* obj);


/* LinphoneLDAPContactProvider */

#define LINPHONE_LDAP_CONTACT_PROVIDER(obj) BELLE_SIP_CAST(obj,LinphoneLDAPContactProvider)

BELLE_SIP_DECLARE_CUSTOM_VPTR_BEGIN(LinphoneLDAPContactProvider,LinphoneContactProvider)
BELLE_SIP_DECLARE_CUSTOM_VPTR_END

LinphoneLDAPContactProvider* linphone_ldap_contact_provider_create(LinphoneCore* lc, const LinphoneDictionary* config);
unsigned int linphone_ldap_contact_provider_get_max_result(const LinphoneLDAPContactProvider* obj);
