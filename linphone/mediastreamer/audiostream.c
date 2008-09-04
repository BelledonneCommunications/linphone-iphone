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


#include "mediastream.h"
#ifdef INET6
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>
#endif


#define MAX_RTP_SIZE	1500

/* this code is not part of the library itself, it is part of the mediastream program */
void audio_stream_free(AudioStream *stream)
{
	RtpSession *s;
	RtpSession *destroyed=NULL;
	if (stream->rtprecv!=NULL) {
		s=ms_rtp_recv_get_session(MS_RTP_RECV(stream->rtprecv));
		if (s!=NULL){
			destroyed=s;
			rtp_session_destroy(s);
		}
		ms_filter_destroy(stream->rtprecv);
	}
	if (stream->rtpsend!=NULL) {
		s=ms_rtp_send_get_session(MS_RTP_SEND(stream->rtpsend));
		if (s!=NULL){
			if (s!=destroyed)
				rtp_session_destroy(s);
		}
		ms_filter_destroy(stream->rtpsend);
	}
	if (stream->soundread!=NULL) ms_filter_destroy(stream->soundread);
	if (stream->soundwrite!=NULL) ms_filter_destroy(stream->soundwrite);
	if (stream->encoder!=NULL) ms_filter_destroy(stream->encoder);
	if (stream->decoder!=NULL) ms_filter_destroy(stream->decoder);
	if (stream->timer!=NULL) ms_sync_destroy(stream->timer);
	g_free(stream);
}

static int dtmf_tab[16]={'0','1','2','3','4','5','6','7','8','9','*','#','A','B','C','D'};

static void on_dtmf_received(RtpSession *s,gint dtmf,gpointer user_data)
{
	AudioStream *stream=(AudioStream*)user_data;
	if (dtmf>15){
		g_warning("Unsupported telephone-event type.");
		return;
	}
	g_message("Receiving dtmf %c.",dtmf_tab[dtmf]);
	if (stream!=NULL){
		if (strcmp(stream->soundwrite->klass->name,"OssWrite")==0)
			ms_oss_write_play_dtmf(MS_OSS_WRITE(stream->soundwrite),dtmf_tab[dtmf]);
	}
}

static void on_timestamp_jump(RtpSession *s,guint32* ts, gpointer user_data)
{
	g_warning("The remote sip-phone has send data with a future timestamp: %u,"
			"resynchronising session.",*ts);
	rtp_session_reset(s);
}

static const char *ip4local="0.0.0.0";
static const char *ip6local="::";

const char *get_local_addr_for(const char *remote)
{
	const char *ret;
#ifdef INET6
	struct addrinfo hints, *res0;
	int err;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	err = getaddrinfo(remote,"8000", &hints, &res0);
	if (err!=0) {
		g_warning ("get_local_addr_for: %s", gai_strerror(err));
		return ip4local;
	}
	ret=(res0->ai_addr->sa_family==AF_INET6) ? ip6local : ip4local; 
	freeaddrinfo(res0);
#else
	ret=ip4local;
#endif
	return ret;
}

void create_duplex_rtpsession(RtpProfile *profile, int locport,char *remip,int remport,
				int payload,int jitt_comp,
			RtpSession **recvsend){
	RtpSession *rtpr;
	rtpr=rtp_session_new(RTP_SESSION_SENDRECV);
	rtp_session_set_recv_buf_size(rtpr,MAX_RTP_SIZE);
	rtp_session_set_profile(rtpr,profile);
	rtp_session_set_local_addr(rtpr,get_local_addr_for(remip),locport);
	if (remport>0) rtp_session_set_remote_addr(rtpr,remip,remport);
	rtp_session_set_scheduling_mode(rtpr,0);
	rtp_session_set_blocking_mode(rtpr,0);
	rtp_session_set_payload_type(rtpr,payload);
	rtp_session_set_jitter_compensation(rtpr,jitt_comp);
	rtp_session_enable_adaptive_jitter_compensation(rtpr,TRUE);
	/*rtp_session_signal_connect(rtpr,"timestamp_jump",(RtpCallback)on_timestamp_jump,NULL);*/
	*recvsend=rtpr;
}

