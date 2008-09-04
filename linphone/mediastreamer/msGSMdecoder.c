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


#include "msGSMdecoder.h"

extern MSFilter * ms_GSMencoder_new(void);

MSCodecInfo GSMinfo={
	{
		"GSM codec",
		0,
		MS_FILTER_AUDIO_CODEC,
		ms_GSMencoder_new,
		"This is the codec widely used in european mobile phones. This implementation was done by "
		"Jutta Degener and Carsten Bormann."
	},
	ms_GSMencoder_new,
	ms_GSMdecoder_new,
	320,
	33,
	13800,
	8000,
	3,
	"GSM",
	1,
	1,
};

static MSGSMDecoderClass *ms_GSMdecoder_class=NULL;

MSFilter * ms_GSMdecoder_new(void)
{
	MSGSMDecoder *r;
	
	r=g_new(MSGSMDecoder,1);
	ms_GSMdecoder_init(r);
	if (ms_GSMdecoder_class==NULL)
	{
		ms_GSMdecoder_class=g_new(MSGSMDecoderClass,1);
		ms_GSMdecoder_class_init(ms_GSMdecoder_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_GSMdecoder_class);
	return(MS_FILTER(r));
}
	

/* FOR INTERNAL USE*/
void ms_GSMdecoder_init(MSGSMDecoder *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->inqueues=r->q_inputs;
	MS_FILTER(r)->outfifos=r->f_outputs;
	MS_FILTER(r)->r_mingran=33;
	memset(r->q_inputs,0,sizeof(MSFifo*)*MSGSMDECODER_MAX_INPUTS);
	memset(r->f_outputs,0,sizeof(MSFifo*)*MSGSMDECODER_MAX_INPUTS);
	r->gsm_handle=gsm_create();
}

void ms_GSMdecoder_class_init(MSGSMDecoderClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"GSMDecoder");
	MS_FILTER_CLASS(klass)->info=(MSFilterInfo*)&GSMinfo;
	MS_FILTER_CLASS(klass)->max_qinputs=MSGSMDECODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->max_foutputs=MSGSMDECODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->w_maxgran=2*160;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_GSMdecoder_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_GSMdecoder_process;
}
	
void ms_GSMdecoder_process(MSGSMDecoder *r)
{
	MSFifo *fo;
	MSQueue *qi;
	void *d;
	MSMessage *inm;
	
	/* process output fifos, but there is only one for this class of filter*/
	
	qi=r->q_inputs[0];
	fo=r->f_outputs[0];
	inm=ms_queue_get(qi);
	ms_fifo_get_write_ptr(fo,160*2,&d);
	if (d!=NULL)
		gsm_decode(r->gsm_handle,(guchar*)inm->data,(gsm_signal*)d);
	ms_message_destroy(inm);
}

void ms_GSMdecoder_uninit(MSGSMDecoder *obj)
{
	gsm_destroy(obj->gsm_handle);
}

void ms_GSMdecoder_destroy( MSGSMDecoder *obj)
{
	ms_GSMdecoder_uninit(obj);
	g_free(obj);
}
