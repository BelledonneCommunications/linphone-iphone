 /*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/** 
 * \file rtpsession.h
 * \brief The RtpSession api
 *
 * The RtpSession objects represent a RTP session: once it is configured with 
 * local and remote network addresses and a payload type is given, it let you send
 * and recv a media stream.
**/


#ifndef RTPSESSION_H
#define RTPSESSION_H


#include <ortp/port.h>
#include <ortp/rtp.h>
#include <ortp/payloadtype.h>
#include <ortp/sessionset.h>
#include <ortp/rtcp.h>
#include <ortp/str_utils.h>
#include <ortp/rtpsignaltable.h>
#include <ortp/event.h>



typedef enum {
	RTP_SESSION_RECVONLY,
	RTP_SESSION_SENDONLY,
	RTP_SESSION_SENDRECV
} RtpSessionMode;


/*! Jitter buffer parameters
*/
typedef struct _JBParameters{
	int min_size; /**< in milliseconds*/
	int nom_size; /**< idem */
	int max_size; /**< idem */
	bool_t adaptive;
	bool_t pad[3];
	int max_packets; /**< max number of packets allowed to be queued in the jitter buffer */
} JBParameters;

typedef struct _JitterControl
{
	int count;
	int jitt_comp;   /* the user jitt_comp in miliseconds*/
	int jitt_comp_ts; /* the jitt_comp converted in rtp time (same unit as timestamp) */
	int adapt_jitt_comp_ts;
	int64_t slide;
	int64_t prev_slide;
	float jitter;
	int olddiff;
	float inter_jitter;	/* interarrival jitter as defined in the RFC */
	int corrective_step;
	int corrective_slide;
	bool_t adaptive;
	bool_t enabled;
} JitterControl;

typedef struct _WaitPoint
{
	ortp_mutex_t lock;
	ortp_cond_t  cond;
	uint32_t time;
	bool_t wakeup;
} WaitPoint;

typedef struct _RtpTransport
{
	void *data;
	ortp_socket_t (*t_getsocket)(struct _RtpTransport *t);
	int  (*t_sendto)(struct _RtpTransport *t, mblk_t *msg , int flags, const struct sockaddr *to, socklen_t tolen);
	int  (*t_recvfrom)(struct _RtpTransport *t, mblk_t *msg, int flags, struct sockaddr *from, socklen_t *fromlen);
	struct _RtpSession *session;//<back pointer to the owning session, set by oRTP
}  RtpTransport;


	
typedef struct _RtpStream
{
	ortp_socket_t socket;
        struct _RtpTransport *tr; 
	int sockfamily;
	int max_rq_size;
	int time_jump;
	uint32_t ts_jump;
	queue_t rq;
	queue_t tev_rq;
	mblk_t *cached_mp;
	int loc_port;
#ifdef ORTP_INET6
	struct sockaddr_storage rem_addr;
#else
	struct sockaddr_in rem_addr;
#endif
	int rem_addrlen;
	JitterControl jittctl;
	uint32_t snd_time_offset;/*the scheduler time when the application send its first timestamp*/	
	uint32_t snd_ts_offset;	/* the first application timestamp sent by the application */
	uint32_t snd_rand_offset;	/* a random number added to the user offset to make the stream timestamp*/
	uint32_t snd_last_ts;	/* the last stream timestamp sended */
	uint32_t rcv_time_offset; /*the scheduler time when the application ask for its first timestamp*/
	uint32_t rcv_ts_offset;  /* the first stream timestamp */
	uint32_t rcv_query_ts_offset;	/* the first user timestamp asked by the application */
	uint32_t rcv_last_ts;	/* the last stream timestamp got by the application */
	uint32_t rcv_last_app_ts; /* the last application timestamp asked by the application */	
	uint32_t rcv_last_ret_ts; /* the timestamp of the last sample returned (only for continuous audio)*/
	uint32_t hwrcv_extseq; /* last received on socket extended sequence number */
	uint32_t hwrcv_seq_at_last_SR;
	uint32_t hwrcv_since_last_SR;
	uint32_t last_rcv_SR_ts;     /* NTP timestamp (middle 32 bits) of last received SR */
	struct timeval last_rcv_SR_time;   /* time at which last SR was received  */
	uint16_t snd_seq; /* send sequence number */
	uint32_t last_rtcp_report_snt_r;	/* the time of the last rtcp report sent, in recv timestamp unit */
	uint32_t last_rtcp_report_snt_s;	/* the time of the last rtcp report sent, in send timestamp unit */
	uint32_t rtcp_report_snt_interval; /* the interval in timestamp unit between rtcp report sent */
	uint32_t last_rtcp_packet_count; /*the sender's octet count in the last sent RTCP SR*/
	uint32_t sent_payload_bytes; /*used for RTCP sender reports*/
	unsigned int sent_bytes; /* used for bandwidth estimation */
	struct timeval send_bw_start; /* used for bandwidth estimation */
	unsigned int recv_bytes; /* used for bandwidth estimation */
	struct timeval recv_bw_start; /* used for bandwidth estimation */
	rtp_stats_t stats;
	int recv_errno;
	int send_errno;
	int snd_socket_size;
	int rcv_socket_size;
}RtpStream;

