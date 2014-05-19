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

#ifndef quality_reporting_h
#define quality_reporting_h

#include "linphonecore.h"

#ifdef __cplusplus
extern "C"{
#endif


/**
 * Linphone quality report sub object storing address related information (ip / port / MAC).
 */
typedef struct reporting_addr {
	char * ip;
	int port;
	uint32_t ssrc;
} reporting_addr_t;

/**
 * Linphone quality report sub object storing media metrics information as required by RFC035.
 */

typedef struct reporting_content_metrics {
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
		char * fmtp;
		int packet_loss_concealment; // in voip metrics - audio only
	} session_description;

	// jitter buffet - optional
	struct {
		int adaptive; // constant
		int nominal; // no may vary during the call <- average? worst score?
		int max; // no may vary during the call <- average?
		int abs_max; // constant
	} jitter_buffer;

	// packet loss - optional
	struct {
		float network_packet_loss_rate;
		float jitter_buffer_discard_rate;
	} packet_loss;

	// delay - optional
	struct {
		int round_trip_delay; // no - vary
		int end_system_delay; // no - not implemented yet
		int symm_one_way_delay; // no - vary (depends on round_trip_delay) + not implemented (depends on end_system_delay)
		int interarrival_jitter; // no - not implemented yet
		int mean_abs_jitter; // to check
	} delay;

	// signal - optional
	struct {
		int level; // no - vary
		int noise_level; // no - vary
	} signal;

	// quality estimates - optional
	struct {
		int rlq; // linked to moslq - in [0..120]
		int rcq; //voip metrics R factor - no - vary or avg  in [0..120]
		float moslq; // no - vary or avg - voip metrics - in [0..4.9]
		float moscq; // no - vary or avg - voip metrics - in [0..4.9]
	} quality_estimates;
} reporting_content_metrics_t;


/**
 * Linphone quality report main object created by function linphone_reporting_new().
 * It contains all fields required by RFC6035
 */
typedef struct reporting_session_report {
	struct {
		char * call_id;
		char * local_id;
		char * remote_id;
		char * orig_id;
		reporting_addr_t local_addr;
		reporting_addr_t remote_addr;
		char * local_group;
		char * remote_group;

		char * local_mac_addr; // optional
		char * remote_mac_addr; // optional
	} info;

	reporting_content_metrics_t local_metrics;
	reporting_content_metrics_t remote_metrics; // optional

	char * dialog_id; // optional
} reporting_session_report_t;

reporting_session_report_t * linphone_reporting_new();
void linphone_reporting_destroy(reporting_session_report_t * report);

/**
 * Fill media information about a given call. This function must be called before
 * stopping the media stream.
 * @param call #LinphoneCall object to consider
 * @param stats_type the media type (LINPHONE_CALL_STATS_AUDIO or LINPHONE_CALL_STATS_VIDEO)
 *
 */
void linphone_reporting_update(LinphoneCall * call, int stats_type);

/**
 * Fill IP information about a given call. This function must be called each
 * time state is 'LinphoneCallStreamsRunning' since IP might be updated (if we
 * found a direct route between caller and callee for example).
 * @param call #LinphoneCall object to consider
 *
 */
void linphone_reporting_update_ip(LinphoneCall * call);

/**
 * Publish the report on the call end.
 * @param call #LinphoneCall object to consider
 *
 */
void linphone_reporting_publish(LinphoneCall* call);

/**
 * Update publish report data with fresh RTCP stats, if needed.
 * @param call #LinphoneCall object to consider
 * @param stats_type the media type (LINPHONE_CALL_STATS_AUDIO or LINPHONE_CALL_STATS_VIDEO)
 *
 */
void linphone_reporting_call_stats_updated(LinphoneCall *call, int stats_type);

#ifdef __cplusplus
}
#endif

#endif
