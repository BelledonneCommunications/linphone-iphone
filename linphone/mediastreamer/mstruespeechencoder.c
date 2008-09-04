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


#include "mstruespeechencoder.h"
#include "mscodec.h"

#define TRUESPEECH_CBSIZE 32

extern MSCodecInfo TrueSpeechinfo;

static MSTrueSpeechEncoderClass *ms_truespeechencoder_class = 0;

/* FOR INTERNAL USE*/
void ms_truespeechencoder_init(MSTrueSpeechEncoder *r);
void ms_truespeechencoder_class_init(MSTrueSpeechEncoderClass *klass);
void ms_truespeechencoder_destroy(MSTrueSpeechEncoder *obj);
void ms_truespeechencoder_process(MSTrueSpeechEncoder *r);

MSFilter * ms_truespeechencoder_new(void)
{
  MSTrueSpeechEncoder *r = 0;

  if (!ms_truespeechencoder_class)
  {
    ms_truespeechencoder_class = g_new(MSTrueSpeechEncoderClass, 1);
    ms_truespeechencoder_class_init(ms_truespeechencoder_class);
  }
	
  r = g_new(MSTrueSpeechEncoder, 1);
  MS_FILTER(r)->klass = MS_FILTER_CLASS(ms_truespeechencoder_class);
  ms_truespeechencoder_init(r);
  return MS_FILTER(r);
}
	

/* FOR INTERNAL USE*/
void ms_truespeechencoder_init(MSTrueSpeechEncoder *r)
{
  ms_filter_init(MS_FILTER(r));
  MS_FILTER(r)->infifos  = r->f_inputs;
  MS_FILTER(r)->outfifos = r->f_outputs;

  WAVEFORMATEX* wf = ms_truespeechencoder_wf_create();
        
  r->codec = win32codec_create(wf, 1);
  free(wf);
                                     
  MS_FILTER(r)->r_mingran = r->codec->min_insize;

  MS_FILTER_CLASS(ms_truespeechencoder_class)->r_maxgran =
    r->codec->min_insize;
  MS_FILTER_CLASS(ms_truespeechencoder_class)->w_maxgran =
    r->codec->min_outsize;
        
  memset(r->f_inputs,  0, sizeof(MSFifo*) * MS_TRUESPEECH_CODEC_MAX_IN_OUT);
  memset(r->f_outputs, 0, sizeof(MSFifo*) * MS_TRUESPEECH_CODEC_MAX_IN_OUT);
}

void ms_truespeechencoder_class_init(MSTrueSpeechEncoderClass *klass)
{
  ms_filter_class_init(MS_FILTER_CLASS(klass));
  ms_filter_class_set_name(MS_FILTER_CLASS(klass), "TrueSpeechEncoder");
  MS_FILTER_CLASS(klass)->max_finputs  = MS_TRUESPEECH_CODEC_MAX_IN_OUT;
  MS_FILTER_CLASS(klass)->max_foutputs = MS_TRUESPEECH_CODEC_MAX_IN_OUT;
  MS_FILTER_CLASS(klass)->r_maxgran = 0; /* filled in by first instance */
  MS_FILTER_CLASS(klass)->w_maxgran = 0; /* filled in by first instance */
  MS_FILTER_CLASS(klass)->destroy = (MSFilterDestroyFunc)ms_truespeechencoder_destroy;
  MS_FILTER_CLASS(klass)->process = (MSFilterProcessFunc)ms_truespeechencoder_process;
  MS_FILTER_CLASS(klass)->info = MS_FILTER_INFO(&TrueSpeechinfo);
  klass->driver = win32codec_create_driver(TRUESPEECH_DLL,
                                           TRUESPEECH_FORMAT_TAG, 1);
}
	
void ms_truespeechencoder_process(MSTrueSpeechEncoder *r)
{
  MSFifo *fi,*fo;
  int err1;
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



void ms_truespeechencoder_uninit(MSTrueSpeechEncoder *obj)
{
  win32codec_destroy(obj->codec);
}

void ms_truespeechencoder_destroy(MSTrueSpeechEncoder *obj)
{
  ms_truespeechencoder_uninit(obj);
  g_free(obj);
}


WAVEFORMATEX* ms_truespeechencoder_wf_create()
{
  WAVEFORMATEX* ts_wf = 0;
  long* iptr = 0;
  
  ts_wf = malloc(sizeof(WAVEFORMATEX) + TRUESPEECH_CBSIZE);
  if (!ts_wf)
  {
    return 0;
  }
  
  memset(ts_wf, 0, sizeof(*ts_wf) + TRUESPEECH_CBSIZE);
  
  ts_wf->wFormatTag      = TRUESPEECH_FORMAT_TAG;
  ts_wf->nChannels       = 1;
  ts_wf->nSamplesPerSec  = 8000;
  ts_wf->wBitsPerSample  = 1;
  ts_wf->nBlockAlign     = 32;
  ts_wf->nAvgBytesPerSec = 1067;
  ts_wf->cbSize          = TRUESPEECH_CBSIZE;

  /* write extra data needed by TrueSpeech codec found
     from examining a TrueSpeech .wav file header
  */
  iptr = (long*)(ts_wf + 1);
  *iptr = 0x00f00001;

  return ts_wf;
}
