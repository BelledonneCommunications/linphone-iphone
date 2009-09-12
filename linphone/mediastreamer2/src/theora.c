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

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msvideo.h"

#include <theora/theora.h>

typedef struct EncState{
	theora_state tstate;
	theora_info tinfo;
	yuv_buffer yuv;
	mblk_t *packed_conf;
	uint64_t start_time;
	uint64_t conf_time;
	unsigned int mtu;
	unsigned int nframes;
} EncState;

static void enc_init(MSFilter *f){
	EncState *s=(EncState *)ms_new(EncState,1);
	theora_info_init(&s->tinfo);
	s->tinfo.width=MS_VIDEO_SIZE_CIF_W;
	s->tinfo.height=MS_VIDEO_SIZE_CIF_H;
	s->tinfo.frame_width=MS_VIDEO_SIZE_CIF_W;
	s->tinfo.frame_height=MS_VIDEO_SIZE_CIF_H;
	s->tinfo.offset_x=0;
	s->tinfo.offset_y=0;
	s->tinfo.target_bitrate=500000;
	s->tinfo.pixelformat=OC_PF_420;
	s->tinfo.fps_numerator=15;
	s->tinfo.fps_denominator=1;
	s->tinfo.aspect_numerator=1;
	s->tinfo.aspect_denominator=1;
	s->tinfo.colorspace=OC_CS_UNSPECIFIED;
	s->tinfo.dropframes_p=0;
	s->tinfo.quick_p=1;
	s->tinfo.quality=63;
	s->tinfo.keyframe_auto_p=1;
	s->tinfo.keyframe_frequency=64;
	s->tinfo.keyframe_frequency_force=64;
	s->tinfo.keyframe_data_target_bitrate=s->tinfo.target_bitrate*1.2;
	s->tinfo.keyframe_auto_threshold=80;
	s->tinfo.keyframe_mindistance=8;
	s->tinfo.noise_sensitivity=1;
	s->packed_conf=NULL;
	s->start_time=0;
	s->conf_time=0;
	s->mtu=ms_get_payload_max_size()-6;
	s->nframes=0;
	f->data=s;
}

static void enc_uninit(MSFilter *f){
	EncState *s=(EncState*)f->data;
	theora_info_clear(&s->tinfo);
	ms_free(s);
}

static int enc_set_vsize(MSFilter *f, void*data){
	MSVideoSize *vs=(MSVideoSize*)data;
	EncState *s=(EncState*)f->data;
	s->tinfo.width=vs->width;
	s->tinfo.height=vs->height;
	s->tinfo.frame_width=vs->width;
	s->tinfo.frame_height=vs->height;
	return 0;
}

static int enc_get_vsize(MSFilter *f, void *data){
	EncState *s=(EncState*)f->data;
	MSVideoSize *vs=(MSVideoSize*)data;
	vs->width=s->tinfo.width;
	vs->height=s->tinfo.height;
	return 0;
}

static int enc_add_attr(MSFilter *f, void*data){
	/*const char *attr=(const char*)data;
	EncState *s=(EncState*)f->data;*/
	return 0;
}

static int enc_set_fps(MSFilter *f, void *data){
	float *fps=(float*)data;
	EncState *s=(EncState*)f->data;
	s->tinfo.fps_numerator=*fps;
	s->tinfo.keyframe_frequency=(*fps)*5;
	s->tinfo.keyframe_frequency_force=(*fps)*5;
	return 0;
}

static int enc_get_fps(MSFilter *f, void *data){
	EncState *s=(EncState*)f->data;
	float *fps=(float*)data;
	*fps=s->tinfo.fps_numerator;
	return 0;
}

