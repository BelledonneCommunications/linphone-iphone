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

#include "private.h"
#include "mediastreamer2/upnp_igd.h"

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
	ortp_logv(level, fmt, list);
}

void linphone_upnp_igd_callback(void *cookie, upnp_igd_event event, void *arg) {
}

int linphone_upnp_init(LinphoneCore *lc) {
	lc->upnp_igd_ctxt = NULL;
	return 0;
}
void linphone_upnp_destroy(LinphoneCore *lc) {

}
