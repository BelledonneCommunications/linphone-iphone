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

#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msrtp.h"


#if defined(WIN32) || defined(_WIN32_WCE)
/* avoid double declaration of ms_win_display_desc
   -> Including twice msvideoout.h inside mediastreamer2
   is NOT support with MSVC compiler.
*/
#define MS_VIDEO_OUT_HANDLE_RESIZING MS_FILTER_METHOD_NO_ARG(MS_VIDEO_OUT_ID,1)
#define MS_VIDEO_OUT_AUTO_FIT	MS_FILTER_METHOD(MS_VIDEO_OUT_ID,3,int)
#else
#include "mediastreamer2/msvideoout.h"
#endif

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

extern RtpSession * create_duplex_rtpsession( int locport, bool_t ipv6);

#define MAX_RTP_SIZE	UDP_MAX_SIZE

/* this code is not part of the library itself, it is part of the mediastream program */
void video_stream_free (VideoStream * stream)
{
	if (stream->session!=NULL){
		rtp_session_unregister_event_queue(stream->session,stream->evq);
		rtp_session_destroy(stream->session);
	}
	if (stream->rtprecv != NULL)
		ms_filter_destroy (stream->rtprecv);
	if (stream->rtpsend!=NULL) 
		ms_filter_destroy (stream->rtpsend);
	if (stream->source != NULL)
		ms_filter_destroy (stream->source);
	if (stream->output != NULL)
		ms_filter_destroy (stream->output);
	if (stream->decoder != NULL)
		ms_filter_destroy (stream->decoder);
	if (stream->sizeconv != NULL)
		ms_filter_destroy (stream->sizeconv);
	if (stream->pixconv!=NULL)
		ms_filter_destroy(stream->pixconv);
	if (stream->tee!=NULL)
		ms_filter_destroy(stream->tee);
	if (stream->ticker != NULL)
		ms_ticker_destroy (stream->ticker);
	if (stream->evq!=NULL)
		ortp_ev_queue_destroy(stream->evq);
	ms_free (stream);
}

/*this function must be called from the MSTicker thread:
it replaces one filter by another one.
This is a dirty hack that works anyway.
It would be interesting to have something that does the job
simplier within the MSTicker api
*/
void video_stream_change_decoder(VideoStream *stream, int payload){
	RtpSession *session=stream->session;
	RtpProfile *prof=rtp_session_get_profile(session);
	PayloadType *pt=rtp_profile_get_payload(prof,payload);
	if (pt!=NULL){
		MSFilter *dec=ms_filter_create_decoder(pt->mime_type);
		if (dec!=NULL){
			ms_filter_unlink(stream->rtprecv, 0, stream->decoder, 0);
			ms_filter_unlink(stream->decoder,0,stream->output,0);
			ms_filter_postprocess(stream->decoder);
			ms_filter_destroy(stream->decoder);
			stream->decoder=dec;
			if (pt->recv_fmtp!=NULL)
				ms_filter_call_method(stream->decoder,MS_FILTER_ADD_FMTP,(void*)pt->recv_fmtp);
			ms_filter_link (stream->rtprecv, 0, stream->decoder, 0);
			ms_filter_link (stream->decoder,0 , stream->output, 0);
			ms_filter_preprocess(stream->decoder,stream->ticker);
			
		}else{
			ms_warning("No decoder found for %s",pt->mime_type);
		}
	}else{
		ms_warning("No payload defined with number %i",payload);
	}
}

static void video_stream_adapt_bitrate(VideoStream *stream, int jitter, float lost){
	if (stream->encoder!=NULL){
		if (lost>10){
			int bitrate=0;
			int new_bitrate;
			ms_warning("Remote reports bad receiving experience, trying to reduce bitrate of video encoder.");
			
			ms_filter_call_method(stream->encoder,MS_FILTER_GET_BITRATE,&bitrate);
			if (bitrate==0){
				ms_error("Video encoder does not implement MS_FILTER_GET_BITRATE.");
				return;
			}
			if (bitrate>=20000){
				new_bitrate=bitrate-10000;
				ms_warning("Encoder bitrate reduced from %i to %i b/s.",bitrate,new_bitrate);
				ms_filter_call_method(stream->encoder,MS_FILTER_SET_BITRATE,&new_bitrate);
			}else{
				ms_warning("Video encoder bitrate already at minimum.");
			}
			
		}
	}
}

