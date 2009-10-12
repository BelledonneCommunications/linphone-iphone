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

#ifdef HAVE_LIBAVCODEC_AVCODEC_H
#include <libavcodec/avcodec.h>
#else
#include <ffmpeg/avcodec.h>
#endif

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msticker.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netinet/in.h>			/* ntohl(3) */
#endif

#include "rfc2429.h"

static bool_t avcodec_initialized=FALSE;

#ifdef ENABLE_LOG_FFMPEG

void ms_ffmpeg_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
	static char message[8192];

    vsnprintf(message, sizeof message, fmt, vl);
	ms_message(message);
}

#endif

void ms_ffmpeg_check_init(){
	if(!avcodec_initialized){
		avcodec_init();
		avcodec_register_all();
		avcodec_initialized=TRUE;
#ifdef ENABLE_LOG_FFMPEG
		av_log_set_level(AV_LOG_WARNING);
		av_log_set_callback(&ms_ffmpeg_log_callback);
#endif
	}
}

typedef struct EncState{
	AVCodecContext av_context;
	AVCodec *av_codec;
	enum CodecID codec;
	mblk_t *comp_buf;
	MSVideoSize vsize;
	int mtu;	/* network maximum transmission unit in bytes */
	int profile;
	float fps;
	int maxbr;
	int qmin;
	uint32_t framenum;
	bool_t req_vfu;
}EncState;

static int enc_set_fps(MSFilter *f, void *arg){
	EncState *s=(EncState*)f->data;
	s->fps=*(float*)arg;
	return 0;
}

static int enc_get_fps(MSFilter *f, void *arg){
	EncState *s=(EncState*)f->data;
	*(float*)arg=s->fps;
	return 0;
}

static int enc_set_vsize(MSFilter *f,void *arg){
	EncState *s=(EncState*)f->data;
	s->vsize=*(MSVideoSize*)arg;
	return 0;
}

static int enc_get_vsize(MSFilter *f,void *arg){
	EncState *s=(EncState*)f->data;
	*(MSVideoSize*)arg=s->vsize;
	return 0;
}

static int enc_set_mtu(MSFilter *f,void *arg){
	EncState *s=(EncState*)f->data;
	s->mtu=*(int*)arg;
	return 0;
}

static bool_t parse_video_fmtp(const char *fmtp, float *fps, MSVideoSize *vsize){
	char *tmp=ms_strdup(fmtp);
	char *semicolon;
	char *equal;
	bool_t ret=TRUE;

	ms_message("parsing %s",fmtp);
	/*extract fisrt pair */
	if ((semicolon=strchr(tmp,';'))!=NULL){
		*semicolon='\0';
	}
	if ((equal=strchr(tmp,'='))!=NULL){
		int divider;
		*equal='\0';
		if (strcasecmp(tmp,"CIF")==0){
			if (vsize->width>=MS_VIDEO_SIZE_CIF_W){
				vsize->width=MS_VIDEO_SIZE_CIF_W;
				vsize->height=MS_VIDEO_SIZE_CIF_H;
			}
		}else if (strcasecmp(tmp,"QCIF")==0){
			vsize->width=MS_VIDEO_SIZE_QCIF_W;
			vsize->height=MS_VIDEO_SIZE_QCIF_H;
		}else{
			ms_warning("unsupported video size %s",tmp);
			ret=FALSE;
		}
		divider=atoi(equal+1);
		if (divider!=0){
			float newfps=29.97/divider;
			if (*fps>newfps) *fps=newfps;
		}else{
			ms_warning("Could not find video fps");
			ret=FALSE;
		}
	}else ret=FALSE;
	ms_free(tmp);
	return ret;
}

static int enc_add_fmtp(MSFilter *f,void *arg){
	EncState *s=(EncState*)f->data;
	const char *fmtp=(const char*)arg;
	char val[10];
	if (fmtp_get_value(fmtp,"profile",val,sizeof(val))){
		s->profile=atoi(val);
	}else parse_video_fmtp(fmtp,&s->fps,&s->vsize);
	return 0;
}

static int enc_req_vfu(MSFilter *f, void *unused){
	EncState *s=(EncState*)f->data;
	s->req_vfu=TRUE;
	return 0;
}

