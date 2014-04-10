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

/***************************************************************************
 *  				TODO / REMINDER LIST
 ****************************************************************************/
	// example at http://tools.ietf.org/html/rfc6035#section-4.7.3
	// only if this is a linphone account
	// only if call succeeded
	// executed AFTER BYE's "OK" response has been received
	// to: collector@sip.linphone.com or not
	// expires value?
	// one send by stream (different ssrc)
	// ex RERL 404 code différent potentiellement avec info manquante
	// 3611 pour savoir les valeurs pour les champs non disponibles
	// video : que se passe t-il si on arrete / resume la vidéo
 	// memory leaks

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
		// only a part of it
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
	struct {
		time_t start;
		time_t stop;
	} timestamps;

	// session description - optional
	struct {
		int payload_type;
		char * payload_desc; // mime type
		int sample_rate; // clock rate
		int frame_duration; // to check (ptime?) - audio only
		int frame_ocets; // no
		int frames_per_sec; // no
		int packets_per_sec; // no
		char * fmtp; // pt.recv_fmtp
		int packet_loss_concealment; // in voip metrics - audio only
		char * silence_suppression_state; // no
	} session_description;

	// jitter buffet - optional
	struct {
		int adaptive; // constant
		int rate; // constant
		int nominal; // no may vary during the call <- average? worst score? 
		int max; // no may vary during the call <- average?
		int abs_max; // constant
	} jitter_buffer;

	// packet loss - optional
	struct {
		float network_packet_loss_rate; // voip metrics (loss rate) + conversion
		float jitter_buffer_discard_rate; //idem
	} packet_loss;

	// burst gap loss - optional 
	// (no) currently not implemented
	struct {
		int burst_loss_density; 
		int burst_duration;
		float gap_loss_density;
		int gap_Duration;
		int min_gap_threshold;
	} burst_gap_loss;

	// delay - optional
	struct {
		int round_trip_delay; // no - vary
		int end_system_delay; // no - not implemented yet
		int one_way_delay; // no
		int symm_one_way_delay; // no - vary (depends on round_trip_delay) + not implemented (depends on end_system_delay)
		int interarrival_jitter; // no - not implemented yet
		int mean_abs_jitter; // (no)? - to check
	} delay;

	// signal - optional
	struct {
		int level; // no - vary 
		int noise_level; // no - vary
		int residual_echo_return_loss; // no
	} signal;

	// quality estimates - optional
	struct {
		int rlq; // linked to moslq - in [0..120]
		int rcq; //voip metrics R factor - no - vary or avg  in [0..120]
		float moslq; // no - vary or avg - voip metrics - in [0..4.9]
		float moscq; // no - vary or avg - voip metrics - in [0..4.9]
		

		int extri; // no
		int extro; // no 
		char * rlqestalg; // no to all alg
		char * rcqestalg;
		char * moslqestalg;
		char * moscqestalg;
		char * extriestalg;
		char * extroutestalg;
		char * qoestalg;
	} quality_estimates;
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
	stats.info.local_addr.port = rtp_session_get_local_port(call->audiostream->ms.session);
	stats.info.remote_addr.ssrc = rtp_session_get_recv_ssrc(call->audiostream->ms.session);
	// remote address rem_addr
	stats.info.call_id = call->log->call_id;
	// pour l'ip: itérer sur les streams pour trouver le bon type
	// call->resultdesc->streams[0].type
	// call->resultdesc->streams[0].rtp_addr

	// const rtp_stats_t * rtp_stats = rtp_session_get_stats(call->audiostream->ms.session);
	const MSQualityIndicator * qi = media_stream_get_quality_indicator(&call->audiostream->ms);

	stats.local_metrics.session_description.payload_type = call->params.audio_codec->type;
	stats.local_metrics.session_description.payload_desc = ms_strdup(call->params.audio_codec->mime_type);
	stats.local_metrics.session_description.sample_rate = call->params.audio_codec->clock_rate;
	stats.local_metrics.session_description.fmtp = ms_strdup(call->params.audio_codec->recv_fmtp);
	//stats.local_metrics.session_description.packet_loss_concealment = ms_quality_indicator_get_local_late_rate(qi);

	stats.local_metrics.packet_loss.jitter_buffer_discard_rate = ms_quality_indicator_get_local_loss_rate(qi);

	stats.local_metrics.quality_estimates.rlq = ms_quality_indicator_get_lq_rating(qi);
	if (10 <= stats.local_metrics.quality_estimates.rlq 
		&& stats.local_metrics.quality_estimates.rlq <= 50) {
		stats.local_metrics.quality_estimates.moslq = stats.local_metrics.quality_estimates.rlq / 10.f;
	} else {
		stats.local_metrics.quality_estimates.moslq = -1;
	}
	stats.local_metrics.quality_estimates.rcq = ms_quality_indicator_get_rating(qi);
	if (10 <= stats.local_metrics.quality_estimates.rcq 
		&& stats.local_metrics.quality_estimates.rcq <= 50) {
		stats.local_metrics.quality_estimates.moscq = stats.local_metrics.quality_estimates.rcq / 10.f;
	} else {
		stats.local_metrics.quality_estimates.moscq = -1;
	}

	// NOT FOUND
		// int packet_loss_concealment; // in voip metrics - audio only
		// jitter buffer
		// jitter_buffer_discard_rate

	if (call->dir == LinphoneCallIncoming) {
		stats.info.remote_id = linphone_address_as_string(call->log->from);
		stats.info.local_id = linphone_address_as_string(call->log->to);
		stats.info.orig_id = stats.info.remote_id;
	} else {
		stats.info.remote_id = linphone_address_as_string(call->log->to);
		stats.info.local_id = linphone_address_as_string(call->log->from);
		stats.info.orig_id = stats.info.local_id;
	}
	stats.local_metrics.timestamps.start = call->log->start_date_time;
	stats.local_metrics.timestamps.stop = call->log->start_date_time + linphone_call_get_duration(call);

	return stats;
}

