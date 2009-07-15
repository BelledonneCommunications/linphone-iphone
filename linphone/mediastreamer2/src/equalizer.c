/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2009  Simon MORLAT (simon.morlat@linphone.org)

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

#include <mediastreamer2/msequalizer.h>
#include <mediastreamer2/dsptools.h>

#include <math.h>

#define ABS_GAIN_ZERODB 22000

#ifdef MS_FIXED_POINT
#define GAIN_ZERODB ABS_GAIN_ZERODB
#else
#define GAIN_ZERODB (((float)ABS_GAIN_ZERODB)/32767.0)
#endif
	



typedef struct _EqualizerState{
	int rate;
	int nfft; //number of fft points in time
	ms_word16_t *fft_cpx;
	int fir_len;
	ms_word16_t *fir;
	ms_mem_t *mem; /*memories for filtering computations*/
	float width_coef;
	bool_t needs_update;
	bool_t active;
} EqualizerState;

static void equalizer_state_flatten(EqualizerState *s){
	int i;
	ms_word16_t val=GAIN_ZERODB/s->nfft;
	s->fft_cpx[0]=val;
	for(i=1;i<s->nfft;i+=2)
		s->fft_cpx[i]=val;
}

static EqualizerState * equalizer_state_new(int nfft){
	EqualizerState *s=ms_new0(EqualizerState,1);
	s->rate=8000;
	s->nfft=nfft;
	s->fft_cpx=ms_new0(ms_word16_t,s->nfft);
	equalizer_state_flatten(s);
	s->fir_len=s->nfft;
	s->fir=ms_new(ms_word16_t,s->fir_len);
	s->mem=ms_new0(ms_mem_t,s->fir_len);
	s->width_coef=0.4; /*  when setting a gain at 1000hz, we will affect 800-1200 frequency band*/
	s->needs_update=TRUE;
	s->active=TRUE;
	return s;
}

static void equalizer_state_destroy(EqualizerState *s){
	ms_free(s->fft_cpx);
	ms_free(s->fir);
	ms_free(s->mem);
	ms_free(s);
}

static int equalizer_state_hz_to_index(EqualizerState *s, int hz){
	int ret;
	if (hz<0){
		ms_error("Bad frequency value %i",hz);
		return -1;
	}
	if (hz>(s->rate/2)){
		hz=(s->rate/2);
	}
	/*round to nearest integer*/
	ret=((hz*s->nfft)+(hz/2))/s->rate;
	if (ret==s->nfft/2) ret=(s->nfft/2)-1;
	return ret;
}

static float gain_float(ms_word16_t val){
	return (float)val/GAIN_ZERODB;
}

static ms_word16_t gain_int16(float val){
	ms_word16_t ret=(val*GAIN_ZERODB);
	if (ret>=32767) ret=32767;
	return (ms_word16_t)ret;
}

static float equalizer_state_get(EqualizerState *s, int freqhz){
	int idx=equalizer_state_hz_to_index(s,freqhz);
	if (idx>=0) return gain_float(s->fft_cpx[idx*2])*s->nfft;
	return 0;
}

/* return the frequency band width we want to control around hz*/
static void equalizer_state_get_band(EqualizerState *s, int hz, int *low_index, int *high_index){
	int half_band=(int)((float)hz*s->width_coef*0.5);
	*low_index=equalizer_state_hz_to_index(s,hz-half_band);
	*high_index=equalizer_state_hz_to_index(s,hz+half_band);
}

static void equalizer_state_set(EqualizerState *s, int freqhz, float gain){
	int low,high;
	int i;
	equalizer_state_get_band(s,freqhz,&low,&high);
	for(i=low;i<=high;++i){
		ms_message("Setting gain %f for freq_index %i (freqhz=%i)",gain,i,freqhz);
		s->fft_cpx[1+(i*2)]=gain_int16(gain)/s->nfft;
	}
	s->needs_update=TRUE;
}

static void dump_table(ms_word16_t *t, int len){
	int i;
	for(i=0;i<len;i++)
#ifdef MS_FIXED_POINT
		ms_message("[%i]\t%i",i,t[i]);
#else
		ms_message("[%i]\t%f",i,t[i]);
#endif
}

static void time_shift(ms_word16_t *s, int len){
	int i;
	int half=len/2;
	ms_word16_t tmp;
	for (i=0;i<half;++i){
		tmp=s[i];
		s[i]=s[i+half];
		s[i+half]=tmp;
	}
}

/*
 *hamming:
 * 0.54 - 0.46*cos(2*M_PI*t/T)
 *
 * blackman
 * 0.42 - 0.5*cos(2*M_PI*t/T) + 0.08*cos(4*M_PI*t/T)
*/