static void video_steam_process_rtcp(VideoStream *stream, mblk_t *m){
	do{
		if (rtcp_is_SR(m)){
			const report_block_t *rb;
			ms_message("video_steam_process_rtcp: receiving RTCP SR");
			rb=rtcp_SR_get_report_block(m,0);
			if (rb){
				unsigned int ij;
				float flost;
				ij=report_block_get_interarrival_jitter(rb);
				flost=100.0*report_block_get_fraction_lost(rb)/256.0;
				ms_message("interarrival jitter=%u , lost packets percentage since last report=%f ",ij,flost);
				if (stream->adapt_bitrate) video_stream_adapt_bitrate(stream,ij,flost);
			}
		}
	}while(rtcp_next_packet(m));
}

void video_stream_iterate(VideoStream *stream){
	
	if (stream->output!=NULL)
		ms_filter_call_method_noarg(stream->output,
			MS_VIDEO_OUT_HANDLE_RESIZING);
	
	if (stream->evq){
		OrtpEvent *ev=ortp_ev_queue_get(stream->evq);
		if (ev!=NULL){
			if (ortp_event_get_type(ev)==ORTP_EVENT_RTCP_PACKET_RECEIVED){
				OrtpEventData *evd=ortp_event_get_data(ev);
				video_steam_process_rtcp(stream,evd->packet);
			}
			ortp_event_destroy(ev);
		}
	}
}

static void payload_type_changed(RtpSession *session, unsigned long data){
	VideoStream *stream=(VideoStream*)data;
	int pt=rtp_session_get_recv_payload_type(stream->session);
	video_stream_change_decoder(stream,pt);
}

VideoStream *video_stream_new(int locport, bool_t use_ipv6){
	VideoStream *stream = (VideoStream *)ms_new0 (VideoStream, 1);
	stream->session=create_duplex_rtpsession(locport,use_ipv6);
	stream->evq=ortp_ev_queue_new();
	stream->rtpsend=ms_filter_new(MS_RTP_SEND_ID);
	rtp_session_register_event_queue(stream->session,stream->evq);
	stream->sent_vsize.width=MS_VIDEO_SIZE_CIF_W;
	stream->sent_vsize.height=MS_VIDEO_SIZE_CIF_H;
	return stream;
}

void video_stream_set_sent_video_size(VideoStream *stream, MSVideoSize vsize){
	stream->sent_vsize=vsize;
}

void video_stream_set_relay_session_id(VideoStream *stream, const char *id){
	ms_filter_call_method(stream->rtpsend, MS_RTP_SEND_SET_RELAY_SESSION_ID,(void*)id);
}


void video_stream_enable_adaptive_bitrate_control(VideoStream *s, bool_t yesno){
	s->adapt_bitrate=yesno;
}

