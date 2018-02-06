/*
 * c-call-stats.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "linphone/api/c-call-stats.h"

#include "c-wrapper/c-wrapper.h"

// =============================================================================

void _linphone_call_stats_clone (LinphoneCallStats *dst, const LinphoneCallStats *src);

/**
 * The LinphoneCallStats objects carries various statistic informations regarding quality of audio or video streams.
 *
 * To receive these informations periodically and as soon as they are computed, the application is invited to place a #LinphoneCoreCallStatsUpdatedCb callback in the LinphoneCoreVTable structure
 * it passes for instantiating the LinphoneCore object (see linphone_core_new() ).
 *
 * At any time, the application can access last computed statistics using linphone_call_get_audio_stats() or linphone_call_get_video_stats().
**/
struct _LinphoneCallStats {
	belle_sip_object_t base;
	void *user_data;
	LinphoneStreamType type; /**< Type of the stream which the stats refer to */
	jitter_stats_t jitter_stats; /**<jitter buffer statistics, see oRTP documentation for details */
	mblk_t *received_rtcp; /**<Last RTCP packet received, as a mblk_t structure. See oRTP documentation for details how to extract information from it*/
	mblk_t *sent_rtcp;/**<Last RTCP packet sent, as a mblk_t structure. See oRTP documentation for details how to extract information from it*/
	float round_trip_delay; /**<Round trip propagation time in seconds if known, -1 if unknown.*/
	LinphoneIceState ice_state; /**< State of ICE processing. */
	LinphoneUpnpState upnp_state; /**< State of uPnP processing. */
	float download_bandwidth; /**<Download bandwidth measurement of received stream, expressed in kbit/s, including IP/UDP/RTP headers*/
	float upload_bandwidth; /**<Download bandwidth measurement of sent stream, expressed in kbit/s, including IP/UDP/RTP headers*/
	float local_late_rate; /**<percentage of packet received too late over last second*/
	float local_loss_rate; /**<percentage of lost packet over last second*/
	int updated; /**< Tell which RTCP packet has been updated (received_rtcp or sent_rtcp). Can be either LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE or LINPHONE_CALL_STATS_SENT_RTCP_UPDATE */
	float rtcp_download_bandwidth; /**<RTCP download bandwidth measurement of received stream, expressed in kbit/s, including IP/UDP/RTP headers*/
	float rtcp_upload_bandwidth; /**<RTCP download bandwidth measurement of sent stream, expressed in kbit/s, including IP/UDP/RTP headers*/
	rtp_stats_t rtp_stats; /**< RTP stats */
	int rtp_remote_family; /**< Ip adress family of the remote destination */
	int clockrate;  /*RTP clockrate of the stream, provided here for easily converting timestamp units expressed in RTCP packets in milliseconds*/
	bool_t rtcp_received_via_mux; /*private flag, for non-regression test only*/
	float estimated_download_bandwidth; /**<Estimated download bandwidth measurement of received stream, expressed in kbit/s, including IP/UDP/RTP headers*/
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneCallStats);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneCallStats);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneCallStats, belle_sip_object_t,
	NULL, // destroy
	_linphone_call_stats_clone, // clone
	NULL, // marshal
	FALSE
);

// =============================================================================
// Private functions
// =============================================================================

LinphoneCallStats *_linphone_call_stats_new () {
	LinphoneCallStats *stats = belle_sip_object_new(LinphoneCallStats);
	return stats;
}

void _linphone_call_stats_uninit (LinphoneCallStats *stats) {
	if (stats->received_rtcp) {
		freemsg(stats->received_rtcp);
		stats->received_rtcp=NULL;
	}
	if (stats->sent_rtcp){
		freemsg(stats->sent_rtcp);
		stats->sent_rtcp=NULL;
	}
}

void _linphone_call_stats_clone (LinphoneCallStats *dst, const LinphoneCallStats *src) {
	/*
	 * Save the belle_sip_object_t part, copy the entire structure and restore the belle_sip_object_t part
	 */
	belle_sip_object_t tmp = dst->base;
	memcpy(dst, src, sizeof(LinphoneCallStats));
	dst->base = tmp;

	dst->received_rtcp = NULL;
	dst->sent_rtcp = NULL;
}

void _linphone_call_stats_set_ice_state (LinphoneCallStats *stats, LinphoneIceState state) {
	stats->ice_state = state;
}

void _linphone_call_stats_set_type (LinphoneCallStats *stats, LinphoneStreamType type) {
	stats->type = type;
}

mblk_t *_linphone_call_stats_get_received_rtcp (const LinphoneCallStats *stats) {
	return stats->received_rtcp;
}

void _linphone_call_stats_set_received_rtcp (LinphoneCallStats *stats, mblk_t *m) {
	stats->received_rtcp = m;
}

mblk_t *_linphone_call_stats_get_sent_rtcp (const LinphoneCallStats *stats) {
	return stats->sent_rtcp;
}

void _linphone_call_stats_set_sent_rtcp (LinphoneCallStats *stats, mblk_t *m) {
	stats->sent_rtcp = m;
}

