/*
  The mediastreamer library aims at providing modular media processing and I/O
	for linphone, but also for any telephony application.
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


#include "msrtprecv.h"


/* some utilities to convert mblk_t to MSMessage and vice-versa */
MSMessage *msgb_2_ms_message(mblk_t* mp){
	MSMessage *msg;
	MSBuffer *msbuf;
	if (mp->b_datap->ref_count!=1) return NULL; /* cannot handle properly non-unique buffers*/
	/* create a MSBuffer using the mblk_t buffer */
	msg=ms_message_alloc();
	msbuf=ms_buffer_new_with_buf(mp->b_datap->db_base,mp->b_datap->db_lim-mp->b_datap->db_base,
													freemsg,mp);
	ms_message_set_buf(msg,msbuf);
	msg->size=mp->b_wptr-mp->b_rptr;
	msg->data=mp->b_rptr;
	return msg;
}


static MSRtpRecvClass *ms_rtp_recv_class=NULL;

MSFilter * ms_rtp_recv_new(void)
{
	MSRtpRecv *r;
	
	r=g_new(MSRtpRecv,1);
	ms_rtp_recv_init(r);
	if (ms_rtp_recv_class==NULL)
	{
		ms_rtp_recv_class=g_new0(MSRtpRecvClass,1);
		ms_rtp_recv_class_init(ms_rtp_recv_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_rtp_recv_class);
	return(MS_FILTER(r));
}
	

/* FOR INTERNAL USE*/
void ms_rtp_recv_init(MSRtpRecv *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->outfifos=r->f_outputs;
	MS_FILTER(r)->outqueues=r->q_outputs;
	memset(r->f_outputs,0,sizeof(MSFifo*)*MSRTPRECV_MAX_OUTPUTS);
	memset(r->q_outputs,0,sizeof(MSFifo*)*MSRTPRECV_MAX_OUTPUTS);
	r->rtpsession=NULL;
	r->stream_started=0;
	r->ignore=FALSE;
	r->payload_expected=0;
}

void ms_rtp_recv_class_init(MSRtpRecvClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"RTPRecv");
	MS_FILTER_CLASS(klass)->max_qoutputs=MSRTPRECV_MAX_OUTPUTS;
	MS_FILTER_CLASS(klass)->max_foutputs=MSRTPRECV_MAX_OUTPUTS;
	MS_FILTER_CLASS(klass)->w_maxgran=MSRTPRECV_DEF_GRAN;
	ms_filter_class_set_attr(MS_FILTER_CLASS(klass),FILTER_IS_SOURCE);
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_rtp_recv_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_rtp_recv_process;
	MS_FILTER_CLASS(klass)->setup=(MSFilterSetupFunc)ms_rtp_recv_setup;
}
	
void ms_rtp_recv_process(MSRtpRecv *r)
{
	MSFifo *fo;
	MSQueue *qo;
	MSSync *sync= r->sync;
	void *d;
	mblk_t *mp;
	gint len;
	gint gran=ms_sync_get_samples_per_tick(MS_SYNC(sync));
	
	if (r->rtpsession==NULL) return;
	/* process output fifo and output queue*/
	fo=r->f_outputs[0];
	if (fo!=NULL)
	{
		while( (mp=rtp_session_recvm_with_ts(r->rtpsession,r->prev_ts))!=NULL) {
			/* try to get rtp packets and paste them to the output fifo */
			r->stream_started=1;
			len=mp->b_cont->b_wptr-mp->b_cont->b_rptr;
			ms_fifo_get_write_ptr(fo,len,&d);
			if (d!=NULL){
				memcpy(d,mp->b_cont->b_rptr,len);
			}else ms_warning("ms_rtp_recv_process: no space on output fifo !");
			freemsg(mp);
		}
		r->prev_ts+=gran; 
				
	}
	qo=r->q_outputs[0];
	if (qo!=NULL)
	{
		guint32 clock;
		gint got=0;
		/* we are connected with queues (surely for video)*/
		/* use the sync system time to compute a timestamp */
		PayloadType *pt=rtp_profile_get_payload(r->rtpsession->profile,r->rtpsession->send_pt);
		if (pt==NULL) {
			ms_warning("ms_rtp_recv_process(): NULL RtpPayload- skipping.");
			return;
		}
		clock=(guint32)(((double)sync->time*(double)pt->clock_rate)/1000.0);
		/*g_message("Querying packet with timestamp %u",clock);*/
		/* get rtp packet, and send them through the output queue */
		while ( (mp=rtp_session_recvm_with_ts(r->rtpsession,clock))!=NULL ){
			MSMessage *msg;
			mblk_t *mdata;
			/*g_message("Got packet with timestamp %u",clock);*/
			got++;
			r->stream_started=1;
			if (!r->ignore){
				gboolean markbit=((rtp_header_t*)mp->b_rptr)->markbit;
				mdata=mp->b_cont;
				freeb(mp);
				msg=msgb_2_ms_message(mdata);
				msg->markbit=markbit;
				ms_queue_put(qo,msg);
				
			}else{
				freemsg(mp);
			}
		}
	}
}

void ms_rtp_recv_destroy( MSRtpRecv *obj)
{
	g_free(obj);
}

static void __payload_type_changed(RtpSession *session,MSRtpRecv *obj){
	int pt_num=rtp_session_get_recv_payload_type(session);
	PayloadType *pt=rtp_profile_get_payload(rtp_session_get_profile(session),pt_num);
	if (pt==NULL){
		/* sip phone should ignore payload types they don't understand */
		g_warning("Ignoring payload type %i",pt_num);
		obj->ignore=TRUE;
	}else{
		if (obj->ignore) g_warning("payload type is coming back to something known");
		obj->ignore=FALSE;
	}
}

RtpSession * ms_rtp_recv_set_session(MSRtpRecv *obj,RtpSession *session)
{
	RtpSession *old=obj->rtpsession;
	obj->rtpsession=session;
	rtp_session_signal_connect(session,"payload_type_changed",(RtpCallback)__payload_type_changed,(unsigned long)obj);
	obj->prev_ts=0;
	return old;
}


void ms_rtp_recv_setup(MSRtpRecv *r,MSSync *sync)
{
	r->sync=sync;
	r->stream_started=0;
}
