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

#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/msticker.h"

#include "ortp/telephonyevents.h"
#if defined(__cplusplus)
#define B64_NO_NAMESPACE
#endif
#include "ortp/b64.h"


struct SenderData {
	RtpSession *session;
	uint32_t tsoff;
	uint32_t skip_until;
	int rate;
	char dtmf;
	char relay_session_id[64];
	int relay_session_id_size;
	uint64_t last_rsi_time;
	bool_t skip;
	bool_t mute_mic;
};

typedef struct SenderData SenderData;

static void sender_init(MSFilter * f)
{
	SenderData *d = (SenderData *)ms_new(SenderData, 1);

	d->session = NULL;
	d->tsoff = 0;
	d->skip_until = 0;
	d->skip = FALSE;
	d->rate = 8000;
	d->dtmf = 0;
	d->mute_mic=FALSE;
	d->relay_session_id_size=0;
	d->last_rsi_time=0;
	f->data = d;
}

static void sender_uninit(MSFilter * f)
{
	SenderData *d = (SenderData *) f->data;

	ms_free(d);
}

static int sender_send_dtmf(MSFilter * f, void *arg)
{
	const char *dtmf = (const char *) arg;
	SenderData *d = (SenderData *) f->data;

	ms_filter_lock(f);
	d->dtmf = dtmf[0];
	ms_filter_unlock(f);
	return 0;
}

static int sender_set_session(MSFilter * f, void *arg)
{
	SenderData *d = (SenderData *) f->data;
	RtpSession *s = (RtpSession *) arg;
	PayloadType *pt =
		rtp_profile_get_payload(rtp_session_get_profile(s),
								rtp_session_get_send_payload_type(s));
	if (pt != NULL) {
		if (strcasecmp("g722", pt->mime_type)==0 )
			d->rate=8000;
		else d->rate = pt->clock_rate;
	} else {
		ms_warning("Sending undefined payload type ?");
	}
	d->session = s;
	return 0;
}

static int sender_mute_mic(MSFilter * f, void *arg)
{
	SenderData *d = (SenderData *) f->data;
	ms_filter_lock(f);
	d->mute_mic=TRUE;
	ms_filter_unlock(f);
	return 0;
}

static int sender_unmute_mic(MSFilter * f, void *arg)
{
	SenderData *d = (SenderData *) f->data;
	ms_filter_lock(f);
	d->mute_mic=FALSE;
	ms_filter_unlock(f);
	return 0;
}

static int sender_set_relay_session_id(MSFilter *f, void*arg){
	SenderData *d = (SenderData *) f->data;
	const char *tmp=(const char *)arg;
	d->relay_session_id_size=b64_decode(tmp, strlen(tmp), (void*)d->relay_session_id, (unsigned int)sizeof(d->relay_session_id));
	return 0;
}

static int sender_get_sr(MSFilter *f, void *arg){
	SenderData *d = (SenderData *) f->data;
	*(int*)arg=d->rate;
	return 0;
}

/* the goal of that function is to return a absolute timestamp closest to real time, with respect of given packet_ts, which is a relative to an undefined origin*/
static uint32_t get_cur_timestamp(MSFilter * f, uint32_t packet_ts)
{
	SenderData *d = (SenderData *) f->data;
	uint32_t curts = (uint32_t)( (f->ticker->time*(uint64_t)d->rate)/(uint64_t)1000) ;
	int diff;
	int delta = d->rate / 50;	/*20 ms at 8000Hz */
	uint32_t netts;

	netts = packet_ts + d->tsoff;
	diff = curts - netts;

#ifdef AMD_HACK
	if (diff > delta) {
		d->tsoff = curts - packet_ts;
		netts = packet_ts + d->tsoff;
		ms_message("synchronizing timestamp, diff=%i", diff);
	}
	else if (diff < -delta) {
		/* d->tsoff = curts - packet_ts; */
		/* hardware clock is going slower than sound card on my PDA... */
	}
#else
	if ((diff > delta) || (diff < -(delta * 5))) {
		d->tsoff = curts - packet_ts;
		netts = packet_ts + d->tsoff;
		ms_message("synchronizing timestamp, diff=%i", diff);
	}
#endif

	/*ms_message("returned ts=%u, orig_ts=%u",netts,packet_ts); */
	return netts;
}

