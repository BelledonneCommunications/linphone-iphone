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

#include <math.h>

#define PRINT2(x, f) printf(#x ": " #f "\n", x)
#define PRINT(x) PRINT2(x, "%s")

// since printf family functions are LOCALE dependent, float separator may differ
// depending on the user's locale (LC_NUMERIC env var).
static char * float_to_one_decimal_string(float f) {
	float rounded_f = floorf(f * 10 + .5f) / 10;

	int floor_part = (int) rounded_f;
	int one_decimal_part = floorf (10 * (rounded_f - floor_part) + .5f);

	return ms_strdup_printf(_("%d.%d"), floor_part, one_decimal_part);
}

static void append_to_buffer_valist(char **buff, size_t *buff_size, size_t *offset, const char *fmt, va_list args) {
	belle_sip_error_code ret;
	size_t prevoffset = *offset;

#ifndef WIN32
        va_list cap;/*copy of our argument list: a va_list cannot be re-used (SIGSEGV on linux 64 bits)*/
        va_copy(cap,args);
		ret = belle_sip_snprintf_valist(*buff, *buff_size, offset, fmt, cap);
        va_end(cap);
#else
		ret = belle_sip_snprintf_valist(*buff, *buff_size, offset, fmt, args);
#endif

	// if we are out of memory, we add some size to buffer
	if (ret == BELLE_SIP_BUFFER_OVERFLOW) {
		ms_warning("Buffer was too small to contain the whole report - doubling its size from %lu to %lu", *buff_size, 2 * *buff_size);
		*buff_size += 2048;
		*buff = (char *) ms_realloc(*buff, *buff_size);

		*offset = prevoffset;
		// recall myself since we did not write all things into the buffer but
		// only part of it
		append_to_buffer_valist(buff, buff_size, offset, fmt, args);
	}
}

static void append_to_buffer(char **buff, size_t *buff_size, size_t *offset, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	append_to_buffer_valist(buff, buff_size, offset, fmt, args);
	va_end(args);
}


struct _reporting_addr_st {
	char * ip;
	int port;
	uint32_t ssrc;
};

struct _reporting_content_metrics_st {
	// timestamps - mandatory
	time_t ts_start;
	time_t ts_stop;

	// session description - optional
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

	// jitter buffet - optional
	int jb_jba;
	int jb_jbr;
	int jb_jbn;
	int jb_jbm;
	int jb_jbx;

	// packet loss - optional
	float pl_nlr;
	float pl_jdr;

	// burst gap loss - optional
	int bgl_bld;
	int bgl_bd;
	float bgl_gld;
	int bgl_gd;
	int bgl_gmin;

	// delay - optional
	int d_rtd;
	int d_esd;
	int d_sowd;
	int d_iaj;
	int d_maj;

	// signal - optional
	int s_sl;
	int s_nl;
	int s_rerl;

	// quality estimates - optional
	int qe_rlq;
	int qe_rcq;
	int qe_extri;
	float qe_moslq;
	float qe_moscq;
	char * qe_qoeestalg;
};

struct _reporting_session_report_st {
	struct {
		char * call_id;
		char * local_id;
		char * remote_id;
		char * orig_id;
		struct _reporting_addr_st local_addr;
		struct _reporting_addr_st remote_addr;
		char * local_group;
		char * remote_group;

		char * local_mac_addr; // optional
		char * remote_mac_addr; // optional
	} info;

	struct _reporting_content_metrics_st local_metrics;
	struct _reporting_content_metrics_st remote_metrics; // optional

	char * dialog_id; // optional
};

struct _reporting_session_report_st get_stats(LinphoneCall * call) {
	struct _reporting_session_report_st stats = {{0}};
	stats.info.local_addr.ssrc = rtp_session_get_send_ssrc(call->audiostream->ms.session);
	stats.info.remote_addr.ssrc = rtp_session_get_recv_ssrc(call->audiostream->ms.session);
	stats.info.call_id = call->log->call_id;
	if (call->dir == LinphoneCallIncoming) {
		stats.info.remote_id = linphone_address_as_string(call->log->from);
		stats.info.local_id = linphone_address_as_string(call->log->to);
		stats.info.orig_id = stats.info.remote_id;
	} else {
		stats.info.remote_id = linphone_address_as_string(call->log->to);
		stats.info.local_id = linphone_address_as_string(call->log->from);
		stats.info.orig_id = stats.info.local_id;
	}
	stats.local_metrics.ts_start = call->log->start_date_time;
	stats.local_metrics.ts_stop = call->log->start_date_time + linphone_call_get_duration(call);

	return stats;
}


