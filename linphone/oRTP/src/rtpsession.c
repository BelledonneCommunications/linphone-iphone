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


#if defined(WIN32) || defined(_WIN32_WCE)
#include "ortp-config-win32.h"
#else
#include "ortp-config.h"
#endif

#include "ortp/ortp.h"
#include "ortp/telephonyevents.h"
#include "ortp/rtcp.h"
#include "jitterctl.h"
#include "scheduler.h"
#include "utils.h"
#include "rtpsession_priv.h"

extern mblk_t *rtcp_create_simple_bye_packet(uint32_t ssrc, const char *reason);
extern int rtcp_sr_init(RtpSession *session, char *buf, int size);
extern int rtcp_rr_init(RtpSession *session, char *buf, int size);



/* this function initialize all session parameter's that depend on the payload type */
static void payload_type_changed(RtpSession *session, PayloadType *pt){
	jitter_control_set_payload(&session->rtp.jittctl,pt);
	session->rtp.rtcp_report_snt_interval=RTCP_DEFAULT_REPORT_INTERVAL*pt->clock_rate;
	rtp_session_set_time_jump_limit(session,session->rtp.time_jump);
	if (pt->type==PAYLOAD_VIDEO){
		session->permissive=TRUE;
		ortp_message("Using permissive algorithm");
	}
	else session->permissive=FALSE;
}

void wait_point_init(WaitPoint *wp){
	ortp_mutex_init(&wp->lock,NULL);
	ortp_cond_init(&wp->cond,NULL);
	wp->time=0;
	wp->wakeup=FALSE;
}
void wait_point_uninit(WaitPoint *wp){
	ortp_cond_destroy(&wp->cond);
	ortp_mutex_destroy(&wp->lock);
}

#define wait_point_lock(wp) ortp_mutex_lock(&(wp)->lock)
#define wait_point_unlock(wp) ortp_mutex_unlock(&(wp)->lock)

void wait_point_wakeup_at(WaitPoint *wp, uint32_t t, bool_t dosleep){
	wp->time=t;
	wp->wakeup=TRUE;
	if (dosleep) ortp_cond_wait(&wp->cond,&wp->lock);
}


bool_t wait_point_check(WaitPoint *wp, uint32_t t){
	bool_t ok=FALSE;
	
	if (wp->wakeup){
		if (TIME_IS_NEWER_THAN(t,wp->time)){
			wp->wakeup=FALSE;
			ok=TRUE;
			
		}
	}
	return ok;
}
#define wait_point_wakeup(wp) ortp_cond_signal(&(wp)->cond);

extern void rtp_parse(RtpSession *session, mblk_t *mp, uint32_t local_str_ts,
		struct sockaddr *addr, socklen_t addrlen);


static uint32_t uint32_t_random(){
	return random();
}


#define RTP_SEQ_IS_GREATER(seq1,seq2)\
	((uint16_t)((uint16_t)(seq1) - (uint16_t)(seq2))< (uint16_t)(1<<15))

/* put an rtp packet in queue. It is called by rtp_parse()*/
void rtp_putq(queue_t *q, mblk_t *mp)
{
	mblk_t *tmp;
	rtp_header_t *rtp=(rtp_header_t*)mp->b_rptr,*tmprtp;
	/* insert message block by increasing time stamp order : the last (at the bottom)
		message of the queue is the newest*/
	ortp_debug("rtp_putq(): Enqueuing packet with ts=%i and seq=%i",rtp->timestamp,rtp->seq_number);
	
	if (qempty(q)) {
		putq(q,mp);
		return;
	}
	tmp=qlast(q);
	/* we look at the queue from bottom to top, because enqueued packets have a better chance
	to be enqueued at the bottom, since there are surely newer */
	while (!qend(q,tmp))
	{
		tmprtp=(rtp_header_t*)tmp->b_rptr;
		ortp_debug("rtp_putq(): Seeing packet with seq=%i",tmprtp->seq_number);
		
 		if (rtp->seq_number == tmprtp->seq_number)
 		{
 			/* this is a duplicated packet. Don't queue it */
 			ortp_debug("rtp_putq: duplicated message.");
 			freemsg(mp);
 			return;
		}else if (RTP_SEQ_IS_GREATER(rtp->seq_number,tmprtp->seq_number)){
			
			insq(q,tmp->b_next,mp);
			return;
 		}
		tmp=tmp->b_prev;
	}
	/* this packet is the oldest, it has to be 
	placed on top of the queue */
	insq(q,qfirst(q),mp);
	
}



mblk_t *rtp_getq(queue_t *q,uint32_t timestamp, int *rejected)
{
	mblk_t *tmp,*ret=NULL,*old=NULL;
	rtp_header_t *tmprtp;
	uint32_t ts_found=0;
	
	*rejected=0;
	ortp_debug("rtp_getq(): Timestamp %i wanted.",timestamp);

	if (qempty(q))
	{
		/*ortp_debug("rtp_getq: q is empty.");*/
		return NULL;
	}
	/* return the packet with ts just equal or older than the asked timestamp */
	/* packets with older timestamps are discarded */
	while ((tmp=qfirst(q))!=NULL)
	{
		tmprtp=(rtp_header_t*)tmp->b_rptr;
		ortp_debug("rtp_getq: Seeing packet with ts=%i",tmprtp->timestamp);
		if ( RTP_TIMESTAMP_IS_NEWER_THAN(timestamp,tmprtp->timestamp) )
		{
			if (ret!=NULL && tmprtp->timestamp==ts_found) {
				/* we've found two packets with same timestamp. return the first one */
				break;
			}
			if (old!=NULL) {
				ortp_debug("rtp_getq: discarding too old packet with ts=%i",ts_found);
				(*rejected)++;
				freemsg(old);
			}
			ret=getq(q); /* dequeue the packet, since it has an interesting timestamp*/
			ts_found=tmprtp->timestamp;
			ortp_debug("rtp_getq: Found packet with ts=%i",tmprtp->timestamp);
			old=ret;
		}
		else
		{
			break;
		}
	}
	return ret;
}

mblk_t *rtp_getq_permissive(queue_t *q,uint32_t timestamp, int *rejected)
{
	mblk_t *tmp,*ret=NULL;
	rtp_header_t *tmprtp;
	
	*rejected=0;
	ortp_debug("rtp_getq_permissive(): Timestamp %i wanted.",timestamp);

	if (qempty(q))
	{
		/*ortp_debug("rtp_getq: q is empty.");*/
		return NULL;
	}
	/* return the packet with the older timestamp (provided that it is older than
	the asked timestamp) */
	tmp=qfirst(q);
	tmprtp=(rtp_header_t*)tmp->b_rptr;
	ortp_debug("rtp_getq_permissive: Seeing packet with ts=%i",tmprtp->timestamp);
	if ( RTP_TIMESTAMP_IS_NEWER_THAN(timestamp,tmprtp->timestamp) )
	{
		ret=getq(q); /* dequeue the packet, since it has an interesting timestamp*/
		ortp_debug("rtp_getq_permissive: Found packet with ts=%i",tmprtp->timestamp);
	}
	return ret;
}