int _linphone_call_stats_get_updated (const LinphoneCallStats *stats) {
	return stats->updated;
}

void _linphone_call_stats_set_updated (LinphoneCallStats *stats, int updated) {
	stats->updated = updated;
}

void _linphone_call_stats_set_rtp_stats (LinphoneCallStats *stats, const rtp_stats_t *rtpStats) {
	memcpy(&(stats->rtp_stats), rtpStats, sizeof(*rtpStats));
}

void _linphone_call_stats_set_download_bandwidth (LinphoneCallStats *stats, float bandwidth) {
	stats->download_bandwidth = bandwidth;
}

void _linphone_call_stats_set_upload_bandwidth (LinphoneCallStats *stats, float bandwidth) {
	stats->upload_bandwidth = bandwidth;
}

void _linphone_call_stats_set_rtcp_download_bandwidth (LinphoneCallStats *stats, float bandwidth) {
	stats->rtcp_download_bandwidth = bandwidth;
}

void _linphone_call_stats_set_rtcp_upload_bandwidth (LinphoneCallStats *stats, float bandwidth) {
	stats->rtcp_upload_bandwidth = bandwidth;
}

void _linphone_call_stats_set_ip_family_of_remote (LinphoneCallStats *stats, LinphoneAddressFamily family) {
	stats->rtp_remote_family = family;
}

bool_t _linphone_call_stats_rtcp_received_via_mux (const LinphoneCallStats *stats) {
	return stats->rtcp_received_via_mux;
}

// =============================================================================
// Public functions
// =============================================================================

LinphoneCallStats *linphone_call_stats_ref (LinphoneCallStats* stats) {
	belle_sip_object_ref(stats);
	return stats;
}

void linphone_call_stats_unref (LinphoneCallStats* stats) {
	belle_sip_object_unref(stats);
}

void *linphone_call_stats_get_user_data (const LinphoneCallStats *stats) {
	return stats->user_data;
}

void linphone_call_stats_set_user_data (LinphoneCallStats *stats, void *data) {
	stats->user_data = data;
}

void linphone_call_stats_update (LinphoneCallStats *stats, MediaStream *stream) {
	PayloadType *pt;
	RtpSession *session = stream->sessions.rtp_session;
	const MSQualityIndicator *qi = media_stream_get_quality_indicator(stream);
	if (qi) {
		stats->local_late_rate=ms_quality_indicator_get_local_late_rate(qi);
		stats->local_loss_rate=ms_quality_indicator_get_local_loss_rate(qi);
	}
	media_stream_get_local_rtp_stats(stream, &stats->rtp_stats);
	pt = rtp_profile_get_payload(rtp_session_get_profile(session), rtp_session_get_send_payload_type(session));
	stats->clockrate = pt ? pt->clock_rate : 8000;
}

/*do not change the prototype of this function, it is also used internally in linphone-daemon.*/
void linphone_call_stats_fill (LinphoneCallStats *stats, MediaStream *ms, OrtpEvent *ev) {
	OrtpEventType evt=ortp_event_get_type(ev);
	OrtpEventData *evd=ortp_event_get_data(ev);

	if (evt == ORTP_EVENT_RTCP_PACKET_RECEIVED) {
		stats->round_trip_delay = rtp_session_get_round_trip_propagation(ms->sessions.rtp_session);
		if(stats->received_rtcp != NULL)
			freemsg(stats->received_rtcp);
		stats->received_rtcp = evd->packet;
		stats->rtcp_received_via_mux = evd->info.socket_type == OrtpRTPSocket;
		evd->packet = NULL;
		stats->updated = LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE;
		linphone_call_stats_update(stats,ms);
	} else if (evt == ORTP_EVENT_RTCP_PACKET_EMITTED) {
		memcpy(&stats->jitter_stats, rtp_session_get_jitter_stats(ms->sessions.rtp_session), sizeof(jitter_stats_t));
		if (stats->sent_rtcp != NULL)
			freemsg(stats->sent_rtcp);
		stats->sent_rtcp = evd->packet;
		evd->packet = NULL;
		stats->updated = LINPHONE_CALL_STATS_SENT_RTCP_UPDATE;
		linphone_call_stats_update(stats,ms);
	}
}

LinphoneStreamType linphone_call_stats_get_type (const LinphoneCallStats *stats) {
	return stats->type;
}

float linphone_call_stats_get_sender_loss_rate (const LinphoneCallStats *stats) {
	const report_block_t *srb = NULL;

	if (!stats || !stats->sent_rtcp)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->sent_rtcp->b_cont != NULL)
		msgpullup(stats->sent_rtcp, (size_t)-1);

	do{
		if (rtcp_is_SR(stats->sent_rtcp))
			srb = rtcp_SR_get_report_block(stats->sent_rtcp, 0);
		else if (rtcp_is_RR(stats->sent_rtcp))
			srb = rtcp_RR_get_report_block(stats->sent_rtcp, 0);
		if (srb) break;
	}while (rtcp_next_packet(stats->sent_rtcp));
	rtcp_rewind(stats->sent_rtcp);
	if (!srb)
		return 0.0;
	return 100.0f * (float)report_block_get_fraction_lost(srb) / 256.0f;
}

