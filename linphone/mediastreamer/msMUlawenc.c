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


#include "msMUlawenc.h"
#include "g711common.h"

extern MSCodecInfo MULAWinfo;

static MSMULAWEncoderClass *ms_MULAWencoder_class=NULL;

MSFilter * ms_MULAWencoder_new(void)
{
	MSMULAWEncoder *r;
	
	r=g_new(MSMULAWEncoder,1);
	ms_MULAWencoder_init(r);
	if (ms_MULAWencoder_class==NULL)
	{
		ms_MULAWencoder_class=g_new(MSMULAWEncoderClass,1);
		ms_MULAWencoder_class_init(ms_MULAWencoder_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_MULAWencoder_class);
	return(MS_FILTER(r));
}
	

/* FOR INTERNAL USE*/
void ms_MULAWencoder_init(MSMULAWEncoder *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->infifos=r->f_inputs;
	MS_FILTER(r)->outfifos=r->f_outputs;
	MS_FILTER(r)->r_mingran=MULAW_ENCODER_RMAXGRAN; /* the filter can be called as soon as there is
	something to process */
	memset(r->f_inputs,0,sizeof(MSFifo*)*MSMULAWENCODER_MAX_INPUTS);
	memset(r->f_outputs,0,sizeof(MSFifo*)*MSMULAWENCODER_MAX_INPUTS);
	
}

void ms_MULAWencoder_class_init(MSMULAWEncoderClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"MULAWEncoder");
	MS_FILTER_CLASS(klass)->info=(MSFilterInfo*)&MULAWinfo;
	MS_FILTER_CLASS(klass)->max_finputs=MSMULAWENCODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->max_foutputs=MSMULAWENCODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->r_maxgran=MULAW_ENCODER_RMAXGRAN;
	MS_FILTER_CLASS(klass)->w_maxgran=MULAW_ENCODER_WMAXGRAN;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_MULAWencoder_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_MULAWencoder_process;
}
	
void ms_MULAWencoder_process(MSMULAWEncoder *r)
{
	MSFifo *fi,*fo;
	int inlen,outlen;
	gchar *s,*d;
	int i;
	/* process output fifos, but there is only one for this class of filter*/
	
	fi=r->f_inputs[0];
	fo=r->f_outputs[0];
	inlen=ms_fifo_get_read_ptr(fi,MULAW_ENCODER_RMAXGRAN,(void**)&s);
 	outlen=ms_fifo_get_write_ptr(fo,MULAW_ENCODER_WMAXGRAN,(void**)&d);
 	if (d!=NULL)
 	{
 		for(i=0;i<MULAW_ENCODER_WMAXGRAN;i++)
 		{
 			d[i]=s16_to_ulaw( *((gint16*)s) );
 			s+=2;
 		}
 	}
 	else g_warning("MSMULAWDecoder: Discarding samples !!");
}



void ms_MULAWencoder_destroy( MSMULAWEncoder *obj)
{
	g_free(obj);
}
