/*
mediastreamer2 x264 plugin
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

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/rfc3984.h"

#include "ortp/b64.h"

#include <x264.h>

#ifdef HAVE_LIBAVCODEC_AVCODEC_H
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#else
#include <ffmpeg/avcodec.h>
#include <ffmpeg/swscale.h>
#endif

#define REMOVE_PREVENTING_BYTES 1

typedef struct _EncData{
	x264_t *enc;
	MSVideoSize vsize;
	int bitrate;
	float fps;
	int mode;
	uint64_t framenum;
	Rfc3984Context packer;
	int keyframe_int;
	bool_t generate_keyframe;
}EncData;


static void enc_init(MSFilter *f){
	EncData *d=ms_new(EncData,1);
	d->enc=NULL;
	d->bitrate=384000;
	d->vsize=MS_VIDEO_SIZE_CIF;
	d->fps=30;
	d->keyframe_int=10; /*10 seconds */
	d->mode=0;
	d->framenum=0;
	d->generate_keyframe=FALSE;
	f->data=d;
}

static void enc_uninit(MSFilter *f){
	EncData *d=(EncData*)f->data;
	ms_free(d);
}

static void enc_preprocess(MSFilter *f){
	EncData *d=(EncData*)f->data;
	x264_param_t params;
	
	rfc3984_init(&d->packer);
	rfc3984_set_mode(&d->packer,d->mode);
	rfc3984_enable_stap_a(&d->packer,FALSE);
	
	x264_param_default(&params);
	params.i_threads=1;
	params.i_sync_lookahead=0;
	params.i_width=d->vsize.width;
	params.i_height=d->vsize.height;
	params.i_fps_num=(int)d->fps;
	params.i_fps_den=1;
	params.i_slice_max_size=ms_get_payload_max_size()-100; /*-100 security margin*/
	/*params.i_level_idc=30;*/
	
	params.rc.i_rc_method = X264_RC_ABR;
	params.rc.i_bitrate=(int)( ( ((float)d->bitrate)*0.8)/1000.0);
	params.rc.f_rate_tolerance=0.1;
	params.rc.i_vbv_max_bitrate=(int) (((float)d->bitrate)*0.9/1000.0);
	params.rc.i_vbv_buffer_size=params.rc.i_vbv_max_bitrate;
	params.rc.f_vbv_buffer_init=0.5;
	params.rc.i_lookahead=0;
	/*enable this by config ?*/
	/*
	params.i_keyint_max = (int)d->fps*d->keyframe_int;
	params.i_keyint_min = (int)d->fps;
	*/
	params.b_repeat_headers=1;
	params.b_cabac=0;//disable cabac to be baseline
	params.i_bframe=0;/*no B frames*/
	d->enc=x264_encoder_open(&params);
	if (d->enc==NULL) ms_error("Fail to create x264 encoder.");
	d->framenum=0;
}

static void x264_nals_to_msgb(x264_nal_t *xnals, int num_nals, MSQueue * nalus){
	int i;
	mblk_t *m;
	/*int bytes;*/
	for (i=0;i<num_nals;++i){
		m=allocb(xnals[i].i_payload+10,0);
		
		memcpy(m->b_wptr,xnals[i].p_payload+4,xnals[i].i_payload-4);
		m->b_wptr+=xnals[i].i_payload-4;
		if (xnals[i].i_type==7) {
			ms_message("A SPS is being sent.");
		}else if (xnals[i].i_type==8) {
			ms_message("A PPS is being sent.");
		}
		ms_queue_put(nalus,m);
	}
}

static void enc_process(MSFilter *f){
	EncData *d=(EncData*)f->data;
	uint32_t ts=f->ticker->time*90LL;
	mblk_t *im;
	MSPicture pic;
	MSQueue nalus;
	ms_queue_init(&nalus);
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		if (yuv_buf_init_from_mblk(&pic,im)==0){
			x264_picture_t xpic;
			x264_picture_t oxpic;
			x264_nal_t *xnals=NULL;
			int num_nals=0;

			/*send I frame 2 seconds and 4 seconds after the beginning */
			if (d->framenum==(int)d->fps*2 || d->framenum==(int)d->fps*4)
				d->generate_keyframe=TRUE;

			if (d->generate_keyframe){
				xpic.i_type=X264_TYPE_IDR;
				d->generate_keyframe=FALSE;
			}else xpic.i_type=X264_TYPE_AUTO;
			xpic.i_qpplus1=0;
			xpic.i_pts=d->framenum;
			xpic.param=NULL;
			xpic.img.i_csp=X264_CSP_I420;
			xpic.img.i_plane=3;
			xpic.img.i_stride[0]=pic.strides[0];
			xpic.img.i_stride[1]=pic.strides[1];
			xpic.img.i_stride[2]=pic.strides[2];
			xpic.img.i_stride[3]=0;
			xpic.img.plane[0]=pic.planes[0];
			xpic.img.plane[1]=pic.planes[1];
			xpic.img.plane[2]=pic.planes[2];
			xpic.img.plane[3]=0;
			if (x264_encoder_encode(d->enc,&xnals,&num_nals,&xpic,&oxpic)>=0){
				x264_nals_to_msgb(xnals,num_nals,&nalus);
				rfc3984_pack(&d->packer,&nalus,f->outputs[0],ts);
				d->framenum++;
			}else{
				ms_error("x264_encoder_encode() error.");
			}
		}
		freemsg(im);
	}
}

