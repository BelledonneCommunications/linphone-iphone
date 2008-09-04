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


#include "msAlawenc.h"
#include "g711common.h"

extern MSCodecInfo ALAWinfo;

static MSALAWEncoderClass *ms_ALAWencoder_class=NULL;

MSFilter * ms_ALAWencoder_new(void)
{
	MSALAWEncoder *r;
	
	r=g_new(MSALAWEncoder,1);
	ms_ALAWencoder_init(r);
	if (ms_ALAWencoder_class==NULL)
	{
		ms_ALAWencoder_class=g_new(MSALAWEncoderClass,1);
		ms_ALAWencoder_class_init(ms_ALAWencoder_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_ALAWencoder_class);
	return(MS_FILTER(r));
}
	

/* FOR INTERNAL USE*/
void ms_ALAWencoder_init(MSALAWEncoder *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->infifos=r->f_inputs;
	MS_FILTER(r)->outfifos=r->f_outputs;
	MS_FILTER(r)->r_mingran=ALAW_ENCODER_RMAXGRAN; /* the filter can be called as soon as there is
	something to process */
	memset(r->f_inputs,0,sizeof(MSFifo*)*MSALAWENCODER_MAX_INPUTS);
	memset(r->f_outputs,0,sizeof(MSFifo*)*MSALAWENCODER_MAX_INPUTS);
	
}

void ms_ALAWencoder_class_init(MSALAWEncoderClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"ALAWEncoder");
	MS_FILTER_CLASS(klass)->info=(MSFilterInfo*)&ALAWinfo;
	MS_FILTER_CLASS(klass)->max_finputs=MSALAWENCODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->max_foutputs=MSALAWENCODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->r_maxgran=ALAW_ENCODER_RMAXGRAN;
	MS_FILTER_CLASS(klass)->w_maxgran=ALAW_ENCODER_WMAXGRAN;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_ALAWencoder_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_ALAWencoder_process;
}
	
void ms_ALAWencoder_process(MSALAWEncoder *r)
{
	MSFifo *fi,*fo;
	int inlen,outlen;
	gchar *s,*d;
	int i;
	/* process output fifos, but there is only one for this class of filter*/
	
	/* this is the sophisticated design of the process function:
	Here the filter declares that it can be called as soon as there is something
	to read on the input fifo by setting r_mingran=0.
	Then it ask for the fifo to get as many data as possible by calling:
	inlen=ms_fifo_get_read_ptr(fi,0,(void**)&s);
	This avoid multiple call to the process function to process all data available
	on the input fifo... but the writing of the process function is a bit
	more difficult, because althoug ms_fifo_get_read_ptr() returns N bytes,
	we cannot ask ms_fifo_get_write_ptr to return N bytes if
	N>MS_FILTER_CLASS(klass)->w_maxgran. This is forbidden by the MSFifo
	mechanism.
	This is an open issue.
	For the moment what is done here is that ms_fifo_get_write_ptr() is called
	several time with its maximum granularity in order to try to write the output.
	...
	One solution:
	-create a new function ms_fifo_get_rw_ptr(fifo1,p1, fifo2,p2) to
		return the number of bytes able to being processed according to the input
		and output fifo, and their respective data pointers
	*/
	
	
	fi=r->f_inputs[0];
	fo=r->f_outputs[0];
	
 	inlen=ms_fifo_get_read_ptr(fi,ALAW_ENCODER_RMAXGRAN,(void**)&s);
	if (s==NULL) return;
 	outlen=ms_fifo_get_write_ptr(fo,ALAW_ENCODER_WMAXGRAN,(void**)&d);
 	if (d!=NULL)
 	{
 		for(i=0;i<ALAW_ENCODER_WMAXGRAN;i++)
 		{
 			d[i]=s16_to_alaw( *((gint16*)s) );
 			s+=2;
 		}
 	}
 	else g_warning("MSALAWDecoder: Discarding samples !!");
	
}



void ms_ALAWencoder_destroy( MSALAWEncoder *obj)
{
	g_free(obj);
}