void
rtp_session_init (RtpSession * session, int mode)
{
	JBParameters jbp;
	if (session == NULL) 
	{
	    ortp_debug("rtp_session_init: Invalid paramter (session=NULL)");
	    return;
	}
	
	memset (session, 0, sizeof (RtpSession));
	session->mode = (RtpSessionMode) mode;
	if ((mode == RTP_SESSION_RECVONLY) || (mode == RTP_SESSION_SENDRECV))
	{
		rtp_session_set_flag (session, RTP_SESSION_RECV_SYNC);
		rtp_session_set_flag (session, RTP_SESSION_RECV_NOT_STARTED);
		
	}
	if ((mode == RTP_SESSION_SENDONLY) || (mode == RTP_SESSION_SENDRECV))
	{
		rtp_session_set_flag (session, RTP_SESSION_SEND_NOT_STARTED);
		session->snd.ssrc=uint32_t_random();
		/* set default source description */
		rtp_session_set_source_description(session,"unknown@unknown",NULL,NULL,
				NULL,NULL,"oRTP-" ORTP_VERSION,"This is free sofware (LGPL) !");
	}
	session->snd.telephone_events_pt=-1;	/* not defined a priori */
	session->rcv.telephone_events_pt=-1;	/* not defined a priori */
	rtp_session_set_profile (session, &av_profile); /*the default profile to work with */
	session->rtp.socket=-1;
	session->rtcp.socket=-1;
#ifndef WIN32
	session->rtp.snd_socket_size=0;	/*use OS default value unless on windows where they are definitely too short*/
	session->rtp.rcv_socket_size=0;
#else
	session->rtp.snd_socket_size=session->rtp.rcv_socket_size=65536;
#endif
	session->dscp=RTP_DEFAULT_DSCP;
	session->multicast_ttl=RTP_DEFAULT_MULTICAST_TTL;
	session->multicast_loopback=RTP_DEFAULT_MULTICAST_LOOPBACK;
	qinit(&session->rtp.rq);
	qinit(&session->rtp.tev_rq);
	qinit(&session->contributing_sources);
	session->eventqs=NULL;
	/* init signal tables */
	rtp_signal_table_init (&session->on_ssrc_changed, session,"ssrc_changed");
	rtp_signal_table_init (&session->on_payload_type_changed, session,"payload_type_changed");
	rtp_signal_table_init (&session->on_telephone_event, session,"telephone-event");
	rtp_signal_table_init (&session->on_telephone_event_packet, session,"telephone-event_packet");
	rtp_signal_table_init (&session->on_timestamp_jump,session,"timestamp_jump");
	rtp_signal_table_init (&session->on_network_error,session,"network_error");
	rtp_signal_table_init (&session->on_rtcp_bye,session,"rtcp_bye");
	wait_point_init(&session->snd.wp);
	wait_point_init(&session->rcv.wp);
	/*defaults send payload type to 0 (pcmu)*/
	rtp_session_set_send_payload_type(session,0);
	/*sets supposed recv payload type to undefined */
	rtp_session_set_recv_payload_type(session,-1);
	/* configure jitter buffer with working default parameters */
	jbp.min_size=RTP_DEFAULT_JITTER_TIME;
	jbp.nom_size=RTP_DEFAULT_JITTER_TIME;
	jbp.max_size=-1;
	jbp.max_packets= 100;/* maximum number of packet allowed to be queued */
	jbp.adaptive=TRUE;
	rtp_session_enable_jitter_buffer(session,TRUE);
	rtp_session_set_jitter_buffer_params(session,&jbp);
	rtp_session_set_time_jump_limit(session,5000);
	rtp_session_enable_rtcp(session,TRUE);
	session->recv_buf_size = UDP_MAX_SIZE;
	session->symmetric_rtp = FALSE;
	session->permissive=FALSE;
	msgb_allocator_init(&session->allocator);
}


/**
 * Creates a new rtp session.
 * If the session is able to send data (RTP_SESSION_SENDONLY or
 * RTP_SESSION_SENDRECV), then a random SSRC number is choosed for 
 * the outgoing stream.
 * @param mode One of the RtpSessionMode flags.	
 *
 * @return the newly created rtp session.
**/
RtpSession *
rtp_session_new (int mode)
{
	RtpSession *session;
	session = (RtpSession *) ortp_malloc (sizeof (RtpSession));
	if (session == NULL)
	{
	    ortp_error("rtp_session_new: Memory allocation failed");
	    return NULL;
	}
      rtp_session_init (session, mode);
	return session;
}

/**
 * Sets the scheduling mode of the rtp session. If @yesno is TRUE, the rtp session is in
 *	the scheduled mode, that means that you can use session_set_select() to block until it's time
 *	to receive or send on this session according to the timestamp passed to the respective functions.
 *  You can also use blocking mode (see rtp_session_set_blocking_mode() ), to simply block within
 *	the receive and send functions.
 *	If @yesno is FALSE, the ortp scheduler will not manage those sessions, meaning that blocking mode 
 *  and the use of session_set_select() for this session are disabled.
 *@param session a rtp session.
 *@param yesno 	a boolean to indicate the scheduling mode.
 *
 *
**/
void
rtp_session_set_scheduling_mode (RtpSession * session, int yesno)
{
	if (yesno)
	{
		RtpScheduler *sched;
		sched = ortp_get_scheduler ();
		if (sched != NULL)
		{
			rtp_session_set_flag (session, RTP_SESSION_SCHEDULED);
			session->sched = sched;
			rtp_scheduler_add_session (sched, session);
		}
		else
			ortp_warning
				("rtp_session_set_scheduling_mode: Cannot use scheduled mode because the "
				 "scheduler is not started. Use ortp_scheduler_init() before.");
	}
	else
		rtp_session_unset_flag (session, RTP_SESSION_SCHEDULED);
}


/**
 *	This function implicitely enables the scheduling mode if yesno is TRUE.
 *	rtp_session_set_blocking_mode() defines the behaviour of the rtp_session_recv_with_ts() and 
 *	rtp_session_send_with_ts() functions. If @yesno is TRUE, rtp_session_recv_with_ts()
 *	will block until it is time for the packet to be received, according to the timestamp
 *	passed to the function. After this time, the function returns.
 *	For rtp_session_send_with_ts(), it will block until it is time for the packet to be sent.
 *	If @yesno is FALSE, then the two functions will return immediately.
 *
 *  @param session a rtp session
 *  @param yesno a boolean
**/
void
rtp_session_set_blocking_mode (RtpSession * session, int yesno)
{
	if (yesno){
		rtp_session_set_scheduling_mode(session,TRUE);
		rtp_session_set_flag (session, RTP_SESSION_BLOCKING_MODE);
	}else
		rtp_session_unset_flag (session, RTP_SESSION_BLOCKING_MODE);
}

/**
 *	Set the RTP profile to be used for the session. By default, all session are created by
 *	rtp_session_new() are initialized with the AV profile, as defined in RFC 3551. The application
 *	can set any other profile instead using that function.
 *
 * @param session a rtp session
 * @param profile a rtp profile
**/

void
rtp_session_set_profile (RtpSession * session, RtpProfile * profile)
{
	session->snd.profile = profile;
	session->rcv.profile = profile;
	rtp_session_telephone_events_supported(session);
}

/**
 *	By default oRTP automatically sends RTCP SR or RR packets. If
 *	yesno is set to FALSE, the RTCP sending of packet is disabled.
 *	This functionnality might be needed for some equipments that do not
 *	support RTCP, leading to a traffic of ICMP errors on the network.
 *	It can also be used to save bandwidth despite the RTCP bandwidth is 
 *	actually and usually very very low.
**/
void rtp_session_enable_rtcp(RtpSession *session, bool_t yesno){
	session->rtcp.enabled=yesno;
}

