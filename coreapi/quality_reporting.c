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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "linphone/core.h"
#include "private.h"
#include "sal/sal.h"
#include "ortp/rtpsession.h"

#include <math.h>

#if TARGET_OS_IPHONE
#include <sys/sysctl.h>
#endif

#define STR_REASSIGN(dest, src) {\
	if (dest != NULL) \
		ms_free(dest); \
	dest = src; \
}

/*since printf family functions are LOCALE dependent, float separator may differ
depending on the user's locale (LC_NUMERIC environment var).*/
static char * float_to_one_decimal_string(float f) {
	float rounded_f = floorf(f * 10 + .5f) / 10;

	int floor_part = (int) rounded_f;
	int one_decimal_part = (int)floorf(10 * (rounded_f - floor_part) + .5f);

	return ms_strdup_printf("%d.%d", floor_part, one_decimal_part);
}

static void append_to_buffer_valist(char **buff, size_t *buff_size, size_t *offset, const char *fmt, va_list args) {
	belle_sip_error_code ret;
	size_t prevoffset = *offset;

	#ifndef _WIN32
		va_list cap;/*copy of our argument list: a va_list cannot be re-used (SIGSEGV on linux 64 bits)*/
		va_copy(cap,args);
		ret = belle_sip_snprintf_valist(*buff, *buff_size, offset, fmt, cap);
		va_end(cap);
	#else
		ret = belle_sip_snprintf_valist(*buff, *buff_size, offset, fmt, args);
	#endif

	/*if we are out of memory, we add some size to buffer*/
	if (ret == BELLE_SIP_BUFFER_OVERFLOW) {
		/*some compilers complain that size_t cannot be formatted as unsigned long, hence forcing cast*/
		ms_debug("QualityReporting: Buffer was too small to contain the whole report - increasing its size from %lu to %lu",
			(unsigned long)*buff_size, (unsigned long)*buff_size + 2048);
		*buff_size += 2048;
		*buff = (char *) ms_realloc(*buff, *buff_size);

		*offset = prevoffset;
		/*recall itself since we did not write all things into the buffer but
		only a part of it*/
		append_to_buffer_valist(buff, buff_size, offset, fmt, args);
	}
}

static void append_to_buffer(char **buff, size_t *buff_size, size_t *offset, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	append_to_buffer_valist(buff, buff_size, offset, fmt, args);
	va_end(args);
}

static void reset_avg_metrics(reporting_session_report_t * report){
	int i;
	reporting_content_metrics_t * metrics[2] = {&report->local_metrics, &report->remote_metrics};

	for (i = 0; i < 2; i++) {
		metrics[i]->rtcp_sr_count = 0;
		metrics[i]->rtcp_xr_count = 0;
		metrics[i]->jitter_buffer.nominal = 0;
		metrics[i]->jitter_buffer.max = 0;

		metrics[i]->quality_estimates.moslq = 0;
		metrics[i]->quality_estimates.moscq = 0;

		metrics[i]->delay.round_trip_delay = 0;
	}
	report->last_report_date = ms_time(NULL);
}

#define APPEND_IF_NOT_NULL_STR(buffer, size, offset, fmt, arg) if (arg != NULL) append_to_buffer(buffer, size, offset, fmt, arg)
#define APPEND_IF_NUM_IN_RANGE(buffer, size, offset, fmt, arg, inf, sup) if (inf <= arg && arg <= sup) append_to_buffer(buffer, size, offset, fmt, arg)
#define APPEND_IF(buffer, size, offset, fmt, arg, cond) if (cond) append_to_buffer(buffer, size, offset, fmt, arg)
#define IF_NUM_IN_RANGE(num, inf, sup, statement) if (inf <= num && num <= sup) statement

#define METRICS_PACKET_LOSS 1 << 0
#define METRICS_QUALITY_ESTIMATES 1 << 1
#define METRICS_SESSION_DESCRIPTION 1 << 2
#define METRICS_JITTER_BUFFER 1 << 3
#define METRICS_DELAY 1 << 4
#define METRICS_SIGNAL 1 << 5

