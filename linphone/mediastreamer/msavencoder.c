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

#include "msavencoder.h"
#include "msutils.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netinet/in.h>			/* ntohl(3) */
#endif

extern MSCodecInfo MPEG4info;
extern MSCodecInfo MPEGinfo;
extern MSCodecInfo h263pinfo;

static MSAVEncoderClass *ms_avencoder_class=NULL;
static void ms_AVencoder_rtp_callback (AVCodecContext *ctx,void *data, int size, int packet_number);

MSFilter *ms_h263p_encoder_new()
{	
	/* temporily prefer CODEC_ID_H263 to CODEC_ID_H263P since 
		ffmpeg does not comply to H233P at profile=0.
	*/
	return ms_AVencoder_new_with_codec(CODEC_ID_H263P,&h263pinfo);
}

MSFilter *ms_mpeg_encoder_new()
{
	return ms_AVencoder_new_with_codec(CODEC_ID_MPEG1VIDEO, &MPEGinfo);
}

MSFilter *ms_mpeg4_encoder_new()
{
	return ms_AVencoder_new_with_codec(CODEC_ID_MPEG4,&MPEG4info);
}

MSFilter * ms_AVencoder_new_with_codec(enum CodecID codec_id, MSCodecInfo *info)
{
	MSAVEncoder *enc;
	AVCodec *avc;
	enc=g_malloc0(sizeof(MSAVEncoder));
	if (ms_avencoder_class==NULL)
	{
		 ms_avencoder_class=g_malloc0(sizeof(MSAVEncoderClass));
		 ms_AVencoder_class_init(ms_avencoder_class);
	}
	MS_FILTER(enc)->klass=(MSFilterClass*)ms_avencoder_class;
	avc=avcodec_find_encoder(codec_id);
	if (avc==NULL) g_error("unknown av codec.");
	ms_AVencoder_init(enc,avc);
	return MS_FILTER(enc);
}


void ms_AVencoder_init(MSAVEncoder *enc, AVCodec *codec)
{
	AVCodecContext *c=&enc->av_context;
	
	ms_filter_init(MS_FILTER(enc));
	MS_FILTER(enc)->inqueues=enc->q_inputs;
	MS_FILTER(enc)->outqueues=enc->q_outputs;
	/* put default values */
	memset(c, 0, sizeof(AVCodecContext));
	avcodec_get_context_defaults(c);
	
	/* put sample parameters */
	c->bit_rate = 50000;
	/* resolution must be a multiple of two */
	c->width = VIDEO_SIZE_CIF_W;  
	c->height = VIDEO_SIZE_CIF_H;
	ms_AVencoder_set_frame_rate(enc,15,1);
	c->gop_size = 10; /* emit one intra frame every x frames */
	c->rtp_mode = 1;
	c->rtp_payload_size = 1000;
	c->opaque = (void *) enc;
	c->rtp_callback = ms_AVencoder_rtp_callback;
	c->pix_fmt=PIX_FMT_YUV420P;
	
	enc->av_opened=0;
	enc->av_codec=codec;
	enc->yuv_buf=NULL;
	enc->comp_buf=NULL;
	/*set default input format */
	ms_AVencoder_set_format(enc,"RGB24");
	enc->outm=NULL;
}

void ms_AVencoder_class_init(MSAVEncoderClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	MS_FILTER_CLASS(klass)->info=0;
	MS_FILTER_CLASS(klass)->max_qinputs=MSAVENCODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->max_qoutputs=MSAVENCODER_MAX_OUTPUTS;
	MS_FILTER_CLASS(klass)->r_maxgran=0;
	MS_FILTER_CLASS(klass)->w_maxgran=0;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_AVencoder_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_AVencoder_process;
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"AVEncoder");
	
}

void ms_AVencoder_uninit(MSAVEncoder *enc)
{
	if (enc->av_opened)
		avcodec_close(&enc->av_context);
	if (enc->comp_buf!=NULL) {
		ms_buffer_destroy(enc->comp_buf);
		enc->comp_buf=NULL;
	}
	if (enc->yuv_buf!=NULL) {
		ms_buffer_destroy(enc->yuv_buf);
		enc->yuv_buf=NULL;
	}
	if (enc->outm!=NULL) ms_message_destroy(enc->outm);
		
}
void ms_AVencoder_destroy( MSAVEncoder *obj)
{
	ms_AVencoder_uninit(obj);
	g_free(obj);
}


void ms_AVencoder_set_frame_rate(MSAVEncoder *obj, gint frame_rate, gint frame_rate_base)
{
	obj->av_context.time_base.num = frame_rate;
	obj->av_context.time_base.den = frame_rate_base;
}

