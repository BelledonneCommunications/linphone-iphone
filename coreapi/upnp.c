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

#include "upnp.h"
#include "private.h"

/* Convert uPnP IGD logs to ortp logs */
void linphone_upnp_igd_print(void *cookie, upnp_igd_print_level level, const char *fmt, va_list list) {
	int ortp_level = ORTP_DEBUG;
	switch(level) {
	case UPNP_IGD_MESSAGE:
		ortp_level = ORTP_MESSAGE;
		break;
	case UPNP_IGD_WARNING:
		ortp_level = ORTP_WARNING;
		break;
	case UPNP_IGD_ERROR:
		ortp_level = ORTP_ERROR;
		break;
	default:
		break;
	}
	ortp_logv(ortp_level, fmt, list);
}

void linphone_upnp_igd_callback(void *cookie, upnp_igd_event event, void *arg) {
	LinphoneCore *lc = (LinphoneCore *)cookie;
	UpnpContext *lupnp = &lc->upnp;
	switch(event) {
	case UPNP_IGD_EXTERNAL_IPADDRESS_CHANGED:
	case UPNP_IGD_NAT_ENABLED_CHANGED:
	case UPNP_IGD_CONNECTION_STATUS_CHANGED:
		break;

	default:
		break;
	}
}

int upnp_context_init(LinphoneCore *lc) {
	UpnpContext *lupnp = &lc->upnp;
	lupnp->upnp_igd_ctxt = NULL;
	lupnp->upnp_igd_ctxt = upnp_igd_create(linphone_upnp_igd_callback, linphone_upnp_igd_print, lc);
	if(lupnp->upnp_igd_ctxt == NULL) {
		ms_error("Can't create uPnP IGD context");
		return -1;
	}
	return 0;
}

void upnp_context_uninit(LinphoneCore *lc) {
	UpnpContext *lupnp = &lc->upnp;
	if(lupnp->upnp_igd_ctxt != NULL) {
		upnp_igd_destroy(lupnp->upnp_igd_ctxt);
	}
}

UpnpSession* upnp_session_new() {
	return NULL;
}