static void norm_and_apodize(ms_word16_t *s, int len){
	int i;
	float x;
	float w;
	for(i=0;i<len;++i){
		x=(float)i*2*M_PI/(float)len;
		w=0.42 - (0.5*cos(x)) + (0.08*cos(2*x));
		s[i]=w*(float)s[i];
	}	
}

static void equalizer_state_compute_impulse_response(EqualizerState *s){
	void *fft_handle=ms_fft_init(s->nfft);
	ms_message("Spectral domain:");
	dump_table(s->fft_cpx,s->nfft);
	ms_ifft(fft_handle,s->fft_cpx,s->fir);
	ms_fft_destroy(fft_handle);
	ms_message("Inverse fft result:");
	dump_table(s->fir,s->fir_len);
	time_shift(s->fir,s->fir_len);
	ms_message("Time shifted:");
	dump_table(s->fir,s->fir_len);
	norm_and_apodize(s->fir,s->fir_len);
	ms_message("Apodized impulse response:");
	dump_table(s->fir,s->fir_len);
	s->needs_update=FALSE;
}



#ifdef MS_FIXED_POINT
#define INT16_TO_WORD16(i,w,l) w=(i)
#define WORD16_TO_INT16(i,w,l) i=(w)
#else

static void int16_to_word16(const int16_t *is, ms_word16_t *w, int l){
	int i;
	for(i=0;i<l;++i){
		w[i]=(ms_word16_t)is[i];
	}
}

static void word16_to_int16(const ms_word16_t *w, int16_t *is, int l){
	int i;
	for (i=0;i<l;++i)
		is[i]=(int16_t)w[i];
}

#define INT16_TO_WORD16(i,w,l) w=(ms_word16_t*)alloca(sizeof(ms_word16_t)*(l));int16_to_word16(i,w,l)
#define WORD16_TO_INT16(w,i,l) word16_to_int16(w,i,l)
#endif

static void equalizer_state_run(EqualizerState *s, int16_t *samples, int nsamples){
	if (s->needs_update)
		equalizer_state_compute_impulse_response(s);
	ms_word16_t *w;
	INT16_TO_WORD16(samples,w,nsamples);
	ms_fir_mem16(w,s->fir,w,nsamples,s->fir_len,s->mem);
	WORD16_TO_INT16(w,samples,nsamples);
}


void equalizer_init(MSFilter *f){
	f->data=equalizer_state_new(128);
}

void equalizer_uninit(MSFilter *f){
	equalizer_state_destroy((EqualizerState*)f->data);
}

void equalizer_process(MSFilter *f){
	mblk_t *m;
	EqualizerState *s=(EqualizerState*)f->data;
	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		if (s->active){
			equalizer_state_run(s,(int16_t*)m->b_rptr,(m->b_wptr-m->b_rptr)/2);
		}
		ms_queue_put(f->outputs[0],m);
	}
}

int equalizer_set_gain(MSFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	MSEqualizerGain *d=(MSEqualizerGain*)data;
	equalizer_state_set(s,d->frequency,d->gain);
	return 0;
}

int equalizer_get_gain(MSFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	MSEqualizerGain *d=(MSEqualizerGain*)data;
	d->gain=equalizer_state_get(s,d->frequency);
	return 0;
}

int equalizer_set_rate(MSFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	s->rate=*(int*)data;
	return 0;
}

int equalizer_set_active(MSFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	s->active=*(int*)data;
	return 0;
}

int equalizer_set_freq_width_coef(MSFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	s->width_coef=*(float*)data;
	return 0;
}

static MSFilterMethod equalizer_methods[]={
	{	MS_EQUALIZER_SET_GAIN		,	equalizer_set_gain	},
	{	MS_EQUALIZER_GET_GAIN		,	equalizer_get_gain	},
	{	MS_EQUALIZER_SET_ACTIVE		,	equalizer_set_active	},
	{	MS_FILTER_SET_SAMPLE_RATE	,	equalizer_set_rate	},
	{	MS_EQUALIZER_SET_FREQ_WIDTH_COEF,	equalizer_set_freq_width_coef},
	{	0				,	NULL			}
};

MSFilterDesc ms_equalizer_desc={
	.id= MS_EQUALIZER_ID,
	.name="MSEqualizer",
	.text=N_("Parametric sound equalizer."),
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=1,
	.init=equalizer_init,
	.process=equalizer_process,
	.uninit=equalizer_uninit,
	.methods=equalizer_methods
};

MS_FILTER_DESC_EXPORT(ms_equalizer_desc)