static void sender_process(MSFilter * f)
{
	SenderData *d = (SenderData *) f->data;
	RtpSession *s = d->session;

	mblk_t *im;
	uint32_t timestamp;

	if (s == NULL){
		ms_queue_flush(f->inputs[0]);
		return;
	}

	if (d->relay_session_id_size>0 && 
		( (f->ticker->time-d->last_rsi_time)>5000 || d->last_rsi_time==0) ) {
		ms_message("relay session id sent in RTCP APP");
		rtp_session_send_rtcp_APP(s,0,"RSID",(const uint8_t *)d->relay_session_id,d->relay_session_id_size);
		d->last_rsi_time=f->ticker->time;
	}

	while ((im = ms_queue_get(f->inputs[0])) != NULL) {
		mblk_t *header;

		timestamp = get_cur_timestamp(f, mblk_get_timestamp_info(im));
		ms_filter_lock(f);
		if (d->dtmf != 0) {
			rtp_session_send_dtmf(s, d->dtmf, timestamp);
			ms_debug("RFC2833 dtmf sent.");
			d->dtmf = 0;
			d->skip_until = timestamp + (3 * 160);
			d->skip = TRUE;
			freemsg(im);
		}else if (d->skip) {
			ms_debug("skipping..");
			if (RTP_TIMESTAMP_IS_NEWER_THAN(timestamp, d->skip_until)) {
				d->skip = FALSE;
			}
			freemsg(im);
		}else{
			if (d->mute_mic==FALSE){
				int pt = mblk_get_payload_type(im);
				header = rtp_session_create_packet(s, 12, NULL, 0);
				if (pt>0)
					rtp_set_payload_type(header, pt);
				rtp_set_markbit(header, mblk_get_marker_info(im));
				header->b_cont = im;
				rtp_session_sendm_with_ts(s, header, timestamp);
			}
			else{
				freemsg(im);
			}
		}
		ms_filter_unlock(f);
	}
}

static MSFilterMethod sender_methods[] = {
	{MS_RTP_SEND_MUTE_MIC, sender_mute_mic},
	{MS_RTP_SEND_UNMUTE_MIC, sender_unmute_mic},
	{MS_RTP_SEND_SET_SESSION, sender_set_session},
	{MS_RTP_SEND_SEND_DTMF, sender_send_dtmf},
	{MS_RTP_SEND_SET_RELAY_SESSION_ID, sender_set_relay_session_id},
	{MS_FILTER_GET_SAMPLE_RATE, sender_get_sr },
	{0, NULL}
};

#ifdef _MSC_VER

MSFilterDesc ms_rtp_send_desc = {
	MS_RTP_SEND_ID,
	"MSRtpSend",
	N_("RTP output filter"),
	MS_FILTER_OTHER,
	NULL,
	1,
	0,
	sender_init,
	NULL,
	sender_process,
	NULL,
	sender_uninit,
	sender_methods
};

#else

MSFilterDesc ms_rtp_send_desc = {
	.id = MS_RTP_SEND_ID,
	.name = "MSRtpSend",
	.text = N_("RTP output filter"),
	.category = MS_FILTER_OTHER,
	.ninputs = 1,
	.noutputs = 0,
	.init = sender_init,
	.process = sender_process,
	.uninit = sender_uninit,
	.methods = sender_methods
};

#endif

struct ReceiverData {
	RtpSession *session;
	int rate;
};

typedef struct ReceiverData ReceiverData;

static void receiver_init(MSFilter * f)
{
	ReceiverData *d = (ReceiverData *)ms_new(ReceiverData, 1);
	d->session = NULL;
	d->rate = 8000;
	f->data = d;
}

