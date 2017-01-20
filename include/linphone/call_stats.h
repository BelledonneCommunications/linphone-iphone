/*
call_stats.h
Copyright (C) 2010-2016 Belledonne Communications SARL

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

#ifndef LINPHONE_CALL_STATS_H
#define LINPHONE_CALL_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup call_misc
 * @{
 */

#define LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE (1 << 0) /**< received_rtcp field of LinphoneCallStats object has been updated */
#define LINPHONE_CALL_STATS_SENT_RTCP_UPDATE (1 << 1) /**< sent_rtcp field of LinphoneCallStats object has been updated */
#define LINPHONE_CALL_STATS_PERIODICAL_UPDATE (1 << 2) /**< Every seconds LinphoneCallStats object has been updated */

/**
 * The LinphoneCallStats objects carries various statistic informations regarding quality of audio or video streams.
 *
 * To receive these informations periodically and as soon as they are computed, the application is invited to place a #LinphoneCoreCallStatsUpdatedCb callback in the LinphoneCoreVTable structure
 * it passes for instanciating the LinphoneCore object (see linphone_core_new() ).
 *
 * At any time, the application can access last computed statistics using linphone_call_get_audio_stats() or linphone_call_get_video_stats().
**/
typedef struct _LinphoneCallStats LinphoneCallStats;

/**
 * The LinphoneCallStats objects carries various statistic informations regarding quality of audio or video streams.
 *
 * To receive these informations periodically and as soon as they are computed, the application is invited to place a #LinphoneCoreCallStatsUpdatedCb callback in the LinphoneCoreVTable structure
 * it passes for instantiating the LinphoneCore object (see linphone_core_new() ).
 *
 * At any time, the application can access last computed statistics using linphone_call_get_audio_stats() or linphone_call_get_video_stats().
**/
struct _LinphoneCallStats {
    LinphoneStreamType type; /**< Type of the stream which the stats refer to */
    jitter_stats_t jitter_stats; /**<jitter buffer statistics, see oRTP documentation for details */
    mblk_t *received_rtcp; /**<Last RTCP packet received, as a mblk_t structure. See oRTP documentation for details how to extract information from it*/
    mblk_t *sent_rtcp;/**<Last RTCP packet sent, as a mblk_t structure. See oRTP documentation for details how to extract information from it*/
    float round_trip_delay; /**<Round trip propagation time in seconds if known, -1 if unknown.*/
    LinphoneIceState ice_state; /**< State of ICE processing. */
    LinphoneUpnpState	upnp_state; /**< State of uPnP processing. */
    float download_bandwidth; /**<Download bandwidth measurement of received stream, expressed in kbit/s, including IP/UDP/RTP headers*/
    float upload_bandwidth; /**<Download bandwidth measurement of sent stream, expressed in kbit/s, including IP/UDP/RTP headers*/
    float local_late_rate; /**<percentage of packet received too late over last second*/
    float local_loss_rate; /**<percentage of lost packet over last second*/
    int updated; /**< Tell which RTCP packet has been updated (received_rtcp or sent_rtcp). Can be either LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE or LINPHONE_CALL_STATS_SENT_RTCP_UPDATE */
    float rtcp_download_bandwidth; /**<RTCP download bandwidth measurement of received stream, expressed in kbit/s, including IP/UDP/RTP headers*/
    float rtcp_upload_bandwidth; /**<RTCP download bandwidth measurement of sent stream, expressed in kbit/s, including IP/UDP/RTP headers*/
    rtp_stats_t rtp_stats; /**< RTP stats */
    bool_t rtcp_received_via_mux; /*private flag, for non-regression test only*/
    int rtp_remote_family; /* Ip adress family of the remote destination */
};

/**
 * Get the local loss rate since last report
 * @return The sender loss rate
**/
LINPHONE_PUBLIC float linphone_call_stats_get_sender_loss_rate(const LinphoneCallStats *stats);

/**
 * Gets the remote reported loss rate since last report
 * @return The receiver loss rate
**/
LINPHONE_PUBLIC float linphone_call_stats_get_receiver_loss_rate(const LinphoneCallStats *stats);

/**
 * Gets the local interarrival jitter
 * @return The interarrival jitter at last emitted sender report
**/
LINPHONE_PUBLIC float linphone_call_stats_get_sender_interarrival_jitter(const LinphoneCallStats *stats, LinphoneCall *call);

/**
 * Gets the remote reported interarrival jitter
 * @return The interarrival jitter at last received receiver report
**/
LINPHONE_PUBLIC float linphone_call_stats_get_receiver_interarrival_jitter(const LinphoneCallStats *stats, LinphoneCall *call);

LINPHONE_PUBLIC rtp_stats_t linphone_call_stats_get_rtp_stats(const LinphoneCallStats *statss);

/**
 * Gets the cumulative number of late packets
 * @return The cumulative number of late packets
**/
LINPHONE_PUBLIC uint64_t linphone_call_stats_get_late_packets_cumulative_number(const LinphoneCallStats *stats, LinphoneCall *call);

/**
 * Get the bandwidth measurement of the received stream, expressed in kbit/s, including IP/UDP/RTP headers.
 * @param[in] stats LinphoneCallStats object
 * @return The bandwidth measurement of the received stream in kbit/s.
 */
LINPHONE_PUBLIC float linphone_call_stats_get_download_bandwidth(const LinphoneCallStats *stats);

/**
 * Get the bandwidth measurement of the sent stream, expressed in kbit/s, including IP/UDP/RTP headers.
 * @param[in] stats LinphoneCallStats object
 * @return The bandwidth measurement of the sent stream in kbit/s.
 */
LINPHONE_PUBLIC float linphone_call_stats_get_upload_bandwidth(const LinphoneCallStats *stats);

/**
 * Get the state of ICE processing.
 * @param[in] stats LinphoneCallStats object
 * @return The state of ICE processing.
 */
LINPHONE_PUBLIC LinphoneIceState linphone_call_stats_get_ice_state(const LinphoneCallStats *stats);

/**
 * Get the state of uPnP processing.
 * @param[in] stats LinphoneCallStats object
 * @return The state of uPnP processing.
 */
LINPHONE_PUBLIC LinphoneUpnpState linphone_call_stats_get_upnp_state(const LinphoneCallStats *stats);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_ADDRESS_H */