static uint8_t are_metrics_filled(const reporting_content_metrics_t rm) {
	uint8_t ret = 0;

	IF_NUM_IN_RANGE(rm.packet_loss.network_packet_loss_rate, 0, 255, ret|=METRICS_PACKET_LOSS);
	IF_NUM_IN_RANGE(rm.packet_loss.jitter_buffer_discard_rate, 0, 255, ret|=METRICS_PACKET_LOSS);

	if (rm.session_description.payload_type != -1) ret|=METRICS_SESSION_DESCRIPTION;
	if (rm.session_description.payload_desc != NULL) ret|=METRICS_SESSION_DESCRIPTION;
	if (rm.session_description.sample_rate != -1) ret|=METRICS_SESSION_DESCRIPTION;
	if (rm.session_description.fmtp != NULL) ret|=METRICS_SESSION_DESCRIPTION;

	IF_NUM_IN_RANGE(rm.jitter_buffer.adaptive, 0, 3, ret|=METRICS_JITTER_BUFFER);
	IF_NUM_IN_RANGE(rm.jitter_buffer.abs_max, 0, 65535, ret|=METRICS_JITTER_BUFFER);

	IF_NUM_IN_RANGE(rm.delay.end_system_delay, 0, 65535, ret|=METRICS_DELAY);
	IF_NUM_IN_RANGE(rm.delay.interarrival_jitter, 0, 65535, ret|=METRICS_DELAY);
	IF_NUM_IN_RANGE(rm.delay.mean_abs_jitter, 0, 65535, ret|=METRICS_DELAY);

	if (rm.signal.level != 127) ret|=METRICS_SIGNAL;
	if (rm.signal.noise_level != 127) ret|=METRICS_SIGNAL;

	IF_NUM_IN_RANGE(rm.quality_estimates.moslq, 1, 5, ret|=METRICS_QUALITY_ESTIMATES);
	IF_NUM_IN_RANGE(rm.quality_estimates.moscq, 1, 5, ret|=METRICS_QUALITY_ESTIMATES);

	if (rm.rtcp_xr_count>0){
		IF_NUM_IN_RANGE(rm.jitter_buffer.nominal/rm.rtcp_xr_count, 0, 65535, ret|=METRICS_JITTER_BUFFER);
		IF_NUM_IN_RANGE(rm.jitter_buffer.max/rm.rtcp_xr_count, 0, 65535, ret|=METRICS_JITTER_BUFFER);
	}
	if (rm.rtcp_sr_count+rm.rtcp_xr_count>0){
		IF_NUM_IN_RANGE(rm.delay.round_trip_delay/(rm.rtcp_sr_count+rm.rtcp_xr_count), 0, 65535, ret|=METRICS_DELAY);
	}

	return ret;
}

static bool_t quality_reporting_enabled(const LinphoneCall * call) {
	return (call->dest_proxy != NULL && linphone_proxy_config_quality_reporting_enabled(call->dest_proxy));
}

static bool_t media_report_enabled(LinphoneCall * call, int stats_type){
	if (! quality_reporting_enabled(call))
		return FALSE;

	if (stats_type == LINPHONE_CALL_STATS_VIDEO && !linphone_call_params_video_enabled(linphone_call_get_current_params(call)))
		return FALSE;

	if (stats_type == LINPHONE_CALL_STATS_TEXT && !linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(call)))
		return FALSE;

	return (call->log->reporting.reports[stats_type] != NULL);
}