int video_stream_start (VideoStream *stream, RtpProfile *profile, const char *remip, int remport,
	int rem_rtcp_port, int payload, int jitt_comp, MSWebCam *cam){
	PayloadType *pt;
	RtpSession *rtps=stream->session;
	MSPixFmt format;
	MSVideoSize vsize,cam_vsize,disp_size;
	float fps=15;
	int tmp;
	JBParameters jbp;
	const int socket_buf_size=2000000;

	pt=rtp_profile_get_payload(profile,payload);
	if (pt==NULL){
		ms_error("videostream.c: undefined payload type.");
		return -1;
	}
	stream->encoder=ms_filter_create_encoder(pt->mime_type);
	stream->decoder=ms_filter_create_decoder(pt->mime_type);
	if ((stream->encoder==NULL) || (stream->decoder==NULL)){
		/* big problem: we have not a registered codec for this payload...*/
		ms_error("videostream.c: No codecs available for payload %i:%s.",payload,pt->mime_type);
		return -1;
	}
	
	rtp_session_set_profile(rtps,profile);
	if (remport>0) rtp_session_set_remote_addr_full(rtps,remip,remport,rem_rtcp_port);
	rtp_session_set_payload_type(rtps,payload);
	rtp_session_set_jitter_compensation(rtps,jitt_comp);

	rtp_session_signal_connect(stream->session,"payload_type_changed",
			(RtpCallback)payload_type_changed,(unsigned long)stream);

	rtp_session_set_recv_buf_size(stream->session,MAX_RTP_SIZE);

	rtp_session_get_jitter_buffer_params(stream->session,&jbp);
	jbp.max_packets=1000;//needed for high resolution video
	rtp_session_set_jitter_buffer_params(stream->session,&jbp);

	rtp_session_set_rtp_socket_recv_buffer_size(stream->session,socket_buf_size);
	rtp_session_set_rtp_socket_send_buffer_size(stream->session,socket_buf_size);

	/* creates two rtp filters to recv send streams (remote part) */
	if (remport>0) ms_filter_call_method(stream->rtpsend,MS_RTP_SEND_SET_SESSION,stream->session);
	
	stream->rtprecv = ms_filter_new (MS_RTP_RECV_ID);
	ms_filter_call_method(stream->rtprecv,MS_RTP_RECV_SET_SESSION,stream->session);

	/* creates the filters */
	stream->source = ms_web_cam_create_reader(cam);
	stream->tee = ms_filter_new(MS_TEE_ID);
	stream->output=ms_filter_new(MS_VIDEO_OUT_ID);
	stream->sizeconv=ms_filter_new(MS_SIZE_CONV_ID);
	
	if (pt->normal_bitrate>0){
		ms_message("Limiting bitrate of video encoder to %i bits/s",pt->normal_bitrate);
		ms_filter_call_method(stream->encoder,MS_FILTER_SET_BITRATE,&pt->normal_bitrate);
	}
	/* set parameters to the encoder and decoder*/
	if (pt->send_fmtp){
		ms_filter_call_method(stream->encoder,MS_FILTER_ADD_FMTP,pt->send_fmtp);
		ms_filter_call_method(stream->decoder,MS_FILTER_ADD_FMTP,pt->send_fmtp);
	}
	ms_filter_call_method(stream->encoder,MS_FILTER_GET_VIDEO_SIZE,&vsize);
	vsize=ms_video_size_min(vsize,stream->sent_vsize);
	ms_filter_call_method(stream->encoder,MS_FILTER_SET_VIDEO_SIZE,&vsize);
	ms_filter_call_method(stream->encoder,MS_FILTER_GET_FPS,&fps);
	ms_message("Setting vsize=%ix%i, fps=%f",vsize.width,vsize.height,fps);
	/* configure the filters */
	ms_filter_call_method(stream->source,MS_FILTER_SET_FPS,&fps);
	ms_filter_call_method(stream->source,MS_FILTER_SET_VIDEO_SIZE,&vsize);

	/* get the output format for webcam reader */
	ms_filter_call_method(stream->source,MS_FILTER_GET_PIX_FMT,&format);
	if (format==MS_MJPEG){
		stream->pixconv=ms_filter_new(MS_MJPEG_DEC_ID);
	}else{
		stream->pixconv = ms_filter_new(MS_PIX_CONV_ID);
		/*set it to the pixconv */
		ms_filter_call_method(stream->pixconv,MS_FILTER_SET_PIX_FMT,&format);

		ms_filter_call_method(stream->source,MS_FILTER_GET_VIDEO_SIZE,&cam_vsize);
	
		ms_filter_call_method(stream->pixconv,MS_FILTER_SET_VIDEO_SIZE,&cam_vsize);
	}

	ms_filter_call_method(stream->sizeconv,MS_FILTER_SET_VIDEO_SIZE,&vsize);

	/*force the decoder to output YUV420P */
	format=MS_YUV420P;
	ms_filter_call_method(stream->decoder,MS_FILTER_SET_PIX_FMT,&format);

	disp_size.width=MS_VIDEO_SIZE_CIF_W;
	disp_size.height=MS_VIDEO_SIZE_CIF_H;
	tmp=1;
	ms_filter_call_method(stream->output,MS_FILTER_SET_VIDEO_SIZE,&disp_size);
	ms_filter_call_method(stream->output,MS_VIDEO_OUT_AUTO_FIT,&tmp);
	ms_filter_call_method(stream->output,MS_FILTER_SET_PIX_FMT,&format);

	if (pt->recv_fmtp!=NULL)
		ms_filter_call_method(stream->decoder,MS_FILTER_ADD_FMTP,(void*)pt->recv_fmtp);

	/* and then connect all */
	ms_filter_link (stream->source, 0, stream->pixconv, 0);
	ms_filter_link (stream->pixconv, 0, stream->sizeconv, 0);
	ms_filter_link (stream->sizeconv, 0, stream->tee, 0);
	ms_filter_link (stream->tee, 0 ,stream->encoder, 0 );
	ms_filter_link (stream->encoder,0, stream->rtpsend,0);
	
	ms_filter_link (stream->rtprecv, 0, stream->decoder, 0);
	ms_filter_link (stream->decoder,0 , stream->output, 0);
	/* the source video must be send for preview */
	ms_filter_link(stream->tee,1,stream->output,1);

	/* create the ticker */
	stream->ticker = ms_ticker_new(); 
	/* attach it the graph */
	ms_ticker_attach (stream->ticker, stream->source);
	return 0;
}

