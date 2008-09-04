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


#ifndef MSAVENCODER_H
#define MSAVENCODER_H

#include "msfilter.h"
#include "mscodec.h"
#include <avcodec.h>

/*this is the class that implements a AVencoder filter*/

#define MSAVENCODER_MAX_INPUTS  1 /* max output per filter*/
#define MSAVENCODER_MAX_OUTPUTS 2

struct _MSAVEncoder
{
    /* the MSAVEncoder derivates from MSFilter, so the MSFilter object MUST be the first of the MSAVEncoder object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSQueue *q_inputs[MSAVENCODER_MAX_INPUTS];
    MSQueue *q_outputs[MSAVENCODER_MAX_OUTPUTS];
	AVCodec *av_codec;  /*the AVCodec from which this MSFilter is related */
	AVCodecContext av_context;  /* the context of the AVCodec */
	gint input_pix_fmt;
	gint av_opened;
	MSBuffer *comp_buf;
	MSBuffer *yuv_buf;
	MSMessage *outm;
};

typedef struct _MSAVEncoder MSAVEncoder;
/* MSAVEncoder always outputs planar YUV and accept any incoming format you should setup using
 ms_AVencoder_set_format() 
q_outputs[0] is the compressed video stream output
q_outputs[1] is a YUV planar buffer of the image it receives in input.
*/


struct _MSAVEncoderClass
{
	/* the MSAVEncoder derivates from MSFilter, so the MSFilter class MUST be the first of the MSAVEncoder class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
};

typedef struct _MSAVEncoderClass MSAVEncoderClass;

/* PUBLIC */
#define MS_AVENCODER(filter) ((MSAVEncoder*)(filter))
#define MS_AVENCODER_CLASS(klass) ((MSAVEncoderClass*)(klass))

MSFilter *ms_h263p_encoder_new();
MSFilter *ms_mpeg_encoder_new();
MSFilter *ms_mpeg4_encoder_new();
MSFilter * ms_AVencoder_new_with_codec(enum CodecID codec_id, MSCodecInfo *info);

gint ms_AVencoder_set_format(MSAVEncoder *enc, gchar *fmt);

#define ms_AVencoder_set_width(av,w)	(av)->av_context.width=(w)
#define ms_AVencoder_set_height(av,h)	(av)->av_context.height=(h)
#define ms_AVencoder_set_bit_rate(av,r)		(av)->av_context.bit_rate=(r)

void ms_AVencoder_set_frame_rate(MSAVEncoder *enc, gint frame_rate, gint frame_rate_base);

/* FOR INTERNAL USE*/
void ms_AVencoder_init(MSAVEncoder *r, AVCodec *codec);
void ms_AVencoder_uninit(MSAVEncoder *enc);
void ms_AVencoder_class_init(MSAVEncoderClass *klass);
void ms_AVencoder_destroy( MSAVEncoder *obj);
void ms_AVencoder_process(MSAVEncoder *r);


#endif