static void add_metrics(char ** buffer, size_t * size, size_t * offset, struct _reporting_content_metrics_st rm) { 
	append_to_buffer(buffer, size, offset, "Timestamps:START=%s STOP=%s\r\n", 
		linphone_timestamp_to_rfc3339_string(rm.ts_start),
		linphone_timestamp_to_rfc3339_string(rm.ts_stop));
	append_to_buffer(buffer, size, offset, "SessionDesc:PT=%d PD=%s SR=%d FD=%d FO=%d FPP=%d PPS=%d FMTP=%s PLC=%d SSUP=%s\r\n", 
		rm.sd_pt, rm.sd_pd, rm.sd_sr, rm.sd_fd, rm.sd_fo, rm.sd_fpp, rm.sd_pps, rm.sd_fmtp, rm.sd_plc, rm.sd_ssup);
	append_to_buffer(buffer, size, offset, "JitterBuffer:JBA=%d JBR=%d JBN=%d JBM=%d JBX=%d\r\n", 
		rm.jb_jba, rm.jb_jbr, rm.jb_jbn, rm.jb_jbm, rm.jb_jbx);
	append_to_buffer(buffer, size, offset, "PacketLoss:NLR=%s JDR=%s\r\n", 
		float_to_one_decimal_string(rm.pl_nlr), float_to_one_decimal_string(rm.pl_jdr));
	append_to_buffer(buffer, size, offset, "BurstGapLoss:BLD=%d BD=%d GLD=%s GD=%d GMIN=%d\r\n", 
		rm.bgl_bld, rm.bgl_bd, float_to_one_decimal_string(rm.bgl_gld), rm.bgl_gd, rm.bgl_gmin);
	append_to_buffer(buffer, size, offset, "Delay:RTD=%d ESD=%d SOWD=%d IAJ=%d MAJ=%d\r\n", 
		rm.d_rtd, rm.d_esd, rm.d_sowd, rm.d_iaj, rm.d_maj);
	append_to_buffer(buffer, size, offset, "Signal:SL=%d NL=%d RERL=%d\r\n", 
		rm.s_sl, rm.s_nl, rm.s_rerl);
	append_to_buffer(buffer, size, offset, "QualityEst:RLQ=%d RCQ=%d EXTRI=%d MOSLQ=%s MOSCQ=%s QoEEstAlg=%s\r\n", 
		rm.qe_rlq, rm.qe_rcq, rm.qe_extri, float_to_one_decimal_string(rm.qe_moslq), float_to_one_decimal_string(rm.qe_moscq), rm.qe_qoeestalg);
}

void linphone_quality_reporting_submit(LinphoneCall* call) {
	// example at http://tools.ietf.org/html/rfc6035#section-4.7.3
	// only if this is a linphone account
	// only if call succeeded
	// to: collector@sip.linphone.com
	// executed AFTER BYE's "OK" response has been received
	// expires value?
	// one send by stream (different ssrc)
	// memory leaks strings - append_to_buffer
	// ex RERL 404 code différent potentiellement avec info manquante
	// 3611 pour savoir les valeurs pour les champs non disponibles
	LinphoneContent content = {0};
 	LinphoneAddress *addr;
	int expires = 3600;
	struct _reporting_session_report_st stats = get_stats(call);
	size_t offset = 0;
	size_t size = 2048;
	char * buffer = (char *) malloc(sizeof(char) * size);
	content.type = ms_strdup("application");
	content.subtype = ms_strdup("vq-rtcpxr");

	append_to_buffer(&buffer, &size, &offset, "VQSessionReport: CallTerm\r\n");
	append_to_buffer(&buffer, &size, &offset, "CallID: %s\r\n", stats.info.call_id);
	append_to_buffer(&buffer, &size, &offset, "LocalID: %s\r\n", stats.info.local_id);
	append_to_buffer(&buffer, &size, &offset, "RemoteID: %s\r\n", stats.info.remote_id);
	append_to_buffer(&buffer, &size, &offset, "OrigID: %s\r\n", stats.info.orig_id);

	append_to_buffer(&buffer, &size, &offset, "LocalGroup: %s\r\n", stats.info.local_group);
	append_to_buffer(&buffer, &size, &offset, "RemoteGroup: %s\r\n", stats.info.remote_group);
	append_to_buffer(&buffer, &size, &offset, "LocalAddr: IP=%s PORT=%d SSRC=%d\r\n", stats.info.local_addr.ip, stats.info.local_addr.port, stats.info.local_addr.ssrc);
	append_to_buffer(&buffer, &size, &offset, "LocalMAC: %s\r\n", stats.info.local_mac_addr);
	append_to_buffer(&buffer, &size, &offset, "RemoteAddr: IP=%s PORT=%d SSRC=%d\r\n", stats.info.remote_addr.ip, stats.info.remote_addr.port, stats.info.remote_addr.ssrc);
	append_to_buffer(&buffer, &size, &offset, "RemoteMAC: %s\r\n", stats.info.remote_mac_addr);
	
	append_to_buffer(&buffer, &size, &offset, "LocalMetrics:\r\n");
	add_metrics(&buffer, &size, &offset, stats.local_metrics);
		
	append_to_buffer(&buffer, &size, &offset, "RemoteMetrics:\r\n");
	add_metrics(&buffer, &size, &offset, stats.remote_metrics);
	if (stats.dialog_id != NULL) {
		append_to_buffer(&buffer, &size, &offset, "DialogID: %s\r\n", stats.dialog_id);
	}

	content.data = buffer;

	// for debug purpose only
 	PRINT(content.data);

	content.size = strlen((char*)content.data);

	addr = linphone_address_new("sip:collector@sip.linphone.org");
	linphone_core_publish(call->core, addr, "vq-rtcpxr", expires, &content);
	linphone_address_destroy(addr);
}