static void append_metrics_to_buffer(char ** buffer, size_t * size, size_t * offset, const reporting_content_metrics_t rm) {
	char * timestamps_start_str = NULL;
	char * timestamps_stop_str = NULL;
	char * network_packet_loss_rate_str = NULL;
	char * jitter_buffer_discard_rate_str = NULL;
	/*char * gap_loss_density_str = NULL;*/
	char * moslq_str = NULL;
	char * moscq_str = NULL;
	uint8_t available_metrics = are_metrics_filled(rm);

	if (rm.timestamps.start > 0)
		timestamps_start_str = linphone_timestamp_to_rfc3339_string(rm.timestamps.start);
	if (rm.timestamps.stop > 0)
		timestamps_stop_str = linphone_timestamp_to_rfc3339_string(rm.timestamps.stop);

	append_to_buffer(buffer, size, offset, "Timestamps:");
		APPEND_IF_NOT_NULL_STR(buffer, size, offset, " START=%s", timestamps_start_str);
		APPEND_IF_NOT_NULL_STR(buffer, size, offset, " STOP=%s", timestamps_stop_str);

	if ((available_metrics & METRICS_SESSION_DESCRIPTION) != 0){
		append_to_buffer(buffer, size, offset, "\r\nSessionDesc:");
			APPEND_IF(buffer, size, offset, " PT=%d", rm.session_description.payload_type, rm.session_description.payload_type != -1);
			APPEND_IF_NOT_NULL_STR(buffer, size, offset, " PD=%s", rm.session_description.payload_desc);
			APPEND_IF(buffer, size, offset, " SR=%d", rm.session_description.sample_rate, rm.session_description.sample_rate != -1);
			APPEND_IF(buffer, size, offset, " FD=%d", rm.session_description.frame_duration, rm.session_description.frame_duration != -1);
			APPEND_IF_NOT_NULL_STR(buffer, size, offset, " FMTP=\"%s\"", rm.session_description.fmtp);
			APPEND_IF(buffer, size, offset, " PLC=%d", rm.session_description.packet_loss_concealment, rm.session_description.packet_loss_concealment != -1);
	}

	if ((available_metrics & METRICS_JITTER_BUFFER) != 0){
		append_to_buffer(buffer, size, offset, "\r\nJitterBuffer:");
			APPEND_IF_NUM_IN_RANGE(buffer, size, offset, " JBA=%d", rm.jitter_buffer.adaptive, 0, 3);
			if (rm.rtcp_xr_count){
				APPEND_IF_NUM_IN_RANGE(buffer, size, offset, " JBN=%d", rm.jitter_buffer.nominal/rm.rtcp_xr_count, 0, 65535);
				APPEND_IF_NUM_IN_RANGE(buffer, size, offset, " JBM=%d", rm.jitter_buffer.max/rm.rtcp_xr_count, 0, 65535);
			}
			APPEND_IF_NUM_IN_RANGE(buffer, size, offset, " JBX=%d",  rm.jitter_buffer.abs_max, 0, 65535);

		append_to_buffer(buffer, size, offset, "\r\nPacketLoss:");
			IF_NUM_IN_RANGE(rm.packet_loss.network_packet_loss_rate, 0, 255, network_packet_loss_rate_str = float_to_one_decimal_string(rm.packet_loss.network_packet_loss_rate / 256));
			IF_NUM_IN_RANGE(rm.packet_loss.jitter_buffer_discard_rate, 0, 255, jitter_buffer_discard_rate_str = float_to_one_decimal_string(rm.packet_loss.jitter_buffer_discard_rate / 256));

			APPEND_IF_NOT_NULL_STR(buffer, size, offset, " NLR=%s", network_packet_loss_rate_str);
			APPEND_IF_NOT_NULL_STR(buffer, size, offset, " JDR=%s", jitter_buffer_discard_rate_str);
	}

		/*append_to_buffer(buffer, size, offset, "\r\nBurstGapLoss:");*/
			/*IF_NUM_IN_RANGE(rm.burst_gap_loss.gap_loss_density, 0, 10, gap_loss_density_str = float_to_one_decimal_string(rm.burst_gap_loss.gap_loss_density));*/
		/*	append_to_buffer(buffer, size, offset, " BLD=%d", rm.burst_gap_loss.burst_loss_density);*/
		/*	append_to_buffer(buffer, size, offset, " BD=%d", rm.burst_gap_loss.burst_duration);*/
		/*	APPEND_IF_NOT_NULL_STR(buffer, size, offset, " GLD=%s", gap_loss_density_str);*/
		/*	append_to_buffer(buffer, size, offset, " GD=%d", rm.burst_gap_loss.gap_duration);*/
		/*	append_to_buffer(buffer, size, offset, " GMIN=%d", rm.burst_gap_loss.min_gap_threshold);*/

	if ((available_metrics & METRICS_DELAY) != 0){
		append_to_buffer(buffer, size, offset, "\r\nDelay:");
			if (rm.rtcp_xr_count+rm.rtcp_sr_count){
				APPEND_IF_NUM_IN_RANGE(buffer, size, offset, " RTD=%d", rm.delay.round_trip_delay/(rm.rtcp_xr_count+rm.rtcp_sr_count), 0, 65535);
			}
			APPEND_IF_NUM_IN_RANGE(buffer, size, offset, " ESD=%d", rm.delay.end_system_delay, 0, 65535);
			APPEND_IF_NUM_IN_RANGE(buffer, size, offset, " IAJ=%d", rm.delay.interarrival_jitter, 0, 65535);
			APPEND_IF_NUM_IN_RANGE(buffer, size, offset, " MAJ=%d", rm.delay.mean_abs_jitter, 0, 65535);
	}

	if ((available_metrics & METRICS_SIGNAL) != 0){
		append_to_buffer(buffer, size, offset, "\r\nSignal:");
			APPEND_IF(buffer, size, offset, " SL=%d", rm.signal.level, rm.signal.level != 127);
			APPEND_IF(buffer, size, offset, " NL=%d", rm.signal.noise_level, rm.signal.noise_level != 127);
	}

	/*if quality estimates metrics are available, rtcp_xr_count should be always not null*/
	if ((available_metrics & METRICS_QUALITY_ESTIMATES) != 0){
		IF_NUM_IN_RANGE(rm.quality_estimates.moslq, 1, 5, moslq_str = float_to_one_decimal_string(rm.quality_estimates.moslq));
		IF_NUM_IN_RANGE(rm.quality_estimates.moscq, 1, 5, moscq_str = float_to_one_decimal_string(rm.quality_estimates.moscq));

		append_to_buffer(buffer, size, offset, "\r\nQualityEst:");
			APPEND_IF_NOT_NULL_STR(buffer, size, offset, " MOSLQ=%s", moslq_str);
			APPEND_IF_NOT_NULL_STR(buffer, size, offset, " MOSCQ=%s", moscq_str);
	}

	if (rm.user_agent!=NULL){
		append_to_buffer(buffer, size, offset, "\r\nLinphoneExt:");
			APPEND_IF_NOT_NULL_STR(buffer, size, offset, " UA=\"%s\"", rm.user_agent);
	}

	append_to_buffer(buffer, size, offset, "\r\n");

	ms_free(timestamps_start_str);
	ms_free(timestamps_stop_str);
	ms_free(network_packet_loss_rate_str);
	ms_free(jitter_buffer_discard_rate_str);
	/*ms_free(gap_loss_density_str);*/
	ms_free(moslq_str);
	ms_free(moscq_str);
}