static void enc_init(MSFilter *f, enum CodecID codec)
{
	EncState *s=(EncState *)ms_new(EncState,1);
	f->data=s;
	ms_ffmpeg_check_init();
	s->profile=0;/*always default to profile 0*/
	s->comp_buf=NULL;
	s->fps=15;
	s->mtu=ms_get_payload_max_size()-2;/*-2 for the H263 payload header*/
	s->maxbr=500000;
	s->codec=codec;
	s->vsize.width=MS_VIDEO_SIZE_CIF_W;
	s->vsize.height=MS_VIDEO_SIZE_CIF_H;
	s->qmin=2;
	s->req_vfu=FALSE;
	s->framenum=0;
	s->av_context.codec=NULL;
}

static void enc_h263_init(MSFilter *f){
	enc_init(f,CODEC_ID_H263P);
}

static void enc_mpeg4_init(MSFilter *f){
	enc_init(f,CODEC_ID_MPEG4);
}

static void enc_snow_init(MSFilter *f){
	enc_init(f,CODEC_ID_SNOW);
}

static void enc_mjpeg_init(MSFilter *f){
	enc_init(f,CODEC_ID_MJPEG);
}

static void prepare(EncState *s){
	AVCodecContext *c=&s->av_context;
	avcodec_get_context_defaults(c);
	if (s->codec==CODEC_ID_MJPEG)
	{
		ms_message("Codec bitrate set to %i",c->bit_rate);
		c->width = s->vsize.width;  
		c->height = s->vsize.height;
		c->time_base.num = 1;
		c->time_base.den = (int)s->fps;
		c->gop_size=(int)s->fps*5; /*emit I frame every 5 seconds*/
		c->pix_fmt=PIX_FMT_YUVJ420P;
		s->comp_buf=allocb(c->bit_rate*2,0);
		return;
	}

	/* put codec parameters */
	c->bit_rate=(float)s->maxbr*0.7;
	c->bit_rate_tolerance=s->fps!=1?(float)c->bit_rate/(s->fps-1):c->bit_rate;

	if (s->codec!=CODEC_ID_SNOW && s->maxbr<256000){
		/*snow does not like 1st pass rate control*/
		/*and rate control eats too much cpu with CIF high fps pictures*/
		c->rc_max_rate=(float)s->maxbr*0.8;
		c->rc_min_rate=0;
		c->rc_buffer_size=c->rc_max_rate;
	}else{
		/*use qmin instead*/
		c->qmin=s->qmin;
	}

	ms_message("Codec bitrate set to %i",c->bit_rate);
	c->width = s->vsize.width;  
	c->height = s->vsize.height;
	c->time_base.num = 1;
	c->time_base.den = (int)s->fps;
	c->gop_size=(int)s->fps*5; /*emit I frame every 5 seconds*/
	c->pix_fmt=PIX_FMT_YUV420P;
	s->comp_buf=allocb(c->bit_rate*2,0);
	if (s->codec==CODEC_ID_SNOW){
		c->strict_std_compliance=-2;
	}
	
}

static void prepare_h263(EncState *s){
	AVCodecContext *c=&s->av_context;
	/* we don't use the rtp_callback but use rtp_mode that forces ffmpeg to insert
	Start Codes as much as possible in the bitstream */
#if LIBAVCODEC_VERSION_INT < ((52<<16)+(0<<8)+0)
        c->rtp_mode = 1;
#endif
	c->rtp_payload_size = s->mtu/2;
	if (s->profile==0){
		s->codec=CODEC_ID_H263;
	}else{
		c->flags|=CODEC_FLAG_H263P_UMV;
		c->flags|=CODEC_FLAG_AC_PRED;
		c->flags|=CODEC_FLAG_H263P_SLICE_STRUCT;
		/*
		c->flags|=CODEC_FLAG_OBMC;
		c->flags|=CODEC_FLAG_AC_PRED;
		*/
		s->codec=CODEC_ID_H263P;
	}
}

static void prepare_mpeg4(EncState *s){
	AVCodecContext *c=&s->av_context;
	c->max_b_frames=0; /*don't use b frames*/
	c->flags|=CODEC_FLAG_AC_PRED;
	c->flags|=CODEC_FLAG_H263P_UMV;
	/*c->flags|=CODEC_FLAG_QPEL;*/ /*don't enable this one: this forces profile_level to advanced simple profile */
	c->flags|=CODEC_FLAG_4MV;
	c->flags|=CODEC_FLAG_GMC;
	c->flags|=CODEC_FLAG_LOOP_FILTER;
	c->flags|=CODEC_FLAG_H263P_SLICE_STRUCT;
}

