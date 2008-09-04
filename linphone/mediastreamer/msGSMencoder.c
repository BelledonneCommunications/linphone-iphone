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


#include "msGSMencoder.h"
#include "mscodec.h"

extern MSCodecInfo GSMinfo;

static MSGSMEncoderClass *ms_GSMencoder_class=NULL;

MSFilter * ms_GSMencoder_new(void)
{
	MSGSMEncoder *r;
	
	r=g_new(MSGSMEncoder,1);
	ms_GSMencoder_init(r);
	if (ms_GSMencoder_class==NULL)
	{
		ms_GSMencoder_class=g_new(MSGSMEncoderClass,1);
		ms_GSMencoder_class_init(ms_GSMencoder_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_GSMencoder_class);
	return(MS_FILTER(r));
}
	

/* FOR INTERNAL USE*/
void ms_GSMencoder_init(MSGSMEncoder *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->infifos=r->f_inputs;
	MS_FILTER(r)->outqueues=r->q_outputs;
	MS_FILTER(r)->r_mingran=2*160;
	memset(r->f_inputs,0,sizeof(MSFifo*)*MSGSMENCODER_MAX_INPUTS);
	memset(r->q_outputs,0,sizeof(MSFifo*)*MSGSMENCODER_MAX_INPUTS);
	r->gsm_handle=gsm_create();
}

void ms_GSMencoder_class_init(MSGSMEncoderClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"GSMEncoder");
	MS_FILTER_CLASS(klass)->max_finputs=MSGSMENCODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->max_qoutputs=MSGSMENCODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->r_maxgran=2*160;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_GSMencoder_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_GSMencoder_process;
	MS_FILTER_CLASS(klass)->info=MS_FILTER_INFO(&GSMinfo);
}
	
void ms_GSMencoder_process(MSGSMEncoder *r)
{
	MSFifo *fi;
	MSQueue *qo;
	int err1;
	void *s;
	
	/* process output fifos, but there is only one for this class of filter*/
	
	fi=r->f_inputs[0];
	qo=r->q_outputs[0];
	err1=ms_fifo_get_read_ptr(fi,160*2,&s);
	if (err1>0){
		MSMessage *m=ms_message_new(33);
		gsm_encode(r->gsm_handle,(gsm_signal*)s,(gsm_byte*)m->data);
		ms_queue_put(qo,m);
	}
		
}

void ms_GSMencoder_uninit(MSGSMEncoder *obj)
{
	gsm_destroy(obj->gsm_handle);
}

void ms_GSMencoder_destroy( MSGSMEncoder *obj)
{
	ms_GSMencoder_uninit(obj);
	g_free(obj);
}
