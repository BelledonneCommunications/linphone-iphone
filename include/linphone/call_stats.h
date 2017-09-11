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

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup call_misc
 * @{
 */

#define LINPHONE_CALL_STATS_AUDIO ((int)LinphoneStreamTypeAudio)
#define LINPHONE_CALL_STATS_VIDEO ((int)LinphoneStreamTypeVideo)
#define LINPHONE_CALL_STATS_TEXT  ((int)LinphoneStreamTypeText)

#define LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE (1 << 0) /**< received_rtcp field of LinphoneCallStats object has been updated */
#define LINPHONE_CALL_STATS_SENT_RTCP_UPDATE (1 << 1) /**< sent_rtcp field of LinphoneCallStats object has been updated */
#define LINPHONE_CALL_STATS_PERIODICAL_UPDATE (1 << 2) /**< Every seconds LinphoneCallStats object has been updated */

/**
 * Increment refcount.
 * @param[in] stats LinphoneCallStats object
 * @ingroup misc
**/
LINPHONE_PUBLIC LinphoneCallStats *linphone_call_stats_ref(LinphoneCallStats *stats);

/**
 * Decrement refcount and possibly free the object.
 * @param[in] stats LinphoneCallStats object
 * @ingroup misc
**/
LINPHONE_PUBLIC void linphone_call_stats_unref(LinphoneCallStats *stats);

/**
 * Gets the user data in the LinphoneCallStats object
 * @param[in] stats the LinphoneCallStats
 * @return the user data
 * @ingroup misc
*/
LINPHONE_PUBLIC void *linphone_call_stats_get_user_data(const LinphoneCallStats *stats);

/**
 * Sets the user data in the LinphoneCallStats object
 * @param[in] stats the LinphoneCallStats object
 * @param[in] data the user data
 * @ingroup misc
*/
LINPHONE_PUBLIC void linphone_call_stats_set_user_data(LinphoneCallStats *stats, void *data);

/**
 * Get the type of the stream the stats refer to.
 * @param[in] stats LinphoneCallStats object
 * @return The type of the stream the stats refer to
 */
LINPHONE_PUBLIC LinphoneStreamType linphone_call_stats_get_type(const LinphoneCallStats *stats);

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
 * Get the local loss rate since last report
 * @return The local loss rate
**/
LINPHONE_PUBLIC float linphone_call_stats_get_local_loss_rate(const LinphoneCallStats *stats);

/**
 * Gets the local late rate since last report
 * @return The local late rate
**/
LINPHONE_PUBLIC float linphone_call_stats_get_local_late_rate(const LinphoneCallStats *stats);

/**
 * Gets the local interarrival jitter
 * @param[in] stats LinphoneCallStats object
 * @return The interarrival jitter at last emitted sender report
**/
LINPHONE_PUBLIC float linphone_call_stats_get_sender_interarrival_jitter(const LinphoneCallStats *stats);

/**
 * Gets the remote reported interarrival jitter
 * @param[in] stats LinphoneCallStats object
 * @return The interarrival jitter at last received receiver report
**/
LINPHONE_PUBLIC float linphone_call_stats_get_receiver_interarrival_jitter(const LinphoneCallStats *stats);

LINPHONE_PUBLIC const rtp_stats_t *linphone_call_stats_get_rtp_stats(const LinphoneCallStats *stats);

/**
 * Gets the cumulative number of late packets
 * @param[in] stats LinphoneCallStats object
 * @return The cumulative number of late packets
**/
LINPHONE_PUBLIC uint64_t linphone_call_stats_get_late_packets_cumulative_number(const LinphoneCallStats *stats);

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
 * Get the estimated bandwidth measurement of the received stream, expressed in kbit/s, including IP/UDP/RTP headers.
 * @param[in] stats LinphoneCallStats object
 * @return The estimated bandwidth measurement of the received stream in kbit/s.
 */
LINPHONE_PUBLIC float linphone_call_stats_get_estimated_download_bandwidth(const LinphoneCallStats *stats);

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
 * Get the IP address family of the remote peer.
 * @param[in] stats LinphoneCallStats object
 * @return The IP address family of the remote peer.
 */
LINPHONE_PUBLIC LinphoneAddressFamily linphone_call_stats_get_ip_family_of_remote(const LinphoneCallStats *stats);

/**
 * Get the jitter buffer size in ms.
 * @param[in] stats LinphoneCallStats object
 * @return The jitter buffer size in ms.
 */
LINPHONE_PUBLIC float linphone_call_stats_get_jitter_buffer_size_ms(const LinphoneCallStats *stats);

/**
 * Get the round trip delay in s.
 * @param[in] stats LinphoneCallStats object
 * @return The round trip delay in s.
 */
LINPHONE_PUBLIC float linphone_call_stats_get_round_trip_delay(const LinphoneCallStats *stats);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_ADDRESS_H */