void video_stream_send_vfu(VideoStream *stream){
	if (stream->encoder)
		ms_filter_call_method_noarg(stream->encoder,MS_FILTER_REQ_VFU);
}

void
video_stream_stop (VideoStream * stream)
{
	if (stream->ticker){
		ms_ticker_detach(stream->ticker,stream->source);
	
		rtp_stats_display(rtp_session_get_stats(stream->session),"Video session's RTP statistics");
		
		ms_filter_unlink(stream->source,0,stream->pixconv,0);
		ms_filter_unlink (stream->pixconv, 0, stream->sizeconv, 0);
		ms_filter_unlink (stream->sizeconv, 0, stream->tee, 0);
		ms_filter_unlink(stream->tee,0,stream->encoder,0);
		ms_filter_unlink(stream->encoder, 0, stream->rtpsend,0);
		ms_filter_unlink(stream->rtprecv, 0, stream->decoder, 0);
		ms_filter_unlink(stream->decoder,0,stream->output,0);
		ms_filter_unlink(stream->tee,1,stream->output,1);
	}
	video_stream_free (stream);
}


void video_stream_set_rtcp_information(VideoStream *st, const char *cname, const char *tool){
	if (st->session!=NULL){
		rtp_session_set_source_description(st->session,cname,NULL,NULL,NULL,NULL,tool,
											"This is free software (GPL) !");
	}
}



VideoStream * video_preview_start(MSWebCam *device, MSVideoSize disp_size){
	VideoStream *stream = (VideoStream *)ms_new0 (VideoStream, 1);
	MSVideoSize vsize=disp_size;
	MSPixFmt format;

	/* creates the filters */
	stream->source = ms_web_cam_create_reader(device);
	stream->output = ms_filter_new(MS_VIDEO_OUT_ID);


	/* configure the filters */
	ms_filter_call_method(stream->source,MS_FILTER_SET_VIDEO_SIZE,&vsize);
	ms_filter_call_method(stream->source,MS_FILTER_GET_PIX_FMT,&format);
	ms_filter_call_method(stream->source,MS_FILTER_GET_VIDEO_SIZE,&vsize);
	if (format==MS_MJPEG){
		stream->pixconv=ms_filter_new(MS_MJPEG_DEC_ID);
	}else{
		stream->pixconv=ms_filter_new(MS_PIX_CONV_ID);
		ms_filter_call_method(stream->pixconv,MS_FILTER_SET_PIX_FMT,&format);
		ms_filter_call_method(stream->pixconv,MS_FILTER_SET_VIDEO_SIZE,&vsize);
	}

	format=MS_YUV420P;
	ms_filter_call_method(stream->output,MS_FILTER_SET_PIX_FMT,&format);
	ms_filter_call_method(stream->output,MS_FILTER_SET_VIDEO_SIZE,&disp_size);
	/* and then connect all */

	ms_filter_link(stream->source,0, stream->pixconv,0);
	ms_filter_link(stream->pixconv, 0, stream->output, 0);

	/* create the ticker */
	stream->ticker = ms_ticker_new(); 
	ms_ticker_attach (stream->ticker, stream->source);
	return stream;
}