static int enc_set_br(MSFilter *f, void*data){
	int br=*(int*)data;
	EncState *s=(EncState*)f->data;
	MSVideoSize vsize;
	float fps;
	float codecbr=(float)br;
	vsize.width=s->tinfo.width;
	vsize.height=s->tinfo.height;
	fps=s->tinfo.fps_numerator;
	s->tinfo.target_bitrate=codecbr*0.9;
	s->tinfo.keyframe_data_target_bitrate=codecbr;
	/*those default settings would need to be affined*/
	if (br>=1024000){
		vsize.width = MS_VIDEO_SIZE_4CIF_W;
		vsize.height = MS_VIDEO_SIZE_4CIF_H;
		s->tinfo.quality=15;
		fps=30;
	}else if (br>=512000){
		vsize.width = MS_VIDEO_SIZE_CIF_W;
		vsize.height = MS_VIDEO_SIZE_CIF_H;
		s->tinfo.quality=15;
		fps=15;
	}else if (br>=256000){
		vsize.width = MS_VIDEO_SIZE_CIF_W;
		vsize.height = MS_VIDEO_SIZE_CIF_H;
		s->tinfo.quality=5;
		fps=15;
	}else if(br>=128000){
		vsize.width=MS_VIDEO_SIZE_QCIF_W;
		vsize.height=MS_VIDEO_SIZE_QCIF_H;
		s->tinfo.quality=20;
		fps=10;
	}else if(br>=64000){
		vsize.width=MS_VIDEO_SIZE_QCIF_W;
		vsize.height=MS_VIDEO_SIZE_QCIF_H;
		s->tinfo.quality=7;
		fps=7;
	}
	enc_set_vsize(f,&vsize);
	enc_set_fps(f,&fps);
	return 0;
}

static int enc_set_mtu(MSFilter *f, void*data){
	EncState *s=(EncState*)f->data;
	s->mtu=*(int*)data;
	return 0;
}

#define THEORA_RAW_DATA	0
#define THEORA_PACKED_CONF 1
#define THEORA_COMMENT 2
#define THEORA_RESERVED 3

#define NOT_FRAGMENTED 0
#define START_FRAGMENT 1
#define CONT_FRAGMENT 2
#define END_FRAGMENT 3


static inline void payload_header_set(uint8_t *buf, uint32_t ident, uint8_t ft, uint8_t tdt, uint8_t pkts){
	uint32_t tmp;
	tmp=((ident&0xFFFFFF)<<8) | ((ft&0x3)<<6) | ((tdt&0x3)<<4) | (pkts&0xf);
	*((uint32_t*)buf)=htonl(tmp);
}

static inline uint32_t payload_header_get_ident(uint8_t *buf){
	uint32_t *tmp=(uint32_t*)buf;
	return (ntohl(*tmp)>>8) & 0xFFFFFF;
}

static inline uint32_t payload_header_get_tdt(uint8_t *buf){
	uint32_t *tmp=(uint32_t*)buf;
	return ((ntohl(*tmp))>>4) & 0x3;
}

static inline uint32_t payload_header_get_ft(uint8_t *buf){
	uint32_t *tmp=(uint32_t*)buf;
	return ((ntohl(*tmp))>>6) & 0x3;
}

static inline uint32_t payload_header_get_pkts(uint8_t *buf){
	uint32_t *tmp=(uint32_t*)buf;
	return ntohl(*tmp) & 0xf;
}

static int create_packed_conf(EncState *s){
	ogg_packet p;
	theora_state *tstate=&s->tstate;
	mblk_t *h,*t;
	if (theora_encode_header(tstate,&p)!=0){
		ms_error("theora_encode_header() error.");
		return -1;
	}
	h=allocb(p.bytes,0);
	memcpy(h->b_wptr,p.packet,p.bytes);
	h->b_wptr+=p.bytes;
	if (theora_encode_tables(tstate,&p)!=0){
		ms_error("theora_encode_tables error.");
		freemsg(h);
		return -1;
	}
	t=allocb(p.bytes,0);
	memcpy(t->b_wptr,p.packet,p.bytes);
	t->b_wptr+=p.bytes;
	h->b_cont=t;
	msgpullup(h,-1);
	s->packed_conf=h;
	return 0;
}