void create_rtp_sessions(RtpProfile *profile, int locport,char *remip,int remport,
				int payload,int jitt_comp,
			RtpSession **recv, RtpSession **send){
	RtpSession *rtps,*rtpr;
	/* creates two rtp filters to recv send streams (remote part)*/
	
	rtps=rtp_session_new(RTP_SESSION_SENDONLY);
	rtp_session_set_recv_buf_size(rtps,MAX_RTP_SIZE);
	rtp_session_set_profile(rtps,profile);
#ifdef INET6
	rtp_session_set_local_addr(rtps,"::",locport+2);
#else
	rtp_session_set_local_addr(rtps,"0.0.0.0",locport+2);
#endif
	rtp_session_set_remote_addr(rtps,remip,remport);
	rtp_session_set_scheduling_mode(rtps,0);
	rtp_session_set_blocking_mode(rtps,0);
	rtp_session_set_payload_type(rtps,payload);
	rtp_session_set_jitter_compensation(rtps,jitt_comp);
	
	rtpr=rtp_session_new(RTP_SESSION_RECVONLY);
	rtp_session_set_recv_buf_size(rtpr,MAX_RTP_SIZE);
	rtp_session_set_profile(rtpr,profile);
#ifdef INET6
	rtp_session_set_local_addr(rtpr,"::",locport);
#else
	rtp_session_set_local_addr(rtpr,"0.0.0.0",locport);
#endif
	rtp_session_set_scheduling_mode(rtpr,0);
	rtp_session_set_blocking_mode(rtpr,0);
	rtp_session_set_send_payload_type(rtpr,payload);
	rtp_session_set_recv_payload_type(rtpr,payload);
	rtp_session_set_jitter_compensation(rtpr,jitt_comp);
	rtp_session_signal_connect(rtpr,"timestamp_jump",(RtpCallback)on_timestamp_jump,(unsigned long)NULL);
	*recv=rtpr;
	*send=rtps;
	
}


AudioStream * audio_stream_start_full(RtpProfile *profile, int locport,char *remip,int remport,
				int payload,int jitt_comp, gchar *infile, gchar *outfile, SndCard *playcard, SndCard *captcard)
{
	AudioStream *stream=g_new0(AudioStream,1);
	RtpSession *rtps,*rtpr;
	PayloadType *pt;
	
	//create_rtp_sessions(profile,locport,remip,remport,payload,jitt_comp,&rtpr,&rtps);
	
	create_duplex_rtpsession(profile,locport,remip,remport,payload,jitt_comp,&rtpr);
	rtp_session_signal_connect(rtpr,"telephone-event",(RtpCallback)on_dtmf_received,(unsigned long)stream);
	rtps=rtpr;
	
	stream->rtpsend=ms_rtp_send_new();
	ms_rtp_send_set_session(MS_RTP_SEND(stream->rtpsend),rtps);
	stream->rtprecv=ms_rtp_recv_new();
	ms_rtp_recv_set_session(MS_RTP_RECV(stream->rtprecv),rtpr);
	
	
	/* creates the local part */
	if (infile==NULL) stream->soundread=snd_card_create_read_filter(captcard);
	else stream->soundread=ms_read_new(infile);
	if (outfile==NULL) stream->soundwrite=snd_card_create_write_filter(playcard);
	else stream->soundwrite=ms_write_new(outfile);
	
	/* creates the couple of encoder/decoder */
	pt=rtp_profile_get_payload(profile,payload);
	if (pt==NULL){
		g_error("audiostream.c: undefined payload type.");
		return NULL;
	}
	stream->encoder=ms_encoder_new_with_string_id(pt->mime_type);
	stream->decoder=ms_decoder_new_with_string_id(pt->mime_type);
	if ((stream->encoder==NULL) || (stream->decoder==NULL)){
		/* big problem: we have not a registered codec for this payload...*/
		audio_stream_free(stream);
		g_error("mediastream.c: No decoder available for payload %i.",payload);
		return NULL;
	}
	/* give the sound filters some properties */
	ms_filter_set_property(stream->soundread,MS_FILTER_PROPERTY_FREQ,&pt->clock_rate);
	ms_filter_set_property(stream->soundwrite,MS_FILTER_PROPERTY_FREQ,&pt->clock_rate);
	
	/* give the encoder/decoder some parameters*/
	ms_filter_set_property(stream->encoder,MS_FILTER_PROPERTY_FREQ,&pt->clock_rate);
	ms_filter_set_property(stream->encoder,MS_FILTER_PROPERTY_BITRATE,&pt->normal_bitrate);
	ms_filter_set_property(stream->decoder,MS_FILTER_PROPERTY_FREQ,&pt->clock_rate);
	ms_filter_set_property(stream->decoder,MS_FILTER_PROPERTY_BITRATE,&pt->normal_bitrate);
	
	ms_filter_set_property(stream->encoder,MS_FILTER_PROPERTY_FMTP, (void*)pt->send_fmtp);
	ms_filter_set_property(stream->decoder,MS_FILTER_PROPERTY_FMTP,(void*)pt->recv_fmtp);
	/* create the synchronisation source */
	stream->timer=ms_timer_new();
	
	/* and then connect all */
	ms_filter_add_link(stream->soundread,stream->encoder);
	ms_filter_add_link(stream->encoder,stream->rtpsend);
	ms_filter_add_link(stream->rtprecv,stream->decoder);
	ms_filter_add_link(stream->decoder,stream->soundwrite);
	
	ms_sync_attach(stream->timer,stream->soundread);
	ms_sync_attach(stream->timer,stream->rtprecv);
	
	/* and start */
	ms_start(stream->timer);
	
	return stream;
}