static void enc_uninit(MSFilter  *f){
	EncState *s=(EncState*)f->data;
	ms_free(s);
}
#if 0
static void enc_set_rc(EncState *s, AVCodecContext *c){
	int factor=c->width/MS_VIDEO_SIZE_QCIF_W;
	c->rc_min_rate=0;
	c->bit_rate=400; /* this value makes around 100kbit/s at QCIF=2 */
	c->rc_max_rate=c->bit_rate+1;
	c->rc_buffer_size=20000*factor;	/* food recipe */
}
#endif

static void enc_preprocess(MSFilter *f){
	EncState *s=(EncState*)f->data;
	int error;
	prepare(s);
	if (s->codec==CODEC_ID_H263P || s->codec==CODEC_ID_H263)
		prepare_h263(s);
	else if (s->codec==CODEC_ID_MPEG4)
		prepare_mpeg4(s);
	else if (s->codec==CODEC_ID_SNOW){
		/**/
	}else if (s->codec==CODEC_ID_MJPEG){
		/**/
	}else {
		ms_error("Unsupported codec id %i",s->codec);
		return;
	}
	s->av_codec=avcodec_find_encoder(s->codec);
	if (s->av_codec==NULL){
		ms_error("could not find encoder for codec id %i",s->codec);
		return;
	}
	error=avcodec_open(&s->av_context, s->av_codec);
	if (error!=0) {
		ms_error("avcodec_open() failed: %i",error);
		return;
	}
	ms_debug("image format is %i.",s->av_context.pix_fmt);
	ms_message("qmin=%i qmax=%i",s->av_context.qmin,s->av_context.qmax);
}

static void enc_postprocess(MSFilter *f){
	EncState *s=(EncState*)f->data;
	if (s->av_context.codec!=NULL){
		avcodec_close(&s->av_context);
		s->av_context.codec=NULL;
	}
	if (s->comp_buf!=NULL)	{
		freemsg(s->comp_buf);
		s->comp_buf=NULL;
	}
}

static void add_rfc2190_header(mblk_t **packet, AVCodecContext *context){
	mblk_t *header;
	header = allocb(4, 0);
	memset(header->b_wptr, 0, 4);
	// assume video size is CIF or QCIF
	if (context->width == 352 && context->height == 288) header->b_wptr[1] = 0x60;
	else header->b_wptr[1] = 0x40;
	if (context->coded_frame->pict_type != FF_I_TYPE) header->b_wptr[1] |= 0x10;
	header->b_wptr += 4;
	header->b_cont = *packet;
	*packet = header;
}

static int get_gbsc(uint8_t *psc, uint8_t *end)
{
	int len = end-psc;
	uint32_t buf;
	int i, j, k;
	k = len;
	for (i = 2; i < len-4; i++) {
		buf = *((uint32_t *)(psc+i));
		for (j = 0; j < 8; j++) {
			if (((buf >> j) & 0x00FCFFFF) == 0x00800000) {/*PSC*/
				i += 2;
				k=i;
				break;
			} else if (((buf >> j) & 0x0080FFFF) == 0x00800000) {/*GBSC*/
				i += 2;
				k = i;
				break;
			}
		}
	}
	return k;
}

static void rfc2190_generate_packets(MSFilter *f, EncState *s, mblk_t *frame, uint32_t timestamp){
	mblk_t *packet=NULL;
	
	while (frame->b_rptr<frame->b_wptr){
		packet=dupb(frame);
		frame->b_rptr=packet->b_wptr=packet->b_rptr+get_gbsc(packet->b_rptr, MIN(packet->b_rptr+s->mtu,frame->b_wptr));
		add_rfc2190_header(&packet, &s->av_context);
		mblk_set_timestamp_info(packet,timestamp);
		ms_queue_put(f->outputs[0],packet);
	}
	/* the marker bit is set on the last packet, if any.*/
	mblk_set_marker_info(packet,TRUE);
}

static void mpeg4_fragment_and_send(MSFilter *f,EncState *s,mblk_t *frame, uint32_t timestamp){
	uint8_t *rptr;
	mblk_t *packet=NULL;
	int len;
	for (rptr=frame->b_rptr;rptr<frame->b_wptr;){
		len=MIN(s->mtu,(frame->b_wptr-rptr));
		packet=dupb(frame);
		packet->b_rptr=rptr;
		packet->b_wptr=rptr+len;
		mblk_set_timestamp_info(packet,timestamp);
		ms_queue_put(f->outputs[0],packet);
		rptr+=len;
	}
	/*set marker bit on last packet*/
	mblk_set_marker_info(packet,TRUE);
}