static void enc_postprocess(MSFilter *f){
	EncData *d=(EncData*)f->data;
	rfc3984_uninit(&d->packer);
	if (d->enc!=NULL){
		x264_encoder_close(d->enc);
		d->enc=NULL;
	}
}

static int enc_set_br(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	d->bitrate=*(int*)arg;

	if (d->bitrate>=1024000){
		d->vsize=MS_VIDEO_SIZE_VGA;
		d->fps=25;
	}else if (d->bitrate>=512000){
		d->vsize=MS_VIDEO_SIZE_VGA;
		d->fps=15;
        }else if (d->bitrate>=384000){
		d->vsize=MS_VIDEO_SIZE_CIF;
		d->fps=30;
	}else if (d->bitrate>=256000){
		d->vsize=MS_VIDEO_SIZE_CIF;
		d->fps=15;
	}else if (d->bitrate>=128000){
		d->vsize=MS_VIDEO_SIZE_CIF;
		d->fps=15;
	}else if (d->bitrate>=64000){
		d->vsize=MS_VIDEO_SIZE_CIF;
		d->fps=10;
	}else if (d->bitrate>=32000){
		d->vsize=MS_VIDEO_SIZE_QCIF;
		d->fps=10;
	}else{
		d->vsize=MS_VIDEO_SIZE_QCIF;
		d->fps=5;
	}
	ms_message("bitrate set to %i",d->bitrate);
	return 0;
}

static int enc_set_fps(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	d->fps=*(float*)arg;
	return 0;
}

static int enc_get_fps(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	*(float*)arg=d->fps;
	return 0;
}

static int enc_get_vsize(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	*(MSVideoSize*)arg=d->vsize;
	return 0;
}

static int enc_set_vsize(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	d->vsize=*(MSVideoSize*)arg;
	return 0;
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	const char *fmtp=(const char *)arg;
	char value[12];
	if (fmtp_get_value(fmtp,"packetization-mode",value,sizeof(value))){
		d->mode=atoi(value);
		ms_message("packetization-mode set to %i",d->mode);
	}
	return 0;
}

static int enc_req_vfu(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	d->generate_keyframe=TRUE;
	return 0;
}


static MSFilterMethod enc_methods[]={
	{	MS_FILTER_SET_FPS	,	enc_set_fps	},
	{	MS_FILTER_SET_BITRATE	,	enc_set_br	},
	{	MS_FILTER_GET_FPS	,	enc_get_fps	},
	{	MS_FILTER_GET_VIDEO_SIZE,	enc_get_vsize	},
	{	MS_FILTER_SET_VIDEO_SIZE,	enc_set_vsize	},
	{	MS_FILTER_ADD_FMTP	,	enc_add_fmtp	},
	{	MS_FILTER_REQ_VFU	,	enc_req_vfu	},
	{	0	,			NULL		}
};

static MSFilterDesc x264_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSX264Enc",
	.text="A H264 encoder based on x264 project (with multislicing enabled)",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="H264",
	.ninputs=1,
	.noutputs=1,
	.init=enc_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.postprocess=enc_postprocess,
	.uninit=enc_uninit,
	.methods=enc_methods
};

typedef struct _DecData{
	mblk_t *yuv_msg;
	mblk_t *sps,*pps;
	Rfc3984Context unpacker;
	MSPicture outbuf;
	struct SwsContext *sws_ctx;
	AVCodecContext av_context;
	unsigned int packet_num;
	uint8_t *bitstream;
	int bitstream_size;
}DecData;

static void ffmpeg_init(){
	static bool_t done=FALSE;
	if (!done){
		avcodec_init();
		avcodec_register_all();
		done=TRUE;
	}
}

static void dec_open(DecData *d){
	AVCodec *codec;
	int error;
	codec=avcodec_find_decoder(CODEC_ID_H264);
	if (codec==NULL) ms_fatal("Could not find H264 decoder in ffmpeg.");
	avcodec_get_context_defaults(&d->av_context);
	error=avcodec_open(&d->av_context,codec);
	if (error!=0){
		ms_fatal("avcodec_open() failed.");
	}
}

