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
 	// place pour appeler la fonction submit
		// only if call succeeded
		// executed AFTER BYE's "OK" response has been received
 		// si c l'autre qui raccroche on envoi pas les données là
 	// memory leaks + char* strdup
 	// si aucune data d'une catégorie est renseigné, ne pas mettre la section dans le paquet
 	// stats
		// valeur pour "expires" ?
			// packet_loss_concealment
			// jitter buffer rate / adaptive
			// jitter_buffer_discard_rate;
			// network_packet_loss_rate
		// vérifier les valeurs par défaut etc.
 		// ip local potientellement vide
 		// remote : ip, port, timestamps, session desc
 		// dialog id ?
 	// à voir :
		// video : que se passe t-il si on arrete / resume la vidéo (new stream)
 		// valeurs instanannées : moyenne ? valeur extreme ?
 		// à qui / comment on envoit ? (collector@sip.linphone.com ?)
		// only if this is a linphone account?

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

static reporting_session_report_t * update_report(LinphoneCall * call, int stats_type) {
	int count;
	reporting_session_report_t * report = call->reports[stats_type];
	MediaStream stream;
	const MSQualityIndicator * qi = NULL;
	const PayloadType * payload;
	RtpSession * session = NULL;


	if (report == NULL) {
		ms_warning("No reporting created for this stream");
		return NULL;
	}

	if (stats_type == LINPHONE_CALL_STATS_AUDIO) {
		stream = call->audiostream->ms;
		payload = call->current_params.audio_codec;
	} else {
		stream = call->videostream->ms;
		payload = call->current_params.video_codec;
	}
	session = stream.sessions.rtp_session;
	qi = media_stream_get_quality_indicator(&stream);

	report->info.local_addr.ssrc = rtp_session_get_send_ssrc(session);
	report->info.local_addr.port = rtp_session_get_local_port(session);
	report->info.remote_addr.ssrc = rtp_session_get_recv_ssrc(session);
	// memcpy(report->info.remote_addr.ip, &session->rtp.rem_addr, session->rtp.rem_addrlen);
	report->info.call_id = call->log->call_id;
	report->info.local_group = ms_strdup_printf(_("linphone-%s"), report->info.call_id);
	report->info.remote_group = ms_strdup(report->info.local_group);
	for (count = 0; count < call->resultdesc->n_total_streams; ++count) {
		if (call->resultdesc->streams[count].type == stats_type) {
			report->info.local_addr.ip = ms_strdup(call->resultdesc->streams[count].rtp_addr);
			break;
		}
	}
	if (count == call->resultdesc->n_total_streams) {
		ms_warning("Could not find the associated stream of type %d", stats_type);
	}
	
	if (payload != NULL) {
		report->local_metrics.session_description.payload_type = payload->type;
		report->local_metrics.session_description.payload_desc = ms_strdup(payload->mime_type);
		report->local_metrics.session_description.sample_rate = payload->clock_rate;
		report->local_metrics.session_description.fmtp = ms_strdup(payload->recv_fmtp);
	} else {
		// ...
	}

	report->local_metrics.quality_estimates.rlq = ms_quality_indicator_get_lq_rating(qi);
	if (10 <= report->local_metrics.quality_estimates.rlq 
		&& report->local_metrics.quality_estimates.rlq <= 50) {
		report->local_metrics.quality_estimates.moslq = report->local_metrics.quality_estimates.rlq / 10.f;
	} else {
		report->local_metrics.quality_estimates.moslq = -1;
	}
	report->local_metrics.quality_estimates.rcq = ms_quality_indicator_get_rating(qi);
	if (10 <= report->local_metrics.quality_estimates.rcq 
		&& report->local_metrics.quality_estimates.rcq <= 50) {
		report->local_metrics.quality_estimates.moscq = report->local_metrics.quality_estimates.rcq / 10.f;
	} else {
		report->local_metrics.quality_estimates.moscq = -1;
	}

	if (call->dir == LinphoneCallIncoming) {
		report->info.remote_id = linphone_address_as_string(call->log->from);
		report->info.local_id = linphone_address_as_string(call->log->to);
		report->info.orig_id = report->info.remote_id;
	} else {
		report->info.remote_id = linphone_address_as_string(call->log->to);
		report->info.local_id = linphone_address_as_string(call->log->from);
		report->info.orig_id = report->info.local_id;
	}
	report->local_metrics.timestamps.start = call->log->start_date_time;
	report->local_metrics.timestamps.stop = call->log->start_date_time + linphone_call_get_duration(call);



	report->remote_metrics.quality_estimates.rlq = ms_quality_indicator_get_lq_rating(qi);
	if (10 <= report->remote_metrics.quality_estimates.rlq 
		&& report->remote_metrics.quality_estimates.rlq <= 50) {
		report->remote_metrics.quality_estimates.moslq = report->remote_metrics.quality_estimates.rlq / 10.f;
	} else {
		report->remote_metrics.quality_estimates.moslq = -1;
	}
	report->remote_metrics.quality_estimates.rcq = ms_quality_indicator_get_rating(qi);
	if (10 <= report->remote_metrics.quality_estimates.rcq 
		&& report->remote_metrics.quality_estimates.rcq <= 50) {
		report->remote_metrics.quality_estimates.moscq = report->remote_metrics.quality_estimates.rcq / 10.f;
	} else {
		report->remote_metrics.quality_estimates.moscq = -1;
	}

	return report;
}

