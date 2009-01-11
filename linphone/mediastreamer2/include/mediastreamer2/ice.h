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

#include "ortp/stun_udp.h"
#include "ortp/stun.h"
#include "ortp/ortp.h"

/* list of state for STUN connectivity check */
#define TESTING 0
#define WAITING 1
#define RECV_VALID 2
#define SEND_VALID 3
#define VALID 4
#define INVALID 5

struct SdpCandidate {
    int  seq_num;
    char candidate_id[256];
    char password[256];

    char fqdn[256];
    char ipaddr[256];
    int  port;

    int  mapped_port;
    int  peer_derived; /* 0: local 1: stun 2: turn */

    float priority;
};

struct CandidatePair {

    struct SdpCandidate local_candidate;
    struct SdpCandidate remote_candidate;
    int max_sn;
    int min_sn;
    char max_candidate_id[256];
    UInt96 tid;
    int connectivity_check;
};


#ifdef __cplusplus
extern "C"{
#endif

int ice_sound_send_stun_request(RtpSession *session, struct CandidatePair *remote_candidates, int round);

int ice_process_stun_message(RtpSession *session, struct CandidatePair *remote_candidates, OrtpEvent *evt);

#ifdef __cplusplus
}
#endif

#endif
