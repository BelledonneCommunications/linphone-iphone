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


#ifndef MSAVDECODER_H
#define MSAVDECODER_H

#include "msfilter.h"


#include <avcodec.h>

/*this is the class that implements a AVdecoder filter*/

#define MSAVDECODER_MAX_INPUTS  1 /* max output per filter*/


struct _MSAVDecoder
{
    /* the MSAVDecoder derivates from MSFilter, so the MSFilter object MUST be the first of the MSAVDecoder object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSQueue *q_inputs[MSAVDECODER_MAX_INPUTS];
    MSQueue *q_outputs[MSAVDECODER_MAX_INPUTS];
	AVCodec *av_codec;  /*the AVCodec from which this MSFilter is related */
	AVCodecContext av_context;  /* the context of the AVCodec */
	gint av_opened;
	int output_pix_fmt;
	int width;
	int height;
	int skip_gob;
	unsigned char buf_compressed[100000];
	int buf_size;
	MSBuffer *obufwrap;		/* alternate buffer, when format change is needed*/
};

typedef struct _MSAVDecoder MSAVDecoder;

struct _MSAVDecoderClass
{
	/* the MSAVDecoder derivates from MSFilter, so the MSFilter class MUST be the first of the MSAVDecoder class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
};

typedef struct _MSAVDecoderClass MSAVDecoderClass;

/* PUBLIC */
#define MS_AVDECODER(filter) ((MSAVDecoder*)(filter))
#define MS_AVDECODER_CLASS(klass) ((MSAVDecoderClass*)(klass))

MSFilter *ms_h263p_decoder_new();
MSFilter *ms_mpeg_decoder_new();
MSFilter *ms_mpeg4_decoder_new();
MSFilter * ms_AVdecoder_new_with_codec(enum CodecID codec_id);

gint ms_AVdecoder_set_format(MSAVDecoder *dec, gchar *fmt);
void ms_AVdecoder_set_width(MSAVDecoder *av,gint w);
void ms_AVdecoder_set_height(MSAVDecoder *av,gint h);

/* FOR INTERNAL USE*/
void ms_AVdecoder_init(MSAVDecoder *r, AVCodec *codec);
void ms_AVdecoder_uninit(MSAVDecoder *enc);
void ms_AVdecoder_class_init(MSAVDecoderClass *klass);
void ms_AVdecoder_destroy( MSAVDecoder *obj);
void ms_AVdecoder_process(MSAVDecoder *r);

void ms_AVCodec_init();

#endif