/**
 *	Set the RTP profile to be used for the sending by this session. By default, all session are created by
 *	rtp_session_new() are initialized with the AV profile, as defined in RFC 3551. The application
 *	can set any other profile instead using that function.
 * @param session a rtp session
 * @param profile a rtp profile
 *
**/

void
rtp_session_set_send_profile (RtpSession * session, RtpProfile * profile)
{
	session->snd.profile = profile;
	rtp_session_send_telephone_events_supported(session);
}



/**
 *	Set the RTP profile to be used for the receiveing by this session. By default, all session are created by
 *	rtp_session_new() are initialized with the AV profile, as defined in RFC 3551. The application
 *	can set any other profile instead using that function.
 *
 * @param session a rtp session
 * @param profile a rtp profile
**/

void
rtp_session_set_recv_profile (RtpSession * session, RtpProfile * profile)
{
	session->rcv.profile = profile;
	rtp_session_recv_telephone_events_supported(session);
}

/**
 *@param session a rtp session
 *
 *	DEPRECATED! Returns current send profile.
 *	Use rtp_session_get_send_profile() or rtp_session_get_recv_profile()
 *
**/
RtpProfile *rtp_session_get_profile(RtpSession *session){
	return session->snd.profile;
}


/**
 *@param session a rtp session
 *
 *	Returns current send profile.
 *
**/
RtpProfile *rtp_session_get_send_profile(RtpSession *session){
	return session->snd.profile;
}

/**
 *@param session a rtp session
 *
 *	Returns current receive profile.
 *
**/
RtpProfile *rtp_session_get_recv_profile(RtpSession *session){
	return session->rcv.profile;
}

/**
 *	The default value is UDP_MAX_SIZE bytes, a value which is working for mostly everyone.
 *	However if your application can make assumption on the sizes of received packet,
 *	it can be interesting to set it to a lower value in order to save memory.
 *
 * @param session a rtp session
 * @param bufsize max size in bytes for receiving packets
**/
void rtp_session_set_recv_buf_size(RtpSession *session, int bufsize){
	session->recv_buf_size=bufsize;
}

/**
 *	Set kernel send maximum buffer size for the rtp socket.
 *	A value of zero defaults to the operating system default.
**/
void rtp_session_set_rtp_socket_send_buffer_size(RtpSession * session, unsigned int size){
	session->rtp.snd_socket_size=size;
}

/**
 *	Set kernel recv maximum buffer size for the rtp socket.
 *	A value of zero defaults to the operating system default.
**/
void rtp_session_set_rtp_socket_recv_buffer_size(RtpSession * session, unsigned int size){
	session->rtp.rcv_socket_size=size;
}

/**
 *	This function provides the way for an application to be informed of various events that
 *	may occur during a rtp session. @signal is a string identifying the event, and @cb is 
 *	a user supplied function in charge of processing it. The application can register
 *	several callbacks for the same signal, in the limit of #RTP_CALLBACK_TABLE_MAX_ENTRIES.
 *	Here are name and meaning of supported signals types:
 *
 *	"ssrc_changed" : the SSRC of the incoming stream has changed.
 *
 *	"payload_type_changed" : the payload type of the incoming stream has changed.
 *
 *	"telephone-event_packet" : a telephone-event rtp packet (RFC2833) is received.
 *
 *	"telephone-event" : a telephone event has occured. This is a high-level shortcut for "telephone-event_packet".
 *
 *	"network_error" : a network error happened on a socket. Arguments of the callback functions are
 *						a const char * explaining the error, an int errno error code and the user_data as usual.
 *
 *	"timestamp_jump" : we have received a packet with timestamp in far future compared to last timestamp received.
 *						The farness of far future is set by rtp_sesssion_set_time_jump_limit()
 *  "rtcp_bye": we have received a RTCP bye packet. Arguments of the callback
 *              functions are a const char * containing the leaving reason and
 *              the user_data.
 * 
 *	Returns: 0 on success, -EOPNOTSUPP if the signal does not exists, -1 if no more callbacks
 *	can be assigned to the signal type.
 *
 * @param session 	a rtp session
 * @param signal_name	the name of a signal
 * @param cb		a RtpCallback
 * @param user_data	a pointer to any data to be passed when invoking the callback.
 *
**/
int
rtp_session_signal_connect (RtpSession * session, const char *signal_name,
			    RtpCallback cb, unsigned long user_data)
{
	OList *elem;
	for (elem=session->signal_tables;elem!=NULL;elem=o_list_next(elem)){
		RtpSignalTable *s=(RtpSignalTable*) elem->data;
		if (strcmp(signal_name,s->signal_name)==0){
			return rtp_signal_table_add(s,cb,user_data);
		}
	}
	ortp_warning ("rtp_session_signal_connect: inexistant signal %s",signal_name);
	return -1;
}


/**
 *	Removes callback function @cb to the list of callbacks for signal @signal.
 *
 * @param session a rtp session
 * @param signal_name	a signal name
 * @param cb	a callback function.
 * @return: 0 on success, a negative value if the callback was not found.
**/
int
rtp_session_signal_disconnect_by_callback (RtpSession * session, const char *signal_name,
					   RtpCallback cb)
{
	OList *elem;
	for (elem=session->signal_tables;elem!=NULL;elem=o_list_next(elem)){
		RtpSignalTable *s=(RtpSignalTable*) elem->data;
		if (strcmp(signal_name,s->signal_name)==0){
			return rtp_signal_table_remove_by_callback(s,cb);
		}
	}
	ortp_warning ("rtp_session_signal_connect: inexistant signal %s",signal_name);
	return -1;
}


/**
 * sets the initial sequence number of a sending session.
 * @param session		a rtp session freshly created.
 * @param addr			a 16 bit unsigned number.
 *
**/
void rtp_session_set_seq_number(RtpSession *session, uint16_t seq){
	session->rtp.snd_seq=seq;
}


uint16_t rtp_session_get_seq_number(RtpSession *session){
	return session->rtp.snd_seq;
}


/**
 *	Sets the SSRC for the outgoing stream.
 *  If not done, a random ssrc is used.
 *
 * @param session a rtp session.
 * @param ssrc an unsigned 32bit integer representing the synchronisation source identifier (SSRC).
**/
void
rtp_session_set_ssrc (RtpSession * session, uint32_t ssrc)
{
	session->snd.ssrc = ssrc;
}


void rtp_session_update_payload_type(RtpSession *session, int paytype){
	/* check if we support this payload type */
	PayloadType *pt=rtp_profile_get_payload(session->rcv.profile,paytype);
	if (pt!=0){
		session->hw_recv_pt=paytype;
		ortp_message ("payload type changed to %i(%s) !",
				 paytype,pt->mime_type);
		payload_type_changed(session,pt);
	}else{
		ortp_warning("Receiving packet with unknown payload type %i.",paytype);
	}
}
/**
 *	Sets the payload type of the rtp session. It decides of the payload types written in the
 *	of the rtp header for the outgoing stream, if the session is SENDRECV or SENDONLY.
 *	For payload type in incoming packets, the application can be informed by registering
 *	for the "payload_type_changed" signal, so that it can make the necessary changes
 *	on the downstream decoder that deals with the payload of the packets.
 *
 * @param session a rtp session
 * @param paytype the payload type number
 * @return 0 on success, -1 if the payload is not defined.
**/

int
rtp_session_set_send_payload_type (RtpSession * session, int paytype)
{
	session->snd.pt=paytype;
	return 0;
}

