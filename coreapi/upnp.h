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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef LINPHONE_UPNP_H
#define LINPHONE_UPNP_H

#include "mediastreamer2/upnp_igd.h"
#include "linphone/core.h"
#include "sal/sal.h"

typedef struct _UpnpSession UpnpSession;
typedef struct _UpnpContext UpnpContext;

int linphone_core_update_local_media_description_from_upnp(SalMediaDescription *desc, UpnpSession *session);
int linphone_core_update_upnp_from_remote_media_description(LinphoneCall *call, const SalMediaDescription *md);
int linphone_core_update_upnp(LinphoneCore *lc, LinphoneCall *call);

int linphone_upnp_call_process(LinphoneCall *call);
UpnpSession* linphone_upnp_session_new(LinphoneCall *call);
void linphone_upnp_session_destroy(UpnpSession* session);
LinphoneUpnpState linphone_upnp_session_get_state(UpnpSession *session);

UpnpContext *linphone_upnp_context_new(LinphoneCore *lc);
void linphone_upnp_context_destroy(UpnpContext *ctx);
void linphone_upnp_refresh(UpnpContext *ctx);
LinphoneUpnpState linphone_upnp_context_get_state(UpnpContext *ctx);
const char *linphone_upnp_context_get_external_ipaddress(UpnpContext *ctx);
int linphone_upnp_context_get_external_port(UpnpContext *ctx);
bool_t linphone_upnp_context_is_ready_for_register(UpnpContext *ctx);
void linphone_core_update_upnp_state_in_call_stats(LinphoneCall *call);

#endif //LINPHONE_UPNP_H
