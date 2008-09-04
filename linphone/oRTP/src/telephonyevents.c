/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc1889) stack.
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

#include <ortp/telephonyevents.h>
#include "utils.h"
#include "rtpsession_priv.h"
#include <ortp/ortp.h>

PayloadType	payload_type_telephone_event={
	PAYLOAD_AUDIO_PACKETIZED, /*type */
	8000,	/*clock rate */
	0,		/* bytes per sample N/A */
	NULL,	/* zero pattern N/A*/
	0,		/*pattern_length N/A */
	0,		/*	normal_bitrate */
	"telephone-event",	/* MIME subtype */
	0,		/* Audio Channels N/A */
	0		/*flags */
};

/**
 * Tells whether telephony events payload type is supported within the 
 * context of the rtp session.
 * @param session a rtp session 
 *
 * @return the payload type number used for telephony events if found, -1 if not found.
**/
int rtp_session_telephone_events_supported(RtpSession *session)
{
	/* search for a telephony event payload in the current profile */
	session->snd.telephone_events_pt=rtp_profile_get_payload_number_from_mime(session->snd.profile,"telephone-event");
	session->rcv.telephone_events_pt=rtp_profile_get_payload_number_from_mime(session->rcv.profile,"telephone-event");
	/*printf("Telephone event pt is %i\n",session->telephone_events_pt);*/
	return session->snd.telephone_events_pt;
}


/**
 * Tells whether telephone event payload type is supported for send within the 
 * context of the rtp session.
 * @param session a rtp session 
 *
 * @return the payload type number used for telephony events if found, -1 if not found.
**/
int rtp_session_send_telephone_events_supported(RtpSession *session)
{
	/* search for a telephony event payload in the current profile */
	session->snd.telephone_events_pt=rtp_profile_get_payload_number_from_mime(session->snd.profile,"telephone-event");
	/*printf("Telephone event pt is %i\n",session->telephone_events_pt);*/
	return session->snd.telephone_events_pt;
}

/**
 * Tells whether telephone event payload type is supported for receiving within the 
 * context of the rtp session.
 * @param session a rtp session 
 *
 * @return the payload type number used for telephony events if found, -1 if not found.
**/int rtp_session_recv_telephone_events_supported(RtpSession *session)
{
	/* search for a telephony event payload in the current profile */
	session->rcv.telephone_events_pt=rtp_profile_get_payload_number_from_mime(session->rcv.profile,"telephone-event");
	/*printf("Telephone event pt is %i\n",session->telephone_events_pt);*/
	return session->snd.telephone_events_pt;
}


/**
 *	Allocates a new rtp packet to be used to add named telephony events. The application can use
 *	then rtp_session_add_telephone_event() to add named events to the packet.
 *	Finally the packet has to be sent with rtp_session_sendm_with_ts().
 *
 * @param session a rtp session.
 * @param start boolean to indicate if the marker bit should be set.
 *
 * @return a message block containing the rtp packet if successfull, NULL if the rtp session
 *cannot support telephony event (because the rtp profile it is bound to does not include
 *a telephony event payload type).
**/
mblk_t	*rtp_session_create_telephone_event_packet(RtpSession *session, int start)
{
	mblk_t *mp;
	rtp_header_t *rtp;
	
	return_val_if_fail(session->snd.telephone_events_pt!=-1,NULL);
	
	mp=allocb(RTP_FIXED_HEADER_SIZE+TELEPHONY_EVENTS_ALLOCATED_SIZE,BPRI_MED);
	if (mp==NULL) return NULL;
	rtp=(rtp_header_t*)mp->b_rptr;
	rtp->version = 2;
	rtp->markbit=start;
	rtp->padbit = 0;
	rtp->extbit = 0;
	rtp->cc = 0;
	rtp->ssrc = session->snd.ssrc;
	/* timestamp set later, when packet is sended */
	/*seq number set later, when packet is sended */
	
	/*set the payload type */
	rtp->paytype=session->snd.telephone_events_pt;
	
	/*copy the payload */
	mp->b_wptr+=RTP_FIXED_HEADER_SIZE;
	return mp;
}