static int send_report(LinphoneCall* call, reporting_session_report_t * report, const char * report_event) {
	LinphoneContent *content;
	size_t offset = 0;
	size_t size = 2048;
	char * buffer;
	int ret = 0;
	LinphoneEvent *lev;
	LinphoneAddress *request_uri;
	const char* collector_uri;

	/*if we are on a low bandwidth network, do not send reports to not overload it*/
	if (linphone_call_params_low_bandwidth_enabled(linphone_call_get_current_params(call))){
		ms_message("QualityReporting[%p]: Avoid sending reports on low bandwidth network", call);
		ret = 1;
		goto end;
	}

	/*if the call was hung up too early, we might have invalid IPs information
	in that case, we abort the report since it's not useful data*/
	if (report->info.local_addr.ip == NULL || strlen(report->info.local_addr.ip) == 0
		|| report->info.remote_addr.ip == NULL || strlen(report->info.remote_addr.ip) == 0) {
		ms_message("QualityReporting[%p]: Trying to submit a %s too early (call duration: %d sec) but %s IP could "
			"not be retrieved so dropping this report"
			, call
			, report_event
			, linphone_call_get_duration(call)
			, (report->info.local_addr.ip == NULL || strlen(report->info.local_addr.ip) == 0) ? "local" : "remote");
		ret = 2;
		goto end;
	}

	buffer = (char *) belle_sip_malloc(size);
	content = linphone_content_new();
	linphone_content_set_type(content, "application");
	linphone_content_set_subtype(content, "vq-rtcpxr");

	append_to_buffer(&buffer, &size, &offset, "%s\r\n", report_event);
	append_to_buffer(&buffer, &size, &offset, "CallID: %s\r\n", report->info.call_id);
	append_to_buffer(&buffer, &size, &offset, "LocalID: %s\r\n", report->info.local_addr.id);
	append_to_buffer(&buffer, &size, &offset, "RemoteID: %s\r\n", report->info.remote_addr.id);
	append_to_buffer(&buffer, &size, &offset, "OrigID: %s\r\n", report->info.orig_id);

	APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, "LocalGroup: %s\r\n", report->info.local_addr.group);
	APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, "RemoteGroup: %s\r\n", report->info.remote_addr.group);
	append_to_buffer(&buffer, &size, &offset, "LocalAddr: IP=%s PORT=%d SSRC=%u\r\n", report->info.local_addr.ip, report->info.local_addr.port, report->info.local_addr.ssrc);
	APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, "LocalMAC: %s\r\n", report->info.local_addr.mac);
	append_to_buffer(&buffer, &size, &offset, "RemoteAddr: IP=%s PORT=%d SSRC=%u\r\n", report->info.remote_addr.ip, report->info.remote_addr.port, report->info.remote_addr.ssrc);
	APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, "RemoteMAC: %s\r\n", report->info.remote_addr.mac);

	append_to_buffer(&buffer, &size, &offset, "LocalMetrics:\r\n");
	append_metrics_to_buffer(&buffer, &size, &offset, report->local_metrics);

	if (are_metrics_filled(report->remote_metrics)!=0) {
		append_to_buffer(&buffer, &size, &offset, "RemoteMetrics:\r\n");
		append_metrics_to_buffer(&buffer, &size, &offset, report->remote_metrics);
	}
	APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, "DialogID: %s\r\n", report->dialog_id);

	if (report->qos_analyzer.timestamp!=NULL){
		append_to_buffer(&buffer, &size, &offset, "AdaptiveAlg:");
			APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, " NAME=\"%s\"", report->qos_analyzer.name);
			APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, " TS=\"%s\"", report->qos_analyzer.timestamp);
			APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, " IN_LEG=\"%s\"", report->qos_analyzer.input_leg);
			APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, " IN=\"%s\"", report->qos_analyzer.input);
			APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, " OUT_LEG=\"%s\"", report->qos_analyzer.output_leg);
			APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, " OUT=\"%s\"", report->qos_analyzer.output);
		append_to_buffer(&buffer, &size, &offset, "\r\n");
	}

#if TARGET_OS_IPHONE
	{
		size_t namesize;
		char *machine;
		sysctlbyname("hw.machine", NULL, &namesize, NULL, 0);
		machine = malloc(namesize);
		sysctlbyname("hw.machine", machine, &namesize, NULL, 0);
		APPEND_IF_NOT_NULL_STR(&buffer, &size, &offset, "Device: %s\r\n", machine);
	}
#endif

	linphone_content_set_buffer(content, buffer, strlen(buffer));
	ms_free(buffer);

	if (call->log->reporting.on_report_sent != NULL) {
		SalStreamType type = report == call->log->reporting.reports[0] ? LINPHONE_CALL_STATS_AUDIO : report == call->log->reporting.reports[1] ? LINPHONE_CALL_STATS_VIDEO : LINPHONE_CALL_STATS_TEXT;
		call->log->reporting.on_report_sent(call, type, content);
	}


	collector_uri = linphone_proxy_config_get_quality_reporting_collector(call->dest_proxy);
	if (!collector_uri){
		collector_uri = ms_strdup_printf("sip:%s", linphone_proxy_config_get_domain(call->dest_proxy));
	}
	request_uri = linphone_address_new(collector_uri);
	lev = linphone_core_create_one_shot_publish(call->core, request_uri, "vq-rtcpxr");
	/* Special exception for quality report PUBLISH: if the collector_uri has any transport related parameters
	 * (port, transport, maddr), then it is sent directly.
	 * Otherwise it is routed as any LinphoneEvent publish, following proxy config policy.
	 **/
	if (sal_address_has_uri_param((SalAddress*)request_uri, "transport") || 
		sal_address_has_uri_param((SalAddress*)request_uri, "maddr") ||
		linphone_address_get_port(request_uri) != 0) {
		ms_message("Publishing report with custom route %s", collector_uri);
		sal_op_set_route(lev->op, collector_uri);
	}

	if (linphone_event_send_publish(lev, content) != 0){
		lev=NULL;
		ret=4;
	} else {
		reset_avg_metrics(report);
		STR_REASSIGN(report->qos_analyzer.timestamp, NULL);
		STR_REASSIGN(report->qos_analyzer.input_leg, NULL);
		STR_REASSIGN(report->qos_analyzer.input, NULL);
		STR_REASSIGN(report->qos_analyzer.output_leg, NULL);
		STR_REASSIGN(report->qos_analyzer.output, NULL);
	}

	linphone_address_unref(request_uri);
	linphone_content_unref(content);

	end:
	ms_message("QualityReporting[%p]: Send '%s' with status %d",
		call,
		report_event,
		ret
	);

	return ret;
}