#define APPEND_STR_TO_BUFFER(buffer, size, offset, fmt, arg) if (arg != NULL) append_to_buffer(buffer, size, offset, fmt, arg);

static void append_metrics_to_buffer(char ** buffer, size_t * size, size_t * offset, reporting_content_metrics_t rm) {
	char * timpstamps_start_str = linphone_timestamp_to_rfc3339_string(rm.timestamps.start);
	char * timpstamps_stop_str = linphone_timestamp_to_rfc3339_string(rm.timestamps.stop);
	char * network_packet_loss_rate_str = float_to_one_decimal_string(rm.packet_loss.network_packet_loss_rate);
	char * jitter_buffer_discard_rate_str = float_to_one_decimal_string(rm.packet_loss.jitter_buffer_discard_rate);
	// char * gap_loss_density_str = float_to_one_decimal_string(rm.burst_gap_loss.gap_loss_density);
	char * moslq_str = float_to_one_decimal_string(rm.quality_estimates.moslq);
	char * moscq_str = float_to_one_decimal_string(rm.quality_estimates.moscq);

	append_to_buffer(buffer, size, offset, "Timestamps:START=%s STOP=%s", 
		timpstamps_start_str, timpstamps_stop_str);

	append_to_buffer(buffer, size, offset, "\r\nSessionDesc:");
		append_to_buffer(buffer, size, offset, " PT=%d", rm.session_description.payload_type);
		APPEND_STR_TO_BUFFER(buffer, size, offset, " PD=%s", rm.session_description.payload_desc);
		append_to_buffer(buffer, size, offset, " SR=%d", rm.session_description.sample_rate);
		append_to_buffer(buffer, size, offset, " FD=%d", rm.session_description.frame_duration);
		// append_to_buffer(buffer, size, offset, " FO=%d", rm.session_description.frame_ocets);
		// append_to_buffer(buffer, size, offset, " FPP=%d", rm.session_description.frames_per_sec);
		// append_to_buffer(buffer, size, offset, " PPS=%d", rm.session_description.packets_per_sec);
		APPEND_STR_TO_BUFFER(buffer, size, offset, " FMTP=\"%s\"", rm.session_description.fmtp);
		append_to_buffer(buffer, size, offset, " PLC=%d", rm.session_description.packet_loss_concealment);
		// APPEND_STR_TO_BUFFER(buffer, size, offset, " SSUP=%s", rm.session_description.silence_suppression_state);

	append_to_buffer(buffer, size, offset, "\r\nJitterBuffer:");
		append_to_buffer(buffer, size, offset, " JBA=%d", rm.jitter_buffer.adaptive);
		append_to_buffer(buffer, size, offset, " JBR=%d", rm.jitter_buffer.rate);
		append_to_buffer(buffer, size, offset, " JBN=%d", rm.jitter_buffer.nominal);
		append_to_buffer(buffer, size, offset, " JBM=%d", rm.jitter_buffer.max);
		append_to_buffer(buffer, size, offset, " JBX=%d",  rm.jitter_buffer.abs_max); 

	append_to_buffer(buffer, size, offset, "\r\nPacketLoss:");
		APPEND_STR_TO_BUFFER(buffer, size, offset, " NLR=%s", network_packet_loss_rate_str);
		APPEND_STR_TO_BUFFER(buffer, size, offset, " JDR=%s", jitter_buffer_discard_rate_str);

	// append_to_buffer(buffer, size, offset, "\r\nBurstGapLoss:");
	// 	append_to_buffer(buffer, size, offset, " BLD=%d", rm.burst_gap_loss.burst_loss_density);
	// 	append_to_buffer(buffer, size, offset, " BD=%d", rm.burst_gap_loss.burst_duration);
	// 	APPEND_STR_TO_BUFFER(buffer, size, offset, " GLD=%s", gap_loss_density_str);
	// 	append_to_buffer(buffer, size, offset, " GD=%d", rm.burst_gap_loss.gap_Duration);
	// 	append_to_buffer(buffer, size, offset, " GMIN=%d", rm.burst_gap_loss.min_gap_threshold); 

	append_to_buffer(buffer, size, offset, "\r\nDelay:");
		append_to_buffer(buffer, size, offset, " RTD=%d", rm.delay.round_trip_delay);
		append_to_buffer(buffer, size, offset, " ESD=%d", rm.delay.end_system_delay);
		// append_to_buffer(buffer, size, offset, " OWD=%d", rm.delay.one_way_delay);
		append_to_buffer(buffer, size, offset, " SOWD=%d", rm.delay.symm_one_way_delay);
		append_to_buffer(buffer, size, offset, " IAJ=%d", rm.delay.interarrival_jitter);
		append_to_buffer(buffer, size, offset, " MAJ=%d", rm.delay.mean_abs_jitter);

	append_to_buffer(buffer, size, offset, "\r\nSignal:");
		append_to_buffer(buffer, size, offset, " SL=%d", rm.signal.level);
		append_to_buffer(buffer, size, offset, " NL=%d", rm.signal.noise_level);
		// append_to_buffer(buffer, size, offset, " RERL=%d", rm.signal.residual_echo_return_loss);

	append_to_buffer(buffer, size, offset, "\r\nQualityEst:");
		append_to_buffer(buffer, size, offset, " RLQ=%d", rm.quality_estimates.rlq);
		// APPEND_STR_TO_BUFFER(buffer, size, offset, " RLQEstAlg=%s", rm.quality_estimates.rlqestalg);
		append_to_buffer(buffer, size, offset, " RCQ=%d", rm.quality_estimates.rcq);
		// APPEND_STR_TO_BUFFER(buffer, size, offset, " RCQEstAlgo=%s", rm.quality_estimates.rcqestalg);
		// append_to_buffer(buffer, size, offset, " EXTRI=%d", rm.quality_estimates.extri);
		// APPEND_STR_TO_BUFFER(buffer, size, offset, " ExtRIEstAlg=%s", rm.quality_estimates.extriestalg);
		// append_to_buffer(buffer, size, offset, " EXTRO=%d", rm.quality_estimates.extro);
		// APPEND_STR_TO_BUFFER(buffer, size, offset, " ExtROEstAlg=%s", rm.quality_estimates.extroutestalg);
		APPEND_STR_TO_BUFFER(buffer, size, offset, " MOSLQ=%s", moslq_str);
		// APPEND_STR_TO_BUFFER(buffer, size, offset, " MOSLQEstAlgo=%s", rm.quality_estimates.moslqestalg);
		APPEND_STR_TO_BUFFER(buffer, size, offset, " MOSCQ=%s", moscq_str);
		// APPEND_STR_TO_BUFFER(buffer, size, offset, " MOSCQEstAlgo=%s", rm.quality_estimates.moscqestalg);
		// APPEND_STR_TO_BUFFER(buffer, size, offset, " QoEEstAlg=%s", rm.quality_estimates.qoestalg);
	append_to_buffer(buffer, size, offset, "\r\n");

	free(timpstamps_start_str);
	free(timpstamps_stop_str);
	free(network_packet_loss_rate_str);
	free(jitter_buffer_discard_rate_str);
	// free(gap_loss_density_str);
	free(moslq_str);
	free(moscq_str);
}

