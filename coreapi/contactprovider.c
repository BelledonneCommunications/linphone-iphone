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

struct linphone_contact_provider {
	belle_sip_object_t base;
};

linphone_contact_provider_t* linphone_contact_provider_create()
{
	linphone_contact_provider_t* obj = belle_sip_object_new(linphone_contact_provider_t);
	return obj;
}

static void linphone_contact_provider_destroy()
{

}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(linphone_contact_provider_t);
BELLE_SIP_INSTANCIATE_VPTR(linphone_contact_provider_t, belle_sip_object_t,
						   linphone_contact_provider_destroy, NULL, NULL,FALSE);

