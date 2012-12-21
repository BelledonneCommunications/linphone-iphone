/*
linphone
Copyright (C) 2012  Belledonne Communications SARL

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

#ifndef LINPHONE_UPNP_H
#define LINPHONE_UPNP_H

#include "mediastreamer2/upnp_igd.h"
#include "linphonecore.h"

typedef struct _UpnpSession {

} UpnpSession;

typedef struct _UpnpContext {
	upnp_igd_context *upnp_igd_ctxt;
} UpnpContext;

UpnpSession* upnp_session_new();
int upnp_context_init(LinphoneCore *lc);
void upnp_context_uninit(LinphoneCore *lc);

#endif //LINPHONE_UPNP_H
