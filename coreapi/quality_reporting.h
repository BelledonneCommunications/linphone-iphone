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

typedef struct reporting_addr {
	char * ip;
	int port;
	uint32_t ssrc;
} reporting_addr_t;

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
		// int frame_ocets;
		// int frames_per_sec;
		// int packets_per_sec;
		char * fmtp;
		int packet_loss_concealment; // in voip metrics - audio only
		// char * silence_suppression_state;
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
	// struct {
	// 	int burst_loss_density; 
	// 	int burst_duration;
	// 	float gap_loss_density;
	// 	int gap_Duration;
	// 	int min_gap_threshold;
	// } burst_gap_loss;

	// delay - optional
	struct {
		int round_trip_delay; // no - vary
		int end_system_delay; // no - not implemented yet
		// int one_way_delay;
		int symm_one_way_delay; // no - vary (depends on round_trip_delay) + not implemented (depends on end_system_delay)
		int interarrival_jitter; // no - not implemented yet
		int mean_abs_jitter; // to check
	} delay;

	// signal - optional
	struct {
		int level; // no - vary 
		int noise_level; // no - vary
		// int residual_echo_return_loss;
	} signal;

	// quality estimates - optional
	struct {
		int rlq; // linked to moslq - in [0..120]
		int rcq; //voip metrics R factor - no - vary or avg  in [0..120]
		float moslq; // no - vary or avg - voip metrics - in [0..4.9]
		float moscq; // no - vary or avg - voip metrics - in [0..4.9]
		

		// int extri;
		// int extro;
		// char * rlqestalg;
		// char * rcqestalg;
		// char * moslqestalg;
		// char * moscqestalg;
		// char * extriestalg;
		// char * extroutestalg;
		// char * qoestalg;
	} quality_estimates;
} reporting_content_metrics_t;
 
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


void linphone_reporting_publish(LinphoneCall* call, int stats_type);
void linphone_reporting_call_stats_updated(LinphoneCall *call, int stats_type);
#ifdef __cplusplus
}
#endif

#endif