/**
 *@param session a rtp session
 *
 *@return the payload type currently used in outgoing rtp packets
**/
int rtp_session_get_send_payload_type(const RtpSession *session){
	return session->snd.pt;
}

/**
 *
 *	Sets the expected payload type for incoming packets.
 *	If the actual payload type in incoming packets is different that this expected payload type, thus
 *	the "payload_type_changed" signal is emitted.
 *
 *@param session a rtp session
 *@param paytype the payload type number
 *@return 0 on success, -1 if the payload is not defined.
**/

int
rtp_session_set_recv_payload_type (RtpSession * session, int paytype)
{
	PayloadType *pt;
	session->rcv.pt=paytype;
	session->hw_recv_pt=paytype;
	pt=rtp_profile_get_payload(session->rcv.profile,paytype);
	if (pt!=NULL){
		payload_type_changed(session,pt);
	}
	return 0;
}

/**
 *@param session a rtp session
 *
 * @return the payload type currently used in incoming rtp packets
**/
int rtp_session_get_recv_payload_type(const RtpSession *session){
	return session->rcv.pt;
}

/**
 *	Sets the expected payload type for incoming packets and payload type to be used for outgoing packets.
 *	If the actual payload type in incoming packets is different that this expected payload type, thus
 *	the "payload_type_changed" signal is emitted.
 *
 * @param session a rtp session
 * @param paytype the payload type number
 * @return 0 on success, -1 if the payload is not defined.
**/
int rtp_session_set_payload_type(RtpSession *session, int pt){
	if (rtp_session_set_send_payload_type(session,pt)<0) return -1;
	if (rtp_session_set_recv_payload_type(session,pt)<0) return -1;
	return 0;
}


static void rtp_header_init_from_session(rtp_header_t *rtp, RtpSession *session){
	rtp->version = 2;
	rtp->padbit = 0;
	rtp->extbit = 0;
	rtp->markbit= 0;
	rtp->cc = 0;
	rtp->paytype = session->snd.pt;
	rtp->ssrc = session->snd.ssrc;
	rtp->timestamp = 0;	/* set later, when packet is sended */
	/* set a seq number */
	rtp->seq_number=session->rtp.snd_seq;
}

/**
 *	Allocates a new rtp packet. In the header, ssrc and payload_type according to the session's
 *	context. Timestamp is not set, it will be set when the packet is going to be
 *	sent with rtp_session_sendm_with_ts(). Sequence number is initalized to previous sequence number sent + 1
 *	If payload_size is zero, thus an empty packet (just a RTP header) is returned.
 *
 *@param session a rtp session.
 *@param header_size the rtp header size. For standart size (without extensions), it is RTP_FIXED_HEADER_SIZE
 *@param payload data to be copied into the rtp packet.
 *@param payload_size size of data carried by the rtp packet.
 *@return a rtp packet in a mblk_t (message block) structure.
**/
mblk_t * rtp_session_create_packet(RtpSession *session,int header_size, const uint8_t *payload, int payload_size)
{
	mblk_t *mp;
	int msglen=header_size+payload_size;
	rtp_header_t *rtp;
	
	mp=allocb(msglen,BPRI_MED);
	rtp=(rtp_header_t*)mp->b_rptr;
	rtp_header_init_from_session(rtp,session);
	/*copy the payload, if any */
	mp->b_wptr+=header_size;
	if (payload_size){
		memcpy(mp->b_wptr,payload,payload_size);
		mp->b_wptr+=payload_size;
	}
	return mp;
}

/**
 *	Creates a new rtp packet using the given payload buffer (no copy). The header will be allocated separetely.
 *  In the header, ssrc and payload_type according to the session's
 *	context. Timestamp and seq number are not set, there will be set when the packet is going to be
 *	sent with rtp_session_sendm_with_ts().
 *	oRTP will send this packet using libc's sendmsg() (if this function is availlable!) so that there will be no
 *	packet concatenation involving copies to be done in user-space.
 *  @freefn can be NULL, in that case payload will be kept untouched.
 *
 * @param session a rtp session.
 * @param payload the data to be sent with this packet
 * @param payload_size size of data
 * @param freefn a function that will be called when the payload buffer is no more needed.
 * @return: a rtp packet in a mblk_t (message block) structure.
**/

mblk_t * rtp_session_create_packet_with_data(RtpSession *session, uint8_t *payload, int payload_size, void (*freefn)(void*))
{
	mblk_t *mp,*mpayload;
	int header_size=RTP_FIXED_HEADER_SIZE; /* revisit when support for csrc is done */
	rtp_header_t *rtp;
	
	mp=allocb(header_size,BPRI_MED);
	rtp=(rtp_header_t*)mp->b_rptr;
	rtp_header_init_from_session(rtp,session);
	mp->b_wptr+=header_size;
	/* create a mblk_t around the user supplied payload buffer */
	mpayload=esballoc(payload,payload_size,BPRI_MED,freefn);
	mpayload->b_wptr+=payload_size;
	/* link it with the header */
	mp->b_cont=mpayload;
	return mp;
}


/**
 * Creates a new rtp packet using the buffer given in arguments (no copy). 
 * In the header, ssrc and payload_type according to the session's
 *context. Timestamp and seq number are not set, there will be set when the packet is going to be
 *	sent with rtp_session_sendm_with_ts().
 *  @freefn can be NULL, in that case payload will be kept untouched.
 *
 * @param session a rtp session.
 * @param buffer a buffer that contains first just enough place to write a RTP header, then the data to send.
 * @param size the size of the buffer
 * @param freefn a function that will be called once the buffer is no more needed (the data has been sent).
 * @return a rtp packet in a mblk_t (message block) structure.
**/
mblk_t * rtp_session_create_packet_in_place(RtpSession *session,uint8_t *buffer, int size, void (*freefn)(void*) )
{
	mblk_t *mp;
	rtp_header_t *rtp;
	
	mp=esballoc(buffer,size,BPRI_MED,freefn);

	rtp=(rtp_header_t*)mp->b_rptr;
	rtp_header_init_from_session(rtp,session);
	return mp;
}