/**
 *@param session a rtp session.
 *@param packet a rtp packet as a mblk_t
 *@param event the event type as described in rfc2833, ie one of the TEV_* macros.
 *@param end a boolean to indicate if the end bit should be set. (end of tone)
 *@param volume the volume of the telephony tone, as described in rfc2833
 *@param duration the duration of the telephony tone, in timestamp unit.
 *
 * Adds a named telephony event to a rtp packet previously allocated using
 * rtp_session_create_telephone_event_packet().
 *
 *@return 0 on success.
**/
int rtp_session_add_telephone_event(RtpSession *session,
			mblk_t *packet, uint8_t event, int end, uint8_t volume, uint16_t duration)
{
	mblk_t *mp=packet;
	telephone_event_t *event_hdr;


	/* find the place where to add the new telephony event to the packet */
	while(mp->b_cont!=NULL) mp=mp->b_cont;
	/* see if we need to allocate a new mblk_t */
	if ( ( mp->b_wptr) >= (mp->b_datap->db_lim)){
		mblk_t *newm=allocb(TELEPHONY_EVENTS_ALLOCATED_SIZE,BPRI_MED);
		mp->b_cont=newm;
		mp=mp->b_cont;
	}
	if (mp==NULL) return -1;
	event_hdr=(telephone_event_t*)mp->b_wptr;
	event_hdr->event=event;
	event_hdr->R=0;
	event_hdr->E=end;
	event_hdr->volume=volume;
	event_hdr->duration=htons(duration);
	mp->b_wptr+=sizeof(telephone_event_t);
	return 0;
}
/**
 *	This functions creates telephony events packets for dtmf and sends them.
 *	It uses rtp_session_create_telephone_event_packet() and
 *	rtp_session_add_telephone_event() to create them and finally
 *	rtp_session_sendm_with_ts() to send them.
 *
 * @param session a rtp session
 * @param dtmf a character meaning the dtmf (ex: '1', '#' , '9' ...)
 * @param userts the timestamp
 * @return 0 if successfull, -1 if the session cannot support telephony events or if the dtmf given as argument is not valid.
**/
int rtp_session_send_dtmf(RtpSession *session, char dtmf, uint32_t userts)
{
  return rtp_session_send_dtmf2(session, dtmf, userts, 480);
}

/**
 * A variation of rtp_session_send_dtmf() with duration specified.
 *
 * @param session a rtp session
 * @param dtmf a character meaning the dtmf (ex: '1', '#' , '9' ...)
 * @param userts the timestamp
 * @param duration duration of the dtmf in timestamp units
 * @return 0 if successfull, -1 if the session cannot support telephony events or if the dtmf given as argument is not valid.
**/
int rtp_session_send_dtmf2(RtpSession *session, char dtmf, uint32_t userts, int duration)
{
	mblk_t *m1,*m2,*m3;
	int tev_type;
	int durationtier = duration/3;

	/* create the first telephony event packet */
	switch (dtmf){
		case '1':
			tev_type=TEV_DTMF_1;
		break;
		case '2':
			tev_type=TEV_DTMF_2;
		break;
		case '3':
			tev_type=TEV_DTMF_3;
		break;
		case '4':
			tev_type=TEV_DTMF_4;
		break;
		case '5':
			tev_type=TEV_DTMF_5;
		break;
		case '6':
			tev_type=TEV_DTMF_6;
		break;
		case '7':
			tev_type=TEV_DTMF_7;
		break;
		case '8':
			tev_type=TEV_DTMF_8;
		break;
		case '9':
			tev_type=TEV_DTMF_9;
		break;
		case '*':
			tev_type=TEV_DTMF_STAR;
		break;
		case '0':
			tev_type=TEV_DTMF_0;
		break;
		case '#':
			tev_type=TEV_DTMF_POUND;
		break;

		case 'A':
		case 'a':
		  tev_type=TEV_DTMF_A;
		  break;


		case 'B':
		case 'b':
		  tev_type=TEV_DTMF_B;
		  break;

		case 'C':
		case 'c':
		  tev_type=TEV_DTMF_C;
		  break;

		case 'D':
		case 'd':
		  tev_type=TEV_DTMF_D;
		  break;

		case '!':
		  tev_type=TEV_FLASH;
		  break;


		default:
		ortp_warning("Bad dtmf: %c.",dtmf);
		return -1;
	}

	m1=rtp_session_create_telephone_event_packet(session,1);
	if (m1==NULL) return -1;
	rtp_session_add_telephone_event(session,m1,tev_type,0,10,durationtier);
	/* create a second packet */
	m2=rtp_session_create_telephone_event_packet(session,0);
	if (m2==NULL) return -1;
	rtp_session_add_telephone_event(session,m2,tev_type,0,10, durationtier+durationtier);
		
	/* create a third and final packet */
	m3=rtp_session_create_telephone_event_packet(session,0);
	if (m3==NULL) return -1;
	rtp_session_add_telephone_event(session,m3,tev_type,1,10,duration);
	
	/* and now sends them */
	rtp_session_sendm_with_ts(session,m1,userts);
	rtp_session_sendm_with_ts(session,m2,userts);
	/* the last packet is sent three times in order to improve reliability*/
	m1=copymsg(m3);
	m2=copymsg(m3);
	/*			NOTE:			*/
	/* we need to copymsg() instead of dupmsg() because the buffers are modified when
	the packet is sended because of the host-to-network conversion of timestamp,ssrc, csrc, and
	seq number.
	*/
	rtp_session_sendm_with_ts(session,m3,userts);
	session->rtp.snd_seq--;
	rtp_session_sendm_with_ts(session,m1,userts);
	session->rtp.snd_seq--;
	rtp_session_sendm_with_ts(session,m2,userts);
	return 0;
}


