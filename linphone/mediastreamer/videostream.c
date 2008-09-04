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
#include "msvideosource.h"
#include "msavdecoder.h"
#include "msavencoder.h"
#include "msnosync.h"
#include "mssdlout.h"

#define USE_SDL

extern void create_duplex_rtpsession(RtpProfile *profile, int locport,char *remip,int remport,
				int payload,int jitt_comp,
			RtpSession **recvsend);

#define MAX_RTP_SIZE	UDP_MAX_SIZE

#ifdef VIDEO_ENABLED

/* this code is not part of the library itself, it is part of the mediastream program */
void video_stream_free (VideoStream * stream)
{
	RtpSession *recvs, *sends;
	if (stream->rtprecv != NULL)
	{
		recvs = ms_rtp_recv_get_session (MS_RTP_RECV (stream->rtprecv));
		if (recvs != NULL)
		{
			rtp_session_destroy (recvs);
		}
		ms_filter_destroy (stream->rtprecv);
	}
	if (stream->rtpsend != NULL)
	{
		sends = ms_rtp_send_get_session (MS_RTP_SEND (stream->rtpsend));
		if (sends != NULL && sends!=recvs)
		{
			rtp_session_destroy (sends);
		}
		ms_filter_destroy (stream->rtpsend);
	}
	if (stream->source != NULL)
		ms_filter_destroy (stream->source);
	if (stream->output != NULL)
		ms_filter_destroy (stream->output);
	if (stream->decoder != NULL)
		ms_filter_destroy (stream->decoder);
	if (stream->encoder != NULL)
		ms_filter_destroy (stream->encoder);
	if (stream->timer != NULL)
		ms_sync_destroy (stream->timer);
	g_free (stream);
}


VideoStream *
video_stream_start (RtpProfile *profile, int locport, char *remip, int remport,
			 int payload, int jitt_comp, gboolean show_local, 
			 const gchar *source, const gchar *device)
{
	VideoStream *stream = g_new0 (VideoStream, 1);
	RtpSession *rtps, *rtpr;
	PayloadType *pt;
	gchar *format;
	gint width = VIDEO_SIZE_CIF_W;
	gint height = VIDEO_SIZE_CIF_H;
	float fps;

	create_duplex_rtpsession(profile,locport,remip,remport,payload,jitt_comp,&rtpr);
	rtp_session_enable_adaptive_jitter_compensation(rtpr,FALSE);
	rtps=rtpr;
	ms_trace("sending video to %s:%i", remip4, remport);
	
	/* creates two rtp filters to recv send streams (remote part) */
	rtp_session_set_recv_buf_size(rtpr,MAX_RTP_SIZE);
	stream->rtpsend = ms_rtp_send_new ();
	if (remport>0) ms_rtp_send_set_session (MS_RTP_SEND (stream->rtpsend), rtps);

	
	stream->rtprecv = ms_rtp_recv_new ();
	ms_rtp_recv_set_session (MS_RTP_RECV (stream->rtprecv), rtpr);

	pt=rtp_profile_get_payload(profile,payload);
	if (pt==NULL){
		g_error("videostream.c: undefined payload type.");
		return NULL;
	}
	ms_trace("videostream.c: getting codecs for %s", pt->mime_type);

	/* creates the filters */
	stream->source = ms_filter_new_with_name(source);
	if (stream->source == NULL){
		g_error("videostream.c: failed to create video source %s.", source);
		return NULL;
	}
	
#ifdef USE_SDL
	stream->output=ms_sdl_out_new();
#else
	stream->output = MS_FILTER(ms_video_output_new ());
#endif
	stream->encoder=ms_encoder_new_with_string_id(pt->mime_type);
	g_message("Video encoder created: %x",stream->encoder);
	stream->decoder=ms_decoder_new_with_string_id(pt->mime_type);
	if ((stream->encoder==NULL) || (stream->decoder==NULL)){
		/* big problem: we have not a registered codec for this payload...*/
		video_stream_free(stream);
		g_error("videostream.c: No codecs available for payload %i.",payload);
		return NULL;
	}

	/* configure the filters */
	ms_video_source_set_device(MS_VIDEO_SOURCE(stream->source), device);
	ms_video_source_set_size(MS_VIDEO_SOURCE(stream->source), width, height);
	ms_video_source_set_frame_rate(MS_VIDEO_SOURCE(stream->source), 8, 1);
	fps = MS_VIDEO_SOURCE(stream->source)->frame_rate / MS_VIDEO_SOURCE(stream->source)->frame_rate_base;
	ms_video_source_start(MS_VIDEO_SOURCE(stream->source));
	format = ms_video_source_get_format(MS_VIDEO_SOURCE(stream->source));
	
	ms_AVencoder_set_format (MS_AVENCODER (stream->encoder), format);
	ms_AVencoder_set_width(MS_AVENCODER(stream->encoder), width);
	ms_AVencoder_set_height(MS_AVENCODER(stream->encoder), height);
	/* bitrate is based upon 30fps? adjust by our possibly lower framerate REVISIT later */
	/*ms_AVencoder_set_bit_rate(MS_AVENCODER(stream->encoder), pt->normal_bitrate * 30 / fps );*/
	ms_AVdecoder_set_format (MS_AVDECODER (stream->decoder), "YUV420P");
	ms_AVdecoder_set_width(MS_AVDECODER(stream->decoder), width);
	ms_AVdecoder_set_height(MS_AVDECODER(stream->decoder), height);
#ifdef USE_SDL
	/* we suppose our decoder and pin1 of encoder always outputs YUV420P */
	ms_sdl_out_set_format(MS_SDL_OUT(stream->output),"YUV420P");
#else
	ms_video_output_set_size (MS_VIDEO_OUTPUT (stream->output), width, height);
#endif

	/* and then connect all */
	ms_filter_add_link (stream->source, stream->encoder);
	ms_filter_add_link (stream->encoder, stream->rtpsend);
	
	ms_filter_add_link (stream->rtprecv, stream->decoder);
	ms_filter_add_link (stream->decoder, stream->output);
	if (show_local)
		ms_filter_add_link(stream->encoder,stream->output);

	/* create the synchronisation source */
	stream->timer = ms_timer_new(); 
	ms_sync_attach (stream->timer, stream->source);
	ms_sync_attach (stream->timer, stream->rtprecv);

	/* and start */
	ms_start (stream->timer);
	stream->show_local=show_local;
	return stream;
}



