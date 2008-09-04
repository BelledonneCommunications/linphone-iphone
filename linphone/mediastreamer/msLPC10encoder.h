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


#ifndef MSLPC10ENCODER_H
#define MSLPC10ENCODER_H

#include "mscodec.h"


int
read_16bit_samples(gint16 int16samples[], float speech[], int n);

int
write_16bit_samples(gint16 int16samples[], float speech[], int n);

void
write_bits(unsigned char *data, gint32 *bits, int len);

int
read_bits(unsigned char *data, gint32 *bits, int len);


/*this is the class that implements a LPC10encoder filter*/

#define MSLPC10ENCODER_MAX_INPUTS  1 /* max output per filter*/


typedef struct _MSLPC10Encoder
{
    /* the MSLPC10Encoder derivates from MSFilter, so the MSFilter object MUST be the first of the MSLPC10Encoder object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MSLPC10ENCODER_MAX_INPUTS];
    MSFifo *f_outputs[MSLPC10ENCODER_MAX_INPUTS];
    struct lpc10_encoder_state *lpc10_enc;
} MSLPC10Encoder;

typedef struct _MSLPC10EncoderClass
{
	/* the MSLPC10Encoder derivates from MSFilter, so the MSFilter class MUST be the first of the MSLPC10Encoder class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSLPC10EncoderClass;

/* PUBLIC */
#define MS_LPC10ENCODER(filter) ((MSLPC10Encoder*)(filter))
#define MS_LPC10ENCODER_CLASS(klass) ((MSLPC10EncoderClass*)(klass))
MSFilter * ms_LPC10encoder_new(void);

/* FOR INTERNAL USE*/
void ms_LPC10encoder_init(MSLPC10Encoder *r);
void ms_LPC10encoder_class_init(MSLPC10EncoderClass *klass);
void ms_LPC10encoder_destroy( MSLPC10Encoder *obj);
void ms_LPC10encoder_process(MSLPC10Encoder *r);

#endif