static void add_metrics(char ** buffer, size_t * size, size_t * offset, struct _reporting_content_metrics_st rm) {
	char * timpstamps_start_str = linphone_timestamp_to_rfc3339_string(rm.timestamps.start);
	char * timpstamps_stop_str = linphone_timestamp_to_rfc3339_string(rm.timestamps.stop);
	char * network_packet_loss_rate_str = float_to_one_decimal_string(rm.packet_loss.network_packet_loss_rate);
	char * jitter_buffer_discard_rate_str = float_to_one_decimal_string(rm.packet_loss.jitter_buffer_discard_rate);
	char * gap_loss_density_str = float_to_one_decimal_string(rm.burst_gap_loss.gap_loss_density);
	char * moslq_str = float_to_one_decimal_string(rm.quality_estimates.moslq);
	char * moscq_str = float_to_one_decimal_string(rm.quality_estimates.moscq);

	append_to_buffer(buffer, size, offset, "Timestamps:START=%s STOP=%s\r\n", 
		timpstamps_start_str, timpstamps_stop_str);
	append_to_buffer(buffer, size, offset, "SessionDesc:PT=%d PD=%s SR=%d FD=%d FO=%d FPP=%d PPS=%d FMTP=%s PLC=%d SSUP=%s\r\n", 
		rm.session_description.payload_type, rm.session_description.payload_desc, rm.session_description.sample_rate, 
		rm.session_description.frame_duration, rm.session_description.frame_ocets, rm.session_description.frames_per_sec, 
		rm.session_description.packets_per_sec, rm.session_description.fmtp, rm.session_description.packet_loss_concealment, 
		rm.session_description.silence_suppression_state);
	append_to_buffer(buffer, size, offset, "JitterBuffer:JBA=%d JBR=%d JBN=%d JBM=%d JBX=%d\r\n", 
		rm.jitter_buffer.adaptive, rm.jitter_buffer.rate, rm.jitter_buffer.nominal, rm.jitter_buffer.max, rm.jitter_buffer.abs_max);
	append_to_buffer(buffer, size, offset, "PacketLoss:NLR=%s JDR=%s\r\n", 
		network_packet_loss_rate_str, 
		jitter_buffer_discard_rate_str);
	append_to_buffer(buffer, size, offset, "BurstGapLoss:BLD=%d BD=%d GLD=%s GD=%d GMIN=%d\r\n", 
		rm.burst_gap_loss.burst_loss_density, rm.burst_gap_loss.burst_duration, 
		gap_loss_density_str, rm.burst_gap_loss.gap_Duration, 
		rm.burst_gap_loss.min_gap_threshold);
	append_to_buffer(buffer, size, offset, "Delay:RTD=%d ESD=%d OWD=%d SOWD=%d IAJ=%d MAJ=%d\r\n", 
		rm.delay.round_trip_delay, rm.delay.end_system_delay, rm.delay.one_way_delay, rm.delay.symm_one_way_delay, 
		rm.delay.interarrival_jitter, rm.delay.mean_abs_jitter);
	append_to_buffer(buffer, size, offset, "Signal:SL=%d NL=%d RERL=%d\r\n", 
		rm.signal.level, rm.signal.noise_level, rm.signal.residual_echo_return_loss);
	append_to_buffer(buffer, size, offset, "QualityEst:RLQ=%d RLQEstAlg=%s RCQ=%d RCQEstAlgo=%s EXTRI=%d ExtRIEstAlg=%s EXTRO=%d ExtROEstAlg=%s MOSLQ=%s MOSLQEstAlgo=%s MOSCQ=%s MOSCQEstAlgo=%s QoEEstAlg=%s\r\n", 
		rm.quality_estimates.rlq, rm.quality_estimates.rlqestalg, 
		rm.quality_estimates.rcq, rm.quality_estimates.rcqestalg, 
		rm.quality_estimates.extri, rm.quality_estimates.extriestalg, 
		rm.quality_estimates.extro, rm.quality_estimates.extroutestalg,
		moslq_str, rm.quality_estimates.moslqestalg,
		moscq_str, rm.quality_estimates.moscqestalg,
		rm.quality_estimates.qoestalg);

	free(timpstamps_start_str);
	free(timpstamps_stop_str);
	free(network_packet_loss_rate_str);
	free(jitter_buffer_discard_rate_str);
	free(gap_loss_density_str);
	free(moslq_str);
	free(moscq_str);
}

void linphone_quality_reporting_submit(LinphoneCall* call) {
	LinphoneContent content = {0};
 	LinphoneAddress *addr;
	int expires = 3600;
	struct _reporting_session_report_st stats = get_stats(call);
	size_t offset = 0;
	size_t size = 2048;
	char * buffer = (char *) ms_malloc(size);

	content.type = ms_strdup("application");
	content.subtype = ms_strdup("vq-rtcpxr");

	append_to_buffer(&buffer, &size, &offset, "VQSessionReport: CallTerm\r\n");
	append_to_buffer(&buffer, &size, &offset, "CallID: %s\r\n", stats.info.call_id);
	append_to_buffer(&buffer, &size, &offset, "LocalID: %s\r\n", stats.info.local_id);
	append_to_buffer(&buffer, &size, &offset, "RemoteID: %s\r\n", stats.info.remote_id);
	append_to_buffer(&buffer, &size, &offset, "OrigID: %s\r\n", stats.info.orig_id);

	append_to_buffer(&buffer, &size, &offset, "LocalGroup: %s\r\n", stats.info.local_group); //linphone-CALLID
	append_to_buffer(&buffer, &size, &offset, "RemoteGroup: %s\r\n", stats.info.remote_group); //idem
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