static void ms_AVencoder_rtp_callback (AVCodecContext *ctx, void *data, int size, int packet_number)
{
	MSAVEncoder *r = MS_AVENCODER(ctx->opaque);
	MSQueue *outq = r->q_outputs[0];
	MSMessage *outm;
	MSMessage *prev_outm=r->outm;
	guint32 *p = (guint32 *) data;
	gint gob_num = (ntohl(*p) >> 10) & 0x1f;
	
	/*g_message("ms_AVencoder_rtp_callback: packet %i, size %i, GOB number %i", packet_number, size, gob_num);*/
	ms_trace("ms_AVencoder_rtp_callback: received %08x %08x", ntohl(p[0]), ntohl(p[1]));
	/* set the H.263 Payload Header (RFC 2429) ie H263-1998 */
	p[0] = ntohl( (0x04000000) | (ntohl(p[0]) & 0x0000ffff) ); /* P=1, V=0, PLEN=0 */
	ms_trace("ms_AVencoder_rtp_callback: sending %08x %08x", ntohl(p[0]), ntohl(p[1]));
	outm = ms_message_new(size);
	memcpy(outm->data,data,size);
	r->outm=outm;
	
	/*g_message("output video packet of size %i",size);*/
	if (prev_outm) {
		if (gob_num==0) prev_outm->markbit=TRUE;
		ms_queue_put(outq, prev_outm);
	}
}

void ms_AVencoder_process(MSAVEncoder *r)
{
	AVFrame orig;
	AVFrame pict;
	AVCodecContext *c=&r->av_context;
	MSQueue *inq,*outq;
	MSMessage *inm,*outm;
	gint error;
	
	inq=r->q_inputs[0];
	outq=r->q_outputs[0];
	
	/* get a picture from the input queue */
	inm=ms_queue_get(inq);
	g_return_if_fail(inm!=NULL);
	
	/* allocate a new image */
	if (r->yuv_buf==NULL){
		gint bsize = avpicture_get_size(c->pix_fmt,c->width,c->height);
		r->yuv_buf=ms_buffer_new(bsize);
		r->yuv_buf->ref_count++;
		
		r->comp_buf=ms_buffer_new(bsize/2);
		r->comp_buf->ref_count++;
	}
	if (!r->av_opened || r->av_context.codec == NULL){
		error=avcodec_open(c, r->av_codec);
		ms_trace("image format is %i.",c->pix_fmt);
		if (error!=0) {
			g_warning("avcodec_open() failed: %i",error);
			return;
		}else r->av_opened=1;
	}
	outm=ms_message_alloc();
	/* convert image if necessary */
	if (r->input_pix_fmt!=c->pix_fmt){
		ms_trace("Changing picture format.");
		avpicture_fill((AVPicture*)&orig,inm->data,r->input_pix_fmt,c->width,c->height);
		avpicture_fill((AVPicture*)&pict,r->yuv_buf->buffer,c->pix_fmt,c->width,c->height);
		if (img_convert((AVPicture*)&pict,c->pix_fmt,(AVPicture*)&orig,r->input_pix_fmt,c->width,c->height) < 0) {
			g_warning("img_convert failed");
			return;
		}
		//if (pict.data[0]==NULL) g_error("img_convert failed.");
		ms_message_set_buf(outm,r->yuv_buf);
	}
	else 
	{
		avpicture_fill((AVPicture*)&pict,inm->data,c->pix_fmt,c->width,c->height);
		ms_message_set_buf(outm,inm->buffer);
	}
	/* timestamp used by ffmpeg, unset here */
	pict.pts=AV_NOPTS_VALUE;
    error=avcodec_encode_video(c, r->comp_buf->buffer, r->comp_buf->size, &pict);
    if (error<=0) ms_warning("ms_AVencoder_process: error %i.",error);
	else {
		ms_trace("ms_AVencoder_process: video encoding done");
		/* set the mark bit on the last packet, which contains the end of the frame */
		/*
		MSMessage *last=ms_queue_peek_last(r->q_outputs[0]);
		if (last!=NULL) last->markbit=TRUE;
		else g_warning("No last packet ?");
		*/
	}
	if (r->q_outputs[1]!=NULL) ms_queue_put(r->q_outputs[1],outm);
	else ms_message_destroy(outm);
    ms_message_destroy(inm);
}

gint ms_AVencoder_set_format(MSAVEncoder *enc, gchar *fmt)
{
	gint format;
	if (strcmp(fmt,"YUV420P")==0) format=PIX_FMT_YUV420P;
	else if (strcmp(fmt,"YUV422")==0) format=PIX_FMT_YUV422;
	else if (strcmp(fmt,"RGB24")==0) format=PIX_FMT_RGB24;	
	else if (strcmp(fmt,"BGR24")==0) format=PIX_FMT_BGR24;
	else if (strcmp(fmt,"YUV422P")==0) format=PIX_FMT_YUV422P;
	else if (strcmp(fmt,"YUV444P")==0) format=PIX_FMT_YUV444P;
	else {
		g_warning("ms_AVdecoder_set_format: unsupported format %s.",fmt);
		return -1;
	}
	enc->input_pix_fmt=format;
	
	return 0;
}