static void rfc4629_generate_follow_on_packets(MSFilter *f, EncState *s, mblk_t *frame, uint32_t timestamp, uint8_t *psc, uint8_t *end, bool_t last_packet){
	mblk_t *packet;
	int len=end-psc;
	
	packet=dupb(frame);	
	packet->b_rptr=psc;
	packet->b_wptr=end;
	/*ms_message("generating packet of size %i",end-psc);*/
	rfc2429_set_P(psc,1);
	mblk_set_timestamp_info(packet,timestamp);

	
	if (len>s->mtu){
		/*need to slit the packet using "follow-on" packets */
		/*compute the number of packets need (rounded up)*/
		int num=(len+s->mtu-1)/s->mtu;
		int i;
		uint8_t *pos;
		/*adjust the first packet generated*/
		pos=packet->b_wptr=packet->b_rptr+s->mtu;
		ms_queue_put(f->outputs[0],packet);
		ms_debug("generating %i follow-on packets",num);
		for (i=1;i<num;++i){
			mblk_t *header;
			packet=dupb(frame);
			packet->b_rptr=pos;
			pos=packet->b_wptr=MIN(pos+s->mtu,end);
			header=allocb(2,0);
			header->b_wptr[0]=0;
			header->b_wptr[1]=0;
			header->b_wptr+=2;
			/*no P bit is set */
			header->b_cont=packet;
			packet=header;
			mblk_set_timestamp_info(packet,timestamp);
			ms_queue_put(f->outputs[0],packet);
		}
	}else ms_queue_put(f->outputs[0],packet);
	/* the marker bit is set on the last packet, if any.*/
	mblk_set_marker_info(packet,last_packet);
}

/* returns the last psc position just below packet_size */
static uint8_t *get_psc(uint8_t *begin,uint8_t *end, int packet_size){
	int i;
	uint8_t *ret=NULL;
	uint8_t *p;
	if (begin==end) return NULL;
	for(i=1,p=begin+1;p<end && i<packet_size;++i,++p){
		if (p[-1]==0 && p[0]==0){
			ret=p-1;
		}
		p++;/* to skip possible 0 after the PSC that would make a double detection */
	}
	return ret;
}


struct jpeghdr {
	//unsigned int tspec:8;   /* type-specific field */
	unsigned int off:32;    /* fragment byte offset */
	uint8_t type;            /* id of jpeg decoder params */
	uint8_t q;               /* quantization factor (or table id) */
	uint8_t width;           /* frame width in 8 pixel blocks */
	uint8_t height;          /* frame height in 8 pixel blocks */
};

struct jpeghdr_rst {
	uint16_t dri;
	unsigned int f:1;
	unsigned int l:1;
	unsigned int count:14;
};

struct jpeghdr_qtable {
	uint8_t  mbz;
	uint8_t  precision;
	uint16_t length;
};

#define RTP_JPEG_RESTART           0x40

/* Procedure SendFrame:
 *
 *  Arguments:
 *    start_seq: The sequence number for the first packet of the current
 *               frame.
 *    ts: RTP timestamp for the current frame
 *    ssrc: RTP SSRC value
 *    jpeg_data: Huffman encoded JPEG scan data
 *    len: Length of the JPEG scan data
 *    type: The value the RTP/JPEG type field should be set to
 *    typespec: The value the RTP/JPEG type-specific field should be set
 *              to
 *    width: The width in pixels of the JPEG image
 *    height: The height in pixels of the JPEG image
 *    dri: The number of MCUs between restart markers (or 0 if there
 *         are no restart markers in the data
 *    q: The Q factor of the data, to be specified using the Independent
 *       JPEG group's algorithm if 1 <= q <= 99, specified explicitly
 *       with lqt and cqt if q >= 128, or undefined otherwise.
 *    lqt: The quantization table for the luminance channel if q >= 128
 *    cqt: The quantization table for the chrominance channels if
 *         q >= 128
 *
 *  Return value:
 *    the sequence number to be sent for the first packet of the next
 *    frame.
 *
 * The following are assumed to be defined:
 *
 * PACKET_SIZE                         - The size of the outgoing packet
 * send_packet(u_int8 *data, int len)  - Sends the packet to the network
 */

