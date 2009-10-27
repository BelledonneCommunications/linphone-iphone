/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

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

#ifndef ice_hh
#define ice_hh

#include "msfilter.h"
#include "ortp/stun_udp.h"
#include "ortp/stun.h"
#include "ortp/ortp.h"

/* list of state for STUN connectivity check */
#define ICE_PRUNED -1
#define ICE_FROZEN 0
#define ICE_WAITING 1
#define ICE_IN_PROGRESS 2 /* STUN request was sent */
#define ICE_SUCCEEDED 3
#define ICE_FAILED 4 /* no answer or unrecoverable failure */


struct SdpCandidate {
	/* mandatory attributes: draft 19 */
	int foundation;
	int component_id;
	char transport[20];
	int priority;
	char conn_addr[64];
	int conn_port;
	char cand_type[20];
	char rel_addr[64];
	int rel_port;

	/* optionnal attributes: draft 19 */
	char extension_attr[512]; /* *(SP extension-att-name SP extension-att-value) */
};

struct CandidatePair {

    struct SdpCandidate local_candidate;
    struct SdpCandidate remote_candidate;
    long long pair_priority;
    /* additionnal information */
    int rem_controlling;
    UInt96 tid;
    int connectivity_check;
	int retransmission_number;
	uint64_t retransmission_time;
	int nominated_pair;
};

#define MAX_ICE_CANDIDATES 10

struct IceCheckList {
    struct CandidatePair cand_pairs[MAX_ICE_CANDIDATES];
	int nominated_pair_index;

    char loc_ice_ufrag[256];
    char loc_ice_pwd[256];
    char rem_ice_ufrag[256];
    char rem_ice_pwd[256];

    int rem_controlling;
    uint64_t tiebreak_value;

#define ICE_CL_RUNNING 0
#define ICE_CL_COMPLETED 1
#define ICE_CL_FAILED 2
    int state;

    int RTO;
    int Ta;
	uint64_t keepalive_time;
};

#define MS_ICE_SET_SESSION			MS_FILTER_METHOD(MS_ICE_ID,0,RtpSession*)
#define MS_ICE_SET_CANDIDATEPAIRS	MS_FILTER_METHOD(MS_ICE_ID,1,struct CandidatePair*)

extern MSFilterDesc ms_ice_desc;

#endif