static void dec_init(MSFilter *f){
	DecData *d=(DecData*)ms_new(DecData,1);
	ffmpeg_init();
	d->yuv_msg=NULL;
	d->sps=NULL;
	d->pps=NULL;
	d->sws_ctx=NULL;
	rfc3984_init(&d->unpacker);
	d->packet_num=0;
	dec_open(d);
	d->outbuf.w=0;
	d->outbuf.h=0;
	d->bitstream_size=65536;
	d->bitstream=ms_malloc0(d->bitstream_size);
	f->data=d;
}

static void dec_reinit(DecData *d){
	avcodec_close(&d->av_context);
	dec_open(d);
}

static void dec_uninit(MSFilter *f){
	DecData *d=(DecData*)f->data;
	rfc3984_uninit(&d->unpacker);
	avcodec_close(&d->av_context);
	if (d->yuv_msg) freemsg(d->yuv_msg);
	if (d->sps) freemsg(d->sps);
	if (d->pps) freemsg(d->pps);
	ms_free(d->bitstream);
	ms_free(d);
}

static mblk_t *get_as_yuvmsg(MSFilter *f, DecData *s, AVFrame *orig){
	AVCodecContext *ctx=&s->av_context;

	if (s->outbuf.w!=ctx->width || s->outbuf.h!=ctx->height){
		if (s->sws_ctx!=NULL){
			sws_freeContext(s->sws_ctx);
			s->sws_ctx=NULL;
			freemsg(s->yuv_msg);
			s->yuv_msg=NULL;
		}
		ms_message("Getting yuv picture of %ix%i",ctx->width,ctx->height);
		s->yuv_msg=yuv_buf_alloc(&s->outbuf,ctx->width,ctx->height);
		s->outbuf.w=ctx->width;
		s->outbuf.h=ctx->height;
		s->sws_ctx=sws_getContext(ctx->width,ctx->height,ctx->pix_fmt,
			ctx->width,ctx->height,PIX_FMT_YUV420P,SWS_FAST_BILINEAR,
                	NULL, NULL, NULL);
	}
	if (sws_scale(s->sws_ctx,orig->data,orig->linesize, 0,
					ctx->height, s->outbuf.planes, s->outbuf.strides)<0){
		ms_error("%s: error in sws_scale().",f->desc->name);
	}
	return dupmsg(s->yuv_msg);
}

static void update_sps(DecData *d, mblk_t *sps){
	if (d->sps)
		freemsg(d->sps);
	d->sps=dupb(sps);
}

static void update_pps(DecData *d, mblk_t *pps){
	if (d->pps)
		freemsg(d->pps);
	if (pps) d->pps=dupb(pps);
	else d->pps=NULL;
}

static bool_t check_sps_pps_change(DecData *d, mblk_t *sps, mblk_t *pps){
	bool_t ret1=FALSE,ret2=FALSE;
	if (d->sps){
		if (sps){
			ret1=(msgdsize(sps)!=msgdsize(d->sps)) || (memcmp(d->sps->b_rptr,sps->b_rptr,msgdsize(sps))!=0);
			if (ret1) {
				update_sps(d,sps);
				ms_message("SPS changed !");
				update_pps(d,NULL);
			}
		}
	}else if (sps) {
		ms_message("Receiving first SPS");
		update_sps(d,sps);
	}
	if (d->pps){
		if (pps){
			ret2=(msgdsize(pps)!=msgdsize(d->pps)) || (memcmp(d->pps->b_rptr,pps->b_rptr,msgdsize(pps))!=0);
			if (ret2) {
				update_sps(d,pps);
				ms_message("PPS changed ! %i,%i",msgdsize(pps),msgdsize(d->pps));
			}
		}
	}else if (pps) {
		ms_message("Receiving first PPS");
		update_pps(d,pps);
	}
	return ret1 || ret2;
}

static void enlarge_bitstream(DecData *d, int new_size){
	d->bitstream_size=new_size;
	d->bitstream=ms_realloc(d->bitstream,d->bitstream_size);
}