void video_preview_stop(VideoStream *stream){
	ms_ticker_detach(stream->ticker, stream->source);
	ms_filter_unlink(stream->source,0,stream->pixconv,0);
	ms_filter_unlink(stream->pixconv,0,stream->output,0);
	
	video_stream_free(stream);
}


int video_stream_send_only_start(VideoStream* stream, RtpProfile *profile, const char *remip, int remport,
	int rem_rtcp_port, int payload, int jitt_comp, MSWebCam *device){
	PayloadType *pt;
	MSPixFmt format;
	MSVideoSize vsize;
	RtpSession *rtps=stream->session;
	float fps=15;
	
	vsize.width=MS_VIDEO_SIZE_CIF_W;
	vsize.height=MS_VIDEO_SIZE_CIF_H;

	rtp_session_set_profile(rtps,profile);
	if (remport>0) rtp_session_set_remote_addr_full(rtps,remip,remport,rem_rtcp_port);
	rtp_session_set_payload_type(rtps,payload);
	rtp_session_set_jitter_compensation(rtps,jitt_comp);
	
	/* creates rtp filter to send streams (remote part) */
	rtp_session_set_recv_buf_size(rtps,MAX_RTP_SIZE);
	stream->rtpsend =ms_filter_new(MS_RTP_SEND_ID);
	if (remport>0) ms_filter_call_method(stream->rtpsend,MS_RTP_SEND_SET_SESSION,stream->session);

	/* creates the filters */
	pt=rtp_profile_get_payload(profile,payload);
	if (pt==NULL){
		video_stream_free(stream);
		ms_error("videostream.c: undefined payload type.");
		return -1;
	}
	stream->encoder=ms_filter_create_encoder(pt->mime_type);
	if ((stream->encoder==NULL)){
		/* big problem: we have not a registered codec for this payload...*/
		video_stream_free(stream);
		ms_error("videostream.c: No codecs available for payload %i.",payload);
		return -1;
	}

	/* creates the filters */
	stream->source = ms_web_cam_create_reader(device);
	stream->sizeconv=ms_filter_new(MS_SIZE_CONV_ID);

	/* configure the filters */
	if (pt->send_fmtp)
		ms_filter_call_method(stream->encoder,MS_FILTER_ADD_FMTP,pt->send_fmtp);
	ms_filter_call_method(stream->encoder,MS_FILTER_SET_BITRATE,&pt->normal_bitrate);
	ms_filter_call_method(stream->encoder,MS_FILTER_GET_FPS,&fps);
	ms_filter_call_method(stream->encoder,MS_FILTER_GET_VIDEO_SIZE,&vsize);

	ms_filter_call_method(stream->source,MS_FILTER_SET_FPS,&fps);
	ms_filter_call_method(stream->source,MS_FILTER_SET_VIDEO_SIZE,&vsize);
	
	/* get the output format for webcam reader */
	ms_filter_call_method(stream->source,MS_FILTER_GET_PIX_FMT,&format);
	/*set it to the pixconv */

	/* bug fix from AMD: What about MJPEG mode???*/
	if (format==MS_MJPEG){
		stream->pixconv=ms_filter_new(MS_MJPEG_DEC_ID);
	}else{
		stream->pixconv=ms_filter_new(MS_PIX_CONV_ID);
		ms_filter_call_method(stream->pixconv,MS_FILTER_SET_PIX_FMT,&format);

		ms_filter_call_method(stream->source,MS_FILTER_GET_VIDEO_SIZE,&vsize);
		ms_filter_call_method(stream->pixconv,MS_FILTER_SET_VIDEO_SIZE,&vsize);
	}

	ms_filter_call_method(stream->encoder,MS_FILTER_GET_VIDEO_SIZE,&vsize);
	ms_filter_call_method(stream->sizeconv,MS_FILTER_SET_VIDEO_SIZE,&vsize);
	
	ms_message("vsize=%ix%i, fps=%f, send format: %s, capture format: %d, bitrate: %d",
			vsize.width,vsize.height,fps,pt->send_fmtp,format, pt->normal_bitrate);

	/* and then connect all */
	ms_filter_link (stream->source, 0, stream->pixconv, 0);
	ms_filter_link (stream->pixconv, 0, stream->sizeconv, 0);
	ms_filter_link (stream->sizeconv, 0, stream->encoder, 0);
	ms_filter_link (stream->encoder,0, stream->rtpsend,0);

	/* create the ticker */
	stream->ticker = ms_ticker_new(); 
	/* attach it the graph */
	ms_ticker_attach (stream->ticker, stream->source);
	return 0;
}

