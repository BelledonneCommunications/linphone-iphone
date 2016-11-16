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

/* LinphoneContactSearchRequest */

void linphone_contact_search_init(LinphoneContactSearch* obj, const char* predicate, ContactSearchCallback cb, void* cb_data);
ContactSearchID linphone_contact_search_get_id(LinphoneContactSearch* obj);
const char* linphone_contact_search_get_predicate(LinphoneContactSearch* obj);
void linphone_contact_search_invoke_cb(LinphoneContactSearch* req, MSList* friends);
LINPHONE_PUBLIC LinphoneContactSearch* linphone_contact_search_ref(void* obj);
void                    linphone_contact_search_unref(void* obj);
LinphoneContactSearch* linphone_contact_search_cast( void*obj );

/* LinphoneContactProvider */

void          linphone_contact_provider_init(LinphoneContactProvider* obj, LinphoneCore* lc);
LinphoneCore* linphone_contact_provider_get_core(LinphoneContactProvider* obj);
const char*   linphone_contact_provider_get_name(LinphoneContactProvider* obj);
LinphoneContactProvider* linphone_contact_provider_ref(void* obj);
LINPHONE_PUBLIC void                     linphone_contact_provider_unref(void* obj);
LINPHONE_PUBLIC LinphoneContactProvider* linphone_contact_provider_cast( void*obj );

LINPHONE_PUBLIC LinphoneContactSearch* linphone_contact_provider_begin_search(LinphoneContactProvider* obj,
															  const char* predicate,
															  ContactSearchCallback cb,
															  void* data);
unsigned int           linphone_contact_provider_cancel_search(LinphoneContactProvider* obj,
															   LinphoneContactSearch* request);