static void enc_preprocess(MSFilter *f){
	EncState *s=(EncState*)f->data;
	int err;
	if ((err=theora_encode_init(&s->tstate,&s->tinfo))!=0){
		ms_error("error in theora_encode_init() : %i !",err);
	}
	s->yuv.y_width=s->tinfo.width;
	s->yuv.y_height=s->tinfo.height;
	s->yuv.y_stride=s->tinfo.width;
	s->yuv.uv_width=s->tinfo.width/2;
	s->yuv.uv_height=s->tinfo.height/2;
	s->yuv.uv_stride=s->tinfo.width/2;
	create_packed_conf(s);
	s->conf_time=0;
	s->nframes=0;
}

static void enc_postprocess(MSFilter *f){
	EncState *s=(EncState*)f->data;
	theora_clear(&s->tstate);
	
	//If preprocess is called after postprocess,
	//then we loose all info...
	//theora_info_clear(&s->tinfo);
	
	if (s->packed_conf) {
		freemsg(s->packed_conf);
		s->packed_conf=NULL;
	}
}

static void enc_fill_yuv(yuv_buffer *yuv, mblk_t *im){
	yuv->y=(uint8_t*)im->b_rptr;
	yuv->u=(uint8_t*)im->b_rptr+(yuv->y_stride*yuv->y_height);
	yuv->v=(uint8_t*)yuv->u+(yuv->uv_stride*yuv->uv_height);
}


static void packetize_and_send(MSFilter *f, EncState *s, mblk_t *om, uint32_t timestamp, uint8_t tdt){
	mblk_t *packet;
	mblk_t *h;
	int npackets=0;
	static const int ident=0xdede;
	while(om!=NULL){
		if (om->b_wptr-om->b_rptr>=s->mtu){
			packet=dupb(om);
			packet->b_wptr=packet->b_rptr+s->mtu;
			om->b_rptr=packet->b_wptr;
		}else {
			packet=om;
			om=NULL;
		}
		++npackets;
		h=allocb(6,0);
		if (npackets==1){
			if (om==NULL)
				payload_header_set(h->b_wptr,ident,NOT_FRAGMENTED,tdt,1);
			else
				payload_header_set(h->b_wptr,ident,START_FRAGMENT,tdt,1);
		}else{
			if (om==NULL)
				payload_header_set(h->b_wptr,ident,END_FRAGMENT,tdt,1);
			else
				payload_header_set(h->b_wptr,ident,CONT_FRAGMENT,tdt,1);
		}
		h->b_wptr+=4;
		*((uint16_t*)h->b_wptr)=htons(msgdsize(packet));
		h->b_wptr+=2;
		h->b_cont=packet;
		mblk_set_timestamp_info(h,timestamp);
		ms_debug("sending theora frame of size %i",msgdsize(h));
		ms_queue_put(f->outputs[0],h);
	}
}

bool_t need_send_conf(EncState *s, uint64_t elapsed){
	/*send immediately then 10 seconds later */
	if ( (elapsed==0 && s->conf_time==0)
		|| (elapsed>=3000 && s->conf_time==1)
		|| (elapsed>=10000 && s->conf_time==2)){
		s->conf_time++;
		return TRUE;
	}
	return FALSE;
}

static void enc_process(MSFilter *f){
	mblk_t *im,*om;
	ogg_packet op;
	EncState *s=(EncState*)f->data;
	uint64_t timems=f->ticker->time;
	uint32_t timestamp=timems*90;
	uint64_t elapsed;

	
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		/*for the firsts frames only send theora packed conf*/
		om=NULL;
		if (s->nframes==0){
			s->start_time=timems;
		}
		elapsed=timems-s->start_time;

		if (need_send_conf(s,elapsed)){
			if (s->packed_conf) {
				om=dupmsg(s->packed_conf);
				ms_message("sending theora packed conf (%i bytes)",msgdsize(om));
				packetize_and_send(f,s,om,timestamp,THEORA_PACKED_CONF);
			}else {
				ms_error("No packed conf to send.");
			}
		}else{
			enc_fill_yuv(&s->yuv,im);
			ms_debug("subtmitting yuv frame to theora encoder...");
			if (theora_encode_YUVin(&s->tstate,&s->yuv)!=0){
				ms_error("theora_encode_YUVin error.");
			}else{
				if (theora_encode_packetout(&s->tstate,0,&op)==1){
					ms_debug("Got theora coded frame");
					om=allocb(op.bytes,0);
					memcpy(om->b_wptr,op.packet,op.bytes);
					om->b_wptr+=op.bytes;
					packetize_and_send(f,s,om,timestamp,THEORA_RAW_DATA);
				}
			}
		}
		freemsg(im);
		s->nframes++;
	}
}

