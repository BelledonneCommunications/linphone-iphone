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

#ifdef _MSC_VER
#include <malloc.h>
#define alloca _alloca
#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

#ifdef MS_FIXED_POINT
#define GAIN_ZERODB 20000
#else
#define GAIN_ZERODB 1.0
#endif

#define TAPS 128

typedef struct _EqualizerState{
	int rate;
	int nfft; /*number of fft points in time*/
	ms_word16_t *fft_cpx;
	int fir_len;
	ms_word16_t *fir;
	ms_mem_t *mem; /*memories for filtering computations*/
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

/* TODO: rate also beyond 8000 */
static EqualizerState * equalizer_state_new(int nfft){
	EqualizerState *s=(EqualizerState *)ms_new0(EqualizerState,1);
	s->rate=8000;
	s->nfft=nfft;
	s->fft_cpx=(ms_word16_t*)ms_new0(ms_word16_t,s->nfft);
	equalizer_state_flatten(s);
	s->fir_len=s->nfft;
	s->fir=(ms_word16_t*)ms_new(ms_word16_t,s->fir_len);
	s->mem=(ms_mem_t*)ms_new0(ms_mem_t,s->fir_len);
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
	ret=((hz*s->nfft)+(s->rate/2))/s->rate;
	if (ret==s->nfft/2) ret=(s->nfft/2)-1;
	return ret;
}

static int equalizer_state_index2hz(EqualizerState *s, int index){
	return (index * s->rate + s->nfft/2) / s->nfft;
}

static float gain_float(ms_word16_t val){
	return (float)val/GAIN_ZERODB;
}

static float equalizer_state_get(EqualizerState *s, int freqhz){
	int idx=equalizer_state_hz_to_index(s,freqhz);
	if (idx>=0) return gain_float(s->fft_cpx[idx*2])*s->nfft;
	return 0;
}

/* The natural peaking equalizer amplitude transfer function is multiplied to the discrete f-points.
 * Note that for PEQ no sqrt is needed for the overall calculation, applying it to gain yields the
 * same response.
 */
static float equalizer_compute_gainpoint(int f, int freq_0, float sqrt_gain, int freq_bw)
{
	float k1, k2;
	k1 = ((float)(f*f)-(float)(freq_0*freq_0));
	k1*= k1;
	k2 = (float)(f*freq_bw);
	k2*= k2;
	return (k1+k2*sqrt_gain)/(k1+k2/sqrt_gain);
}

static void equalizer_point_set(EqualizerState *s, int i, int f, float gain){
	ms_message("Setting gain %f for freq_index %i (%i Hz)\n",gain,i,f);
	s->fft_cpx[1+((i-1)*2)] = (s->fft_cpx[1+((i-1)*2)]*(int)(gain*32768))/32768;
}

static void equalizer_state_set(EqualizerState *s, int freq_0, float gain, int freq_bw){
	//int low,high;
	int i, f;
	int delta_f = equalizer_state_index2hz(s, 1);
	float sqrt_gain = sqrt(gain);
	int mid = equalizer_state_hz_to_index(s, freq_0);
	freq_bw-= delta_f/2;   /* subtract a constant - compensates for limited fft steepness at low f */
	if (freq_bw < delta_f/2)
		freq_bw = delta_f/2;
	i = mid;
	f = equalizer_state_index2hz(s, i);
	equalizer_point_set(s, i, f, gain);   /* gain according to argument */
	do {	/* note: to better accomodate limited fft steepness, -delta is applied in f-calc ... */
		i++;
		f = equalizer_state_index2hz(s, i);
		gain = equalizer_compute_gainpoint(f-delta_f, freq_0, sqrt_gain, freq_bw);
		equalizer_point_set(s, i, f, gain);
	}
	while (i < s->nfft/2 && (gain>1.1 || gain<0.9));
	i = mid;
	do {	/* ... and here +delta, as to  */
		i--;
		f = equalizer_state_index2hz(s, i);
		gain = equalizer_compute_gainpoint(f+delta_f, freq_0, sqrt_gain, freq_bw);
		equalizer_point_set(s, i, f, gain);
	}
	while (i>=0 && (gain>1.1 || gain<0.9));
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
		w=0.54 - (0.46*cos(x));
		//w=0.42 - (0.5*cos(x)) + (0.08*cos(2*x));
		s[i]=w*(float)s[i];
	}	
}

static void equalizer_state_compute_impulse_response(EqualizerState *s){
	void *fft_handle=ms_fft_init(s->nfft);
	ms_message("Spectral domain:");
	dump_table(s->fft_cpx,s->nfft);
	ms_ifft(fft_handle,s->fft_cpx,s->fir);
	ms_fft_destroy(fft_handle);
	/*
	ms_message("Inverse fft result:");
	dump_table(s->fir,s->fir_len);
	*/
	time_shift(s->fir,s->fir_len);
	/*
	ms_message("Time shifted:");
	dump_table(s->fir,s->fir_len);
	*/
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


static void equalizer_init(MSFilter *f){
	f->data=equalizer_state_new(TAPS);
}

static void equalizer_uninit(MSFilter *f){
	equalizer_state_destroy((EqualizerState*)f->data);
}

static void equalizer_process(MSFilter *f){
	mblk_t *m;
	EqualizerState *s=(EqualizerState*)f->data;
	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		if (s->active){
			equalizer_state_run(s,(int16_t*)m->b_rptr,(m->b_wptr-m->b_rptr)/2);
		}
		ms_queue_put(f->outputs[0],m);
	}
}

static int equalizer_set_gain(MSFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	MSEqualizerGain *d=(MSEqualizerGain*)data;
	equalizer_state_set(s,d->frequency,d->gain,d->width);
	return 0;
}

static int equalizer_get_gain(MSFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	MSEqualizerGain *d=(MSEqualizerGain*)data;
	d->gain=equalizer_state_get(s,d->frequency);
	d->width=0;
	return 0;
}

static int equalizer_set_rate(MSFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	s->rate=*(int*)data;
	s->needs_update=TRUE;
	return 0;
}

static int equalizer_set_active(MSFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	s->active=*(int*)data;
	return 0;
}

static int equalizer_dump(MSFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	float *t=(float*)data;
	int i;
	*t=s->fft_cpx[0];
	t++;
	for (i=1;i<s->nfft;i+=2){
		*t=((float)s->fft_cpx[i]*(float)s->nfft)/(float)GAIN_ZERODB;
		t++;
	}
	return 0;
}

static int equalizer_get_nfreqs(MSFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	*(int*)data=s->nfft/2;
	return 0;
}

static MSFilterMethod equalizer_methods[]={
	{	MS_EQUALIZER_SET_GAIN		,	equalizer_set_gain	},
	{	MS_EQUALIZER_GET_GAIN		,	equalizer_get_gain	},
	{	MS_EQUALIZER_SET_ACTIVE		,	equalizer_set_active	},
	{	MS_FILTER_SET_SAMPLE_RATE	,	equalizer_set_rate	},
	{	MS_EQUALIZER_DUMP_STATE		,	equalizer_dump		},
	{	MS_EQUALIZER_GET_NUM_FREQUENCIES,	equalizer_get_nfreqs	},
	{	0				,	NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_equalizer_desc={
	MS_EQUALIZER_ID,
	"MSEqualizer",
	N_("Parametric sound equalizer."),
	MS_FILTER_OTHER,
	NULL,
	1,
	1,
	equalizer_init,
	NULL,
	equalizer_process,
	NULL,
	equalizer_uninit,
	equalizer_methods
};

#else

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

#endif

MS_FILTER_DESC_EXPORT(ms_equalizer_desc)
