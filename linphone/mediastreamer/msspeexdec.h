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


#ifndef MSSPEEXDEC_H
#define MSSPEEXDEC_H

#include "mscodec.h"
#include <speex/speex.h>

struct _MSSpeexDec
{
	MSFilter parent;
	MSQueue *inq[1]; /* speex has an input q because it can be variable bit rate */
	MSFifo *outf[1];	
	void *speex_state;
	SpeexBits bits;
	int frequency;
	int frame_size;
	int initialized;
};

typedef struct _MSSpeexDec MSSpeexDec;
	

struct _MSSpeexDecClass
{
	MSFilterClass parent;
};

typedef struct _MSSpeexDecClass MSSpeexDecClass;


#define MS_SPEEX_DEC(o)	((MSSpeexDec*)(o))
#define MS_SPEEX_DEC_CLASS(o)	((MSSpeexDecClass*)(o))

/* call this before if don't load the plugin dynamically */
void ms_speex_codec_init();

/* mediastreamer compliant constructor */
MSFilter * ms_speex_dec_new();

void ms_speex_dec_init(MSSpeexDec *obj);
void ms_speex_dec_init_core(MSSpeexDec *obj,const SpeexMode *mode);
void ms_speex_dec_class_init(MSSpeexDecClass *klass);
void ms_speex_dec_uninit(MSSpeexDec *obj);
void ms_speex_dec_uninit_core(MSSpeexDec *obj);

void ms_speex_dec_process(MSSpeexDec *obj);
void ms_speex_dec_destroy(MSSpeexDec *obj);

#endif