void
video_stream_stop (VideoStream * stream)
{

	ms_stop (stream->timer);
	ms_video_source_stop (MS_VIDEO_SOURCE(stream->source));
	ms_sync_detach (stream->timer, stream->source);
	ms_sync_detach (stream->timer, stream->rtprecv);
	
	ms_filter_remove_links(stream->source,stream->encoder);
	ms_filter_remove_links (stream->encoder,
				 stream->rtpsend);
	
	ms_filter_remove_links (stream->rtprecv,
				 stream->decoder);
	ms_filter_remove_links (stream->decoder,
				 stream->output);
	if (stream->show_local) {
		ms_filter_remove_links (stream->encoder,
					 stream->output);
	}
	video_stream_free (stream);
}


void video_stream_set_rtcp_information(VideoStream *st, const char *cname){
	if (st->send_session!=NULL){
		rtp_session_set_source_description(st->send_session,cname,NULL,NULL,NULL,NULL,"linphone-" LINPHONE_VERSION,
											"This is free software (GPL) !");
	}
}



VideoStream * video_preview_start(const gchar *source, const gchar *device){
	VideoStream *stream = g_new0 (VideoStream, 1);
	gchar *format;
	gint width = VIDEO_SIZE_CIF_W;
	gint height = VIDEO_SIZE_CIF_H;

	/* creates the filters */
	stream->source = ms_filter_new_with_name(source);
	if (stream->source == NULL){
		g_error("videostream.c: failed to create video source %s.", source);
		return NULL;
	}
#ifdef USE_SDL
	stream->output=ms_sdl_out_new();
#else
	stream->output = ms_video_output_new ();
#endif
	/* configure the filters */
	ms_video_source_set_device(MS_VIDEO_SOURCE(stream->source), device);
	ms_video_source_set_size(MS_VIDEO_SOURCE(stream->source), width, height);
	ms_video_source_set_frame_rate(MS_VIDEO_SOURCE(stream->source), 8, 1);
	
	ms_video_source_start(MS_VIDEO_SOURCE(stream->source));
	format = ms_video_source_get_format(MS_VIDEO_SOURCE(stream->source));
	
#ifdef USE_SDL
	ms_sdl_out_set_format(MS_SDL_OUT(stream->output),format);
#else
	ms_video_output_set_format(MS_VIDEO_OUTPUT(stream->output),format);
	ms_video_output_set_size (MS_VIDEO_OUTPUT (stream->output), width, height);
	ms_video_output_set_title(MS_VIDEO_OUTPUT(stream->output),"Linphone Video");
#endif
	/* and then connect all */
	ms_filter_add_link (stream->source, stream->output);
	/* create the synchronisation source */
	stream->timer = ms_timer_new(); 
	ms_sync_attach (stream->timer, stream->source);

	/* and start */
	ms_start (stream->timer);

	return stream;
}