static void mjpeg_fragment_and_send(MSFilter *f,EncState *s,mblk_t *frame, uint32_t timestamp,
							 uint8_t type,	uint8_t typespec, int dri,
							 uint8_t q, mblk_t *lqt, mblk_t *cqt) {
	struct jpeghdr jpghdr;
	struct jpeghdr_rst rsthdr;
	struct jpeghdr_qtable qtblhdr;
	int bytes_left = msgdsize(frame);
	int data_len;
	
	mblk_t *packet;
	
	/* Initialize JPEG header
	 */
	//jpghdr.tspec = typespec;
	jpghdr.off = 0;
	jpghdr.type = type | ((dri != 0) ? RTP_JPEG_RESTART : 0);
	jpghdr.q = q;
	jpghdr.width = s->vsize.width / 8;
	jpghdr.height = s->vsize.height / 8;

	/* Initialize DRI header
	 */
	if (dri != 0) {
		rsthdr.dri = htons(dri);
		rsthdr.f = 1;        /* This code does not align RIs */
		rsthdr.l = 1;
		rsthdr.count = 0x3fff;
	}

	/* Initialize quantization table header
	 */
	if (q >= 128) {
		qtblhdr.mbz = 0;
		qtblhdr.precision = 0; /* This code uses 8 bit tables only */
		qtblhdr.length = htons(msgdsize(lqt)+msgdsize(cqt));  /* 2 64-byte tables */
	}

	while (bytes_left > 0) {
		packet = allocb(s->mtu, 0);

		jpghdr.off = htonl(jpghdr.off);
		memcpy(packet->b_wptr, &jpghdr, sizeof(jpghdr));
		jpghdr.off = ntohl(jpghdr.off);
		packet->b_wptr += sizeof(jpghdr);

		if (dri != 0) {
			memcpy(packet->b_wptr, &rsthdr, sizeof(rsthdr));
			packet->b_wptr += sizeof(rsthdr);
		}

		if (q >= 128 && jpghdr.off == 0) {
			memcpy(packet->b_wptr, &qtblhdr, sizeof(qtblhdr));
			packet->b_wptr += sizeof(qtblhdr);
			if (msgdsize(lqt)){
				memcpy(packet->b_wptr, lqt->b_rptr, msgdsize(lqt));
				packet->b_wptr += msgdsize(lqt);
			}
			if (msgdsize(cqt)){
				memcpy(packet->b_wptr, cqt->b_rptr, msgdsize(cqt));
				packet->b_wptr += msgdsize(cqt);
			}
		}

		data_len = s->mtu - (packet->b_wptr - packet->b_rptr);
		if (data_len >= bytes_left) {
			data_len = bytes_left;
			mblk_set_marker_info(packet,TRUE);
		}

		memcpy(packet->b_wptr, frame->b_rptr + jpghdr.off, data_len);	
		packet->b_wptr=packet->b_wptr + data_len;
				
		mblk_set_timestamp_info(packet,timestamp);
		ms_queue_put(f->outputs[0],packet);

		jpghdr.off += data_len;
		bytes_left -= data_len;
	}
}

static int find_marker(uint8_t **pbuf_ptr, uint8_t *buf_end){

	uint8_t *buf_ptr;
	unsigned int v, v2;
	int val;

	buf_ptr = *pbuf_ptr;
	while (buf_ptr < buf_end) {
		v = *buf_ptr++;
		v2 = *buf_ptr;
		if ((v == 0xff) && (v2 >= 0xc0) && (v2 <= 0xfe) && buf_ptr < buf_end) {
			val = *buf_ptr++;
			*pbuf_ptr = buf_ptr;
			return val;
		}
	}
	val = -1;
	return val;
}