typedef struct _RtcpStream
{
	ortp_socket_t socket;
	int sockfamily;
        struct _RtpTransport *tr; 

	mblk_t *cached_mp;
#ifdef ORTP_INET6
	struct sockaddr_storage rem_addr;
#else
	struct sockaddr_in rem_addr;
#endif
	int rem_addrlen;
	bool_t enabled; /*tells whether we can send RTCP packets */
} RtcpStream;

typedef struct _RtpSession RtpSession;


/**
 * An object representing a bi-directional RTP session.
 * It holds sockets, jitter buffer, various counters (timestamp, sequence numbers...)
 * Applications SHOULD NOT try to read things within the RtpSession object but use
 * instead its public API (the rtp_session_* methods) where RtpSession is used as a 
 * pointer.
 * rtp_session_new() allocates and initialize a RtpSession.
**/
struct _RtpSession
{
	RtpSession *next;	/* next RtpSession, when the session are enqueued by the scheduler */
	int mask_pos;	/* the position in the scheduler mask of RtpSession : do not move this field: it is part of the ABI since the session_set macros use it*/
        struct {
	  RtpProfile *profile;
	  int pt;
	  unsigned int ssrc;
	  WaitPoint wp;
	  int telephone_events_pt;	/* the payload type used for telephony events */
	} snd,rcv;
	unsigned int inc_ssrc_candidate;
	int inc_same_ssrc_count;
	int hw_recv_pt; /* recv payload type before jitter buffer */
	int recv_buf_size;
	RtpSignalTable on_ssrc_changed;
	RtpSignalTable on_payload_type_changed;
	RtpSignalTable on_telephone_event_packet;
	RtpSignalTable on_telephone_event;
	RtpSignalTable on_timestamp_jump;
	RtpSignalTable on_network_error;
	RtpSignalTable on_rtcp_bye;
	struct _OList *signal_tables;
	struct _OList *eventqs;
	msgb_allocator_t allocator;
	RtpStream rtp;
	RtcpStream rtcp;
	RtpSessionMode mode;
	struct _RtpScheduler *sched;
	uint32_t flags;
	int dscp;
	int multicast_ttl;
	int multicast_loopback;
	void * user_data;
	/* FIXME: Should be a table for all session participants. */
	struct timeval last_recv_time; /* Time of receiving the RTP/RTCP packet. */
	mblk_t *pending;
	/* telephony events extension */
	mblk_t *current_tev;		/* the pending telephony events */
	mblk_t *sd;
	queue_t contributing_sources;
	bool_t symmetric_rtp;
	bool_t permissive; /*use the permissive algorithm*/
	bool_t use_connect; /* use connect() on the socket */
	bool_t ssrc_set;
};
	



