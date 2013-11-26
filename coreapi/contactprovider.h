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


typedef struct linphone_contact_provider linphone_contact_provider_t;
#define LINPHONE_CONTACT_PROVIDER(obj) BELLE_SIP_CAST(obj,linphone_contact_provider_t)



linphone_contact_provider_t* linphone_contact_provider_create();
