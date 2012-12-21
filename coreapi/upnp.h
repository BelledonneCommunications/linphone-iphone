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

typedef enum {
	UPNP_Idle,
	UPNP_Pending,
	UPNP_Ok,
	UPNP_Ko,
} UpnpState;

typedef struct _UpnpSession  UpnpSession;

typedef struct _UpnpPortBinding {
	ms_mutex_t mutex;
	UpnpState state;
	upnp_igd_ip_protocol protocol;
	int local_port;
	int remote_port;
	int retry;
	int ref;
} UpnpPortBinding;

struct _UpnpSession {
	UpnpPortBinding *audio_rtp;
	UpnpPortBinding *audio_rtcp;
	UpnpPortBinding *video_rtp;
	UpnpPortBinding *video_rtcp;
	UpnpState state;
};

typedef struct _UpnpContext {
	upnp_igd_context *upnp_igd_ctxt;
	UpnpPortBinding *sip_tcp;
	UpnpPortBinding *sip_tls;
	UpnpPortBinding *sip_udp;
	UpnpState state;
	MSList *pending_bindinds;
	ms_mutex_t mutex;
} UpnpContext;


int linphone_core_update_upnp(LinphoneCore *lc, LinphoneCall *call);
int upnp_call_process(LinphoneCall *call);
UpnpSession* upnp_session_new();
void upnp_session_destroy(UpnpSession* session);

int upnp_context_init(LinphoneCore *lc);
void upnp_context_uninit(LinphoneCore *lc);

#endif //LINPHONE_UPNP_H
