
#include "mediastreamer2/msfilter.h"

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
	ms_bufferizer_destroy(obj->bz);
	ms_free(obj);
}

static void resample_init(MSFilter *obj){
	obj->data=resample_data_new();
}

static void resample_uninit(MSFilter *obj){
	resample_data_destroy((ResampleData*)obj->data);
 
}

static void resample_preprocess(MSFilter *obj){
	ResampleData *dt=(ResampleData*)obj->data;
  int err=0;

	dt->handle = speex_resampler_init(1, dt->input_rate, dt->output_rate, SPEEX_RESAMPLER_QUALITY_VOIP, &err);
}

static void resample_postprocess(MSFilter *obj){
	ResampleData *dt=(ResampleData*)obj->data;
  if (dt->handle!=NULL)
  	speex_resampler_destroy((SpeexResamplerState*)dt->handle);
  dt->handle=NULL;
}

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

	if (dt->input_rate<dt->output_rate)
	    size_of_input=320;
	else
	    size_of_input=320;
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
    
    in = (float*) alloca((size_of_input/2)*sizeof(float));
    out = (float*) alloca((size_of_output/2)*sizeof(float));

		/* Convert the samples to floats */
		for (i = 0; i < size_of_input/2; i++)
			in[i] = (float) data[i];

    in_len = size_of_input/2;
    out_len = size_of_output/2;
    err = speex_resampler_process_float(dt->handle, 0, in, &in_len, out, &out_len);

    /* ms_message("resampling info: err=%i in_len=%i, out_len=%i", err, in_len, out_len); */

    for (i=0;i<out_len;i++)
      data_out[i]=floor(.5+out[i]);
    obl->b_wptr=obl->b_wptr+(out_len*2); /* size_of_output; */

		mblk_set_timestamp_info(obl,dt->ts);
		dt->ts+=160;
		ms_queue_put(obj->outputs[0],obl);
	}
}

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
	"frequency resampler",
	MS_FILTER_OTHER,
	NULL,
	1,
	1,
	resample_init,
	resample_preprocess,
	resample_process_ms2,
	resample_postprocess,
	resample_uninit,
	enc_methods
};

#else

MSFilterDesc ms_resample_desc={
	.id=MS_RESAMPLE_ID,
	.name="MSResample",
	.text="frequency resampler",
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=1,
	.init=resample_init,
	.preprocess=resample_preprocess,
	.process=resample_process_ms2,
	.postprocess=resample_postprocess,
	.uninit=resample_uninit,
	.methods=enc_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_resample_desc)
