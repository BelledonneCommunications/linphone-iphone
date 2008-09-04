/*
  Copyright (C) 2003  Robert W. Brewer <rbrewer at op.net>
  
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


#ifndef MSTRUESPEECHENCODER_H
#define MSTRUESPEECHENCODER_H

#include "msfilter.h"
#include <win32codec.h>


#define MS_TRUESPEECH_CODEC_MAX_IN_OUT  1 /* max inputs/outputs per filter*/

#define TRUESPEECH_FORMAT_TAG 0x22
#define TRUESPEECH_DLL "tssoft32.acm"

typedef struct _MSTrueSpeechEncoder
{
    /* the MSTrueSpeechEncoder derives from MSFilter, so the MSFilter
       object MUST be the first of the MSTrueSpeechEncoder object
       in order for the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MS_TRUESPEECH_CODEC_MAX_IN_OUT];
    MSFifo *f_outputs[MS_TRUESPEECH_CODEC_MAX_IN_OUT];
    Win32Codec* codec;
} MSTrueSpeechEncoder;

typedef struct _MSTrueSpeechEncoderClass
{
	/* the MSTrueSpeechEncoder derives from MSFilter,
           so the MSFilter class MUST be the first of the MSTrueSpechEncoder
           class
           in order for the class mechanism to work*/
  MSFilterClass parent_class;
  Win32CodecDriver* driver;
} MSTrueSpeechEncoderClass;

/* PUBLIC */
#define MS_TRUESPEECHENCODER(filter) ((MSTrueSpechMEncoder*)(filter))
#define MS_TRUESPEECHENCODER_CLASS(klass) ((MSTrueSpeechEncoderClass*)(klass))
MSFilter * ms_truespeechencoder_new(void);

/* for internal use only */
WAVEFORMATEX* ms_truespeechencoder_wf_create();


#endif