int
__rtp_session_sendm_with_ts (RtpSession * session, mblk_t *mp, uint32_t packet_ts, uint32_t send_ts)
{
	rtp_header_t *rtp;
	uint32_t packet_time;
	int error = 0;
	int packsize;
	RtpScheduler *sched=session->sched;
	RtpStream *stream=&session->rtp;

	if (session->flags & RTP_SESSION_SEND_NOT_STARTED)
	{
		session->rtp.snd_ts_offset = send_ts;
		/* Set initial last_rcv_time to first send time. */
		if ((session->flags & RTP_SESSION_RECV_NOT_STARTED)
		|| session->mode == RTP_SESSION_SENDONLY)
		{
		gettimeofday(&session->last_recv_time, NULL);
		}
		if (session->flags & RTP_SESSION_SCHEDULED)
		{
			session->rtp.snd_time_offset = sched->time_;
		}
		rtp_session_unset_flag (session,RTP_SESSION_SEND_NOT_STARTED);
	}
	/* if we are in blocking mode, then suspend the process until the scheduler it's time to send  the
	 * next packet */
	/* if the timestamp of the packet queued is older than current time, then you we must
	 * not block */
	if (session->flags & RTP_SESSION_SCHEDULED)
	{
		wait_point_lock(&session->snd.wp);
		packet_time =
			rtp_session_ts_to_time (session,
				     send_ts -
				     session->rtp.snd_ts_offset) +
					session->rtp.snd_time_offset;
		/*ortp_message("rtp_session_send_with_ts: packet_time=%i time=%i",packet_time,sched->time_);*/
		if (TIME_IS_STRICTLY_NEWER_THAN (packet_time, sched->time_))
		{
			wait_point_wakeup_at(&session->snd.wp,packet_time,(session->flags & RTP_SESSION_BLOCKING_MODE)!=0);	
			session_set_clr(&sched->w_sessions,session);	/* the session has written */
		}
		else session_set_set(&sched->w_sessions,session);	/*to indicate select to return immediately */
		wait_point_unlock(&session->snd.wp);
	}
	
	if(mp==NULL) {/*for people who just want to be blocked but
		 do not want to send anything.*/
		session->rtp.snd_last_ts = packet_ts;
		return 0;
	}

	rtp=(rtp_header_t*)mp->b_rptr;
	
	packsize = msgdsize(mp) ;
	
	rtp->timestamp=packet_ts;
	if (session->snd.telephone_events_pt==rtp->paytype)
	{
		rtp->seq_number = session->rtp.snd_seq;
		session->rtp.snd_seq++;
	}
	else
		session->rtp.snd_seq=rtp->seq_number+1;
	session->rtp.snd_last_ts = packet_ts;


	ortp_global_stats.sent += packsize;
	stream->sent_payload_bytes+=packsize-RTP_FIXED_HEADER_SIZE;
	stream->stats.sent += packsize;
	ortp_global_stats.packet_sent++;
	stream->stats.packet_sent++;

	error = rtp_session_rtp_send (session, mp);
	/*send RTCP packet if needed */
	rtp_session_rtcp_process_send(session);
	/* receives rtcp packet if session is send-only*/
	/*otherwise it is done in rtp_session_recvm_with_ts */
	if (session->mode==RTP_SESSION_SENDONLY) rtp_session_rtcp_recv(session);
	return error;
}

/**
 *	Send the rtp datagram @mp to the destination set by rtp_session_set_remote_addr() 
 *	with timestamp @timestamp. For audio data, the timestamp is the number
 *	of the first sample resulting of the data transmitted. See rfc1889 for details.
 *  The packet (@mp) is freed once it is sended.
 *
 *@param session a rtp session.
 *@param mp a rtp packet presented as a mblk_t.
 *@param timestamp the timestamp of the data to be sent.
 * @return the number of bytes sent over the network.
**/

int rtp_session_sendm_with_ts(RtpSession *session, mblk_t *packet, uint32_t timestamp){
	return __rtp_session_sendm_with_ts(session,packet,timestamp,timestamp);
}




/**
 *	Send a rtp datagram to the destination set by rtp_session_set_remote_addr() containing
 *	the data from @buffer with timestamp @userts. This is a high level function that uses
 *	rtp_session_create_packet() and rtp_session_sendm_with_ts() to send the data.
 *
 *@param session a rtp session.
 *@param buffer a buffer containing the data to be sent in a rtp packet.
 *@param len the length of the data buffer, in bytes.
 *@param userts	the timestamp of the data to be sent. Refer to the rfc to know what it is.
 *
 *@param return the number of bytes sent over the network.
**/
int
rtp_session_send_with_ts (RtpSession * session, const uint8_t * buffer, int len,
			  uint32_t userts)
{
	mblk_t *m;
	int err;
#ifdef USE_SENDMSG
	m=rtp_session_create_packet_with_data(session,(uint8_t*)buffer,len,NULL);
#else
	m = rtp_session_create_packet(session,RTP_FIXED_HEADER_SIZE,(uint8_t*)buffer,len);
#endif
	err=rtp_session_sendm_with_ts(session,m,userts);
	return err;
}



extern void rtcp_parse(RtpSession *session, mblk_t *mp);



static void payload_type_changed_notify(RtpSession *session, int paytype){
	PayloadType *pt = rtp_profile_get_payload(session->rcv.profile,paytype);
	if (pt) {
		session->rcv.pt = paytype;
		rtp_signal_table_emit (&session->on_payload_type_changed);
	}
}


/**
 *	Try to get a rtp packet presented as a mblk_t structure from the rtp session.
 *	The @user_ts parameter is relative to the first timestamp of the incoming stream. In other
 *	words, the application does not have to know the first timestamp of the stream, it can
 *	simply call for the first time this function with @user_ts=0, and then incrementing it
 *	as it want. The RtpSession takes care of synchronisation between the stream timestamp
 *	and the user timestamp given here.
 *
 *	This function returns the entire packet (with header).
 *
 *	The behaviour of this function has changed since version 0.15.0. Previously the payload data could be 
 *	accessed using  mblk_t::b_cont::b_rptr field of the returned mblk_t.
 *	This is no more the case.
 *	The convenient way of accessing the payload data is to use rtp_get_payload() :
 *	@code
 *	unsigned char *payload;
 *	int payload_size;
 *	payload_size=rtp_get_payload(mp,&payload);
 *	@endcode
 *	OR simply skip the header this way, the data is then comprised between mp->b_rptr and mp->b_wptr:
 *	@code
 *	rtp_get_payload(mp,&mp->b_rptr);
 *	@endcode
 *
 *
 * @param session a rtp session.
 * @param user_ts a timestamp.
 *
 * @return a rtp packet presented as a mblk_t.
**/

