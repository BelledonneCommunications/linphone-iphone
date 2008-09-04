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


#include "msAlawdec.h"
#include "g711common.h"

extern MSFilter * ms_ALAWencoder_new(void);

MSCodecInfo ALAWinfo={
	{
		"ALAW codec",
		0,
		MS_FILTER_AUDIO_CODEC,
		ms_ALAWencoder_new,
		"This is the classic A-law codec. Good quality, but only usable with high speed network connections."
	},
	ms_ALAWencoder_new,
	ms_ALAWdecoder_new,
	320,
	160,
	64000,
	8000,
	8,
	"PCMA",
	1,
	1,
};

static MSALAWDecoderClass *ms_ALAWdecoder_class=NULL;

MSFilter * ms_ALAWdecoder_new(void)
{
	MSALAWDecoder *r;
	
	r=g_new(MSALAWDecoder,1);
	ms_ALAWdecoder_init(r);
	if (ms_ALAWdecoder_class==NULL)
	{
		ms_ALAWdecoder_class=g_new(MSALAWDecoderClass,1);
		ms_ALAWdecoder_class_init(ms_ALAWdecoder_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_ALAWdecoder_class);
	return(MS_FILTER(r));
}
	

/* FOR INTERNAL USE*/
void ms_ALAWdecoder_init(MSALAWDecoder *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->infifos=r->f_inputs;
	MS_FILTER(r)->outfifos=r->f_outputs;
	MS_FILTER(r)->r_mingran=ALAW_DECODER_RMAXGRAN;
	memset(r->f_inputs,0,sizeof(MSFifo*)*MSALAWDECODER_MAX_INPUTS);
	memset(r->f_outputs,0,sizeof(MSFifo*)*MSALAWDECODER_MAX_INPUTS);
	
}

void ms_ALAWdecoder_class_init(MSALAWDecoderClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"ALAWDecoder");
	MS_FILTER_CLASS(klass)->info=(MSFilterInfo*)&ALAWinfo;
	MS_FILTER_CLASS(klass)->max_finputs=MSALAWDECODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->max_foutputs=MSALAWDECODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->r_maxgran=ALAW_DECODER_RMAXGRAN;
	MS_FILTER_CLASS(klass)->w_maxgran=ALAW_DECODER_WMAXGRAN;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_ALAWdecoder_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_ALAWdecoder_process;
}
	
void ms_ALAWdecoder_process(MSALAWDecoder *r)
{
	MSFifo *fi,*fo;
	int inlen,outlen;
	gchar *s,*d;
	int i;
	/* process output fifos, but there is only one for this class of filter*/
	
	/* this is the simplest process function design:
	the filter declares a r_mingran of ALAW_DECODER_RMAXGRAN, so the mediastreamer's
	scheduler will call the process function each time there is ALAW_DECODER_RMAXGRAN
	bytes to read in the input fifo. If there is more, then it will call it several
	time in order to the fifo to be completetly processed.
	This is very simple, but not very efficient because of the multiple call function
	of MSFilterProcessFunc that may happen.
	The MSAlawEncoder implements another design; see it.
	*/
	
	fi=r->f_inputs[0];
	fo=r->f_outputs[0];
	g_return_if_fail(fi!=NULL);
	g_return_if_fail(fo!=NULL);
	
 	inlen=ms_fifo_get_read_ptr(fi,ALAW_DECODER_RMAXGRAN,(void**)&s);
	if (s==NULL) return;
 	outlen=ms_fifo_get_write_ptr(fo,ALAW_DECODER_WMAXGRAN,(void**)&d);
 	if (d!=NULL)
 	{
 		for(i=0;i<ALAW_DECODER_RMAXGRAN;i++)
 		{
 			((gint16*)d)[i]=alaw_to_s16( (unsigned char) s[i]);
 		}
 	}
 	else g_warning("MSALAWDecoder: Discarding samples !!");
	
}



void ms_ALAWdecoder_destroy( MSALAWDecoder *obj)
{
	g_free(obj);
}