void linphone_reporting_publish(LinphoneCall* call, int stats_type) {
	LinphoneContent content = {0};
 	LinphoneAddress *addr;
	int expires = 3600;
	reporting_session_report_t *report = update_report(call, stats_type);
	size_t offset = 0;
	size_t size = 2048;
	char * buffer;

	// Somehow the reporting was not created, hence no need to go further
	if (report == NULL) {
		PRINT("STATS ARE NULL!");
		return;
	}

	buffer = (char *) ms_malloc(size);

	content.type = ms_strdup("application");
	content.subtype = ms_strdup("vq-rtcpxr");

	append_to_buffer(&buffer, &size, &offset, "VQSessionReport: CallTerm\r\n");
	append_to_buffer(&buffer, &size, &offset, "CallID: %s\r\n", report->info.call_id);
	append_to_buffer(&buffer, &size, &offset, "LocalID: %s\r\n", report->info.local_id);
	append_to_buffer(&buffer, &size, &offset, "RemoteID: %s\r\n", report->info.remote_id);
	append_to_buffer(&buffer, &size, &offset, "OrigID: %s\r\n", report->info.orig_id);

	APPEND_STR_TO_BUFFER(&buffer, &size, &offset, "LocalGroup: %s\r\n", report->info.local_group);
	APPEND_STR_TO_BUFFER(&buffer, &size, &offset, "RemoteGroup: %s\r\n", report->info.remote_group);
	append_to_buffer(&buffer, &size, &offset, "LocalAddr: IP=%s PORT=%d SSRC=%d\r\n", report->info.local_addr.ip, report->info.local_addr.port, report->info.local_addr.ssrc);
	APPEND_STR_TO_BUFFER(&buffer, &size, &offset, "LocalMAC: %s\r\n", report->info.local_mac_addr);
	append_to_buffer(&buffer, &size, &offset, "RemoteAddr: IP=%s PORT=%d SSRC=%d\r\n", report->info.remote_addr.ip, report->info.remote_addr.port, report->info.remote_addr.ssrc);
	APPEND_STR_TO_BUFFER(&buffer, &size, &offset, "RemoteMAC: %s\r\n", report->info.remote_mac_addr);
	
	append_to_buffer(&buffer, &size, &offset, "LocalMetrics:\r\n");
	append_metrics_to_buffer(&buffer, &size, &offset, report->local_metrics);
		
	append_to_buffer(&buffer, &size, &offset, "RemoteMetrics:\r\n");
	append_metrics_to_buffer(&buffer, &size, &offset, report->remote_metrics);
	APPEND_STR_TO_BUFFER(&buffer, &size, &offset, "DialogID: %s\r\n", report->dialog_id);

	content.data = buffer;

	// for debug purpose only
 	PRINT(content.data);

	content.size = strlen((char*)content.data);

	addr = linphone_address_new("sip:collector@sip.linphone.org");
	linphone_core_publish(call->core, addr, "vq-rtcpxr", expires, &content);
	linphone_address_destroy(addr);
}