mblk_t *
rtp_session_recvm_with_ts (RtpSession * session, uint32_t user_ts)
{
	mblk_t *mp = NULL;
	rtp_header_t *rtp;
	uint32_t ts;
	uint32_t packet_time;
	RtpScheduler *sched=session->sched;
	RtpStream *stream=&session->rtp;
	int rejected=0;
	bool_t read_socket=TRUE;

	/* if we are scheduled, remember the scheduler time at which the application has
	 * asked for its first timestamp */

	if (session->flags & RTP_SESSION_RECV_NOT_STARTED)
	{
		session->rtp.rcv_query_ts_offset = user_ts;
		/* Set initial last_rcv_time to first recv time. */
		if ((session->flags & RTP_SESSION_SEND_NOT_STARTED)
		|| session->mode == RTP_SESSION_RECVONLY){
			gettimeofday(&session->last_recv_time, NULL);
		}
		if (session->flags & RTP_SESSION_SCHEDULED)
		{
			session->rtp.rcv_time_offset = sched->time_;
			//ortp_message("setting snd_time_offset=%i",session->rtp.snd_time_offset);
		}
		rtp_session_unset_flag (session,RTP_SESSION_RECV_NOT_STARTED);
	}else{
		/*prevent reading from the sockets when two 
		consecutives calls for a same timestamp*/
		if (user_ts==session->rtp.rcv_last_app_ts)
			read_socket=FALSE;
	}
	session->rtp.rcv_last_app_ts = user_ts;
	if (read_socket){
		rtp_session_rtp_recv (session, user_ts);
		rtp_session_rtcp_recv(session);
	}
	/* check for telephone event first */
	mp=getq(&session->rtp.tev_rq);
	if (mp!=NULL){
		int msgsize=msgdsize(mp);
		ortp_global_stats.recv += msgsize;
		stream->stats.recv += msgsize;
		rtp_signal_table_emit2(&session->on_telephone_event_packet,(long)mp);
		rtp_session_check_telephone_events(session,mp);
		freemsg(mp);
		mp=NULL;
	}
	
	/* then now try to return a media packet, if possible */
	/* first condition: if the session is starting, don't return anything
	 * until the queue size reaches jitt_comp */
	
	if (session->flags & RTP_SESSION_RECV_SYNC)
	{
		queue_t *q = &session->rtp.rq;
		if (qempty(q))
		{
			ortp_debug ("Queue is empty.");
			goto end;
		}
		rtp = (rtp_header_t *) qfirst(q)->b_rptr;
		session->rtp.rcv_ts_offset = rtp->timestamp;
		session->rtp.rcv_last_ret_ts = user_ts;	/* just to have an init value */
		session->rcv.ssrc = rtp->ssrc;
		/* delete the recv synchronisation flag */
		rtp_session_unset_flag (session, RTP_SESSION_RECV_SYNC);
	}

	/*calculate the stream timestamp from the user timestamp */
	ts = jitter_control_get_compensated_timestamp(&session->rtp.jittctl,user_ts);
	if (session->rtp.jittctl.enabled==TRUE){
		if (session->permissive)
			mp = rtp_getq_permissive(&session->rtp.rq, ts,&rejected);
		else{
			mp = rtp_getq(&session->rtp.rq, ts,&rejected);
		}
	}else mp=getq(&session->rtp.rq);/*no jitter buffer at all*/
	
	stream->stats.outoftime+=rejected;
	ortp_global_stats.outoftime+=rejected;

	goto end;

      end:
	if (mp != NULL)
	{
		int msgsize = msgdsize (mp);	/* evaluate how much bytes (including header) is received by app */
		uint32_t packet_ts;
		ortp_global_stats.recv += msgsize;
		stream->stats.recv += msgsize;
		rtp = (rtp_header_t *) mp->b_rptr;
		packet_ts=rtp->timestamp;
		ortp_debug("Returning mp with ts=%i", packet_ts);
		/* check for payload type changes */
		if (session->rcv.pt != rtp->paytype)
		{
			payload_type_changed_notify(session, rtp->paytype);
		}
		/* update the packet's timestamp so that it corrected by the 
		adaptive jitter buffer mechanism */
		if (session->rtp.jittctl.adaptive){
			uint32_t changed_ts;
			/* only update correction offset between packets of different
			timestamps*/
			if (packet_ts!=session->rtp.rcv_last_ts)
				jitter_control_update_corrective_slide(&session->rtp.jittctl);
			changed_ts=packet_ts+session->rtp.jittctl.corrective_slide;
			rtp->timestamp=changed_ts;
			/*ortp_debug("Returned packet has timestamp %u, with clock slide compensated it is %u",packet_ts,rtp->timestamp);*/
		}
		session->rtp.rcv_last_ts = packet_ts;
		if (!(session->flags & RTP_SESSION_FIRST_PACKET_DELIVERED)){
			rtp_session_set_flag(session,RTP_SESSION_FIRST_PACKET_DELIVERED);
		}
	}
	else
	{
		ortp_debug ("No mp for timestamp queried");
		stream->stats.unavaillable++;
		ortp_global_stats.unavaillable++;
	}
	rtp_session_rtcp_process_recv(session);
	
	if (session->flags & RTP_SESSION_SCHEDULED)
	{
		/* if we are in blocking mode, then suspend the calling process until timestamp
		 * wanted expires */
		/* but we must not block the process if the timestamp wanted by the application is older
		 * than current time */
		wait_point_lock(&session->rcv.wp);
		packet_time =
			rtp_session_ts_to_time (session,
				     user_ts -
				     session->rtp.rcv_query_ts_offset) +
			session->rtp.rcv_time_offset;
		ortp_debug ("rtp_session_recvm_with_ts: packet_time=%i, time=%i",packet_time, sched->time_);
		
		if (TIME_IS_STRICTLY_NEWER_THAN (packet_time, sched->time_))
		{
			wait_point_wakeup_at(&session->rcv.wp,packet_time, (session->flags & RTP_SESSION_BLOCKING_MODE)!=0);
			session_set_clr(&sched->r_sessions,session);
		}
		else session_set_set(&sched->r_sessions,session);	/*to unblock _select() immediately */
		wait_point_unlock(&session->rcv.wp);
	}
	return mp;
}


/**
 *	NOTE: use of this function is discouraged when sending payloads other than
 *	pcm/pcmu/pcma/adpcm types.
 *	rtp_session_recvm_with_ts() does better job.
 *
 *	Tries to read the bytes of the incoming rtp stream related to timestamp ts. In case 
 *	where the user supplied buffer @buffer is not large enough to get all the data 
 *	related to timestamp ts, then *( have_more) is set to 1 to indicate that the application
 *	should recall the function with the same timestamp to get more data.
 *	
 *  When the rtp session is scheduled (see rtp_session_set_scheduling_mode() ), and the 
 *	blocking mode is on (see rtp_session_set_blocking_mode() ), then the calling thread
 *	is suspended until the timestamp given as argument expires, whatever a received packet 
 *	fits the query or not.
 *
 *	Important note: it is clear that the application cannot know the timestamp of the first
 *	packet of the incoming stream, because it can be random. The @ts timestamp given to the
 *	function is used relatively to first timestamp of the stream. In simple words, 0 is a good
 *	value to start calling this function.
 *
 *	This function internally calls rtp_session_recvm_with_ts() to get a rtp packet. The content
 *	of this packet is then copied into the user supplied buffer in an intelligent manner:
 *	the function takes care of the size of the supplied buffer and the timestamp given in  
 *	argument. Using this function it is possible to read continous audio data (e.g. pcma,pcmu...)
 *	with for example a standart buffer of size of 160 with timestamp incrementing by 160 while the incoming
 *	stream has a different packet size.
 *
 *Returns: if a packet was availlable with the corresponding timestamp supplied in argument 
 *	then the number of bytes written in the user supplied buffer is returned. If no packets
 *	are availlable, either because the sender has not started to send the stream, or either
 *	because silence packet are not transmitted, or either because the packet was lost during
 *	network transport, then the function returns zero.
 *@param session a rtp session.
 *@param buffer a user supplied buffer to write the data.
 *@param len the length in bytes of the user supplied buffer.
 *@param ts the timestamp wanted.
 *@param have_more the address of an integer to indicate if more data is availlable for the given timestamp.
 *
**/
int rtp_session_recv_with_ts (RtpSession * session, uint8_t * buffer,
			       int len, uint32_t ts, int * have_more){
	mblk_t *mp=NULL;
	int plen,blen=0;
	*have_more=0;
	while(1){
		if (session->pending){
			mp=session->pending;
			session->pending=NULL;
		}else {
			mp=rtp_session_recvm_with_ts(session,ts);
			if (mp!=NULL) rtp_get_payload(mp,&mp->b_rptr);
		}
		if (mp){
			plen=mp->b_wptr-mp->b_rptr;
			if (plen<=len){
				memcpy(buffer,mp->b_rptr,plen);
				buffer+=plen;
				blen+=plen;
				len-=plen;
				freemsg(mp);
				mp=NULL;
			}else{
				memcpy(buffer,mp->b_rptr,len);
				mp->b_rptr+=len;
				buffer+=len;
				blen+=len;
				len=0;
				session->pending=mp;
				*have_more=1;
				break;
			}
		}else break;
	}
	return blen;
}
/**
 *	When the rtp session is scheduled and has started to send packets, this function
 *	computes the timestamp that matches to the present time. Using this function can be 
 *	usefull when sending discontinuous streams. Some time can be elapsed between the end
 *	of a stream burst and the begin of a new stream burst, and the application may be not
 *	not aware of this elapsed time. In order to get a valid (current) timestamp to pass to 
 *	#rtp_session_send_with_ts() or #rtp_session_sendm_with_ts(), the application may
 *	use rtp_session_get_current_send_ts().
 *
 * @param session a rtp session.
 * @return the current send timestamp for the rtp session.
**/
uint32_t rtp_session_get_current_send_ts(RtpSession *session)
{
	uint32_t userts;
	uint32_t session_time;
	RtpScheduler *sched=session->sched;
	PayloadType *payload;
	payload=rtp_profile_get_payload(session->snd.profile,session->snd.pt);
	return_val_if_fail(payload!=NULL, 0);
	if ( (session->flags & RTP_SESSION_SCHEDULED)==0 ){
		ortp_warning("can't guess current timestamp because session is not scheduled.");
		return 0;
	}
	session_time=sched->time_-session->rtp.snd_time_offset;
	userts=  (uint32_t)( ( (double)(session_time) * (double) payload->clock_rate )/ 1000.0)
				+ session->rtp.snd_ts_offset;
	return userts;
}