static mblk_t *skip_jpeg_headers(mblk_t *full_frame, mblk_t **lqt, mblk_t **cqt){
	int err;
	uint8_t *pbuf_ptr=full_frame->b_rptr;
	uint8_t *buf_end=full_frame->b_wptr;	

	ms_message("image size: %i)", buf_end-pbuf_ptr);

	*lqt=NULL;
	*cqt=NULL;

	err = find_marker(&pbuf_ptr, buf_end);
	while (err!=-1)
	{
		ms_message("marker found: %x (offset from beginning%i)", err, pbuf_ptr-full_frame->b_rptr);
		if (err==0xdb)
		{
			/* copy DQT table */
			int len = ntohs(*(uint16_t*)(pbuf_ptr));
			if (*lqt==NULL)
			{
				mblk_t *_lqt = allocb(len-3, 0);
				memcpy(_lqt->b_rptr, pbuf_ptr+3, len-3);
				_lqt->b_wptr += len-3;
				*lqt = _lqt;
				//*cqt = dupb(*lqt);
			}
			else
			{
				mblk_t *_cqt = allocb(len-3, 0);
				memcpy(_cqt->b_rptr, pbuf_ptr+3, len-3);
				_cqt->b_wptr += len-3;
				*cqt = _cqt;
			}
		}
		if (err==0xda)
		{
			uint16_t *bistream=(uint16_t *)pbuf_ptr;
			uint16_t len = ntohs(*bistream);
			full_frame->b_rptr = pbuf_ptr+len;
		}
		err = find_marker(&pbuf_ptr, buf_end);
	}
	return full_frame;
}

static void split_and_send(MSFilter *f, EncState *s, mblk_t *frame){
	uint8_t *lastpsc;
	uint8_t *psc;
	uint32_t timestamp=f->ticker->time*90LL;
	
	if (s->codec==CODEC_ID_MPEG4 || s->codec==CODEC_ID_SNOW)
	{
		mpeg4_fragment_and_send(f,s,frame,timestamp);
		return;
	}
	else if (s->codec==CODEC_ID_MJPEG)
	{
		mblk_t *lqt=NULL;
		mblk_t *cqt=NULL;
		skip_jpeg_headers(frame, &lqt, &cqt);
		mjpeg_fragment_and_send(f,s,frame,timestamp,
								1, /* 420? */
								0,
								0, /* dri ?*/
								255, /* q */
								lqt,
								cqt);
		return;
	}

	ms_debug("processing frame of size %i",frame->b_wptr-frame->b_rptr);
	if (f->desc->id==MS_H263_ENC_ID){
		lastpsc=frame->b_rptr;
		while(1){
			psc=get_psc(lastpsc+2,frame->b_wptr,s->mtu);
			if (psc!=NULL){
				rfc4629_generate_follow_on_packets(f,s,frame,timestamp,lastpsc,psc,FALSE);
				lastpsc=psc;
			}else break;
		}
		/* send the end of frame */
		rfc4629_generate_follow_on_packets(f,s,frame, timestamp,lastpsc,frame->b_wptr,TRUE);
	}else if (f->desc->id==MS_H263_OLD_ENC_ID){
		rfc2190_generate_packets(f,s,frame,timestamp);
	}else{
		ms_fatal("Ca va tres mal.");
	}
}

static void process_frame(MSFilter *f, mblk_t *inm){
	EncState *s=(EncState*)f->data;
	AVFrame pict;
	AVCodecContext *c=&s->av_context;
	int error;
	mblk_t *comp_buf=s->comp_buf;
	int comp_buf_sz=comp_buf->b_datap->db_lim-comp_buf->b_datap->db_base;

	/* convert image if necessary */
	avcodec_get_frame_defaults(&pict);
	avpicture_fill((AVPicture*)&pict,(uint8_t*)inm->b_rptr,c->pix_fmt,c->width,c->height);
	
	/* timestamp used by ffmpeg, unset here */
	pict.pts=AV_NOPTS_VALUE;
		
	if (s->framenum==(int)(s->fps*2.0) || s->framenum==(int)(s->fps*4.0)){
		/*sends an I frame at 2 seconds and 4 seconds after the beginning of the call*/
		s->req_vfu=TRUE;
	}
	if (s->req_vfu){
		pict.pict_type=FF_I_TYPE;
		s->req_vfu=FALSE;
	}
	comp_buf->b_rptr=comp_buf->b_wptr=comp_buf->b_datap->db_base;
	if (s->codec==CODEC_ID_SNOW){
		//prepend picture size
		uint32_t header=((s->vsize.width&0xffff)<<16) | (s->vsize.height&0xffff);
		*(uint32_t*)comp_buf->b_wptr=htonl(header);
		comp_buf->b_wptr+=4;
		comp_buf_sz-=4;
	}
	error=avcodec_encode_video(c, (uint8_t*)comp_buf->b_wptr,comp_buf_sz, &pict);
	if (error<=0) ms_warning("ms_AVencoder_process: error %i.",error);
	else{
		s->framenum++;
		if (c->coded_frame->pict_type==FF_I_TYPE){
			ms_message("Emitting I-frame");
		}
		comp_buf->b_wptr+=error;
		split_and_send(f,s,comp_buf);
	}
	freemsg(inm);
}