void video_preview_stop(VideoStream *stream){
	ms_stop (stream->timer);
	ms_video_source_stop (MS_VIDEO_SOURCE(stream->source));
	ms_sync_detach (stream->timer, stream->source);
	ms_filter_remove_links(stream->source,stream->output);
	video_stream_free(stream);
}


VideoStream * video_stream_send_only_start(RtpProfile *profile, int locport, char *remip, int remport,
			 int payload, const gchar *source, const gchar *device)
{
	VideoStream *stream = g_new0 (VideoStream, 1);
	RtpSession *rtps, *rtpr;
	PayloadType *pt;
	gchar *format;
	gint width = VIDEO_SIZE_CIF_W;
	gint height = VIDEO_SIZE_CIF_H;
	float fps;

	create_duplex_rtpsession(profile,locport,remip,remport,payload,40,&rtpr);
	rtp_session_enable_adaptive_jitter_compensation(rtpr,FALSE);
	rtps=rtpr;
	ms_trace("sending video to %s:%i", remip4, remport);
	
	/* creates two rtp filters to recv send streams (remote part) */
	rtp_session_set_recv_buf_size(rtpr,MAX_RTP_SIZE);
	stream->rtpsend = ms_rtp_send_new ();
	if (remport>0) ms_rtp_send_set_session (MS_RTP_SEND (stream->rtpsend), rtps);

	pt=rtp_profile_get_payload(profile,payload);
	if (pt==NULL){
		g_error("videostream.c: undefined payload type.");
		return NULL;
	}
	ms_trace("videostream.c: getting codecs for %s", pt->mime_type);

	/* creates the filters */
	stream->source = ms_filter_new_with_name(source);
	if (stream->source == NULL){
		g_error("videostream.c: failed to create video source %s.", source);
		return NULL;
	}
	
	stream->encoder=ms_encoder_new_with_string_id(pt->mime_type);
	g_message("Video encoder created: %x",stream->encoder);
	if ((stream->encoder==NULL) ){
		/* big problem: we have not a registered codec for this payload...*/
		video_stream_free(stream);
		g_error("videostream.c: No codecs available for payload %i.",payload);
		return NULL;
	}

	/* configure the filters */
	ms_video_source_set_device(MS_VIDEO_SOURCE(stream->source), device);
	ms_video_source_set_size(MS_VIDEO_SOURCE(stream->source), width, height);
	ms_video_source_set_frame_rate(MS_VIDEO_SOURCE(stream->source), 8, 1);
	fps = MS_VIDEO_SOURCE(stream->source)->frame_rate / MS_VIDEO_SOURCE(stream->source)->frame_rate_base;
	ms_video_source_start(MS_VIDEO_SOURCE(stream->source));
	format = ms_video_source_get_format(MS_VIDEO_SOURCE(stream->source));
	
	ms_AVencoder_set_format (MS_AVENCODER (stream->encoder), format);
	ms_AVencoder_set_width(MS_AVENCODER(stream->encoder), width);
	ms_AVencoder_set_height(MS_AVENCODER(stream->encoder), height);
	/* bitrate is based upon 30fps? adjust by our possibly lower framerate */
	/*ms_AVencoder_set_bit_rate(MS_AVENCODER(stream->encoder), pt->normal_bitrate * 30 / fps );*/

	/* and then connect all */
	ms_filter_add_link (stream->source, stream->encoder);
	ms_filter_add_link (stream->encoder, stream->rtpsend);

	/* create the synchronisation source */
	stream->timer = ms_timer_new(); 
	ms_sync_attach (stream->timer, stream->source);

	/* and start */
	ms_start (stream->timer);
	return stream;
}

void video_stream_send_only_stop(VideoStream *stream){
	ms_stop (stream->timer);
	ms_video_source_stop (MS_VIDEO_SOURCE(stream->source));
	ms_sync_detach (stream->timer, stream->source);
	
	ms_filter_remove_links(stream->source,stream->encoder);
	ms_filter_remove_links (stream->encoder,
				 stream->rtpsend);
	
	video_stream_free(stream);
}

#endif
