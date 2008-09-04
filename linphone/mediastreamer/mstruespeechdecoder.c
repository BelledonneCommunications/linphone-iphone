/*
  Copyright 2003 Robert W. Brewer <rbrewer at op.net>
  
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


#include "mstruespeechdecoder.h"
#include "mscodec.h"

MSCodecInfo TrueSpeechinfo =
{
  {
    "TrueSpeech codec",
    0,
    MS_FILTER_AUDIO_CODEC,
    ms_truespeechencoder_new,
    "This is a proprietary codec by the DSP Group that is used in some "
    "Windows applications.  It has a good quality and bitrate. "
    "It requires the Windows DLLs tssoft32.acm and "
    "tsd32.dll to be available."
  },
  ms_truespeechencoder_new,
  ms_truespeechdecoder_new,
  480,
  32,
  8536,
  8000,
  116, 
  "TSP0",
  1,
  1,
};


static MSTrueSpeechDecoderClass *ms_truespeechdecoder_class = 0;

/* FOR INTERNAL USE*/
void ms_truespeechdecoder_init(MSTrueSpeechDecoder *r);
void ms_truespeechdecoder_class_init(MSTrueSpeechDecoderClass *klass);
void ms_truespeechdecoder_destroy(MSTrueSpeechDecoder *obj);
void ms_truespeechdecoder_process(MSTrueSpeechDecoder *r);

MSFilter * ms_truespeechdecoder_new(void)
{
  MSTrueSpeechDecoder *r = 0;

  if (!ms_truespeechdecoder_class)
  {
    ms_truespeechdecoder_class = g_new(MSTrueSpeechDecoderClass, 1);
    ms_truespeechdecoder_class_init(ms_truespeechdecoder_class);
  }
	
  r = g_new(MSTrueSpeechDecoder, 1);
  MS_FILTER(r)->klass = MS_FILTER_CLASS(ms_truespeechdecoder_class);
  ms_truespeechdecoder_init(r);
  return MS_FILTER(r);
}
	

/* FOR INTERNAL USE*/
void ms_truespeechdecoder_init(MSTrueSpeechDecoder *r)
{
  ms_filter_init(MS_FILTER(r));
  MS_FILTER(r)->infifos  = r->f_inputs;
  MS_FILTER(r)->outfifos = r->f_outputs;

  WAVEFORMATEX* wf = ms_truespeechencoder_wf_create();
        
  r->codec = win32codec_create(wf, 0);
  free(wf);
                                     
  MS_FILTER(r)->r_mingran = r->codec->min_insize;

  MS_FILTER_CLASS(ms_truespeechdecoder_class)->r_maxgran =
    r->codec->min_insize;
  MS_FILTER_CLASS(ms_truespeechdecoder_class)->w_maxgran =
    r->codec->min_outsize;
        
  memset(r->f_inputs,  0, sizeof(MSFifo*) * MS_TRUESPEECH_CODEC_MAX_IN_OUT);
  memset(r->f_outputs, 0, sizeof(MSFifo*) * MS_TRUESPEECH_CODEC_MAX_IN_OUT);
}

void ms_truespeechdecoder_class_init(MSTrueSpeechDecoderClass *klass)
{
  ms_filter_class_init(MS_FILTER_CLASS(klass));
  ms_filter_class_set_name(MS_FILTER_CLASS(klass), "TrueSpeechDecoder");
  MS_FILTER_CLASS(klass)->max_finputs  = MS_TRUESPEECH_CODEC_MAX_IN_OUT;
  MS_FILTER_CLASS(klass)->max_foutputs = MS_TRUESPEECH_CODEC_MAX_IN_OUT;
  MS_FILTER_CLASS(klass)->r_maxgran = 0; /* filled in by first instance */
  MS_FILTER_CLASS(klass)->w_maxgran = 0; /* filled in by first instance */
  MS_FILTER_CLASS(klass)->destroy = (MSFilterDestroyFunc)ms_truespeechdecoder_destroy;
  MS_FILTER_CLASS(klass)->process = (MSFilterProcessFunc)ms_truespeechdecoder_process;
  MS_FILTER_CLASS(klass)->info = MS_FILTER_INFO(&TrueSpeechinfo);
  klass->driver = win32codec_create_driver(TRUESPEECH_DLL,
                                           TRUESPEECH_FORMAT_TAG, 0);
}
	
void ms_truespeechdecoder_process(MSTrueSpeechDecoder *r)
{
  MSFifo *fi,*fo;
  gint err1;
  void *s,*d;
	
  /* process output fifos, but there is only one for this class of filter*/

  fi = r->f_inputs[0];
  fo = r->f_outputs[0];
  if (fi)
  {
    err1 = ms_fifo_get_read_ptr(fi, r->codec->min_insize, &s);
    if (err1 > 0)
    {
      err1 = ms_fifo_get_write_ptr(fo, r->codec->min_outsize, &d);
      if (d)
      {
        signed long n;
        n = win32codec_convert(r->codec,
                           s, r->codec->min_insize,
                           d, r->codec->min_outsize);
      }
    }
		
  }
}



void ms_truespeechdecoder_uninit(MSTrueSpeechDecoder *obj)
{
  win32codec_destroy(obj->codec);
}

void ms_truespeechdecoder_destroy(MSTrueSpeechDecoder *obj)
{
  ms_truespeechdecoder_uninit(obj);
  g_free(obj);
}


