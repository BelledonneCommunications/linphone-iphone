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


typedef struct _UpnpPortBinding {
	ms_mutex_t mutex;
	UpnpState state;
	upnp_igd_ip_protocol protocol;
	char local_addr[LINPHONE_IPADDR_SIZE];
	int local_port;
	char external_addr[LINPHONE_IPADDR_SIZE];
	int external_port;
	int retry;
	int ref;
} UpnpPortBinding;

typedef struct _UpnpStream {
	UpnpPortBinding *rtp;
	UpnpPortBinding *rtcp;
	UpnpState state;
} UpnpStream;

typedef struct _UpnpSession {
	UpnpStream *audio;
	UpnpStream *video;
	UpnpState state;
} UpnpSession;

typedef struct _UpnpContext {
	upnp_igd_context *upnp_igd_ctxt;
	UpnpPortBinding *sip_tcp;
	UpnpPortBinding *sip_tls;
	UpnpPortBinding *sip_udp;
	UpnpState state;
	UpnpState old_state;
	MSList *pending_configs;

	ms_mutex_t mutex;
} UpnpContext;

void linphone_core_update_local_media_description_from_upnp(SalMediaDescription *desc, UpnpSession *session);
int linphone_core_update_upnp(LinphoneCore *lc, LinphoneCall *call);
int upnp_call_process(LinphoneCall *call);
UpnpSession* upnp_session_new();
void upnp_session_destroy(LinphoneCall* call);

int upnp_context_init(LinphoneCore *lc);
void upnp_context_uninit(LinphoneCore *lc);

#endif //LINPHONE_UPNP_H