static void enc_process(MSFilter *f){
	mblk_t *inm;
	EncState *s=(EncState*)f->data;
	if (s->av_context.codec==NULL) {
		ms_queue_flush(f->inputs[0]);
		return;
	}
	ms_filter_lock(f);
	while((inm=ms_queue_get(f->inputs[0]))!=0){
		process_frame(f,inm);
	}
	ms_filter_unlock(f);
}


static int enc_get_br(MSFilter *f, void *arg){
	EncState *s=(EncState*)f->data;
	*(int*)arg=s->maxbr;
	return 0;
}

static int enc_set_br(MSFilter *f, void *arg){
	EncState *s=(EncState*)f->data;
	bool_t snow=s->codec==CODEC_ID_SNOW;
	s->maxbr=*(int*)arg;
	if (s->maxbr>=1024000 && s->codec!=CODEC_ID_H263P){
		s->vsize.width = MS_VIDEO_SIZE_SVGA_W;
		s->vsize.height = MS_VIDEO_SIZE_SVGA_H;
		s->fps=17;
	}else if (s->maxbr>=800000 && s->codec!=CODEC_ID_H263P){
		s->vsize.width = MS_VIDEO_SIZE_VGA_W;
		s->vsize.height = MS_VIDEO_SIZE_VGA_H;
		s->fps=17;
	}else if (s->maxbr>=512000){
		s->vsize.width=MS_VIDEO_SIZE_CIF_W;
		s->vsize.height=MS_VIDEO_SIZE_CIF_H;
		s->fps=17;
	}else if (s->maxbr>=256000){
		s->vsize.width=MS_VIDEO_SIZE_CIF_W;
		s->vsize.height=MS_VIDEO_SIZE_CIF_H;
		s->fps=10;
		s->qmin=3;
	}else if (s->maxbr>=128000){
		s->vsize.width=MS_VIDEO_SIZE_QCIF_W;
		s->vsize.height=MS_VIDEO_SIZE_QCIF_H;
		s->fps=10;
		s->qmin=3;
	}else if (s->maxbr>=64000){
		s->vsize.width=MS_VIDEO_SIZE_QCIF_W;
		s->vsize.height=MS_VIDEO_SIZE_QCIF_H;
		s->fps=snow ? 7 : 5;
		s->qmin=snow ? 4 : 5;
	}else{
		s->vsize.width=MS_VIDEO_SIZE_QCIF_W;
		s->vsize.height=MS_VIDEO_SIZE_QCIF_H;
		s->fps=5;
		s->qmin=5;
	}
	if (s->av_context.codec!=NULL){
		/*apply new settings dynamically*/
		ms_filter_lock(f);
		enc_postprocess(f);
		enc_preprocess(f);
		ms_filter_unlock(f);
	}
	return 0;
}


static MSFilterMethod methods[]={
	{	MS_FILTER_SET_FPS	,	enc_set_fps	},
	{	MS_FILTER_GET_FPS	,	enc_get_fps	},
	{	MS_FILTER_SET_VIDEO_SIZE ,	enc_set_vsize },
	{	MS_FILTER_GET_VIDEO_SIZE ,	enc_get_vsize },
	{	MS_FILTER_ADD_FMTP	,	enc_add_fmtp },
	{	MS_FILTER_SET_BITRATE	,	enc_set_br	},
	{	MS_FILTER_GET_BITRATE	,	enc_get_br	},
	{	MS_FILTER_SET_MTU	,	enc_set_mtu	},
	{	MS_FILTER_REQ_VFU	,	enc_req_vfu	},
	{	0			,	NULL	}
};

#ifdef _MSC_VER

MSFilterDesc ms_h263_enc_desc={
	MS_H263_ENC_ID,
	"MSH263Enc",
	N_("A video H.263 encoder using ffmpeg library."),
	MS_FILTER_ENCODER,
	"H263-1998",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_h263_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	methods
};

MSFilterDesc ms_h263_old_enc_desc={
	MS_H263_OLD_ENC_ID,
	"MSH263OldEnc",
	N_("A video H.263 encoder using ffmpeg library. It is compliant with old RFC2190 spec."),
	MS_FILTER_ENCODER,
	"H263",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_h263_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	methods
};