float linphone_call_stats_get_receiver_loss_rate (const LinphoneCallStats *stats) {
	const report_block_t *rrb = NULL;

	if (!stats || !stats->received_rtcp)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->received_rtcp->b_cont != NULL)
		msgpullup(stats->received_rtcp, (size_t)-1);

	do{
		if (rtcp_is_RR(stats->received_rtcp))
			rrb = rtcp_RR_get_report_block(stats->received_rtcp, 0);
		else if (rtcp_is_SR(stats->received_rtcp))
			rrb = rtcp_SR_get_report_block(stats->received_rtcp, 0);
		if (rrb) break;
	}while (rtcp_next_packet(stats->received_rtcp));
	rtcp_rewind(stats->received_rtcp);
	if (!rrb)
		return 0.0;
	return 100.0f * (float)report_block_get_fraction_lost(rrb) / 256.0f;
}

float linphone_call_stats_get_local_loss_rate (const LinphoneCallStats *stats) {
	return stats->local_loss_rate;
}

float linphone_call_stats_get_local_late_rate (const LinphoneCallStats *stats) {
	return stats->local_late_rate;
}

float linphone_call_stats_get_sender_interarrival_jitter (const LinphoneCallStats *stats) {
	const report_block_t *srb = NULL;

	if (!stats || !stats->sent_rtcp)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->sent_rtcp->b_cont != NULL)
		msgpullup(stats->sent_rtcp, (size_t)-1);
	if (rtcp_is_SR(stats->sent_rtcp))
		srb = rtcp_SR_get_report_block(stats->sent_rtcp, 0);
	else if (rtcp_is_RR(stats->sent_rtcp))
		srb = rtcp_RR_get_report_block(stats->sent_rtcp, 0);
	if (!srb)
		return 0.0;
	if (stats->clockrate == 0)
		return 0.0;
	return (float)report_block_get_interarrival_jitter(srb) / (float)stats->clockrate;
}

float linphone_call_stats_get_receiver_interarrival_jitter (const LinphoneCallStats *stats) {
	const report_block_t *rrb = NULL;

	if (!stats || !stats->received_rtcp)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->received_rtcp->b_cont != NULL)
		msgpullup(stats->received_rtcp, (size_t)-1);
	if (rtcp_is_SR(stats->received_rtcp))
		rrb = rtcp_SR_get_report_block(stats->received_rtcp, 0);
	else if (rtcp_is_RR(stats->received_rtcp))
		rrb = rtcp_RR_get_report_block(stats->received_rtcp, 0);
	if (!rrb)
		return 0.0;
	if (stats->clockrate == 0)
		return 0.0;
	return (float)report_block_get_interarrival_jitter(rrb) / (float)stats->clockrate;
}

const rtp_stats_t *linphone_call_stats_get_rtp_stats (const LinphoneCallStats *stats) {
	return &stats->rtp_stats;
}

uint64_t linphone_call_stats_get_late_packets_cumulative_number (const LinphoneCallStats *stats) {
	return linphone_call_stats_get_rtp_stats(stats)->outoftime;
}

float linphone_call_stats_get_download_bandwidth (const LinphoneCallStats *stats) {
	return stats->download_bandwidth;
}

float linphone_call_stats_get_upload_bandwidth (const LinphoneCallStats *stats) {
	return stats->upload_bandwidth;
}

float linphone_call_stats_get_rtcp_download_bandwidth (const LinphoneCallStats *stats) {
	return stats->rtcp_download_bandwidth;
}

float linphone_call_stats_get_rtcp_upload_bandwidth (const LinphoneCallStats *stats) {
	return stats->rtcp_upload_bandwidth;
}

LinphoneIceState linphone_call_stats_get_ice_state (const LinphoneCallStats *stats) {
	return stats->ice_state;
}

LinphoneUpnpState linphone_call_stats_get_upnp_state (const LinphoneCallStats *stats) {
	return stats->upnp_state;
}

LinphoneAddressFamily linphone_call_stats_get_ip_family_of_remote (const LinphoneCallStats *stats) {
	return (LinphoneAddressFamily)stats->rtp_remote_family;
}

float linphone_call_stats_get_jitter_buffer_size_ms (const LinphoneCallStats *stats) {
	return stats->jitter_stats.jitter_buffer_size_ms;
}

float linphone_call_stats_get_round_trip_delay (const LinphoneCallStats *stats) {
	return stats->round_trip_delay;
}

float linphone_call_stats_get_estimated_download_bandwidth(const LinphoneCallStats *stats) {
	return stats->estimated_download_bandwidth;
}

void linphone_call_stats_set_estimated_download_bandwidth(LinphoneCallStats *stats, float estimated_value) {
	stats->estimated_download_bandwidth = estimated_value;
}