/**
 * Same thing as rtp_session_get_current_send_ts() except that it's for an incoming stream.
 * Works only on scheduled mode.
 *
 * @param session a rtp session.
 * @return the theoritical that would have to be receive now.
 *
**/
uint32_t rtp_session_get_current_recv_ts(RtpSession *session){
	uint32_t userts;
	uint32_t session_time;
	RtpScheduler *sched=ortp_get_scheduler();
	PayloadType *payload;
	payload=rtp_profile_get_payload(session->rcv.profile,session->rcv.pt);
	return_val_if_fail(payload!=NULL, 0);
	if ( (session->flags & RTP_SESSION_SCHEDULED)==0 ){
		ortp_warning("can't guess current timestamp because session is not scheduled.");
		return 0;
	}
	session_time=sched->time_-session->rtp.rcv_time_offset;
	userts=  (uint32_t)( ( (double)(session_time) * (double) payload->clock_rate )/ 1000.0)
				+ session->rtp.rcv_ts_offset;
	return userts;
}

/**
 * oRTP has the possibility to inform the application through a callback registered 
 * with rtp_session_signal_connect about crazy incoming RTP stream that jumps from 
 * a timestamp N to N+some_crazy_value. This lets the opportunity for the application
 * to reset the session in order to resynchronize, or any other action like stopping the call
 * and reporting an error.
 * @param session the rtp session
 * @param ts_step a time interval in miliseconds
 *
**/
void rtp_session_set_time_jump_limit(RtpSession *session, int milisecs){
	uint32_t ts;
	session->rtp.time_jump=milisecs;
	ts=rtp_session_time_to_ts(session,milisecs);
	if (ts==0) session->rtp.ts_jump=1<<31;	/* do not detect ts jump */
	else session->rtp.ts_jump=ts;
}

/**
 * Closes the rtp and rtcp sockets.
**/
void rtp_session_release_sockets(RtpSession *session){
	if (session->rtp.socket>=0) close_socket (session->rtp.socket);
	if (session->rtcp.socket>=0) close_socket (session->rtcp.socket);
	session->rtp.socket=-1;
	session->rtcp.socket=-1;
	
	session->rtp.tr = 0;
	session->rtcp.tr = 0;

	/* don't discard remote addresses, then can be preserved for next use.
	session->rtp.rem_addrlen=0;
	session->rtcp.rem_addrlen=0;
	*/
}

ortp_socket_t rtp_session_get_rtp_socket(const RtpSession *session){
	return rtp_session_using_transport(session, rtp) ? (session->rtp.tr->t_getsocket)(session->rtp.tr) : session->rtp.socket;
}

ortp_socket_t rtp_session_get_rtcp_socket(const RtpSession *session){
	return rtp_session_using_transport(session, rtcp) ? (session->rtcp.tr->t_getsocket)(session->rtcp.tr) : session->rtcp.socket;
}

/**
 * Register an event queue.
 * An application can use an event queue to get informed about various RTP events.
**/
void rtp_session_register_event_queue(RtpSession *session, OrtpEvQueue *q){
	session->eventqs=o_list_append(session->eventqs,q);
}

void rtp_session_unregister_event_queue(RtpSession *session, OrtpEvQueue *q){
	session->eventqs=o_list_remove(session->eventqs,q);
}

void rtp_session_dispatch_event(RtpSession *session, OrtpEvent *ev){
	OList *it;
	int i;
	for(i=0,it=session->eventqs;it!=NULL;it=it->next,++i){
		ortp_ev_queue_put((OrtpEvQueue*)it->data,ortp_event_dup(ev));
	}	
	ortp_event_destroy(ev);
}


void rtp_session_uninit (RtpSession * session)
{
	/* first of all remove the session from the scheduler */
	if (session->flags & RTP_SESSION_SCHEDULED)
	{
		rtp_scheduler_remove_session (session->sched,session);
	}
	/*flush all queues */
	flushq(&session->rtp.rq, FLUSHALL);
	flushq(&session->rtp.tev_rq, FLUSHALL);

	if (session->eventqs!=NULL) o_list_free(session->eventqs);
	/* close sockets */
	rtp_session_release_sockets(session);

	wait_point_uninit(&session->snd.wp);
	wait_point_uninit(&session->rcv.wp);
	if (session->current_tev!=NULL) freemsg(session->current_tev);
	if (session->rtp.cached_mp!=NULL) freemsg(session->rtp.cached_mp);
	if (session->rtcp.cached_mp!=NULL) freemsg(session->rtcp.cached_mp);
	if (session->sd!=NULL) freemsg(session->sd);

	session->signal_tables = o_list_free(session->signal_tables);
	msgb_allocator_uninit(&session->allocator);
}

/**
 * Resynchronize to the incoming RTP streams.
 * This can be useful to handle discoutinuous timestamps.
 * For example, call this function from the timestamp_jump signal handler.
 * @param session the rtp session
**/
void rtp_session_resync(RtpSession *session){
	flushq (&session->rtp.rq, FLUSHALL);
	rtp_session_set_flag(session, RTP_SESSION_RECV_SYNC);
	rtp_session_unset_flag(session,RTP_SESSION_FIRST_PACKET_DELIVERED);
	jitter_control_init(&session->rtp.jittctl,-1,NULL);
}

/**
 * Reset the session: local and remote addresses are kept. It resets timestamp, sequence 
 * number, and calls rtp_session_resync().
 *
 * @param session a rtp session.
**/
void rtp_session_reset (RtpSession * session)
{
	rtp_session_set_flag (session, RTP_SESSION_RECV_NOT_STARTED);
	rtp_session_set_flag (session, RTP_SESSION_SEND_NOT_STARTED);
	//session->ssrc=0;
	session->rtp.snd_time_offset = 0;
	session->rtp.snd_ts_offset = 0;
	session->rtp.snd_rand_offset = 0;
	session->rtp.snd_last_ts = 0;
	session->rtp.rcv_time_offset = 0;
	session->rtp.rcv_ts_offset = 0;
	session->rtp.rcv_query_ts_offset = 0;
	session->rtp.rcv_last_ts = 0;
	session->rtp.rcv_last_app_ts = 0;
	session->rtp.hwrcv_extseq = 0;
	session->rtp.hwrcv_since_last_SR=0;
	session->rtp.snd_seq = 0;
	session->rtp.sent_payload_bytes=0;
	rtp_session_clear_send_error_code(session);
	rtp_session_clear_recv_error_code(session);
	rtp_stats_reset(&session->rtp.stats);
	rtp_session_resync(session);
	session->ssrc_set=FALSE;
}

