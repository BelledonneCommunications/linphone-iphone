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

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#include "ffmpeg-priv.h"

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msvideo.h"
#include "rfc2429.h"


extern void ms_ffmpeg_check_init();

typedef struct DecState{
	AVCodecContext av_context;
	AVCodec *av_codec;
	enum CodecID codec;
	mblk_t *input;
	YuvBuf outbuf;
	mblk_t *yuv_msg;
	struct SwsContext *sws_ctx;
	enum PixelFormat output_pix_fmt;
	uint8_t dci[512];
	int dci_size;
	bool_t snow_initialized;
}DecState;


static void dec_init(MSFilter *f, enum CodecID cid){
	DecState *s=(DecState *)ms_new0(DecState,1);
	ms_ffmpeg_check_init();
	
	avcodec_get_context_defaults(&s->av_context);
	s->av_codec=NULL;
	s->codec=cid;
	s->input=NULL;
	s->yuv_msg=NULL;
	s->output_pix_fmt=PIX_FMT_YUV420P;
	s->snow_initialized=FALSE;
	s->outbuf.w=0;
	s->outbuf.h=0;
	s->sws_ctx=NULL;
	f->data=s;

	s->av_codec=avcodec_find_decoder(s->codec);
	if (s->av_codec==NULL){
		ms_error("Could not find decoder %i!",s->codec);
	}
	/*
	s->av_context.width=MS_VIDEO_SIZE_QCIF_W;
	s->av_context.height=MS_VIDEO_SIZE_QCIF_H;
	*/
}

static void dec_h263_init(MSFilter *f){
	dec_init(f,CODEC_ID_H263);
}

static void dec_mpeg4_init(MSFilter *f){
	dec_init(f,CODEC_ID_MPEG4);
}

static void dec_mjpeg_init(MSFilter *f){
	dec_init(f,CODEC_ID_MJPEG);
}

static void dec_snow_init(MSFilter *f){
	dec_init(f,CODEC_ID_SNOW);
}

static void dec_uninit(MSFilter *f){
	DecState *s=(DecState*)f->data;
	if (s->input!=NULL) freemsg(s->input);
	if (s->yuv_msg!=NULL) freemsg(s->yuv_msg);
	if (s->sws_ctx!=NULL){
		sws_freeContext(s->sws_ctx);
		s->sws_ctx=NULL;
	}
	ms_free(s);
}

static int dec_add_fmtp(MSFilter *f, void *data){
	const char *fmtp=(const char*)data;
	DecState *s=(DecState*)f->data;
	char config[512];
	if (fmtp_get_value(fmtp,"config",config,sizeof(config))){
		/*convert hexa decimal config string into a bitstream */
		int i,j,max=strlen(config);
		char octet[3];
		octet[2]=0;
		for(i=0,j=0;i<max;i+=2,++j){
			octet[0]=config[i];
			octet[1]=config[i+1];
			s->dci[j]=(uint8_t)strtol(octet,NULL,16);
		}
		s->dci_size=j;
		ms_message("Got mpeg4 config string: %s",config);
	}
	return 0;
}

static void dec_preprocess(MSFilter *f){
	DecState *s=(DecState*)f->data;
	int error;
	/* we must know picture size before initializing snow decoder*/
	if (s->codec!=CODEC_ID_SNOW){
		error=avcodec_open(&s->av_context, s->av_codec);
		if (error!=0) ms_error("avcodec_open() failed: %i",error);
		if (s->codec==CODEC_ID_MPEG4 && s->dci_size>0){
			s->av_context.extradata=s->dci;
			s->av_context.extradata_size=s->dci_size;
		}
	}
}

static void dec_postprocess(MSFilter *f){
	DecState *s=(DecState*)f->data;
	if (s->av_context.codec!=NULL){
		avcodec_close(&s->av_context);
		s->av_context.codec=NULL;
	}
}

static mblk_t * skip_rfc2190_header(mblk_t *inm){
	if (msgdsize(inm) >= 4) {
		uint8_t *ph = inm->b_rptr;
		int F = (ph[0]>>7) & 0x1;
		int P = (ph[0]>>6) & 0x1;
		if (F == 0) inm->b_rptr += 4;  // mode A
		else if (P == 0) inm->b_rptr += 8; // mode B
		else inm->b_rptr += 12;   // mode C
	} else {
		freemsg(inm);
		inm=NULL;
	}
	return inm;
}

