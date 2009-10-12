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


static u_char lum_dc_codelens[] = {
        0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
};

static u_char lum_dc_symbols[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static u_char lum_ac_codelens[] = {
        0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d,
};

static u_char lum_ac_symbols[] = {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
        0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
        0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
        0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
        0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
        0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
        0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa,
};

static u_char chm_dc_codelens[] = {
        0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
};

static u_char chm_dc_symbols[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

static u_char chm_ac_codelens[] = {
        0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77,
};

static u_char chm_ac_symbols[] = {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
        0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
        0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
        0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
        0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
        0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
        0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
        0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
        0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
        0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa,
};

static u_char *
MakeQuantHeader(u_char *p, u_char *qt, int tableNo, int table_len)
{
        *p++ = 0xff;
        *p++ = 0xdb;            /* DQT */
        *p++ = 0;               /* length msb */
        *p++ = table_len+3;              /* length lsb */
        *p++ = tableNo;
        memcpy(p, qt, table_len);
        return (p + table_len);
}

static u_char *
MakeHuffmanHeader(u_char *p, u_char *codelens, int ncodes,
                  u_char *symbols, int nsymbols, int tableNo,
                  int tableClass)
{
        *p++ = 0xff;
        *p++ = 0xc4;            /* DHT */
        *p++ = 0;               /* length msb */
        *p++ = 3 + ncodes + nsymbols; /* length lsb */
        *p++ = (tableClass << 4) | tableNo;
        memcpy(p, codelens, ncodes);
        p += ncodes;
        memcpy(p, symbols, nsymbols);
        p += nsymbols;
        return (p);
}

static u_char *
MakeDRIHeader(u_char *p, u_short dri) {
        *p++ = 0xff;
        *p++ = 0xdd;            /* DRI */
        *p++ = 0x0;             /* length msb */
        *p++ = 4;               /* length lsb */
        *p++ = dri >> 8;        /* dri msb */
        *p++ = dri & 0xff;      /* dri lsb */
        return (p);
}

/*
 *  Arguments:
 *    type, width, height: as supplied in RTP/JPEG header
 *    lqt, cqt: quantization tables as either derived from
 *         the Q field using MakeTables() or as specified
 *         in section 4.2.
 *    dri: restart interval in MCUs, or 0 if no restarts.
 *
 *    p: pointer to return area
 *
 *  Return value:
 *    The length of the generated headers.
 *
 *    Generate a frame and scan headers that can be prepended to the
 *    RTP/JPEG data payload to produce a JPEG compressed image in
 *    interchange format (except for possible trailing garbage and
 *    absence of an EOI marker to terminate the scan).
 */
static int MakeHeaders(u_char *p, int type, int w, int h, u_char *lqt,
                u_char *cqt, unsigned table_len, u_short dri)
{
        u_char *start = p;

        /* convert from blocks to pixels */
        w <<= 3;
        h <<= 3;

        *p++ = 0xff;
        *p++ = 0xd8;            /* SOI */

		if (table_len>64)
		{
			p = MakeQuantHeader(p, lqt, 0, table_len/2);
	        p = MakeQuantHeader(p, cqt, 1, table_len/2);
		}
		else
		{
			p = MakeQuantHeader(p, lqt, 0, table_len);
			//p = MakeQuantHeader(p, lqt, 1, table_len);
		}
        if (dri != 0)
                p = MakeDRIHeader(p, dri);

        *p++ = 0xff;
        *p++ = 0xc0;            /* SOF */
        *p++ = 0;               /* length msb */
        *p++ = 17;              /* length lsb */
        *p++ = 8;               /* 8-bit precision */
        *p++ = h >> 8;          /* height msb */
        *p++ = h;               /* height lsb */
        *p++ = w >> 8;          /* width msb */
        *p++ = w;               /* wudth lsb */
        *p++ = 3;               /* number of components */
        *p++ = 0;               /* comp 0 */
        if (type == 0)
                *p++ = 0x21;    /* hsamp = 2, vsamp = 1 */
        else
                *p++ = 0x22;    /* hsamp = 2, vsamp = 2 */
        *p++ = 0;               /* quant table 0 */
        *p++ = 1;               /* comp 1 */
        *p++ = 0x11;            /* hsamp = 1, vsamp = 1 */
        *p++ = table_len <= 64 ? 0x00 : 0x01; //1   /* quant table 1 */
        *p++ = 2;               /* comp 2 */
        *p++ = 0x11;            /* hsamp = 1, vsamp = 1 */
        *p++ = table_len <= 64 ? 0x00 : 0x01; //1;  /* quant table 1 */
        p = MakeHuffmanHeader(p, lum_dc_codelens,
                              sizeof(lum_dc_codelens),
                              lum_dc_symbols,
                              sizeof(lum_dc_symbols), 0, 0);
        p = MakeHuffmanHeader(p, lum_ac_codelens,
                              sizeof(lum_ac_codelens),
                              lum_ac_symbols,
                              sizeof(lum_ac_symbols), 0, 1);
        p = MakeHuffmanHeader(p, chm_dc_codelens,
                              sizeof(chm_dc_codelens),
                              chm_dc_symbols,
                              sizeof(chm_dc_symbols), 1, 0);
        p = MakeHuffmanHeader(p, chm_ac_codelens,
                              sizeof(chm_ac_codelens),
                              chm_ac_symbols,
                              sizeof(chm_ac_symbols), 1, 1);

        *p++ = 0xff;
        *p++ = 0xda;            /* SOS */
        *p++ = 0;               /* length msb */
        *p++ = 12;              /* length lsb */
        *p++ = 3;               /* 3 components */
        *p++ = 0;               /* comp 0 */
        *p++ = 0;               /* huffman table 0 */
        *p++ = 1;               /* comp 1 */
        *p++ = 0x11;            /* huffman table 1 */
        *p++ = 2;               /* comp 2 */
        *p++ = 0x11;            /* huffman table 1 */
        *p++ = 0;               /* first DCT coeff */
        *p++ = 63;              /* last DCT coeff */
        *p++ = 0;               /* sucessive approx. */

        return (p - start);
};


/*
 * Table K.1 from JPEG spec.
 */
static const int jpeg_luma_quantizer[64] = {
	16, 11, 10, 16, 24, 40, 51, 61,
	12, 12, 14, 19, 26, 58, 60, 55,
	14, 13, 16, 24, 40, 57, 69, 56,
	14, 17, 22, 29, 51, 87, 80, 62,
	18, 22, 37, 56, 68, 109, 103, 77,
	24, 35, 55, 64, 81, 104, 113, 92,
	49, 64, 78, 87, 103, 121, 120, 101,
	72, 92, 95, 98, 112, 100, 103, 99
};

/*
 * Table K.2 from JPEG spec.
 */
static const int jpeg_chroma_quantizer[64] = {
	17, 18, 24, 47, 99, 99, 99, 99,
	18, 21, 26, 66, 99, 99, 99, 99,
	24, 26, 56, 99, 99, 99, 99, 99,
	47, 66, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
};

/*
 * Call MakeTables with the Q factor and two u_char[64] return arrays
 */
static void MakeTables(int q, u_char *lqt, u_char *cqt)
{
	int i;
	int factor = q;

	if (q < 1) factor = 1;
	if (q > 99) factor = 99;
	if (q < 50)
		q = 5000 / factor;
	else
		q = 200 - factor*2;

	for (i=0; i < 64; i++) {
		int lq = (jpeg_luma_quantizer[i] * q + 50) / 100;
		int cq = (jpeg_chroma_quantizer[i] * q + 50) / 100;

		/* Limit the quantizers to 1 <= q <= 255 */
		if (lq < 1) lq = 1;
		else if (lq > 255) lq = 255;
		lqt[i] = lq;

		if (cq < 1) cq = 1;
		else if (cq > 255) cq = 255;
		cqt[i] = cq;
	}
}

static mblk_t *
read_rfc2435_header(DecState *s,mblk_t *inm)
{
	if (msgdsize(inm) >= sizeof(struct jpeghdr)) {
		struct jpeghdr *hdr = (struct jpeghdr *)inm->b_rptr;
		uint32_t off = ntohl(*(uint32_t*)inm->b_rptr);
		uint16_t dri=0;
		uint16_t table_len=0;
		int len=0;

		mblk_t *headers=NULL;

		inm->b_rptr += sizeof(struct jpeghdr);
		if (hdr->type>63){
			struct jpeghdr_rst *rsthdr = (struct jpeghdr_rst *)inm->b_rptr;
			dri = ntohs(rsthdr->dri);
			inm->b_rptr += sizeof(struct jpeghdr_rst);
		}
			
		if (off==0){
			if (hdr->q>=128){
				inm->b_rptr++; /* MBZ */
				inm->b_rptr++; /* Precision */
				table_len = ntohs(*((uint16_t*)(inm->b_rptr)));
				inm->b_rptr++; /* len */
				inm->b_rptr++; /* len */
				headers = allocb(495 + table_len*2 + (dri > 0 ? 6 : 0), 0);
				len = MakeHeaders(headers->b_rptr, hdr->type, hdr->width, hdr->height,
					inm->b_rptr, inm->b_rptr+table_len/2, table_len, dri);
				inm->b_rptr += table_len;
				headers->b_wptr += len;
			}else{
				uint8_t lqt_cqt[128];
				MakeTables(hdr->q, lqt_cqt, lqt_cqt+64);
				table_len=128;
				headers = allocb(495 + table_len + (dri > 0 ? 6 : 0), 0);
				len = MakeHeaders(headers->b_rptr, hdr->type, hdr->width, hdr->height,
					lqt_cqt, lqt_cqt+64, table_len, dri);
				headers->b_wptr += len; 
			}
		}

		if (headers!=NULL)
		{
			/* prepend headers to JPEG RTP data */
			if (mblk_get_marker_info(inm))
				mblk_set_marker_info(headers, TRUE);
			headers->b_cont=inm;
			msgpullup(headers, -1);
			return headers;
		}
	} else {
		freemsg(inm);
		inm=NULL;
	}
	return inm;
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
	else if (s->codec==CODEC_ID_MJPEG && f->desc->id==MS_JPEG_DEC_ID) inm=read_rfc2435_header(s,inm);
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

MSFilterDesc ms_jpeg_dec_desc={
	MS_JPEG_DEC_ID,
	"MSJpegDec",
	N_("A RTP/JPEG decoder using ffmpeg library"),
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

MSFilterDesc ms_mjpeg_dec_desc={
	MS_MJPEG_DEC_ID,
	"MSMJpegDec",
	N_("A MJPEG decoder using ffmpeg library"),
	MS_FILTER_DECODER,
	"MJPEG",
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

MSFilterDesc ms_jpeg_dec_desc={
	.id=MS_JPEG_DEC_ID,
	.name="MSJpegDec",
	.text="A RTP/MJEPG decoder using ffmpeg library",
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

MSFilterDesc ms_mjpeg_dec_desc={
	.id=MS_MJPEG_DEC_ID,
	.name="MSMJpegDec",
	.text="A MJEPG decoder using ffmpeg library",
	.category=MS_FILTER_DECODER,
	.enc_fmt="MJPEG",
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
MS_FILTER_DESC_EXPORT(ms_snow_dec_desc)

/* decode JPEG image with RTP/jpeg headers */
MS_FILTER_DESC_EXPORT(ms_jpeg_dec_desc)
/* decode JPEG image with jpeg headers */
MS_FILTER_DESC_EXPORT(ms_mjpeg_dec_desc)
