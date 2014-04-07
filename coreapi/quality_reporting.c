/*
linphone
Copyright (C) 2014 - Belledonne Communications, Grenoble, France

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

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "linphonecore.h"
#include "private.h"
#include "sal/sal.h"

#define PRINT2(x, f) ms_message(#x ": " #f, x)
#define PRINT(x) PRINT2(x, "%s")

void linphone_quality_reporting_submit(LinphoneCall* call) {
	// example at http://tools.ietf.org/html/rfc6035#section-4.7.3
	// only if this is a linphone account
	// only if call succeeded
	// to: collector@sip.linphone.com
	// executed AFTER BYE's "OK" response has been received
	// expires value?
	// un envoi à faire par stream ? (ssrc différent pour chaque stream)
	// memory leaks
	
	LinphoneContent content = {0};
 	LinphoneAddress *addr;
	int expires = 3600;
	const char *remote_identity;
	const char *local_identity;
	const char *orig_identity;
	const char *local_ip = call->localdesc->addr; //or call->localip ?
	const char *remote_ip = "TODO";
	uint32_t local_ssrc = rtp_session_get_send_ssrc(call->audiostream->ms.session);
	uint32_t remote_ssrc = rtp_session_get_recv_ssrc(call->audiostream->ms.session);

	int local_port = 0; //TODO
	int remote_port = 0;//linphone_address_get_port(linphone_call_get_remote_address(call));

	if (call->dir == LinphoneCallIncoming) {
		remote_identity = linphone_address_as_string(call->log->from);
		local_identity = linphone_address_as_string(call->log->to);
		orig_identity = remote_identity;
	} else {
		remote_identity = linphone_address_as_string(call->log->to);
		local_identity = linphone_address_as_string(call->log->from);
		orig_identity = local_identity;
	}

	ms_message("Submitting PUBLISH packet for call between %s and %s", local_identity, remote_identity);

	PRINT(call->dest_proxy->contact_params);
	PRINT(call->dest_proxy->contact_uri_params);
	PRINT(call->dest_proxy->dial_prefix);
	PRINT(call->dest_proxy->realm);
	PRINT(call->dest_proxy->type);
	PRINT(call->dest_proxy->reg_identity);
	PRINT(call->dest_proxy->reg_proxy);
	PRINT(call->dest_proxy->reg_route);

	PRINT(sal_op_get_route(call->op));
	PRINT(sal_op_get_from(call->op));
	PRINT(sal_op_get_to(call->op));
	PRINT(sal_op_get_proxy(call->op));
	PRINT(sal_op_get_remote_contact(call->op));
	PRINT(sal_op_get_network_origin(call->op));

	PRINT(call->log->refkey);
	PRINT(call->localdesc->addr);

	PRINT2(call->log->start_date_time, "%ld");
	PRINT2(call->log->start_date_time + call->log->duration, "%ld");
	PRINT(linphone_timestamp_to_rfc3339_string(call->log->start_date_time));


	content.type = ms_strdup("application");
	content.subtype = ms_strdup("vq-rtcpxr");
	content.data = ms_strdup_printf(_("VQSessionReport: CallTerm\r\n"));
	content.data = ms_strdup_printf(_("%sCallID: %s\r\n"), content.data, call->log->call_id);
	content.data = ms_strdup_printf(_("%sLocalID: %s\r\n"), content.data, local_identity);
	content.data = ms_strdup_printf(_("%sRemoteID: %s\r\n"), content.data, remote_identity);
	content.data = ms_strdup_printf(_("%sOrigID: %s\r\n"), content.data, orig_identity);

	// content.data = ms_strdup_printf(_("%sLocalGroup: %s\r\n"), content.data, "TO_DO");
	// content.data = ms_strdup_printf(_("%sRemoteGroup: %s\r\n"), content.data, "TO_DO");
	content.data = ms_strdup_printf(_("%sLocalAddr: IP=%s PORT=%d SSRC=%d\r\n"), content.data, local_ip, local_port, local_ssrc);
	// content.data = ms_strdup_printf(_("%sLocalMAC: %s\r\n"), content.data, "TO_DO");
	content.data = ms_strdup_printf(_("%sRemoteAddr: IP=%s PORT=%d SSRC=%d\r\n"), content.data, remote_ip, remote_port, remote_ssrc);
	// content.data = ms_strdup_printf(_("%sRemoteMAC: %s\r\n"), content.data, "TO_DO");
	
	content.data = ms_strdup_printf(_("%sLocalMetrics:\r\n"), content.data);
	content.data = ms_strdup_printf(_("%sTimestamps: START=%s STOP=%s\r\n"), content.data, 
		linphone_timestamp_to_rfc3339_string(call->log->start_date_time), 
		linphone_timestamp_to_rfc3339_string(call->log->start_date_time + call->log->duration));
	content.data = ms_strdup_printf(_("%sRemoteMetrics:\r\n"), content.data, "TO_DO");

	content.data = ms_strdup_printf(_("%sRemoteMetrics:\r\n"), content.data, "TO_DO");
	content.data = ms_strdup_printf(_("%sTimestamps: %s\r\n"), content.data, "TO_DO");
	content.data = ms_strdup_printf(_("%sDialogID: %s\r\n"), content.data, "TO_DO");

	content.size = strlen((char*)content.data);
 
	addr = linphone_address_new("sip:collector@sip.linphone.org");

	ms_message("packet content: %s", (char*)content.data);
	linphone_core_publish(call->core, addr, "vq-rtcpxr", expires, &content);

	linphone_address_destroy(addr);
}

