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

#include "msrtpsend.h"
#include <ortp/telephonyevents.h>
#include "mssync.h"
#include "mscodec.h"



static MSRtpSendClass *ms_rtp_send_class=NULL;

MSFilter * ms_rtp_send_new(void)
{
	MSRtpSend *r;
	
	r=g_new(MSRtpSend,1);
	
	if (ms_rtp_send_class==NULL)
	{
		ms_rtp_send_class=g_new(MSRtpSendClass,1);
		ms_rtp_send_class_init(ms_rtp_send_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_rtp_send_class);
	ms_rtp_send_init(r);
	return(MS_FILTER(r));
}
	

void ms_rtp_send_init(MSRtpSend *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->infifos=r->f_inputs;
	MS_FILTER(r)->inqueues=r->q_inputs;
	MS_FILTER(r)->r_mingran=MSRTPSEND_DEF_GRAN;
	memset(r->f_inputs,0,sizeof(MSFifo*)*MSRTPSEND_MAX_INPUTS);
	memset(r->q_inputs,0,sizeof(MSFifo*)*MSRTPSEND_MAX_INPUTS);
	r->rtpsession=NULL;
	r->ts=0;
	r->ts_inc=0;
	r->flags=0;
	r->delay=0;
}

void ms_rtp_send_class_init(MSRtpSendClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"RTPSend");
	MS_FILTER_CLASS(klass)->max_qinputs=MSRTPSEND_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->max_finputs=MSRTPSEND_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->r_maxgran=MSRTPSEND_DEF_GRAN;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_rtp_send_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_rtp_send_process;
	MS_FILTER_CLASS(klass)->setup=(MSFilterSetupFunc)ms_rtp_send_setup;
}

void ms_rtp_send_set_timing(MSRtpSend *r, guint32 ts_inc, gint payload_size)
{
	r->ts_inc=ts_inc;
	r->packet_size=payload_size;
	if (r->ts_inc!=0) r->flags|=RTPSEND_CONFIGURED;
	else r->flags&=~RTPSEND_CONFIGURED;
	MS_FILTER(r)->r_mingran=payload_size;	
	/*g_message("ms_rtp_send_set_timing: ts_inc=%i",ts_inc);*/
}

guint32 get_new_timestamp(MSRtpSend *r,guint32 synctime)
{
	guint32 clockts;
	/* use the sync system time to compute a timestamp */
	PayloadType *pt=rtp_profile_get_payload(r->rtpsession->profile,r->rtpsession->send_pt);
	g_return_val_if_fail(pt!=NULL,0);
	clockts=(guint32)(((double)synctime * (double)pt->clock_rate)/1000.0);
	ms_trace("ms_rtp_send_process: sync->time=%i clock=%i",synctime,clockts);
	if (r->flags & RTPSEND_CONFIGURED){
		if (RTP_TIMESTAMP_IS_STRICTLY_NEWER_THAN(clockts,r->ts+(2*r->ts_inc) )){
			r->ts=clockts;
		}
		else r->ts+=r->ts_inc;
	}else{
		r->ts=clockts;
	}
	return r->ts;
}


void ms_rtp_send_process(MSRtpSend *r)
{
	MSFifo *fi;
	MSQueue *qi;
	MSSync *sync= r->sync;
	int gran=ms_sync_get_samples_per_tick(sync);
	guint32 ts;
	void *s;
	guint skip;
	guint32 synctime=sync->time;
	
	g_return_if_fail(gran>0);
	if (r->rtpsession==NULL) return;

	ms_filter_lock(MS_FILTER(r));
	skip=r->delay!=0;
	if (skip) r->delay--;
	/* process output fifo and output queue*/
	fi=r->f_inputs[0];
	if (fi!=NULL)
	{
		ts=get_new_timestamp(r,synctime);
		/* try to read r->packet_size bytes and send them in a rtp packet*/
		ms_fifo_get_read_ptr(fi,r->packet_size,&s);
		if (!skip){
			rtp_session_send_with_ts(r->rtpsession,s,r->packet_size,ts);
			ms_trace("len=%i, ts=%i ",r->packet_size,ts);
		}
	}
	qi=r->q_inputs[0];
	if (qi!=NULL)
	{
		MSMessage *msg;
		/* read a MSMessage and send it through the network*/
		while ( (msg=ms_queue_get(qi))!=NULL){
			ts=get_new_timestamp(r,synctime);
			if (!skip) {
				/*g_message("Sending packet with ts=%u",ts);*/
				mblk_t *packet=rtp_session_create_packet_with_data(r->rtpsession,msg->data,msg->size,NULL);
				rtp_set_markbit(packet,msg->markbit);
				rtp_session_sendm_with_ts(r->rtpsession,packet,ts);
				
			}
			ms_message_destroy(msg);
		}
	}
	ms_filter_unlock(MS_FILTER(r));
}

void ms_rtp_send_destroy( MSRtpSend *obj)
{
	g_free(obj);
}

RtpSession * ms_rtp_send_set_session(MSRtpSend *obj,RtpSession *session)
{
	RtpSession *old=obj->rtpsession;
	obj->rtpsession=session;
	obj->ts=0;
	obj->ts_inc=0;
	return old;
}

void ms_rtp_send_setup(MSRtpSend *r, MSSync *sync)
{
	MSFilter *codec;
	MSCodecInfo *info;
	r->sync=sync;
	codec=ms_filter_search_upstream_by_type(MS_FILTER(r),MS_FILTER_AUDIO_CODEC);
	if (codec==NULL) codec=ms_filter_search_upstream_by_type(MS_FILTER(r),MS_FILTER_VIDEO_CODEC);
	if (codec==NULL){
		g_warning("ms_rtp_send_setup: could not find upstream codec.");
		return;
	}
	info=MS_CODEC_INFO(codec->klass->info);
	if (info->info.type==MS_FILTER_AUDIO_CODEC){
		int ts_inc=info->fr_size/2;
		int psize=info->dt_size;
		if (ts_inc==0){
			/* dont'use the normal frame size: this is a variable frame size codec */
			/* use the MS_FILTER(codec)->r_mingran */
			ts_inc=MS_FILTER(codec)->r_mingran/2;
			psize=0;
		}
		ms_rtp_send_set_timing(r,ts_inc,psize);
	}
}

gint ms_rtp_send_dtmf(MSRtpSend *r, gchar dtmf)
{
	gint res;

	if (r->rtpsession==NULL) return -1;
	if (rtp_session_telephone_events_supported(r->rtpsession)==-1){
		g_warning("ERROR : telephone events not supported.\n");
 		return -1;
	}

	ms_filter_lock(MS_FILTER(r));
	g_message("Sending DTMF.");
	res=rtp_session_send_dtmf(r->rtpsession, dtmf, r->ts);
	if (res==0){
		//r->ts+=r->ts_inc;
		r->delay+=2;
	}else g_warning("Could not send dtmf.");

	ms_filter_unlock(MS_FILTER(r));

	return res;
}
