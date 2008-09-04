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


#ifndef MSSPEEXENC_H
#define MSSPEEXENC_H

#include "mscodec.h"
#include <speex/speex.h>

struct _MSSpeexEnc
{
	MSFilter parent;
	MSFifo *inf[1];
	MSQueue *outq[1];	/* speex has an output q because it can be variable bit rate */
	void *speex_state;
	SpeexBits bits;
	int frequency;
	int bitrate;
	int initialized;
};

typedef struct _MSSpeexEnc MSSpeexEnc;
	

struct _MSSpeexEncClass
{
	MSFilterClass parent;
};

typedef struct _MSSpeexEncClass MSSpeexEncClass;


#define MS_SPEEX_ENC(o)	((MSSpeexEnc*)(o))
#define MS_SPEEX_ENC_CLASS(o)	((MSSpeexEncClass*)(o))

/* generic constructor */
MSFilter * ms_speex_enc_new();

void ms_speex_enc_init_core(MSSpeexEnc *obj,const SpeexMode *mode, gint quality);
void ms_speex_enc_uninit_core(MSSpeexEnc *obj);
void ms_speex_enc_init(MSSpeexEnc *obj);
void ms_speex_enc_class_init(MSSpeexEncClass *klass);


void ms_speex_enc_process(MSSpeexEnc *obj);
void ms_speex_enc_destroy(MSSpeexEnc *obj);

#endif