static int nalusToFrame(DecData *d, MSQueue *naluq, bool_t *new_sps_pps){
	mblk_t *im;
	uint8_t *dst=d->bitstream,*src,*end;
	int nal_len;
	bool_t start_picture=TRUE;
	uint8_t nalu_type;
	*new_sps_pps=FALSE;
	end=d->bitstream+d->bitstream_size;
	while((im=ms_queue_get(naluq))!=NULL){
		src=im->b_rptr;
		nal_len=im->b_wptr-src;
		if (dst+nal_len+100>end){
			int pos=dst-d->bitstream;
			enlarge_bitstream(d, d->bitstream_size+nal_len+100);
			dst=d->bitstream+pos;
			end=d->bitstream+d->bitstream_size;
		}
		nalu_type=(*src) & ((1<<5)-1);
		if (nalu_type==7)
			*new_sps_pps=check_sps_pps_change(d,im,NULL) || *new_sps_pps;
		if (nalu_type==8)
			*new_sps_pps=check_sps_pps_change(d,NULL,im) || *new_sps_pps;
		if (start_picture || nalu_type==7/*SPS*/ || nalu_type==8/*PPS*/ ){
			*dst++=0;
			start_picture=FALSE;
		}
		/*prepend nal marker*/
		*dst++=0;
		*dst++=0;
		*dst++=1;
		*dst++=*src++;
		while(src<(im->b_wptr-3)){
			if (src[0]==0 && src[1]==0 && src[2]<3){
				*dst++=0;
				*dst++=0;
				*dst++=3;
				src+=2;
			}
			*dst++=*src++;
		}
		*dst++=*src++;
		*dst++=*src++;
		*dst++=*src++;
		freemsg(im);
	}
	return dst-d->bitstream;
}

static void dec_process(MSFilter *f){
	DecData *d=(DecData*)f->data;
	mblk_t *im;
	MSQueue nalus;
	AVFrame orig;
	ms_queue_init(&nalus);
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		/*push the sps/pps given in sprop-parameter-sets if any*/
		if (d->packet_num==0 && d->sps && d->pps){
			mblk_set_timestamp_info(d->sps,mblk_get_timestamp_info(im));
			mblk_set_timestamp_info(d->pps,mblk_get_timestamp_info(im));
			rfc3984_unpack(&d->unpacker,d->sps,&nalus);
			rfc3984_unpack(&d->unpacker,d->pps,&nalus);
			d->sps=NULL;
			d->pps=NULL;
		}
		rfc3984_unpack(&d->unpacker,im,&nalus);
		if (!ms_queue_empty(&nalus)){
			int size;
			uint8_t *p,*end;
			bool_t need_reinit=FALSE;

			size=nalusToFrame(d,&nalus,&need_reinit);
			if (need_reinit)
				dec_reinit(d);
			p=d->bitstream;
			end=d->bitstream+size;
			while (end-p>0) {
				int len;
				int got_picture=0;
				AVPacket pkt;
				avcodec_get_frame_defaults(&orig);
				av_init_packet(&pkt);
				pkt.data = p;
				pkt.size = end-p;
				len=avcodec_decode_video2(&d->av_context,&orig,&got_picture,&pkt);
				if (len<=0) {
					ms_warning("ms_AVdecoder_process: error %i.",len);
					break;
				}
				if (got_picture) {
					ms_queue_put(f->outputs[0],get_as_yuvmsg(f,d,&orig));
				}
				p+=len;
			}
		}
		d->packet_num++;
	}
}

static int dec_add_fmtp(MSFilter *f, void *arg){
	DecData *d=(DecData*)f->data;
	const char *fmtp=(const char *)arg;
	char value[256];
	if (fmtp_get_value(fmtp,"sprop-parameter-sets",value,sizeof(value))){
		char * b64_sps=value;
		char * b64_pps=strchr(value,',');
		if (b64_pps){
			*b64_pps='\0';
			++b64_pps;
			ms_message("Got sprop-parameter-sets : sps=%s , pps=%s",b64_sps,b64_pps);
			d->sps=allocb(sizeof(value),0);
			d->sps->b_wptr+=b64_decode(b64_sps,strlen(b64_sps),d->sps->b_wptr,sizeof(value));
			d->pps=allocb(sizeof(value),0);
			d->pps->b_wptr+=b64_decode(b64_pps,strlen(b64_pps),d->pps->b_wptr,sizeof(value));
		}
	}
	return 0;
}

static MSFilterMethod  h264_dec_methods[]={
	{	MS_FILTER_ADD_FMTP	,	dec_add_fmtp	},
	{	0			,	NULL	}
};

static MSFilterDesc h264_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSH264Dec",
	.text="A H264 decoder based on ffmpeg project.",
	.category=MS_FILTER_DECODER,
	.enc_fmt="H264",
	.ninputs=1,
	.noutputs=1,
	.init=dec_init,
	.process=dec_process,
	.uninit=dec_uninit,
	.methods=h264_dec_methods
};

void libmsx264_init(void){
	ms_filter_register(&x264_enc_desc);
	ms_filter_register(&h264_dec_desc);
}