static mblk_t * skip_rfc2429_header(mblk_t *inm){
	if (msgdsize(inm) >= 2){
		uint32_t *p = (uint32_t*)inm->b_rptr;
		uint8_t *ph=inm->b_rptr;
		int PLEN;
		int gob_num;
		bool_t P;
		
		P=rfc2429_get_P(ph);
		PLEN=rfc2429_get_PLEN(ph);
		/*printf("receiving new packet; P=%i; V=%i; PLEN=%i; PEBIT=%i\n",P,rfc2429_get_V(ph),PLEN,rfc2429_get_PEBIT(ph));
		*/
		gob_num = (ntohl(*p) >> 10) & 0x1f;
		/*ms_message("gob %i, size %i", gob_num, msgdsize(inm));
		ms_message("ms_AVdecoder_process: received %08x %08x", ntohl(p[0]), ntohl(p[1]));*/
		
		/* remove H.263 Payload Header */
		if (PLEN>0){
			/* we ignore the redundant picture header and
			directly go to the bitstream */
			inm->b_rptr+=PLEN;
		}
		if (P){
			inm->b_rptr[0]=inm->b_rptr[1]=0;
		}else{
			/* no PSC omitted */
			inm->b_rptr+=2;
		}
		return inm;
	}else freemsg(inm);
	return NULL;
}

static mblk_t * parse_snow_header(DecState *s,mblk_t *inm){
	if (msgdsize(inm) >= 4){
		uint32_t h = ntohl(*(uint32_t*)inm->b_rptr);
		if (!s->snow_initialized){
			int error;
			s->av_context.width=h>>16;
			s->av_context.height=h&0xffff;
			error=avcodec_open(&s->av_context, s->av_codec);
			if (error!=0) ms_error("avcodec_open() failed for snow: %i",error);
			else {
				s->snow_initialized=TRUE;
				ms_message("Snow decoder initialized,size=%ix%i",
				s->av_context.width,
				s->av_context.height);
			}
		}
		inm->b_rptr+=4;
		return inm;
	}else {
		freemsg(inm);
		return NULL;
	}
}

static mblk_t *get_as_yuvmsg(MSFilter *f, DecState *s, AVFrame *orig){
	AVCodecContext *ctx=&s->av_context;

	if (s->outbuf.w!=ctx->width || s->outbuf.h!=ctx->height){
		if (s->sws_ctx!=NULL){
			sws_freeContext(s->sws_ctx);
			s->sws_ctx=NULL;
		}
		s->yuv_msg=yuv_buf_alloc(&s->outbuf,ctx->width,ctx->height);
		s->outbuf.w=ctx->width;
		s->outbuf.h=ctx->height;
		s->sws_ctx=sws_getContext(ctx->width,ctx->height,ctx->pix_fmt,
			ctx->width,ctx->height,s->output_pix_fmt,SWS_FAST_BILINEAR,
                	NULL, NULL, NULL);
	}
	if (sws_scale(s->sws_ctx,orig->data,orig->linesize, 0,
					ctx->height, s->outbuf.planes, s->outbuf.strides)<0){
		ms_error("%s: error in sws_scale().",f->desc->name);
	}
	return dupmsg(s->yuv_msg);
}

static void dec_process_frame(MSFilter *f, mblk_t *inm){
	DecState *s=(DecState*)f->data;
	AVFrame orig;
	int got_picture;
	/* get a picture from the input queue */
	
	if (f->desc->id==MS_H263_DEC_ID) inm=skip_rfc2429_header(inm);
	else if (f->desc->id==MS_H263_OLD_DEC_ID) inm=skip_rfc2190_header(inm);
	else if (s->codec==CODEC_ID_SNOW && s->input==NULL) inm=parse_snow_header(s,inm);
	if (inm){
		/* accumulate the video packet until we have the rtp markbit*/
		if (s->input==NULL){
			s->input=inm;
		}else{
			concatb(s->input,inm);
		}
		
		if (mblk_get_marker_info(inm)){
			mblk_t *frame;
			int remain,len;
			/*ms_message("got marker bit !");*/
			/*append some padding bytes for ffmpeg to safely 
			read extra bytes...*/
			msgpullup(s->input,msgdsize(s->input)+8);
			frame=s->input;
			s->input=NULL;
			while ( (remain=frame->b_wptr-frame->b_rptr)> 0) {
				AVPacket pkt;
				av_init_packet(&pkt);
				pkt.data = frame->b_rptr;
				pkt.size = remain;
				len=avcodec_decode_video2(&s->av_context,&orig,&got_picture,&pkt);
				/*len=avcodec_decode_video(&s->av_context,&orig,&got_picture,(uint8_t*)frame->b_rptr,remain );*/
				if (len<=0) {
					ms_warning("ms_AVdecoder_process: error %i.",len);
					break;
				}
				if (got_picture) {
					ms_queue_put(f->outputs[0],get_as_yuvmsg(f,s,&orig));
				}
				frame->b_rptr+=len;
			}
			freemsg(frame);
		}
	}
}

static void dec_process(MSFilter *f){
	mblk_t *inm;
	while((inm=ms_queue_get(f->inputs[0]))!=0){
		dec_process_frame(f,inm);
	}
}