static const SalStreamDescription * get_media_stream_for_desc(const SalMediaDescription * smd, SalStreamType sal_stream_type) {
	int count;
	if (smd != NULL) {
		for (count = 0; count < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; ++count) {
			if (sal_stream_description_active(&smd->streams[count]) && smd->streams[count].type == sal_stream_type) {
				return &smd->streams[count];
			}
		}
	}
	return NULL;
}

static void update_ip(LinphoneCall * call, int stats_type) {
	SalStreamType sal_stream_type = stats_type == LINPHONE_CALL_STATS_AUDIO ? SalAudio : stats_type == LINPHONE_CALL_STATS_VIDEO ? SalVideo : SalText;
	const SalStreamDescription * local_desc = get_media_stream_for_desc(call->localdesc, sal_stream_type);
	const SalStreamDescription * remote_desc = get_media_stream_for_desc(sal_call_get_remote_media_description(call->op), sal_stream_type);

	if (local_desc != NULL) {
		/*since this function might be called for video stream AFTER it has been uninitialized, local description might
		be invalid. In any other case, IP/port should be always filled and valid*/
		if (strlen(local_desc->rtp_addr) > 0) {
			call->log->reporting.reports[stats_type]->info.local_addr.port = local_desc->rtp_port;
			STR_REASSIGN(call->log->reporting.reports[stats_type]->info.local_addr.ip, ms_strdup(local_desc->rtp_addr));
		}
	}

	if (remote_desc != NULL) {
		/*port is always stored in stream description struct*/
		call->log->reporting.reports[stats_type]->info.remote_addr.port = remote_desc->rtp_port;

		/*for IP it can be not set if we are using a direct route*/
		if (strlen(remote_desc->rtp_addr) > 0) {
			STR_REASSIGN(call->log->reporting.reports[stats_type]->info.remote_addr.ip, ms_strdup(remote_desc->rtp_addr));
		} else {
			STR_REASSIGN(call->log->reporting.reports[stats_type]->info.remote_addr.ip, ms_strdup(sal_call_get_remote_media_description(call->op)->addr));
		}
	}
}

static void qos_analyzer_on_action_suggested(void *user_data, int datac, const char** datav){
	reporting_session_report_t *report = (reporting_session_report_t*)user_data;
	LinphoneCall *call = report->call;
	char * appendbuf;
	int i;
	int ptime = -1;
	int bitrate[3] = {-1, -1, -1};
	int up_bw[3] = {-1, -1, -1};
	int down_bw[3] = {-1, -1, -1};
	MediaStream *streams[3] = { (MediaStream*) call->audiostream, (MediaStream *) call->videostream, (MediaStream *) call->textstream };
	for (i = 0; i < 3; i++){
		if (streams[i] != NULL){
			if (streams[i]->encoder != NULL){
				if (ms_filter_has_method(streams[i]->encoder,MS_FILTER_GET_BITRATE)){
					ms_filter_call_method(streams[i]->encoder,MS_FILTER_GET_BITRATE,&bitrate[i]);
					bitrate[i] /= 1000;
				}
			}
			up_bw[i] = (int)(media_stream_get_up_bw(streams[i])/1000.f);
			down_bw[i] = (int)(media_stream_get_down_bw(streams[i])/1000.f);
		}
	}
	if (call->audiostream!=NULL){
		if (call->audiostream->ms.encoder!=NULL){
			if(ms_filter_has_method(call->audiostream->ms.encoder,MS_AUDIO_ENCODER_GET_PTIME)){
				ms_filter_call_method(call->audiostream->ms.encoder,MS_AUDIO_ENCODER_GET_PTIME,&ptime);
			}
		}
	}

	appendbuf=ms_strdup_printf("%s%d;", report->qos_analyzer.timestamp?report->qos_analyzer.timestamp:"", ms_time(0));
	STR_REASSIGN(report->qos_analyzer.timestamp,appendbuf);

	STR_REASSIGN(report->qos_analyzer.input_leg, ms_strdup_printf("%s aenc_ptime aenc_br a_dbw a_ubw venc_br v_dbw v_ubw tenc_br t_dbw t_ubw", datav[0]));
	appendbuf=ms_strdup_printf("%s%s %d %d %d %d %d %d %d %d %d %d;", report->qos_analyzer.input?report->qos_analyzer.input:"", datav[1],
		ptime, bitrate[0], down_bw[0], up_bw[0], bitrate[1], down_bw[1], up_bw[1], bitrate[2], down_bw[2], up_bw[2]);
	STR_REASSIGN(report->qos_analyzer.input,appendbuf);
	STR_REASSIGN(report->qos_analyzer.output_leg, ms_strdup(datav[2]));
	appendbuf=ms_strdup_printf("%s%s;", report->qos_analyzer.output?report->qos_analyzer.output:"", datav[3]);
	STR_REASSIGN(report->qos_analyzer.output, appendbuf);
}