static void receiver_postprocess(MSFilter * f){
	/*ReceiverData *d = (ReceiverData *) f->data;*/
}

static void receiver_uninit(MSFilter * f){
	ReceiverData *d = (ReceiverData *) f->data;
	ms_free(d);
}

static int receiver_set_session(MSFilter * f, void *arg)
{
	ReceiverData *d = (ReceiverData *) f->data;
	RtpSession *s = (RtpSession *) arg;
	PayloadType *pt = rtp_profile_get_payload(rtp_session_get_profile(s),
											  rtp_session_get_recv_payload_type
											  (s));
	if (pt != NULL) {
		if (strcasecmp("g722", pt->mime_type)==0 )
			d->rate=8000;
		else d->rate = pt->clock_rate;
	} else {
		ms_warning("Receiving undefined payload type ?");
	}
	d->session = s;

	return 0;
}

static int receiver_get_sr(MSFilter *f, void *arg){
	ReceiverData *d = (ReceiverData *) f->data;
	PayloadType *pt;
	if (d->session==NULL) {
		ms_warning("Could not obtain sample rate, session is not set.");
		return -1;
	}
	pt=rtp_profile_get_payload(rtp_session_get_profile(d->session),
									rtp_session_get_recv_payload_type(d->session));
	if (pt != NULL) {
		*(int*)arg=pt->clock_rate;
	}else{
		ms_warning("Could not obtain sample rate, payload type is unknown.");
		return -1;
	}
	return 0;
}

static void receiver_preprocess(MSFilter * f){
	ReceiverData *d = (ReceiverData *) f->data;
	if (d->session){
		PayloadType *pt=rtp_profile_get_payload(
			rtp_session_get_profile(d->session),
			rtp_session_get_recv_payload_type(d->session));
		if (pt){
			if (pt->type!=PAYLOAD_VIDEO)
				rtp_session_flush_sockets(d->session);
		}
	}
}

static void receiver_process(MSFilter * f)
{
	ReceiverData *d = (ReceiverData *) f->data;
	mblk_t *m;
	uint32_t timestamp;

	if (d->session == NULL)
		return;

	timestamp = (uint32_t) (f->ticker->time * (d->rate/1000));
	while ((m = rtp_session_recvm_with_ts(d->session, timestamp)) != NULL) {
		mblk_set_timestamp_info(m, rtp_get_timestamp(m));
		mblk_set_marker_info(m, rtp_get_markbit(m));
		mblk_set_payload_type(m, rtp_get_payload_type(m));
		rtp_get_payload(m,&m->b_rptr);
		ms_queue_put(f->outputs[0], m);
	}
}

static MSFilterMethod receiver_methods[] = {
	{	MS_RTP_RECV_SET_SESSION	, receiver_set_session	},
	{	MS_FILTER_GET_SAMPLE_RATE	, receiver_get_sr		},
	{	0, NULL}
};

#ifdef _MSC_VER

MSFilterDesc ms_rtp_recv_desc = {
	MS_RTP_RECV_ID,
	"MSRtpRecv",
	N_("RTP input filter"),
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	receiver_init,
	receiver_preprocess,
	receiver_process,
	receiver_postprocess,
	receiver_uninit,
	receiver_methods
};

#else

MSFilterDesc ms_rtp_recv_desc = {
	.id = MS_RTP_RECV_ID,
	.name = "MSRtpRecv",
	.text = N_("RTP input filter"),
	.category = MS_FILTER_OTHER,
	.ninputs = 0,
	.noutputs = 1,
	.init = receiver_init,
	.preprocess = receiver_preprocess,
	.process = receiver_process,
	.postprocess=receiver_postprocess,
	.uninit = receiver_uninit,
	.methods = receiver_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_rtp_send_desc)
MS_FILTER_DESC_EXPORT(ms_rtp_recv_desc)
