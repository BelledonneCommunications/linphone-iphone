
#include "mediastreamer2/msfilter.h"

#include <libresample.h>

typedef struct _ResampleData{
	MSBufferizer *bz;
	uint32_t ts;
	uint32_t input_rate;
	uint32_t output_rate;

	void *handle;
	float factor;
	int nb_unprocessed;
} ResampleData;

static ResampleData * resample_data_new(){
	ResampleData *obj=(ResampleData *)ms_new(ResampleData,1);
	obj->bz=ms_bufferizer_new();
	obj->ts=0;
	obj->input_rate=8000;
	obj->output_rate=16000;

	obj->nb_unprocessed=0;
	obj->factor=obj->output_rate/obj->input_rate;
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

	dt->handle = resample_open(1, dt->factor, dt->factor);
}

static void resample_postprocess(MSFilter *obj){
	ResampleData *dt=(ResampleData*)obj->data;
	resample_close(dt->handle);
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
#if 0
		mblk_t *obl=allocb(size_of_output,0);

		int outpos, o, srcused;
		int srcpos;
		int fwidth;

		int expectedlen = (int)(size_of_input * dt->factor);
		int dstlen = expectedlen + 1000;
		float in[1280];
		float out[2560];

		short *data = (short*)buffer;
		int i;
		/* Convert the samples to floats */
		for (i = dt->nb_unprocessed; i < size_of_input; i++)
			in[i] = (float) data[i-dt->nb_unprocessed];
		dt->nb_unprocessed=0;

		outpos = 0;
		srcpos = 0;
		for(;;) {
		  int srcBlock = MIN(size_of_input-srcpos, size_of_input);
		  int lastFlag = (srcBlock == size_of_input-srcpos);

		  o = resample_process(dt->handle, dt->factor,
							   &in[srcpos], srcBlock,
							   lastFlag, &srcused,
							   &out[outpos], MIN(dstlen-outpos, size_of_input * dt->factor + 10));
		  srcpos += srcused;
		  if (o >= 0)
			 outpos += o;
		  if (o < 0 || (o == 0 && srcpos == size_of_input))
			 break;
		}

		if (outpos>0 && outpos<=size_of_output)
		{
			//resample
			data = (short*)obl->b_wptr;
			for (i = 0; i < outpos/2; i++)
			{
				/* bound checks! */
				int bc=(short) out[i];
				if (bc < -32768)
					bc = -32768;
				else if (bc > 32767)
					bc = 32767;
				*data = bc;
				data++;
			}

			obl->b_wptr=obl->b_wptr+outpos;
			dt->ts+=160;
			ms_queue_put(obj->outputs[0],obl);
		}
		else
		{
			ms_warning("resample failed!");
			freeb(obl);
		}
#else
		mblk_t *obl=allocb(size_of_output,0);

		int srcused;
		int o;

		float in[1280];
		float out[2560];

		short *data = (short*)buffer;
		int i;
		/* Convert the samples to floats */
		for (i = 0; i < size_of_input/2; i++)
			in[i] = (float) data[i-dt->nb_unprocessed];

		o = resample_process(dt->handle, dt->factor,
						   &in[0], size_of_input/2,
						   0, &srcused,
						   &out[0], size_of_output/2);

		if (o>0 && o<=size_of_output/2)
		{
			data = (short*)obl->b_wptr;
			for (i = 0; i < o; i++)
			{
				/* bound checks! */
				int bc=(short) out[i];
				if (bc < -32768)
					bc = -32768;
				else if (bc > 32767)
					bc = 32767;
				data[i] = bc;
			}
			obl->b_wptr=obl->b_wptr+(o*2); /* size_of_output; */
			mblk_set_timestamp_info(obl,dt->ts);
			dt->ts+=160;
			ms_queue_put(obj->outputs[0],obl);
		}
		else
		{
			ms_warning("resample failed!");
			freeb(obl);
		}
#endif
	}
}

int ms_resample_set_sr(MSFilter *obj, void *arg){
  ResampleData *dt=(ResampleData*)obj->data;
  dt->input_rate=((int*)arg)[0];
  dt->factor=((float)dt->output_rate/(float)dt->input_rate);
  return 0;
}

int ms_resample_set_output_sr(MSFilter *obj, void *arg){
  ResampleData *dt=(ResampleData*)obj->data;
  dt->output_rate=((int*)arg)[0];
  dt->factor=((float)dt->output_rate/(float)dt->input_rate);
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