void linphone_reporting_update_ip(LinphoneCall * call) {
	update_ip(call, LINPHONE_CALL_STATS_AUDIO);
	update_ip(call, LINPHONE_CALL_STATS_VIDEO);
	update_ip(call, LINPHONE_CALL_STATS_TEXT);
}

void linphone_reporting_update_media_info(LinphoneCall * call, int stats_type) {
	MediaStream * stream = NULL;
	const PayloadType * local_payload = NULL;
	const PayloadType * remote_payload = NULL;
	const LinphoneCallParams * current_params = linphone_call_get_current_params(call);
	reporting_session_report_t * report = call->log->reporting.reports[stats_type];
	char * dialog_id;

	// call->op might be already released if hanging up in state LinphoneCallOutgoingInit
	if (!media_report_enabled(call, stats_type) || call->op == NULL)
		return;

	dialog_id = sal_op_get_dialog_id(call->op);

	STR_REASSIGN(report->info.call_id, ms_strdup(call->log->call_id));

	STR_REASSIGN(report->local_metrics.user_agent, ms_strdup(linphone_core_get_user_agent(call->core)));
	STR_REASSIGN(report->remote_metrics.user_agent, ms_strdup(linphone_call_get_remote_user_agent(call)));

	// RFC states: "LocalGroupID provides the identification for the purposes
	// of aggregation for the local endpoint.".
	STR_REASSIGN(report->info.local_addr.group, ms_strdup_printf("%s-%s-%s"
		, dialog_id ? dialog_id : ""
		, "local"
		, report->local_metrics.user_agent ? report->local_metrics.user_agent : ""
		)
	);
	STR_REASSIGN(report->info.remote_addr.group, ms_strdup_printf("%s-%s-%s"
		, dialog_id ? dialog_id : ""
		, "remote"
		, report->remote_metrics.user_agent ? report->remote_metrics.user_agent : ""
		)
	);


	if (call->dir == LinphoneCallIncoming) {
		STR_REASSIGN(report->info.remote_addr.id, linphone_address_as_string(call->log->from));
		STR_REASSIGN(report->info.local_addr.id, linphone_address_as_string(call->log->to));
		STR_REASSIGN(report->info.orig_id, ms_strdup(report->info.remote_addr.id));
	} else {
		STR_REASSIGN(report->info.remote_addr.id, linphone_address_as_string(call->log->to));
		STR_REASSIGN(report->info.local_addr.id, linphone_address_as_string(call->log->from));
		STR_REASSIGN(report->info.orig_id, ms_strdup(report->info.local_addr.id));
	}


	report->local_metrics.timestamps.start = call->log->start_date_time;
	report->local_metrics.timestamps.stop = call->log->start_date_time + linphone_call_get_duration(call);

	/*we use same timestamps for remote too*/
	report->remote_metrics.timestamps.start = call->log->start_date_time;
	report->remote_metrics.timestamps.stop = call->log->start_date_time + linphone_call_get_duration(call);


	/*yet we use the same payload config for local and remote, since this is the largest use case*/
	if (stats_type == LINPHONE_CALL_STATS_AUDIO && call->audiostream != NULL) {
		stream = &call->audiostream->ms;
		local_payload = linphone_call_params_get_used_audio_codec(current_params);
		remote_payload = local_payload;
	} else if (stats_type == LINPHONE_CALL_STATS_VIDEO && call->videostream != NULL) {
		stream = &call->videostream->ms;
		local_payload = linphone_call_params_get_used_video_codec(current_params);
		remote_payload = local_payload;
	} else if (stats_type == LINPHONE_CALL_STATS_TEXT && call->textstream != NULL) {
		stream = &call->textstream->ms;
		local_payload = linphone_call_params_get_used_text_codec(current_params);
		remote_payload = local_payload;
	}

	if (stream != NULL) {
		RtpSession * session = stream->sessions.rtp_session;

		report->info.local_addr.ssrc = rtp_session_get_send_ssrc(session);
		report->info.remote_addr.ssrc = rtp_session_get_recv_ssrc(session);

		if (stream->qi != NULL){
			report->local_metrics.quality_estimates.moslq = ms_quality_indicator_get_average_lq_rating(stream->qi) >= 0 ?
				MAX(1, ms_quality_indicator_get_average_lq_rating(stream->qi)) : -1;
			report->local_metrics.quality_estimates.moscq = ms_quality_indicator_get_average_rating(stream->qi) >= 0 ?
				MAX(1, ms_quality_indicator_get_average_rating(stream->qi)) : -1;
		}
	}

	STR_REASSIGN(report->dialog_id, ms_strdup_printf("%s;%u", dialog_id ? dialog_id : "", report->info.local_addr.ssrc));

	if (local_payload != NULL) {
		report->local_metrics.session_description.payload_type = local_payload->type;
		if (local_payload->mime_type!=NULL) STR_REASSIGN(report->local_metrics.session_description.payload_desc, ms_strdup(local_payload->mime_type));
		report->local_metrics.session_description.sample_rate = local_payload->clock_rate;
		if (local_payload->recv_fmtp!=NULL) STR_REASSIGN(report->local_metrics.session_description.fmtp, ms_strdup(local_payload->recv_fmtp));
	}

	if (remote_payload != NULL) {
		report->remote_metrics.session_description.payload_type = remote_payload->type;
		STR_REASSIGN(report->remote_metrics.session_description.payload_desc, ms_strdup(remote_payload->mime_type));
		report->remote_metrics.session_description.sample_rate = remote_payload->clock_rate;
		STR_REASSIGN(report->remote_metrics.session_description.fmtp, ms_strdup(remote_payload->recv_fmtp));
	}

	ms_free(dialog_id);
}