MSFilterDesc ms_mpeg4_enc_desc={
	MS_MPEG4_ENC_ID,
	"MSMpeg4Enc",
	N_("A video MPEG4 encoder using ffmpeg library."),
	MS_FILTER_ENCODER,
	"MP4V-ES",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_mpeg4_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	methods
};

MSFilterDesc ms_snow_enc_desc={
	MS_SNOW_ENC_ID,
	"MSSnowEnc",
	N_("A video snow encoder using ffmpeg library."),
	MS_FILTER_ENCODER,
	"x-snow",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_snow_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	methods
};

MSFilterDesc ms_mjpeg_enc_desc={
	MS_JPEG_ENC_ID,
	"MSJpegEnc",
	N_("A RTP/MJPEG encoder using ffmpeg library."),
	MS_FILTER_ENCODER,
	"JPEG",
	1, /*MS_YUV420P is assumed on this input */
	1,
	enc_mjpeg_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	methods
};

#else

MSFilterDesc ms_h263_enc_desc={
	.id=MS_H263_ENC_ID,
	.name="MSH263Enc",
	.text=N_("A video H.263 encoder using ffmpeg library."),
	.category=MS_FILTER_ENCODER,
	.enc_fmt="H263-1998",
	.ninputs=1, /*MS_YUV420P is assumed on this input */
	.noutputs=1,
	.init=enc_h263_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.postprocess=enc_postprocess,
	.uninit=enc_uninit,
	.methods=methods
};

MSFilterDesc ms_h263_old_enc_desc={
	.id=MS_H263_OLD_ENC_ID,
	.name="MSH263Enc",
	.text=N_("A video H.263 encoder using ffmpeg library, compliant with old RFC2190 spec."),
	.category=MS_FILTER_ENCODER,
	.enc_fmt="H263",
	.ninputs=1, /*MS_YUV420P is assumed on this input */
	.noutputs=1,
	.init=enc_h263_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.postprocess=enc_postprocess,
	.uninit=enc_uninit,
	.methods=methods
};

MSFilterDesc ms_mpeg4_enc_desc={
	.id=MS_MPEG4_ENC_ID,
	.name="MSMpeg4Enc",
	.text=N_("A video MPEG4 encoder using ffmpeg library."),
	.category=MS_FILTER_ENCODER,
	.enc_fmt="MP4V-ES",
	.ninputs=1, /*MS_YUV420P is assumed on this input */
	.noutputs=1,
	.init=enc_mpeg4_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.postprocess=enc_postprocess,
	.uninit=enc_uninit,
	.methods=methods
};

MSFilterDesc ms_snow_enc_desc={
	.id=MS_SNOW_ENC_ID,
	.name="MSSnowEnc",
	.text=N_("The snow codec is royalty-free and is open-source. \n"
		"It uses innovative techniques that makes it one of most promising video "
		"codec. It is implemented within the ffmpeg project.\n"
		"However it is under development, quite unstable and compatibility with other versions "
		"cannot be guaranteed."),
	.category=MS_FILTER_ENCODER,
	.enc_fmt="x-snow",
	.ninputs=1, /*MS_YUV420P is assumed on this input */
	.noutputs=1,
	.init=enc_snow_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.postprocess=enc_postprocess,
	.uninit=enc_uninit,
	.methods=methods
};

MSFilterDesc ms_mjpeg_enc_desc={
	.id=MS_JPEG_ENC_ID,
	.name="MSMJpegEnc",
	.text=N_("A MJPEG encoder using ffmpeg library."),
	.category=MS_FILTER_ENCODER,
	.enc_fmt="JPEG",
	.ninputs=1, /*MS_YUV420P is assumed on this input */
	.noutputs=1,
	.init=enc_mjpeg_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.postprocess=enc_postprocess,
	.uninit=enc_uninit,
	.methods=methods
};

#endif

void __register_ffmpeg_encoders_if_possible(void){
	ms_ffmpeg_check_init();
	if (avcodec_find_encoder(CODEC_ID_MPEG4))
		ms_filter_register(&ms_mpeg4_enc_desc);
	if (avcodec_find_encoder(CODEC_ID_H263)){
		ms_filter_register(&ms_h263_enc_desc);
		ms_filter_register(&ms_h263_old_enc_desc);
	}
	if (avcodec_find_encoder(CODEC_ID_SNOW))
		ms_filter_register(&ms_snow_enc_desc);
	if (avcodec_find_encoder(CODEC_ID_MJPEG))
	{
		ms_filter_register(&ms_mjpeg_enc_desc);
	}
}

