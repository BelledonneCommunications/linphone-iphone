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


#ifndef MSLPC10DECODER_H
#define MSLPC10DECODER_H

#include <msfilter.h>
#include <mscodec.h>
#include <lpc10.h>

/*this is the class that implements a LPC10decoder filter*/

#define MSLPC10DECODER_MAX_INPUTS  1 /* max output per filter*/


typedef struct _MSLPC10Decoder
{
    /* the MSLPC10Decoder derivates from MSFilter, so the MSFilter object MUST be the first of the MSLPC10Decoder object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MSLPC10DECODER_MAX_INPUTS];
    MSFifo *f_outputs[MSLPC10DECODER_MAX_INPUTS];
    struct lpc10_decoder_state *lpc10_dec;
} MSLPC10Decoder;

typedef struct _MSLPC10DecoderClass
{
	/* the MSLPC10Decoder derivates from MSFilter, so the MSFilter class MUST be the first of the MSLPC10Decoder class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSLPC10DecoderClass;

/* PUBLIC */
#define MS_LPC10DECODER(filter) ((MSLPC10Decoder*)(filter))
#define MS_LPC10DECODER_CLASS(klass) ((MSLPC10DecoderClass*)(klass))
MSFilter * ms_LPC10decoder_new(void);

/* FOR INTERNAL USE*/
void ms_LPC10decoder_init(MSLPC10Decoder *r);
void ms_LPC10decoder_class_init(MSLPC10DecoderClass *klass);
void ms_LPC10decoder_destroy( MSLPC10Decoder *obj);
void ms_LPC10decoder_process(MSLPC10Decoder *r);

extern MSCodecInfo LPC10info;

#endif