void video_stream_send_only_stop(VideoStream *stream){
	if (stream->ticker){
		ms_ticker_detach (stream->ticker, stream->source);
		ms_filter_unlink(stream->source,0,stream->pixconv,0);
		ms_filter_unlink (stream->pixconv, 0, stream->sizeconv, 0);
		ms_filter_unlink (stream->sizeconv, 0, stream->encoder, 0);
		ms_filter_unlink(stream->encoder,0,stream->rtpsend,0);
	}
	video_stream_free(stream);
}

int video_stream_recv_only_start (VideoStream *stream, RtpProfile *profile, const char *remip, int remport,int payload, int jitt_comp){
	PayloadType *pt;
	MSPixFmt format;
	MSVideoSize vsize;
	RtpSession *rtps=stream->session;
	
	vsize.width=MS_VIDEO_SIZE_CIF_W;
	vsize.height=MS_VIDEO_SIZE_CIF_H;

	rtp_session_set_profile(rtps,profile);
	if (remport>0) rtp_session_set_remote_addr(rtps,remip,remport);
	rtp_session_set_payload_type(rtps,payload);
	rtp_session_set_jitter_compensation(rtps,jitt_comp);
	
	/* creates rtp filters to recv streams */
	rtp_session_set_recv_buf_size(rtps,MAX_RTP_SIZE);
	stream->rtprecv = ms_filter_new (MS_RTP_RECV_ID);
	ms_filter_call_method(stream->rtprecv,MS_RTP_RECV_SET_SESSION,rtps);

	/* creates the filters */
	pt=rtp_profile_get_payload(profile,payload);
	if (pt==NULL){
		ms_error("videostream.c: undefined payload type.");
		return -1;
	}
	stream->decoder=ms_filter_create_decoder(pt->mime_type);
	if (stream->decoder==NULL){
		/* big problem: we have not a registered codec for this payload...*/
		ms_error("videostream.c: No codecs available for payload %i:%s.",payload,pt->mime_type);
		return -1;
	}
	stream->output=ms_filter_new(MS_VIDEO_OUT_ID);

	/*force the decoder to output YUV420P */
	format=MS_YUV420P;
	/*ask the size-converter to always output CIF */
	vsize.width=MS_VIDEO_SIZE_CIF_W;
	vsize.height=MS_VIDEO_SIZE_CIF_H;
	ms_message("Setting output vsize=%ix%i",vsize.width,vsize.height);
	
	ms_filter_call_method(stream->decoder,MS_FILTER_SET_PIX_FMT,&format);
	ms_filter_call_method(stream->output,MS_FILTER_SET_PIX_FMT,&format);
	ms_filter_call_method(stream->output,MS_FILTER_SET_VIDEO_SIZE,&vsize);

	if (pt->recv_fmtp!=NULL) {
		ms_message("pt->recv_fmtp: %s", pt->recv_fmtp);
		ms_filter_call_method(stream->decoder,MS_FILTER_ADD_FMTP,(void*)pt->recv_fmtp);
	}

	/* and then connect all */
	ms_filter_link (stream->rtprecv, 0, stream->decoder, 0);
	ms_filter_link (stream->decoder,0 , stream->output, 0);

	/* create the ticker */
	stream->ticker = ms_ticker_new(); 
	/* attach it the graph */
	ms_ticker_attach (stream->ticker, stream->rtprecv);
	return 0;
}

void video_stream_recv_only_stop (VideoStream * stream){
	if (stream->ticker!=NULL){
		ms_ticker_detach(stream->ticker, stream->rtprecv);
		rtp_stats_display(rtp_session_get_stats(stream->session),"Video session's RTP statistics");
		ms_filter_unlink(stream->rtprecv, 0, stream->decoder, 0);
		ms_filter_unlink(stream->decoder,0,stream->output,0);
	}
	video_stream_free (stream);
}

