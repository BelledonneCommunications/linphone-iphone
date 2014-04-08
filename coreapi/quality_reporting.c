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

#define PRINT2(x, f) printf(#x ": " #f "\n", x)
#define PRINT(x) PRINT2(x, "%s")

// since printf family functions are LOCALE dependent, float separator may differ
// depending on the user's locale (LC_NUMERIC env var).
static char * float_to_one_decimal_string(float f) {
	int floor_part = (int)f;
	int one_decimal_part = (int) (10.f * (f - floor_part));

	return ms_strdup_printf(_("%d.%d"), floor_part, one_decimal_part);
}

struct _reporting_metrics_st {
	time_t ts_start;
	time_t ts_stop;

	int sd_pt;
	char * sd_pd;
	int sd_sr;
	int sd_fd;
	int sd_fo;
	int sd_fpp;
	int sd_pps;
	char * sd_fmtp;
	int sd_plc;
	char * sd_ssup;

	int jb_jba;
	int jb_jbr;
	int jb_jbn;
	int jb_jbm;
	int jb_jbx;

	float pl_nlr;
	float pl_jdr;

	int bgl_bld;
	int bgl_bd;
	float bgl_gld;
	int bgl_gd;
	int bgl_gmin;

	int d_rtd;
	int d_esd;
	int d_sowd;
	int d_iaj;
	int d_maj;

	int s_sl;
	int s_nl;
	int s_rerl;

	int qe_rlq;
	int qe_rcq;
	int qe_extri;
	float qe_moslq;
	float qe_moscq;
	char * qe_qoeestalg;
};

static char * add_metrics(char * dest, struct _reporting_metrics_st rm) { 
	char * tmp = dest;

	dest = ms_strdup_printf(_("%sTimestamps:START=%s STOP=%s\r\n"), tmp, 
		linphone_timestamp_to_rfc3339_string(rm.ts_start),
		linphone_timestamp_to_rfc3339_string(rm.ts_stop));
	ms_free(tmp);
	tmp = dest;
	dest = ms_strdup_printf(_("%sSessionDesc:PT=%d PD=%s SR=%d FD=%d FO=%d FPP=%d PPS=%d FMTP=%s PLC=%d SSUP=%s\r\n"), 
		tmp, rm.sd_pt, rm.sd_pd, rm.sd_sr, rm.sd_fd, rm.sd_fo, rm.sd_fpp, rm.sd_pps, rm.sd_fmtp, rm.sd_plc, rm.sd_ssup);
	ms_free(tmp);
	tmp = dest;
	dest = ms_strdup_printf(_("%sJitterBuffer:JBA=%d JBR=%d JBN=%d JBM=%d JBX=%d\r\n"), 
		tmp, rm.jb_jba, rm.jb_jbr, rm.jb_jbn, rm.jb_jbm, rm.jb_jbx);
	ms_free(tmp);
	tmp = dest;
	dest = ms_strdup_printf(_("%sPacketLoss:NLR=%s JDR=%s\r\n"), 
		tmp, float_to_one_decimal_string(rm.pl_nlr), float_to_one_decimal_string(rm.pl_jdr));
	ms_free(tmp);
	tmp = dest;
	dest = ms_strdup_printf(_("%sBurstGapLoss:BLD=%d BD=%d GLD=%s GD=%d GMIN=%d\r\n"), 
		tmp, rm.bgl_bld, rm.bgl_bd, float_to_one_decimal_string(rm.bgl_gld), rm.bgl_gd, rm.bgl_gmin);
	ms_free(tmp);
	tmp = dest;
	dest = ms_strdup_printf(_("%sDelay:RTD=%d ESD=%d SOWD=%d IAJ=%d MAJ=%d\r\n"), 
		tmp, rm.d_rtd, rm.d_esd, rm.d_sowd, rm.d_iaj, rm.d_maj);
	ms_free(tmp);
	tmp = dest;
	dest = ms_strdup_printf(_("%sSignal:SL=%d NL=%d RERL=%d\r\n"), 
		tmp, rm.s_sl, rm.s_nl, rm.s_rerl);
	ms_free(tmp);
	tmp = dest;
	dest = ms_strdup_printf(_("%sQualityEst:RLQ=%d RCQ=%d EXTRI=%d MOSLQ=%s MOSCQ=%s QoEEstAlg=%s\r\n"), 
		tmp, rm.qe_rlq, rm.qe_rcq, rm.qe_extri, float_to_one_decimal_string(rm.qe_moslq), float_to_one_decimal_string(rm.qe_moscq), rm.qe_qoeestalg);
	ms_free(tmp);

	return dest;
}

void linphone_quality_reporting_submit(LinphoneCall* call) {
	// example at http://tools.ietf.org/html/rfc6035#section-4.7.3
	// only if this is a linphone account
	// only if call succeeded
	// to: collector@sip.linphone.com
	// executed AFTER BYE's "OK" response has been received
	// expires value?
	// un envoi à faire par stream ? (ssrc différent pour chaque stream)
	// memory leaks
	// belle_sip_snprintf
	//ex  RERL 404 code différent potentiellement avec info manquante
	// 3611 pour savoir les valeurs pour les champs non disponibles
	LinphoneContent content = {0};
 	LinphoneAddress *addr;
	int expires = 3600;
	const char *local_ip = "TODO";//stream dependentcall->localdesc->addr; //or call->localip ?
	const char *remote_ip = "TODO";
	int local_port = 0; //TODO
	int remote_port = 0;//linphone_address_get_port(linphone_call_get_remote_address(call));
	uint32_t local_ssrc = rtp_session_get_send_ssrc(call->audiostream->ms.session);
	uint32_t remote_ssrc = rtp_session_get_recv_ssrc(call->audiostream->ms.session);
	struct _reporting_metrics_st local_metrics = {0};
	struct _reporting_metrics_st remote_metrics = {0};

	const char *remote_identity;
	const char *local_identity;
	const char *orig_identity;
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
	local_metrics.ts_start = call->log->start_date_time;
	local_metrics.ts_stop = call->log->start_date_time + call->log->duration;
	content.data = add_metrics((char*)content.data, local_metrics);
		
	content.data = ms_strdup_printf(_("%sRemoteMetrics:\r\n"), content.data);
	remote_metrics.bgl_gld = 42.34f;
	content.data = add_metrics((char*)content.data, remote_metrics);
	content.data = ms_strdup_printf(_("%sDialogID: %s\r\n"), content.data, "TO_DO");

	// for debug purpose only
 	PRINT(content.data);

	content.size = strlen((char*)content.data);

	addr = linphone_address_new("sip:collector@sip.linphone.org");
	linphone_core_publish(call->core, addr, "vq-rtcpxr", expires, &content);
	linphone_address_destroy(addr);
}