void linphone_reporting_call_stats_updated(LinphoneCall *call, int stats_type) {
	reporting_session_report_t * report = call->reports[stats_type];
	reporting_content_metrics_t * metrics = NULL;
	reporting_addr_t * addr = NULL;

	LinphoneCallStats stats = call->stats[stats_type];
    mblk_t *block = NULL;

    if (stats.updated == LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE) {
		metrics = &report->remote_metrics;
		addr = &report->info.remote_addr;
        if (rtcp_is_XR(stats.received_rtcp) == TRUE) {
            block = stats.received_rtcp;
        }
    } else if (stats.updated == LINPHONE_CALL_STATS_SENT_RTCP_UPDATE) {
		metrics = &report->local_metrics;
		addr = &report->info.local_addr;
        if (rtcp_is_XR(stats.sent_rtcp) == TRUE) {
            block = stats.sent_rtcp;
        }
    }
    if (block != NULL) {
        switch (rtcp_XR_get_block_type(block)) {
            case RTCP_XR_STAT_SUMMARY:
                // rtcp_XR_stat_summary_get_flags(block);
                // rtcp_XR_stat_summary_get_ssrc(block);
                // rtcp_XR_stat_summary_get_begin_seq(block);
                // rtcp_XR_stat_summary_get_end_seq(block);
                // rtcp_XR_stat_summary_get_lost_packets(block);
                // rtcp_XR_stat_summary_get_dup_packets(block);
                // rtcp_XR_stat_summary_get_min_jitter(block);
                // rtcp_XR_stat_summary_get_max_jitter(block);
                // rtcp_XR_stat_summary_get_mean_jitter(block);
                // rtcp_XR_stat_summary_get_dev_jitter(block);
                // rtcp_XR_stat_summary_get_min_ttl_or_hl(block);
                // rtcp_XR_stat_summary_get_max_ttl_or_hl(block);
                // rtcp_XR_stat_summary_get_mean_ttl_or_hl(block);
                // rtcp_XR_stat_summary_get_dev_ttl_or_hl(block);
                break;
            case RTCP_XR_VOIP_METRICS:
                // rtcp_XR_voip_metrics_get_ssrc(block);
                // rtcp_XR_voip_metrics_get_loss_rate(block);
                // rtcp_XR_voip_metrics_get_discard_rate(block);
                // rtcp_XR_voip_metrics_get_burst_density(block);
                // rtcp_XR_voip_metrics_get_gap_density(block);
                // rtcp_XR_voip_metrics_get_burst_duration(block);
                // rtcp_XR_voip_metrics_get_gap_duration(block);
                // rtcp_XR_voip_metrics_get_round_trip_delay(block);
                // rtcp_XR_voip_metrics_get_end_system_delay(block);
                // rtcp_XR_voip_metrics_get_signal_level(block);
                // rtcp_XR_voip_metrics_get_noise_level(block);
                // rtcp_XR_voip_metrics_get_rerl(block);
                // rtcp_XR_voip_metrics_get_gmin(block);
                metrics->quality_estimates.rlq = rtcp_XR_voip_metrics_get_r_factor(block);
                metrics->quality_estimates.moslq = rtcp_XR_voip_metrics_get_mos_lq(block);
                metrics->quality_estimates.moscq = rtcp_XR_voip_metrics_get_mos_cq(block);
                // rtcp_XR_voip_metrics_get_rx_config(block);
                metrics->jitter_buffer.nominal = rtcp_XR_voip_metrics_get_jb_nominal(block);
                metrics->jitter_buffer.max = rtcp_XR_voip_metrics_get_jb_maximum(block);
                metrics->jitter_buffer.abs_max = rtcp_XR_voip_metrics_get_jb_abs_max(block);
                break;
            default:
                break;
        }
    }
}
