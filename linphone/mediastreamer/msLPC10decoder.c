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


#include "msLPC10decoder.h"
#include "msLPC10encoder.h"
#include <stdlib.h>
#include <lpc10.h>

extern MSFilter * ms_LPC10encoder_new(void);

MSCodecInfo LPC10info={
	{
		"LPC10-15 codec",
		0,
		MS_FILTER_AUDIO_CODEC,
		ms_LPC10encoder_new,
		"A low quality but very low bit rate codec from the U.S. Department of Defense."
	},
	ms_LPC10encoder_new,
	ms_LPC10decoder_new,
	360,
	7,
	2400,
	8000,
	115,
	"1015",
	1,
	1,
};

static MSLPC10DecoderClass *ms_LPC10decoder_class=NULL;

MSFilter * ms_LPC10decoder_new(void)
{
	MSLPC10Decoder *r;
	
	r=g_new(MSLPC10Decoder,1);
	ms_LPC10decoder_init(r);
	if (ms_LPC10decoder_class==NULL)
	{
		ms_LPC10decoder_class=g_new(MSLPC10DecoderClass,1);
		ms_LPC10decoder_class_init(ms_LPC10decoder_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_LPC10decoder_class);
	return(MS_FILTER(r));
}
	

/* FOR INTERNAL USE*/
void ms_LPC10decoder_init(MSLPC10Decoder *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->infifos=r->f_inputs;
	MS_FILTER(r)->outfifos=r->f_outputs;
	MS_FILTER(r)->r_mingran=7;
	memset(r->f_inputs,0,sizeof(MSFifo*)*MSLPC10DECODER_MAX_INPUTS);
	memset(r->f_outputs,0,sizeof(MSFifo*)*MSLPC10DECODER_MAX_INPUTS);
	r->lpc10_dec=create_lpc10_decoder_state();
}

void ms_LPC10decoder_class_init(MSLPC10DecoderClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"LPC10Dec");
	MS_FILTER_CLASS(klass)->max_finputs=MSLPC10DECODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->max_foutputs=MSLPC10DECODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->r_maxgran=7;
	MS_FILTER_CLASS(klass)->w_maxgran=LPC10_SAMPLES_PER_FRAME*2;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_LPC10decoder_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_LPC10decoder_process;
	MS_FILTER_CLASS(klass)->info=(MSFilterInfo*)&LPC10info;
}
	
void ms_LPC10decoder_process(MSLPC10Decoder *r)
{
	MSFifo *fi,*fo;
	int err1;
	void *s,*d;
	float speech[LPC10_SAMPLES_PER_FRAME];
	INT32 bits[LPC10_BITS_IN_COMPRESSED_FRAME];
	
	/* process output fifos, but there is only one for this class of filter*/
	
	fi=r->f_inputs[0];
	fo=r->f_outputs[0];
	if (fi!=NULL)
	{
		err1=ms_fifo_get_read_ptr(fi,7,&s);
		if (err1>0)
		{
			err1=ms_fifo_get_write_ptr(fo,LPC10_SAMPLES_PER_FRAME*2,&d);
			if (d!=NULL)
			{
				read_bits(s, bits, LPC10_BITS_IN_COMPRESSED_FRAME);
				lpc10_decode(bits,speech, r->lpc10_dec);
				write_16bit_samples((INT16*)d, speech, LPC10_SAMPLES_PER_FRAME);
			}
		}
	}
}

void ms_LPC10decoder_uninit(MSLPC10Decoder *obj)
{
	free(obj->lpc10_dec);
}

void ms_LPC10decoder_destroy( MSLPC10Decoder *obj)
{
	ms_LPC10decoder_uninit(obj);
	g_free(obj);
}