#ifdef __cplusplus
extern "C"
{
#endif

/* public API */
RtpSession *rtp_session_new(int mode);
void rtp_session_set_scheduling_mode(RtpSession *session, int yesno);
void rtp_session_set_blocking_mode(RtpSession *session, int yesno);
void rtp_session_set_profile(RtpSession *session, RtpProfile *profile);
void rtp_session_set_send_profile(RtpSession *session,RtpProfile *profile);
void rtp_session_set_recv_profile(RtpSession *session,RtpProfile *profile);
RtpProfile *rtp_session_get_profile(RtpSession *session);
RtpProfile *rtp_session_get_send_profile(RtpSession *session);
RtpProfile *rtp_session_get_recv_profile(RtpSession *session);
int rtp_session_signal_connect(RtpSession *session,const char *signal_name, RtpCallback cb, unsigned long user_data);
int rtp_session_signal_disconnect_by_callback(RtpSession *session,const char *signal_name, RtpCallback cb);
void rtp_session_set_ssrc(RtpSession *session, uint32_t ssrc);
void rtp_session_set_seq_number(RtpSession *session, uint16_t seq);
uint16_t rtp_session_get_seq_number(RtpSession *session);

void rtp_session_enable_jitter_buffer(RtpSession *session , bool_t enabled);
bool_t rtp_session_jitter_buffer_enabled(const RtpSession *session);
void rtp_session_set_jitter_buffer_params(RtpSession *session, const JBParameters *par);
void rtp_session_get_jitter_buffer_params(RtpSession *session, JBParameters *par);

/*deprecated jitter control functions*/
void rtp_session_set_jitter_compensation(RtpSession *session, int milisec);
void rtp_session_enable_adaptive_jitter_compensation(RtpSession *session, bool_t val);
bool_t rtp_session_adaptive_jitter_compensation_enabled(RtpSession *session);

void rtp_session_set_time_jump_limit(RtpSession *session, int miliseconds);
int rtp_session_set_local_addr(RtpSession *session,const char *addr, int port);
int rtp_session_get_local_port(const RtpSession *session);

int
rtp_session_set_remote_addr_full (RtpSession * session, const char * addr, int rtp_port, int rtcp_port);
/*same as previous function, old name:*/
int rtp_session_set_remote_addr_and_port (RtpSession * session, const char * addr, int rtp_port, int rtcp_port);
int rtp_session_set_remote_addr(RtpSession *session,const char *addr, int port);
/* alternatively to the set_remote_addr() and set_local_addr(), an application can give
a valid socket (potentially connect()ed )to be used by the RtpSession */
void rtp_session_set_sockets(RtpSession *session, int rtpfd, int rtcpfd);
void rtp_session_set_transports(RtpSession *session, RtpTransport *rtptr, RtpTransport *rtcptr);

/*those methods are provided for people who wants to send non-RTP messages using the RTP/RTCP sockets */
ortp_socket_t rtp_session_get_rtp_socket(const RtpSession *session);
ortp_socket_t rtp_session_get_rtcp_socket(const RtpSession *session);


/* QOS / DSCP */
int rtp_session_set_dscp(RtpSession *session, int dscp);
int rtp_session_get_dscp(const RtpSession *session);


/* Multicast methods */
int rtp_session_set_multicast_ttl(RtpSession *session, int ttl);
int rtp_session_get_multicast_ttl(RtpSession *session);

int rtp_session_set_multicast_loopback(RtpSession *session, int yesno);
int rtp_session_get_multicast_loopback(RtpSession *session);



int rtp_session_set_send_payload_type(RtpSession *session, int paytype);
int rtp_session_get_send_payload_type(const RtpSession *session);

int rtp_session_get_recv_payload_type(const RtpSession *session);
int rtp_session_set_recv_payload_type(RtpSession *session, int pt);

int rtp_session_set_payload_type(RtpSession *session, int pt);

void rtp_session_set_symmetric_rtp (RtpSession * session, bool_t yesno);

void rtp_session_set_connected_mode(RtpSession *session, bool_t yesno);

void rtp_session_enable_rtcp(RtpSession *session, bool_t yesno);

/*low level recv and send functions */
mblk_t * rtp_session_recvm_with_ts (RtpSession * session, uint32_t user_ts);
mblk_t * rtp_session_create_packet(RtpSession *session,int header_size, const uint8_t *payload, int payload_size);
mblk_t * rtp_session_create_packet_with_data(RtpSession *session, uint8_t *payload, int payload_size, void (*freefn)(void*));
mblk_t * rtp_session_create_packet_in_place(RtpSession *session,uint8_t *buffer, int size, void (*freefn)(void*) );
int rtp_session_sendm_with_ts (RtpSession * session, mblk_t *mp, uint32_t userts);
/* high level recv and send functions */
int rtp_session_recv_with_ts(RtpSession *session, uint8_t *buffer, int len, uint32_t ts, int *have_more);
int rtp_session_send_with_ts(RtpSession *session, const uint8_t *buffer, int len, uint32_t userts);

/* event API*/
void rtp_session_register_event_queue(RtpSession *session, OrtpEvQueue *q);
void rtp_session_unregister_event_queue(RtpSession *session, OrtpEvQueue *q);


/* IP bandwidth usage estimation functions, returning bits/s*/
float rtp_session_compute_send_bandwidth(RtpSession *session);
float rtp_session_compute_recv_bandwidth(RtpSession *session);

void rtp_session_send_rtcp_APP(RtpSession *session, uint8_t subtype, const char *name, const uint8_t *data, int datalen);

uint32_t rtp_session_get_current_send_ts(RtpSession *session);
uint32_t rtp_session_get_current_recv_ts(RtpSession *session);
void rtp_session_flush_sockets(RtpSession *session);
void rtp_session_release_sockets(RtpSession *session);
void rtp_session_resync(RtpSession *session);
void rtp_session_reset(RtpSession *session);
void rtp_session_destroy(RtpSession *session);

const rtp_stats_t * rtp_session_get_stats(const RtpSession *session);
void rtp_session_reset_stats(RtpSession *session);

void rtp_session_set_data(RtpSession *session, void *data);
void *rtp_session_get_data(const RtpSession *session);

void rtp_session_set_recv_buf_size(RtpSession *session, int bufsize);
void rtp_session_set_rtp_socket_send_buffer_size(RtpSession * session, unsigned int size);
void rtp_session_set_rtp_socket_recv_buffer_size(RtpSession * session, unsigned int size);

/* in use with the scheduler to convert a timestamp in scheduler time unit (ms) */
uint32_t rtp_session_ts_to_time(RtpSession *session,uint32_t timestamp);
uint32_t rtp_session_time_to_ts(RtpSession *session, int millisecs);
/* this function aims at simulating senders with "imprecise" clocks, resulting in 
rtp packets sent with timestamp uncorrelated with the system clock .
This is only availlable to sessions working with the oRTP scheduler */
void rtp_session_make_time_distorsion(RtpSession *session, int milisec);

/*RTCP functions */
void rtp_session_set_source_description(RtpSession *session, const char *cname,
	const char *name, const char *email, const char *phone, 
    const char *loc, const char *tool, const char *note);
void rtp_session_add_contributing_source(RtpSession *session, uint32_t csrc, 
    const char *cname, const char *name, const char *email, const char *phone, 
    const char *loc, const char *tool, const char *note);
void rtp_session_remove_contributing_sources(RtpSession *session, uint32_t csrc);
mblk_t* rtp_session_create_rtcp_sdes_packet(RtpSession *session);

void rtp_session_get_last_recv_time(RtpSession *session, struct timeval *tv);
int rtp_session_bye(RtpSession *session, const char *reason);

int rtp_session_get_last_send_error_code(RtpSession *session);
void rtp_session_clear_send_error_code(RtpSession *session);
int rtp_session_get_last_recv_error_code(RtpSession *session);
void rtp_session_clear_recv_error_code(RtpSession *session);

/*private */
void rtp_session_init(RtpSession *session, int mode);
#define rtp_session_set_flag(session,flag) (session)->flags|=(flag)
#define rtp_session_unset_flag(session,flag) (session)->flags&=~(flag)
void rtp_session_uninit(RtpSession *session);

#ifdef __cplusplus
}
#endif

#endif