/**
 *	Reads telephony events from a rtp packet. *@tab points to the beginning of the event buffer.
 *
 * @param session a rtp session from which telephony events are received.
 * @param packet a rtp packet as a mblk_t.
 * @param tab the address of a pointer.
 * @return the number of events in the packet if successfull, 0 if the packet did not contain telephony events.
**/
int rtp_session_read_telephone_event(RtpSession *session,
		mblk_t *packet,telephone_event_t **tab)
{
	int datasize;
	int num;
	int i;
	telephone_event_t *tev;
	rtp_header_t *hdr=(rtp_header_t*)packet->b_rptr;
	unsigned char *payload;
	if (hdr->paytype!=session->rcv.telephone_events_pt) return 0;  /* this is not tel ev.*/
	datasize=rtp_get_payload(packet,&payload);
	tev=*tab=(telephone_event_t*)payload;
	/* convert from network to host order what should be */
	num=datasize/sizeof(telephone_event_t);
	for (i=0;i<num;i++)
	{
		tev[i].duration=ntohs(tev[i].duration);
	}
	return num;
}

static void notify_tev(RtpSession *session, telephone_event_t *event){
	OrtpEvent *ev;
	OrtpEventData *evd;
	rtp_signal_table_emit2(&session->on_telephone_event,(long)(long)event[0].event);
	if (session->eventqs!=NULL){
		ev=ortp_event_new(ORTP_EVENT_TELEPHONE_EVENT);
		evd=ortp_event_get_data(ev);
		evd->packet=dupmsg(session->current_tev);
		evd->info.telephone_event=event[0].event;
		rtp_session_dispatch_event(session,ev);
	}
}

static void notify_events_ended(RtpSession *session, telephone_event_t *events, int num){
	int i;
	for (i=0;i<num;i++){
		if (events[i].E==1){
			notify_tev(session, &events[i]);
		}
	}
}

/* for high level telephony event callback */
void rtp_session_check_telephone_events(RtpSession *session, mblk_t *m0)
{
	telephone_event_t *events,*evbuf;
	int num,num2;
	int i;
	rtp_header_t *hdr;
	mblk_t *cur_tev;
	unsigned char *payload;
	int datasize;
	
	hdr=(rtp_header_t*)m0->b_rptr;
	
	datasize=rtp_get_payload(m0,&payload);

	num=datasize/sizeof(telephone_event_t);
	events=(telephone_event_t*)payload;
	
	
	if (hdr->markbit==1)
	{
		/* this is a start of new events. Store the event buffer for later use*/
		if (session->current_tev!=NULL) {
			freemsg(session->current_tev);
			session->current_tev=NULL;
		}
		session->current_tev=copymsg(m0);
		/* handle the case where the events are short enough to end within the packet that has the marker bit*/
		notify_events_ended(session,events,num);
	}
	/* whatever there is a markbit set or not, we parse the packet and compare it to previously received one */
	cur_tev=session->current_tev;
	if (cur_tev!=NULL)
	{
		/* first compare timestamp, they must be identical */
		if (((rtp_header_t*)cur_tev->b_rptr)->timestamp==
			((rtp_header_t*)m0->b_rptr)->timestamp)
		{
			datasize=rtp_get_payload(cur_tev,&payload);
			num2=datasize/sizeof(telephone_event_t);
			evbuf=(telephone_event_t*)payload;
			for (i=0;i<MIN(num,num2);i++)
			{
				if (events[i].E==1)
				{
					/* update events that have ended */
					if (evbuf[i].E==0){
						evbuf[i].E=1;
						/* this is a end of event, report it */
						notify_tev(session,&events[i]);
					}
				}
			}
		}
		else
		{
			/* timestamp are not identical: this is not the same events*/
			if (session->current_tev!=NULL) {
				freemsg(session->current_tev);
				session->current_tev=NULL;
			}
			session->current_tev=copymsg(m0);
			notify_events_ended(session,events,num);
		}
	}
	else
	{
		/* there is no pending events, but we did not received marked bit packet
		either the sending implementation is not compliant, either it has been lost, 
		we must deal with it anyway.*/
		session->current_tev=copymsg(m0);
		/* inform the application if there are tone ends */
		notify_events_ended(session,events,num);
	}
}