/**
 * Retrieve the session's statistics.
**/
const rtp_stats_t * rtp_session_get_stats(const RtpSession *session){
	return &session->rtp.stats;
}

void rtp_session_reset_stats(RtpSession *session){
	memset(&session->rtp.stats,0,sizeof(rtp_stats_t));
}

/**
 * Stores some application specific data into the session, so that it is easy to retrieve it from the signal callbacks using rtp_session_get_data().
 * @param session a rtp session
 * @param data an opaque pointer to be stored in the session
**/

void rtp_session_set_data(RtpSession *session, void *data){
	session->user_data=data;
}

/**
 * @param session a rtp session
 * @return the void pointer previously set using rtp_session_set_data()
**/
void *rtp_session_get_data(const RtpSession *session){
	return session->user_data;
}

/**
 * Enable or disable the "rtp symmetric" hack which consists of the following:
 * after the first packet is received, the source address of the packet 
 * is set to be the destination address for all next packets.
 * This is useful to pass-through firewalls.
 * @param session a rtp session
 * @param yesno a boolean to enable or disable the feature
 *
**/
void
rtp_session_set_symmetric_rtp (RtpSession * session, bool_t yesno)
{
	session->symmetric_rtp =yesno;
}

/**
 *	If yesno is TRUE, thus a connect() syscall is done on the socket to 
 *	the destination address set by rtp_session_set_remote_addr(), or
 *	if the session does symmetric rtp (see rtp_session_set_symmetric_rtp())
 *	a the connect() is done to the source address of the first packet received.
 *	Connecting a socket has effect of rejecting all incoming packets that 
 *	don't come from the address specified in connect().
 *	It also makes ICMP errors (such as connection refused) available to the
 *	application.
 *	@param session a rtp session
 *	@param yesno a boolean to enable or disable the feature
 *
**/
void rtp_session_set_connected_mode(RtpSession *session, bool_t yesno){
	session->use_connect=yesno;
}

static float compute_bw(struct timeval *orig, unsigned int bytes){
	struct timeval current;
	float bw;
	float time;
	if (bytes==0) return 0;
	gettimeofday(&current,NULL);
	time=(float)(current.tv_sec - orig->tv_sec) +
		((float)(current.tv_usec - orig->tv_usec)*1e-6);
	bw=((float)bytes)*8/(time+0.001); 
	/*+0.0001 avoids a division by zero without changing the results significatively*/
	return bw;
}

float rtp_session_compute_recv_bandwidth(RtpSession *session){
	float bw;
	bw=compute_bw(&session->rtp.recv_bw_start,session->rtp.recv_bytes);
	session->rtp.recv_bytes=0;
	return bw;
}

float rtp_session_compute_send_bandwidth(RtpSession *session){
	float bw;
	bw=compute_bw(&session->rtp.send_bw_start,session->rtp.sent_bytes);
	session->rtp.sent_bytes=0;
	return bw;
}

int rtp_session_get_last_send_error_code(RtpSession *session){
	return session->rtp.send_errno;
}

void rtp_session_clear_send_error_code(RtpSession *session){
	session->rtp.send_errno=0;
}

int rtp_session_get_last_recv_error_code(RtpSession *session){
	return session->rtp.recv_errno;
}

void rtp_session_clear_recv_error_code(RtpSession *session){
	session->rtp.send_errno=0;
}

/**
 * Destroys a rtp session.
 * All memory allocated for the RtpSession is freed.
 *
 * @param session a rtp session.
**/
void rtp_session_destroy (RtpSession * session)
{
	rtp_session_uninit (session);
	ortp_free (session);
}

void rtp_session_make_time_distorsion(RtpSession *session, int milisec)
{
	session->rtp.snd_time_offset+=milisec;
}


/* packet api */

void rtp_add_csrc(mblk_t *mp, uint32_t csrc)
{
	rtp_header_t *hdr=(rtp_header_t*)mp->b_rptr;
	hdr->csrc[hdr->cc]=csrc;
	hdr->cc++;
}

/**
 * Get a pointer to the beginning of the payload data of the RTP packet.
 * @param packet a RTP packet represented as a mblk_t
 * @param start a pointer to the beginning of the payload data, pointing inside the packet.
 * @return the length of the payload data.
**/
int rtp_get_payload(mblk_t *packet, unsigned char **start){
	unsigned char *tmp;
	int header_len=RTP_FIXED_HEADER_SIZE+(rtp_get_cc(packet)*4);
	tmp=packet->b_rptr+header_len;
	if (tmp>packet->b_wptr){
		if (packet->b_cont!=NULL){
			tmp=packet->b_cont->b_rptr+(header_len- (packet->b_wptr-packet->b_rptr));
			if (tmp<=packet->b_cont->b_wptr){
				*start=tmp;
				return packet->b_cont->b_wptr-tmp;
			}
		}
		ortp_warning("Invalid RTP packet");
		return -1;
	}
	*start=tmp;
	return packet->b_wptr-tmp;
}


/**
 *  Gets last time a valid RTP or RTCP packet was received.
 * @param session RtpSession to get last receive time from.
 * @param tv Pointer to struct timeval to fill.
 *
**/
void
rtp_session_get_last_recv_time(RtpSession *session, struct timeval *tv)
{
#ifdef PERF
	ortp_error("rtp_session_get_last_recv_time() feature disabled.");
#else
    	*tv = session->last_recv_time;
#endif
}



uint32_t rtp_session_time_to_ts(RtpSession *session, int millisecs){
	PayloadType *payload;
	payload =
		rtp_profile_get_payload (session->snd.profile,
					 session->snd.pt);
	if (payload == NULL)
	{
		ortp_warning
			("rtp_session_ts_to_t: use of unsupported payload type %d.", session->snd.pt);
		return 0;
	}
	/* the return value is in milisecond */
	return (uint32_t) (payload->clock_rate*(double) (millisecs/1000.0f));
}

/* function used by the scheduler only:*/
uint32_t rtp_session_ts_to_time (RtpSession * session, uint32_t timestamp)
{
	PayloadType *payload;
	payload =
		rtp_profile_get_payload (session->snd.profile,
					 session->snd.pt);
	if (payload == NULL)
	{
		ortp_warning
			("rtp_session_ts_to_t: use of unsupported payload type %d.", session->snd.pt);
		return 0;
	}
	/* the return value is in milisecond */
	return (uint32_t) (1000.0 *
			  ((double) timestamp /
			   (double) payload->clock_rate));
}


/* time is the number of miliseconds elapsed since the start of the scheduler */
void rtp_session_process (RtpSession * session, uint32_t time, RtpScheduler *sched)
{
	wait_point_lock(&session->snd.wp);
	if (wait_point_check(&session->snd.wp,time)){
		session_set_set(&sched->w_sessions,session);
		wait_point_wakeup(&session->snd.wp);
	}
	wait_point_unlock(&session->snd.wp);
	
	wait_point_lock(&session->rcv.wp);
	if (wait_point_check(&session->rcv.wp,time)){
		session_set_set(&sched->r_sessions,session);
		wait_point_wakeup(&session->rcv.wp);
	}
	wait_point_unlock(&session->rcv.wp);
}