static MSFilterMethod enc_methods[]={
	{	MS_FILTER_SET_VIDEO_SIZE, enc_set_vsize },
	{	MS_FILTER_SET_FPS,	enc_set_fps	},
	{	MS_FILTER_GET_VIDEO_SIZE, enc_get_vsize },
	{	MS_FILTER_GET_FPS,	enc_get_fps	},
	{	MS_FILTER_ADD_ATTR, enc_add_attr	},
	{	MS_FILTER_SET_BITRATE, enc_set_br	},
	{	MS_FILTER_SET_MTU, enc_set_mtu	},
	{	0			, NULL }
};

#ifdef _MSC_VER

MSFilterDesc ms_theora_enc_desc={
	MS_THEORA_ENC_ID,
	"MSTheoraEnc",
	N_("The theora video encoder from xiph.org"),
	MS_FILTER_ENCODER,
	"theora",
	1,
	1,
	enc_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	enc_methods
};

#else

MSFilterDesc ms_theora_enc_desc={
	.id=MS_THEORA_ENC_ID,
	.name="MSTheoraEnc",
	.text=N_("The open-source and royalty-free 'theora' video codec from xiph.org"),
	.category=MS_FILTER_ENCODER,
	.enc_fmt="theora",
	.ninputs=1,
	.noutputs=1,
	.init=enc_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.postprocess=enc_postprocess,
	.uninit=enc_uninit,
	.methods=enc_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_theora_enc_desc)

typedef struct DecState{
	theora_state tstate;
	theora_info tinfo;
	mblk_t *yuv;
	mblk_t *curframe;
	bool_t ready;
}DecState;

static void dec_init(MSFilter *f){
	DecState *s=(DecState *)ms_new(DecState,1);
	s->ready=FALSE;
	theora_info_init(&s->tinfo);
	s->yuv=NULL;
	s->curframe=NULL;
	f->data=s;
}

static void dec_uninit(MSFilter *f){
	DecState *s=(DecState*)f->data;
	if (s->yuv!=NULL) freemsg(s->yuv);
	if (s->curframe!=NULL) freemsg(s->curframe);
	theora_info_clear(&s->tinfo);
	ms_free(s);
}

static bool_t dec_init_theora(DecState *s, ogg_packet *op){
	theora_comment tcom;
	static const int ident_packet_size=42; 
	theora_comment_init(&tcom);
	tcom.vendor="dummy";
	op->b_o_s=1;
	if (theora_decode_header(&s->tinfo,&tcom,op)==0){
		op->packet+=ident_packet_size;
		op->bytes-=ident_packet_size;
		/*recall once to decode tables*/
		if (theora_decode_header(&s->tinfo,&tcom,op)==0){
			if (theora_decode_init(&s->tstate,&s->tinfo)==0){
				ms_debug("theora decoder ready, pixfmt=%i",
					s->tinfo.pixelformat);
				return TRUE;	
			}
		}else{
			ms_warning("error decoding theora tables");
		}
	}else{
		ms_warning("error decoding theora header");
	}
	return FALSE;
}
/* remove payload header and agregates fragmented packets */
static mblk_t *dec_unpacketize(MSFilter *f, DecState *s, mblk_t *im, int *tdt){
	uint8_t ft;
	*tdt=payload_header_get_tdt((uint8_t*)im->b_rptr);
	ft=payload_header_get_ft((uint8_t*)im->b_rptr);
	im->b_rptr+=6;
	
	if (ft==NOT_FRAGMENTED)	return im;
	if (ft==START_FRAGMENT){
		if (s->curframe!=NULL)
			freemsg(s->curframe);
		s->curframe=im;
	}else if (ft==CONT_FRAGMENT){
		if (s->curframe!=NULL)
			concatb(s->curframe,im);
		else
			freemsg(im);
	}else{/*end fragment*/
		if (s->curframe!=NULL){
			mblk_t *ret;
			concatb(s->curframe,im);
			msgpullup(s->curframe,-1);
			ret=s->curframe;
			s->curframe=NULL;
			return ret;
		}else
			freemsg(im);
	}
	return NULL;
}