static int defcard=0;

void audio_stream_set_default_card(int cardindex){
	defcard=cardindex;
}

AudioStream * audio_stream_start_with_files(RtpProfile *prof,int locport,char *remip,
		int remport,int profile,int jitt_comp,gchar *infile, gchar*outfile)
{
	return audio_stream_start_full(prof,locport,remip,remport,profile,jitt_comp,infile,outfile,NULL,NULL);
}

AudioStream * audio_stream_start(RtpProfile *prof,int locport,char *remip,int remport,int profile,int jitt_comp)
{
	SndCard *sndcard;
	sndcard=snd_card_manager_get_card(snd_card_manager,defcard);
	return audio_stream_start_full(prof,locport,remip,remport,profile,jitt_comp,NULL,NULL,sndcard,sndcard);
}

AudioStream *audio_stream_start_with_sndcards(RtpProfile *prof,int locport,char *remip,int remport,int profile,int jitt_comp,SndCard *playcard, SndCard *captcard)
{
	g_return_val_if_fail(playcard!=NULL,NULL);
	g_return_val_if_fail(captcard!=NULL,NULL);
	return audio_stream_start_full(prof,locport,remip,remport,profile,jitt_comp,NULL,NULL,playcard,captcard);
}

void audio_stream_set_rtcp_information(AudioStream *st, const char *cname){
	if (st->send_session!=NULL){
		rtp_session_set_source_description(st->send_session,cname,NULL,NULL,NULL,NULL,"linphone-" LINPHONE_VERSION,
											"This is free software (GPL) !");
	}
}

void audio_stream_stop(AudioStream * stream)
{
	
	ms_stop(stream->timer);
	ortp_global_stats_display();
	ms_sync_detach(stream->timer,stream->soundread);
	ms_sync_detach(stream->timer,stream->rtprecv);
	
	ms_filter_remove_links(stream->soundread,stream->encoder);
	ms_filter_remove_links(stream->encoder,stream->rtpsend);
	ms_filter_remove_links(stream->rtprecv,stream->decoder);
	ms_filter_remove_links(stream->decoder,stream->soundwrite);
	
	audio_stream_free(stream);
}

RingStream * ring_start(gchar *file,gint interval,SndCard *sndcard)
{
   return ring_start_with_cb(file,interval,sndcard,NULL,NULL);
}

RingStream * ring_start_with_cb(gchar *file,gint interval,SndCard *sndcard, MSFilterNotifyFunc func,gpointer user_data)
{
	RingStream *stream;
	int tmp;
	g_return_val_if_fail(sndcard!=NULL,NULL);
	stream=g_new0(RingStream,1);
	stream->source=ms_ring_player_new(file,interval);
	if (stream->source==NULL) {
		g_warning("Could not create ring player. Probably the ring file (%s) does not exist.",file);
		return NULL;
	}
  if (func!=NULL) ms_filter_set_notify_func(MS_FILTER(stream->source),func,user_data);
	stream->sndwrite=snd_card_create_write_filter(sndcard);
	ms_filter_get_property(stream->source,MS_FILTER_PROPERTY_FREQ,&tmp);
	ms_filter_set_property(stream->sndwrite,MS_FILTER_PROPERTY_FREQ,&tmp);
	ms_filter_get_property(stream->source,MS_FILTER_PROPERTY_CHANNELS,&tmp);
	ms_filter_set_property(stream->sndwrite,MS_FILTER_PROPERTY_CHANNELS,&tmp);
	stream->timer=ms_timer_new();
	ms_filter_add_link(stream->source,stream->sndwrite);
	ms_sync_attach(stream->timer,stream->source);
	ms_start(stream->timer);
	return stream;
}

void ring_stop(RingStream *stream)
{
	ms_stop(stream->timer);
	ms_sync_detach(stream->timer,stream->source);
	ms_sync_destroy(stream->timer);
	ms_filter_remove_links(stream->source,stream->sndwrite);
	ms_filter_destroy(stream->source);
	ms_filter_destroy(stream->sndwrite);
	g_free(stream);
}

/* returns the latency in samples if the audio device with id dev_id is openable in full duplex mode, else 0 */
gint test_audio_dev(int dev_id)
{
	gint err;
	SndCard *sndcard=snd_card_manager_get_card(snd_card_manager,dev_id);
	if (sndcard==NULL) return -1;
	err=snd_card_probe(sndcard,16,0,8000);
	return err;  /* return latency in number of sample */
}

gint audio_stream_send_dtmf(AudioStream *stream, gchar dtmf)
{
	ms_rtp_send_dtmf(MS_RTP_SEND(stream->rtpsend), dtmf);
	ms_oss_write_play_dtmf(MS_OSS_WRITE(stream->soundwrite),dtmf);
	return 0;
}
