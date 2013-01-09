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
#include "sal.h"

typedef enum {
	LinphoneUpnpStateIdle,
	LinphoneUpnpStatePending,
	LinphoneUpnpStateAdding,   // Only used by port binding
	LinphoneUpnpStateRemoving, // Only used by port binding
	LinphoneUpnpStateNotAvailable,  // Only used by uPnP context
	LinphoneUpnpStateOk,
	LinphoneUpnpStateKo,
} UpnpState;

typedef struct _UpnpSession UpnpSession;
typedef struct _UpnpContext UpnpContext;

int linphone_core_update_local_media_description_from_upnp(SalMediaDescription *desc, UpnpSession *session);
int linphone_core_update_upnp_from_remote_media_description(LinphoneCall *call, const SalMediaDescription *md);
int linphone_core_update_upnp(LinphoneCore *lc, LinphoneCall *call);

int upnp_call_process(LinphoneCall *call);
UpnpSession* upnp_session_new(LinphoneCall *call);
void upnp_session_destroy(UpnpSession* session);
UpnpState upnp_session_get_state(UpnpSession *session);

UpnpContext *upnp_context_new(LinphoneCore *lc);
void upnp_context_destroy(UpnpContext *ctx);
UpnpState upnp_context_get_state(UpnpContext *ctx);
const char *upnp_context_get_external_ipaddress(UpnpContext *ctx);

#endif //LINPHONE_UPNP_H