/* generate random float in interval ] 0.9 t ; 1.1 t [*/
static float reporting_rand(float t){
	return t * (.2f * (rand() / (RAND_MAX * 1.0f)) + 0.9f);
}

void linphone_reporting_on_rtcp_update(LinphoneCall *call, SalStreamType stats_type) {
	reporting_session_report_t * report = call->log->reporting.reports[stats_type];
	reporting_content_metrics_t * metrics = NULL;
	LinphoneCallStats stats = call->stats[stats_type];
	mblk_t *block = NULL;
	int report_interval;

	if (! media_report_enabled(call,stats_type))
		return;

	report_interval = linphone_proxy_config_get_quality_reporting_interval(call->dest_proxy);

	if (stats.updated == LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE) {
		metrics = &report->remote_metrics;
		block = stats.received_rtcp;
	} else if (stats.updated == LINPHONE_CALL_STATS_SENT_RTCP_UPDATE) {
		metrics = &report->local_metrics;
		block = stats.sent_rtcp;
	}
	do{
		if (rtcp_is_XR(block) && (rtcp_XR_get_block_type(block) == RTCP_XR_VOIP_METRICS)){

			uint8_t config = rtcp_XR_voip_metrics_get_rx_config(block);

			metrics->rtcp_xr_count++;

			// for local mos rating, we'll use the quality indicator directly
			// because rtcp XR might not be enabled
			if (stats.updated == LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE){
				metrics->quality_estimates.moslq = (rtcp_XR_voip_metrics_get_mos_lq(block)==127) ?
					127 : rtcp_XR_voip_metrics_get_mos_lq(block) / 10.f;
				metrics->quality_estimates.moscq = (rtcp_XR_voip_metrics_get_mos_cq(block)==127) ?
					127 : rtcp_XR_voip_metrics_get_mos_cq(block) / 10.f;
			}

			metrics->jitter_buffer.nominal += rtcp_XR_voip_metrics_get_jb_nominal(block);
			metrics->jitter_buffer.max += rtcp_XR_voip_metrics_get_jb_maximum(block);
			metrics->jitter_buffer.abs_max = rtcp_XR_voip_metrics_get_jb_abs_max(block);
			metrics->jitter_buffer.adaptive = (config >> 4) & 0x3;
			metrics->packet_loss.network_packet_loss_rate = rtcp_XR_voip_metrics_get_loss_rate(block);
			metrics->packet_loss.jitter_buffer_discard_rate = rtcp_XR_voip_metrics_get_discard_rate(block);

			metrics->session_description.packet_loss_concealment = (config >> 6) & 0x3;

			metrics->delay.round_trip_delay += rtcp_XR_voip_metrics_get_round_trip_delay(block);
		}else if (rtcp_is_SR(block)){
			MediaStream *ms=(stats_type==0 ? &call->audiostream->ms : &call->videostream->ms);
			float rtt = rtp_session_get_round_trip_propagation(ms->sessions.rtp_session);

			if (rtt > 1e-6){
				metrics->rtcp_sr_count++;
				metrics->delay.round_trip_delay += (int)(1000*rtt);
			}
		}
	}while(rtcp_next_packet(block));

	/* check if we should send an interval report - use a random sending time to
	dispatch reports and avoid sending them too close from each other */
	if ((report_interval > 0) && ((float)(ms_time(NULL) - report->last_report_date) > reporting_rand((float)report_interval))) {
		linphone_reporting_update_media_info(call, stats_type);
		send_report(call, report, "VQIntervalReport");
	}
}

static int publish_report(LinphoneCall *call, const char *event_type){
	int ret = 0;
	int i;
	for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++){
		int stream_index = i == call->main_audio_stream_index ? LINPHONE_CALL_STATS_AUDIO : call->main_video_stream_index ? LINPHONE_CALL_STATS_VIDEO : LINPHONE_CALL_STATS_TEXT;
		if (media_report_enabled(call, stream_index)) {
			int sndret;
			linphone_reporting_update_media_info(call, stream_index);
			sndret=send_report(call, call->log->reporting.reports[stream_index], event_type);
			if (sndret>0){
				ret += 10+(i+1)*sndret;
			}
		} else{
			ret += i+1;
		}
	}
	return ret;
}

int linphone_reporting_publish_session_report(LinphoneCall* call, bool_t call_term) {
	char * session_type = call_term?"VQSessionReport: CallTerm":"VQSessionReport";
	return publish_report(call, session_type);
}

int linphone_reporting_publish_interval_report(LinphoneCall* call) {
	return publish_report(call, "VQIntervalReport");
}

static bool_t set_on_action_suggested_cb(MediaStream *stream,void (*on_action_suggested)(void*,int,const char**),void* u) {
	if (stream&&stream->rc){
		MSQosAnalyzer *analyzer=ms_bitrate_controller_get_qos_analyzer(stream->rc);
		if (analyzer){
			ms_qos_analyzer_set_on_action_suggested(analyzer,
													on_action_suggested,
													u);
			return TRUE;
		}
	}
	return FALSE;
}

