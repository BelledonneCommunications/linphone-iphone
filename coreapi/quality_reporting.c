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

reporting_session_report_t * update_stats(LinphoneCall * call) {
	int count;
	reporting_session_report_t * stats = call->audio_reporting;
	const MSQualityIndicator * qi = media_stream_get_quality_indicator(&call->audiostream->ms);
	const PayloadType * payload;

	if (stats == NULL) {
		ms_warning("No reporting created for this stream");
		return NULL;
	}

	stats->info.local_addr.ssrc = rtp_session_get_send_ssrc(call->audiostream->ms.session);
	stats->info.local_addr.port = rtp_session_get_local_port(call->audiostream->ms.session);
	stats->info.remote_addr.ssrc = rtp_session_get_recv_ssrc(call->audiostream->ms.session);
	// memcpy(stats->info.remote_addr.ip, &call->audiostream->ms.session->rtp.rem_addr, call->audiostream->ms.session->rtp.rem_addrlen);
	stats->info.call_id = call->log->call_id;
	for (count = 0; count < call->resultdesc->n_total_streams; ++count) {
		if (call->resultdesc->streams[count].type == SalAudio) {
			stats->info.local_addr.ip = call->resultdesc->streams[count].rtp_addr;
			break;
		}
	}
	if (count == call->resultdesc->n_total_streams) {
		ms_warning("Could not find the associated stream of type %d", SalAudio);
	}
	
	payload = call->params.audio_codec;
	if (payload != NULL) {
		stats->local_metrics.session_description.payload_type = payload->type;
		stats->local_metrics.session_description.payload_desc = ms_strdup(payload->mime_type);
		stats->local_metrics.session_description.sample_rate = payload->clock_rate;
		stats->local_metrics.session_description.fmtp = ms_strdup(payload->recv_fmtp);
	} else {
		// ...
	}
	
	//stats->local_metrics.session_description.packet_loss_concealment = ms_quality_indicator_get_local_late_rate(qi);
	stats->local_metrics.packet_loss.jitter_buffer_discard_rate = ms_quality_indicator_get_local_loss_rate(qi);
	stats->local_metrics.quality_estimates.rlq = ms_quality_indicator_get_lq_rating(qi);
	if (10 <= stats->local_metrics.quality_estimates.rlq 
		&& stats->local_metrics.quality_estimates.rlq <= 50) {
		stats->local_metrics.quality_estimates.moslq = stats->local_metrics.quality_estimates.rlq / 10.f;
	} else {
		stats->local_metrics.quality_estimates.moslq = -1;
	}
	stats->local_metrics.quality_estimates.rcq = ms_quality_indicator_get_rating(qi);
	if (10 <= stats->local_metrics.quality_estimates.rcq 
		&& stats->local_metrics.quality_estimates.rcq <= 50) {
		stats->local_metrics.quality_estimates.moscq = stats->local_metrics.quality_estimates.rcq / 10.f;
	} else {
		stats->local_metrics.quality_estimates.moscq = -1;
	}

	// NOT FOUND
		// int packet_loss_concealment; // in voip metrics - audio only
		// jitter buffer
		// jitter_buffer_discard_rate

	if (call->dir == LinphoneCallIncoming) {
		stats->info.remote_id = linphone_address_as_string(call->log->from);
		stats->info.local_id = linphone_address_as_string(call->log->to);
		stats->info.orig_id = stats->info.remote_id;
	} else {
		stats->info.remote_id = linphone_address_as_string(call->log->to);
		stats->info.local_id = linphone_address_as_string(call->log->from);
		stats->info.orig_id = stats->info.local_id;
	}
	stats->local_metrics.timestamps.start = call->log->start_date_time;
	stats->local_metrics.timestamps.stop = call->log->start_date_time + linphone_call_get_duration(call);

	return stats;
}

static void append_metrics_to_buffer(char ** buffer, size_t * size, size_t * offset, reporting_content_metrics_t rm) {
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
	reporting_session_report_t *stats = update_stats(call);
	size_t offset = 0;
	size_t size = 2048;
	char * buffer;

	// Somehow the reporting was not created, hence no need to go further
	if (stats == NULL) {
		PRINT("STATS ARE NULL!");
		return;
	}

	buffer = (char *) ms_malloc(size);

	content.type = ms_strdup("application");
	content.subtype = ms_strdup("vq-rtcpxr");

	append_to_buffer(&buffer, &size, &offset, "VQSessionReport: CallTerm\r\n");
	append_to_buffer(&buffer, &size, &offset, "CallID: %s\r\n", stats->info.call_id);
	append_to_buffer(&buffer, &size, &offset, "LocalID: %s\r\n", stats->info.local_id);
	append_to_buffer(&buffer, &size, &offset, "RemoteID: %s\r\n", stats->info.remote_id);
	append_to_buffer(&buffer, &size, &offset, "OrigID: %s\r\n", stats->info.orig_id);

	append_to_buffer(&buffer, &size, &offset, "LocalGroup: %s\r\n", stats->info.local_group); //linphone-CALLID
	append_to_buffer(&buffer, &size, &offset, "RemoteGroup: %s\r\n", stats->info.remote_group); //idem
	append_to_buffer(&buffer, &size, &offset, "LocalAddr: IP=%s PORT=%d SSRC=%d\r\n", stats->info.local_addr.ip, stats->info.local_addr.port, stats->info.local_addr.ssrc);
	append_to_buffer(&buffer, &size, &offset, "LocalMAC: %s\r\n", stats->info.local_mac_addr);
	append_to_buffer(&buffer, &size, &offset, "RemoteAddr: IP=%s PORT=%d SSRC=%d\r\n", stats->info.remote_addr.ip, stats->info.remote_addr.port, stats->info.remote_addr.ssrc);
	append_to_buffer(&buffer, &size, &offset, "RemoteMAC: %s\r\n", stats->info.remote_mac_addr);
	
	append_to_buffer(&buffer, &size, &offset, "LocalMetrics:\r\n");
	append_metrics_to_buffer(&buffer, &size, &offset, stats->local_metrics);
		
	append_to_buffer(&buffer, &size, &offset, "RemoteMetrics:\r\n");
	append_metrics_to_buffer(&buffer, &size, &offset, stats->remote_metrics);
	if (stats->dialog_id != NULL) {
		append_to_buffer(&buffer, &size, &offset, "DialogID: %s\r\n", stats->dialog_id);
	}

	content.data = buffer;

	// for debug purpose only
 	PRINT(content.data);

	content.size = strlen((char*)content.data);

	addr = linphone_address_new("sip:collector@sip.linphone.org");
	linphone_core_publish(call->core, addr, "vq-rtcpxr", expires, &content);
	linphone_address_destroy(addr);
}