static MSFilterMethod methods[]={
	{		MS_FILTER_ADD_FMTP		,	dec_add_fmtp	},
	{		0		,		NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_h263_dec_desc={
	MS_H263_DEC_ID,
	"MSH263Dec",
	N_("A H.263 decoder using ffmpeg library"),
	MS_FILTER_DECODER,
	"H263-1998",
	1,
	1,
	dec_h263_init,
	dec_preprocess,
	dec_process,
	dec_postprocess,
	dec_uninit,
	methods
};

MSFilterDesc ms_h263_old_dec_desc={
	MS_H263_OLD_DEC_ID,
	"MSH263OldDec",
	N_("A H.263 decoder using ffmpeg library"),
	MS_FILTER_DECODER,
	"H263",
	1,
	1,
	dec_h263_init,
	dec_preprocess,
	dec_process,
	dec_postprocess,
	dec_uninit,
	methods
};


MSFilterDesc ms_mpeg4_dec_desc={
	MS_MPEG4_DEC_ID,
	"MSMpeg4Dec",
	N_("A MPEG4 decoder using ffmpeg library"),
	MS_FILTER_DECODER,
	"MP4V-ES",
	1,
	1,
	dec_mpeg4_init,
	dec_preprocess,
	dec_process,
	dec_postprocess,
	dec_uninit,
	methods
};

MSFilterDesc ms_mjpeg_dec_desc={
	MS_MJPEG_DEC_ID,
	"MSMJpegDec",
	N_("A MJPEG decoder using ffmpeg library"),
	MS_FILTER_DECODER,
	"JPEG",
	1,
	1,
	dec_mjpeg_init,
	dec_preprocess,
	dec_process,
	dec_postprocess,
	dec_uninit,
	methods
};

MSFilterDesc ms_snow_dec_desc={
	MS_SNOW_DEC_ID,
	"MSSnowDec",
	N_("A snow decoder using ffmpeg library"),
	MS_FILTER_DECODER,
	"snow",
	1,
	1,
	dec_snow_init,
	dec_preprocess,
	dec_process,
	dec_postprocess,
	dec_uninit,
	methods
};

#else

MSFilterDesc ms_h263_dec_desc={
	.id=MS_H263_DEC_ID,
	.name="MSH263Dec",
	.text=N_("A H.263 decoder using ffmpeg library"),
	.category=MS_FILTER_DECODER,
	.enc_fmt="H263-1998",
	.ninputs=1,
	.noutputs=1,
	.init=dec_h263_init,
	.preprocess=dec_preprocess,
	.process=dec_process,
	.postprocess=dec_postprocess,
	.uninit=dec_uninit,
	.methods= methods
};

MSFilterDesc ms_h263_old_dec_desc={
	.id=MS_H263_OLD_DEC_ID,
	.name="MSH263OldDec",
	.text=N_("A H.263 decoder using ffmpeg library"),
	.category=MS_FILTER_DECODER,
	.enc_fmt="H263",
	.ninputs=1,
	.noutputs=1,
	.init=dec_h263_init,
	.preprocess=dec_preprocess,
	.process=dec_process,
	.postprocess=dec_postprocess,
	.uninit=dec_uninit,
	.methods= methods
};


MSFilterDesc ms_mpeg4_dec_desc={
	.id=MS_MPEG4_DEC_ID,
	.name="MSMpeg4Dec",
	.text="A MPEG4 decoder using ffmpeg library",
	.category=MS_FILTER_DECODER,
	.enc_fmt="MP4V-ES",
	.ninputs=1,
	.noutputs=1,
	.init=dec_mpeg4_init,
	.preprocess=dec_preprocess,
	.process=dec_process,
	.postprocess=dec_postprocess,
	.uninit=dec_uninit,
	.methods= methods
};

MSFilterDesc ms_mjpeg_dec_desc={
	.id=MS_MJPEG_DEC_ID,
	.name="MSMJpegDec",
	.text="A MJEPG decoder using ffmpeg library",
	.category=MS_FILTER_DECODER,
	.enc_fmt="JPEG",
	.ninputs=1,
	.noutputs=1,
	.init=dec_mjpeg_init,
	.preprocess=dec_preprocess,
	.process=dec_process,
	.postprocess=dec_postprocess,
	.uninit=dec_uninit,
	.methods= methods
};

MSFilterDesc ms_snow_dec_desc={
	.id=MS_SNOW_DEC_ID,
	.name="MSSnowDec",
	.text="A snow decoder using ffmpeg library",
	.category=MS_FILTER_DECODER,
	.enc_fmt="x-snow",
	.ninputs=1,
	.noutputs=1,
	.init=dec_snow_init,
	.preprocess=dec_preprocess,
	.process=dec_process,
	.postprocess=dec_postprocess,
	.uninit=dec_uninit,
	.methods= methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_mpeg4_dec_desc)
MS_FILTER_DESC_EXPORT(ms_h263_dec_desc)
MS_FILTER_DESC_EXPORT(ms_h263_old_dec_desc)
MS_FILTER_DESC_EXPORT(ms_mjpeg_dec_desc)
MS_FILTER_DESC_EXPORT(ms_snow_dec_desc)