void linphone_reporting_call_state_updated(LinphoneCall *call){
	LinphoneCallState state=linphone_call_get_state(call);
	if (state == LinphoneCallReleased||!quality_reporting_enabled(call)){
		return;
	}
	switch (state){
		case LinphoneCallStreamsRunning:{
			int i = 0;
			MediaStream *streams[3] = { (MediaStream*) call->audiostream, (MediaStream *) call->videostream, (MediaStream *) call->textstream };
			for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
				int stream_index = i == call->main_audio_stream_index ? LINPHONE_CALL_STATS_AUDIO : call->main_video_stream_index ? LINPHONE_CALL_STATS_VIDEO : LINPHONE_CALL_STATS_TEXT;
				bool_t enabled = media_report_enabled(call, stream_index);
				if (enabled && set_on_action_suggested_cb(streams[stream_index], qos_analyzer_on_action_suggested, call->log->reporting.reports[stream_index])) {
					call->log->reporting.reports[stream_index]->call=call;
					STR_REASSIGN(call->log->reporting.reports[stream_index]->qos_analyzer.name, ms_strdup(ms_qos_analyzer_get_name(ms_bitrate_controller_get_qos_analyzer(streams[stream_index]->rc))));
				}
			}
			linphone_reporting_update_ip(call);
			if (!media_report_enabled(call, LINPHONE_CALL_STATS_VIDEO) && call->log->reporting.was_video_running){
				send_report(call, call->log->reporting.reports[LINPHONE_CALL_STATS_VIDEO], "VQSessionReport");
			}
			call->log->reporting.was_video_running=media_report_enabled(call, LINPHONE_CALL_STATS_VIDEO);
			break;
		}
		case LinphoneCallEnd:{
			set_on_action_suggested_cb(&call->audiostream->ms, NULL, NULL);
			set_on_action_suggested_cb(&call->videostream->ms, NULL, NULL);
			if (call->log->status==LinphoneCallSuccess || call->log->status==LinphoneCallAborted){
				linphone_reporting_publish_session_report(call, TRUE);
			}
			break;
		}
		default:{
			break;
		}
	}
}

reporting_session_report_t * linphone_reporting_new() {
	int i;
	reporting_session_report_t * rm = ms_new0(reporting_session_report_t,1);
	reporting_content_metrics_t * metrics[2] = {&rm->local_metrics, &rm->remote_metrics};

	memset(rm, 0, sizeof(reporting_session_report_t));

	for (i = 0; i < 2; i++) {
		metrics[i]->session_description.payload_type = -1;
		metrics[i]->session_description.sample_rate = -1;
		metrics[i]->session_description.frame_duration = -1;
		metrics[i]->session_description.packet_loss_concealment = -1;

		metrics[i]->packet_loss.network_packet_loss_rate = -1;
		metrics[i]->packet_loss.jitter_buffer_discard_rate = -1;


		metrics[i]->jitter_buffer.adaptive = -1;
		metrics[i]->jitter_buffer.abs_max = -1;

		metrics[i]->delay.end_system_delay = -1;
		metrics[i]->delay.interarrival_jitter = -1;
		metrics[i]->delay.mean_abs_jitter = -1;

		metrics[i]->signal.level = 127;
		metrics[i]->signal.noise_level = 127;
	}

	reset_avg_metrics(rm);
	return rm;
}

void linphone_reporting_destroy(reporting_session_report_t * report) {
	STR_REASSIGN(report->info.call_id, NULL);
	STR_REASSIGN(report->info.local_addr.id, NULL);
	STR_REASSIGN(report->info.remote_addr.id, NULL);
	STR_REASSIGN(report->info.orig_id, NULL);
	STR_REASSIGN(report->info.local_addr.ip, NULL);
	STR_REASSIGN(report->info.remote_addr.ip, NULL);
	STR_REASSIGN(report->info.local_addr.group, NULL);
	STR_REASSIGN(report->info.remote_addr.group, NULL);
	STR_REASSIGN(report->info.local_addr.mac, NULL);
	STR_REASSIGN(report->info.remote_addr.mac, NULL);
	STR_REASSIGN(report->dialog_id, NULL);
	STR_REASSIGN(report->local_metrics.session_description.fmtp, NULL);
	STR_REASSIGN(report->local_metrics.session_description.payload_desc, NULL);
	STR_REASSIGN(report->local_metrics.user_agent, NULL);
	STR_REASSIGN(report->remote_metrics.session_description.fmtp, NULL);
	STR_REASSIGN(report->remote_metrics.session_description.payload_desc, NULL);
	STR_REASSIGN(report->remote_metrics.user_agent, NULL);
	STR_REASSIGN(report->qos_analyzer.name, NULL);
	STR_REASSIGN(report->qos_analyzer.timestamp, NULL);
	STR_REASSIGN(report->qos_analyzer.input_leg, NULL);
	STR_REASSIGN(report->qos_analyzer.input, NULL);
	STR_REASSIGN(report->qos_analyzer.output_leg, NULL);
	STR_REASSIGN(report->qos_analyzer.output, NULL);

	ms_free(report);
}


void linphone_reporting_set_on_report_send(LinphoneCall *call, LinphoneQualityReportingReportSendCb cb){
	call->log->reporting.on_report_sent = cb;
}