static void dec_process_frame(MSFilter *f, DecState *s, ogg_packet *op){
	yuv_buffer yuv;
	if (theora_decode_packetin(&s->tstate,op)==0){
		if (theora_decode_YUVout(&s->tstate,&yuv)==0){
			mblk_t *om;
			int i;
			int ylen=yuv.y_width*yuv.y_height;
			int uvlen=yuv.uv_width*yuv.uv_height;
			ms_debug("Got yuv buffer from theora decoder");
			if (s->yuv==NULL){
				int len=(ylen)+(2*uvlen);
				s->yuv=allocb(len,0);
			}
			om=dupb(s->yuv);
			for(i=0;i<yuv.y_height;++i){
				memcpy(om->b_wptr,yuv.y+yuv.y_stride*i,yuv.y_width);
				om->b_wptr+=yuv.y_width;
			}
			for(i=0;i<yuv.uv_height;++i){
				memcpy(om->b_wptr,yuv.u+yuv.uv_stride*i,yuv.uv_width);
				om->b_wptr+=yuv.uv_width;
			}
			for(i=0;i<yuv.uv_height;++i){
				memcpy(om->b_wptr,yuv.v+yuv.uv_stride*i,yuv.uv_width);
				om->b_wptr+=yuv.uv_width;
			}
			ms_queue_put(f->outputs[0],om);
		}
	}else{
		ms_warning("theora decoding error");
	}
}

static void dec_process(MSFilter *f){
	mblk_t *im;
	mblk_t *m;
	ogg_packet op;
	int tdt;
	DecState *s=(DecState*)f->data;
	while( (im=ms_queue_get(f->inputs[0]))!=0) {
		m=dec_unpacketize(f,s,im,&tdt);
		if (m!=NULL){
			/* now in im we have only the theora data*/
			op.packet=(uint8_t*)m->b_rptr;
			op.bytes=m->b_wptr-m->b_rptr;
			op.b_o_s=0;
			op.e_o_s=0;
			op.granulepos=0;
			op.packetno=0;
			if (tdt!=THEORA_RAW_DATA) /*packed conf*/ {
				if (!s->ready){
					if (dec_init_theora(s,&op))
						s->ready=TRUE;
				}
			}else{
				if (s->ready){
					dec_process_frame(f,s,&op);
				}else{
					ms_warning("skipping theora packet because decoder was not initialized yet with theora header and tables");
				}
			}
			freemsg(m);
		}
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_theora_dec_desc={
	MS_THEORA_DEC_ID,
	"MSTheoraDec",
	N_("The theora video decoder from xiph.org"),
	MS_FILTER_DECODER,
	"theora",
	1,
	1,
	dec_init,
	NULL,
	dec_process,
	NULL,
	dec_uninit,
	NULL
};

#else

MSFilterDesc ms_theora_dec_desc={
	.id=MS_THEORA_DEC_ID,
	.name="MSTheoraDec",
	.text=N_("The theora video decoder from xiph.org"),
	.category=MS_FILTER_DECODER,
	.enc_fmt="theora",
	.ninputs=1,
	.noutputs=1,
	.init=dec_init,
	.process=dec_process,
	.uninit=dec_uninit
};

#endif
MS_FILTER_DESC_EXPORT(ms_theora_dec_desc)
