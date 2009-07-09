/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "mediastreamer2/msfilter.h"

#ifdef _MSC_VER
#include <malloc.h>
#endif

#include <speex/speex_resampler.h>
#include <math.h>

typedef struct _ResampleData{
	MSBufferizer *bz;
	uint32_t ts;
	uint32_t input_rate;
	uint32_t output_rate;

	SpeexResamplerState *handle;
	int nb_unprocessed;
} ResampleData;

static ResampleData * resample_data_new(){
	ResampleData *obj=(ResampleData *)ms_new(ResampleData,1);
	obj->bz=ms_bufferizer_new();
	obj->ts=0;
	obj->input_rate=8000;
	obj->output_rate=16000;
	obj->handle=NULL;

	obj->nb_unprocessed=0;
	return obj;
}

static void resample_data_destroy(ResampleData *obj){
	if (obj->handle!=NULL)
		speex_resampler_destroy(obj->handle);
	ms_bufferizer_destroy(obj->bz);
	ms_free(obj);
}

static void resample_init(MSFilter *obj){
	obj->data=resample_data_new();
}

static void resample_uninit(MSFilter *obj){
	resample_data_destroy((ResampleData*)obj->data);
 
}

#if 0
static void resample_process_ms2(MSFilter *obj){
	ResampleData *dt=(ResampleData*)obj->data;
	MSBufferizer *bz=dt->bz;
	uint8_t buffer[2240];
	int size_of_input;
	int size_of_output;

	mblk_t *m;
	
	if (dt->output_rate==dt->input_rate)
	  {
	    while((m=ms_queue_get(obj->inputs[0]))!=NULL){
	      ms_queue_put(obj->outputs[0],m);
	    }
	    return;
	  }
        if (dt->handle!=NULL){
		unsigned int inrate=0, outrate=0;
		speex_resampler_get_rate(dt->handle,&inrate,&outrate);
		if (inrate!=dt->input_rate || outrate!=dt->output_rate){
			speex_resampler_destroy(dt->handle);
			dt->handle=0;
		}
	}
	if (dt->handle==NULL){
		int err=0;
		dt->handle=speex_resampler_init(1, dt->input_rate, dt->output_rate, SPEEX_RESAMPLER_QUALITY_VOIP, &err);
	}


	if (dt->input_rate<dt->output_rate)
	    size_of_input=320*dt->input_rate/8000;
	else
	    size_of_input=320*dt->input_rate/8000;
	size_of_output = (size_of_input * dt->output_rate)/dt->input_rate;

	while((m=ms_queue_get(obj->inputs[0]))!=NULL){
		ms_bufferizer_put(bz,m);
	}
	while (ms_bufferizer_read(bz,buffer,size_of_input)==size_of_input){
		mblk_t *obl=allocb(size_of_output,0);

		float *in;
		float *out;
		spx_uint32_t in_len;
		spx_uint32_t out_len;
		int err;

		short *data = (short*)buffer;
		short *data_out = (short*)obl->b_wptr;

		int i;
		spx_uint32_t idx;
    
		in = (float*) alloca((size_of_input/2)*sizeof(float));
		out = (float*) alloca((size_of_output/2)*sizeof(float));

		/* Convert the samples to floats */
		for (i = 0; i < size_of_input/2; i++)
			in[i] = (float) data[i];

		in_len = size_of_input/2;
		out_len = size_of_output/2;
		err = speex_resampler_process_float(dt->handle, 0, in, &in_len, out, &out_len);

		/* ms_message("resampling info: err=%i in_len=%i, out_len=%i", err, in_len, out_len); */

		for (idx=0;idx<out_len;idx++)
		  data_out[idx]=(short)floor(.5+out[idx]);
		obl->b_wptr=obl->b_wptr+(out_len*2); /* size_of_output; */

		mblk_set_timestamp_info(obl,dt->ts);
		dt->ts+=160;
		ms_queue_put(obj->outputs[0],obl);
	}
}

#else
static void resample_process_ms2(MSFilter *obj){
	ResampleData *dt=(ResampleData*)obj->data;
	mblk_t *m;
	
	if (dt->output_rate==dt->input_rate){
		while((m=ms_queue_get(obj->inputs[0]))!=NULL){
			ms_queue_put(obj->outputs[0],m);
		}
	    	return;
	}

	if (dt->handle!=NULL){
		unsigned int inrate=0, outrate=0;
		speex_resampler_get_rate(dt->handle,&inrate,&outrate);
		if (inrate!=dt->input_rate || outrate!=dt->output_rate){
			speex_resampler_destroy(dt->handle);
			dt->handle=0;
		}
	}
	if (dt->handle==NULL){
		int err=0;
		dt->handle=speex_resampler_init(1, dt->input_rate, dt->output_rate, SPEEX_RESAMPLER_QUALITY_VOIP, &err);
	}

	
	while((m=ms_queue_get(obj->inputs[0]))!=NULL){
		unsigned int inlen=(m->b_wptr-m->b_rptr)/2;
		unsigned int outlen=((inlen*dt->output_rate)/dt->input_rate)+1;
		unsigned int inlen_orig=inlen;
		mblk_t *om=allocb(outlen*2,0);
		speex_resampler_process_int(dt->handle, 
                                 0, 
                                 (int16_t*)m->b_rptr, 
                                 &inlen, 
                                 (int16_t*)om->b_wptr, 
                                 &outlen);
		if (inlen_orig!=inlen){
			ms_error("Bug in resampler ! only %u samples consumed instead of %u, out=%u",
				inlen,inlen_orig,outlen);
		}
		om->b_wptr+=outlen*2;
		mblk_set_timestamp_info(om,dt->ts);
		dt->ts+=outlen;
		ms_queue_put(obj->outputs[0],om);
		freemsg(m);
	}
}

#endif



int ms_resample_set_sr(MSFilter *obj, void *arg){
  ResampleData *dt=(ResampleData*)obj->data;
  dt->input_rate=((int*)arg)[0];
  return 0;
}

int ms_resample_set_output_sr(MSFilter *obj, void *arg){
  ResampleData *dt=(ResampleData*)obj->data;
  dt->output_rate=((int*)arg)[0];
  return 0;
}

static MSFilterMethod enc_methods[]={
  {	MS_FILTER_SET_SAMPLE_RATE	 ,	ms_resample_set_sr		},
  {	MS_FILTER_SET_OUTPUT_SAMPLE_RATE ,	ms_resample_set_output_sr	},
  {	0				 ,	NULL	}
};

#ifdef _MSC_VER

MSFilterDesc ms_resample_desc={
	MS_RESAMPLE_ID,
	"MSResample",
	N_("frequency resampler"),
	MS_FILTER_OTHER,
	NULL,
	1,
	1,
	resample_init,
	resample_process_ms2,
	resample_uninit,
	enc_methods
};

#else

MSFilterDesc ms_resample_desc={
	.id=MS_RESAMPLE_ID,
	.name="MSResample",
	.text=N_("frequency resampler"),
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=1,
	.init=resample_init,
	.process=resample_process_ms2,
	.uninit=resample_uninit,
	.methods=enc_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_resample_desc)

