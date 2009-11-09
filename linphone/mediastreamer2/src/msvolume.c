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

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#include "mediastreamer2/msvolume.h"
#include "mediastreamer2/msticker.h"
#include <math.h>

#ifdef HAVE_SPEEXDSP
#include <speex/speex_preprocess.h>
#endif

static const float max_e=32767*32767;
static const float coef=0.1;
static const float gain_k=0.02;
static const float en_weight=4.0;
static const float noise_thres=0.1;


typedef struct Volume{
	float energy;
	float norm_en;
	float gain; /*the one really applied, smoothed by noise gate and echo limiter*/
	float static_gain; /*the one fixed by the user*/
	float gain_k;
	float thres;
	float force;
	float target_gain; /*the target gain choosed by echo limiter and noise gate*/
	float last_peer_en;
	int sustain_time; /* time in ms for which echo limiter remains active after resuming from speech to silence.*/
	uint64_t sustain_start;
	MSFilter *peer;
#ifdef HAVE_SPEEXDSP
	SpeexPreprocessState *speex_pp;
#endif
	int sample_rate;
	int nsamples;
	int ng_cut_time; /*noise gate cut time, after last speech detected*/
	int ng_noise_dur;
	float ng_threshold;
	float ng_floorgain;
	MSBufferizer *buffer;
	bool_t ea_active;
	bool_t agc_enabled;
	bool_t noise_gate_enabled;
}Volume;

static void volume_init(MSFilter *f){
	Volume *v=(Volume*)ms_new(Volume,1);
	v->energy=0;
	v->norm_en=0;
	v->static_gain=v->gain=1;
	v->ea_active=FALSE;
	v->gain_k=gain_k;
	v->thres=noise_thres;
	v->force=en_weight;
	v->peer=NULL;
	v->last_peer_en=0;
	v->sustain_time=200;
	v->sustain_start=0;
	v->agc_enabled=FALSE;
	v->buffer=ms_bufferizer_new();
	v->sample_rate=8000;
	v->nsamples=80;
	v->noise_gate_enabled=FALSE;
	v->ng_cut_time=100;/*milliseconds*/
	v->ng_noise_dur=0;
	v->ng_threshold=noise_thres;
	v->ng_floorgain=0;
#ifdef HAVE_SPEEXDSP
	v->speex_pp=NULL;
#endif
	f->data=v;
}

static void volume_uninit(MSFilter *f){
	Volume *v=(Volume*)f->data;
#ifdef HAVE_SPEEXDSP
	if (v->speex_pp)
		speex_preprocess_state_destroy(v->speex_pp);
#endif
	ms_bufferizer_destroy(v->buffer);
	ms_free(f->data);
}

static int volume_get(MSFilter *f, void *arg){
	float *farg=(float*)arg;
	Volume *v=(Volume*)f->data;
	*farg=10*log10f((v->energy+1)/max_e);
	return 0;
}

static int volume_set_sample_rate(MSFilter *f, void *arg){
	Volume *v=(Volume*)f->data;
	v->sample_rate=*(int*)arg;
	return 0;
}

static int volume_get_linear(MSFilter *f, void *arg){
	float *farg=(float*)arg;
	Volume *v=(Volume*)f->data;
	*farg=(v->energy+1)/max_e;
	return 0;
}
#ifdef HAVE_SPEEXDSP
static void volume_agc_process(Volume *v, mblk_t *om){
	speex_preprocess_run(v->speex_pp,(int16_t*)om->b_rptr);
}
#else

static void volume_agc_process(Volume *v, mblk_t *om){
}

#endif


static inline float compute_gain(float static_gain, float energy, float weight){
	float ret=static_gain*(1 - (energy*weight));
	if (ret<0) ret=0;
	return ret;
}

/*
The principle of this algorithm is that we apply a gain to the input signal which is opposite to the 
energy measured by the peer MSVolume.
For example if some noise is played by the speaker, then the signal captured by the microphone will be lowered.
The gain changes smoothly when the peer energy is decreasing, but is immediately changed when the peer energy is 
increasing.
*/

static void volume_echo_avoider_process(Volume *v, uint64_t curtime){
	float peer_e;
	float gain;
	ms_filter_call_method(v->peer,MS_VOLUME_GET_LINEAR,&peer_e);
	peer_e=sqrt(peer_e);
	if (v->ea_active){
		if (peer_e>v->thres){
			/*lower our output*/
			gain=compute_gain(v->static_gain,peer_e,v->force);
			if (peer_e>v->last_peer_en)
				v->gain=gain;
		}else {
			v->sustain_start=curtime;
			v->ea_active=FALSE;
			gain=v->gain;
		}
	}else{
		int peer_active=FALSE;
		ms_filter_call_method(v->peer,MS_VOLUME_GET_EA_STATE,&peer_active);
		if (peer_e>v->thres && ! peer_active){
			/*lower our output*/
			gain=compute_gain(v->static_gain,peer_e,v->force);
			v->ea_active=TRUE;
			v->gain=gain;
		}else {
			if (curtime!=0 && (curtime-v->sustain_start)<v->sustain_time){
				gain=v->gain;
			}else{/*restore normal gain*/
				gain=v->static_gain;
				v->sustain_start=0;
			}
		}
	}
	v->last_peer_en=peer_e;
	v->target_gain=gain;
	ms_message("ea_active=%i, peer_e=%f gain=%f gain_k=%f force=%f",v->ea_active,peer_e,v->gain, v->gain_k,v->force);
}

static void volume_noise_gate_process(Volume *v , float energy, mblk_t *om){
	int nsamples=((om->b_wptr-om->b_rptr)/2);
	if ((energy/max_e)<v->ng_threshold){
		v->ng_noise_dur+=(nsamples*1000)/v->sample_rate;
		if (v->ng_noise_dur>v->ng_cut_time){
			v->target_gain=v->ng_floorgain;
		}
	}else{
		v->ng_noise_dur=0;
		/*let the target gain unchanged, ie let the echo-limiter choose the gain*/
	}
}

static int volume_set_gain(MSFilter *f, void *arg){
	float *farg=(float*)arg;
	Volume *v=(Volume*)f->data;
	v->gain=v->static_gain=v->target_gain=*farg;
	return 0;
}


static int volume_get_ea_state(MSFilter *f, void *arg){
	int *barg=(int*)arg;
	Volume *v=(Volume*)f->data;
	*barg=v->ea_active;
	return 0;
}

static int volume_set_peer(MSFilter *f, void *arg){
	MSFilter *p=(MSFilter*)arg;
	Volume *v=(Volume*)f->data;
	v->peer=p;
	return 0;
}

static int volume_set_agc(MSFilter *f, void *arg){
	Volume *v=(Volume*)f->data;
	v->agc_enabled=*(int*)arg;
	return 0;
}

static int volume_set_ea_threshold(MSFilter *f, void*arg){
	Volume *v=(Volume*)f->data;
	float val=*(float*)arg;
	if (val<0 || val>1) {
		ms_error("Error: threshold must be in range [0..1]");
		return -1;
	}
	v->thres=val;
	return 0;
}

static int volume_set_ea_speed(MSFilter *f, void*arg){
	Volume *v=(Volume*)f->data;
	float val=*(float*)arg;
	if (val<0 || val>1) {
		ms_error("Error: speed must be in range [0..1]");
		return -1;
	}
	v->gain_k=val;
	return 0;
}

static int volume_set_ea_force(MSFilter *f, void*arg){
	Volume *v=(Volume*)f->data;
	float val=*(float*)arg;
	v->force=val;
	return 0;
}

static int volume_set_ea_sustain(MSFilter *f, void *arg){
	Volume *v=(Volume*)f->data;
	v->sustain_time=*(int*)arg;
	return 0;
}

static int volume_enable_noise_gate(MSFilter *f, void *arg){
	Volume *v=(Volume*)f->data;
	v->noise_gate_enabled=*(int*)arg;
	return 0;
}

static int volume_set_noise_gate_threshold(MSFilter *f, void *arg){
	Volume *v=(Volume*)f->data;
	v->ng_threshold=*(float*)arg;
	return 0;
}

static int volume_set_noise_gate_floorgain(MSFilter *f, void *arg){
	Volume *v=(Volume*)f->data;
	v->ng_floorgain=*(float*)arg;
	return 0;
}

static inline int16_t saturate(float val){
	return (val>32767) ? 32767 : ( (val<-32767) ? -32767 : val);
}

static float update_energy(int16_t *signal, int numsamples, float last_energy_value){
	int i;
	float en=last_energy_value;
	for (i=0;i<numsamples;++i){
		float s=(float)signal[i];
		en=(s*s*coef) + (1.0-coef)*en;
	}
	return en;
}

static void apply_gain(Volume *v, mblk_t *m){
	int16_t *sample;
	float gain=v->target_gain;

	if (gain==1 && v->gain==1) return;
	v->gain=(v->gain*(1-v->gain_k)) + (v->gain_k*gain);
	for (	sample=(int16_t*)m->b_rptr;
				sample<(int16_t*)m->b_wptr;
				++sample){
		float s=*sample;
		*sample=saturate(s*v->gain);
	}
}

static void volume_preprocess(MSFilter *f){
	Volume *v=(Volume*)f->data;
	/*process agc by chunks of 10 ms*/
	v->nsamples=(int)(0.01*(float)v->sample_rate);
	if (v->agc_enabled){
		ms_message("AGC is enabled.");
#ifdef HAVE_SPEEXDSP
		if (v->speex_pp==NULL){
			int tmp=1;
			v->speex_pp=speex_preprocess_state_init(v->nsamples,v->sample_rate);
			if (speex_preprocess_ctl(v->speex_pp,SPEEX_PREPROCESS_SET_AGC,&tmp)==-1){
				ms_warning("Speex AGC is not available.");
			}
			tmp=0;
			speex_preprocess_ctl(v->speex_pp,SPEEX_PREPROCESS_SET_VAD,&tmp);
			speex_preprocess_ctl(v->speex_pp,SPEEX_PREPROCESS_SET_DENOISE,&tmp);
			speex_preprocess_ctl(v->speex_pp,SPEEX_PREPROCESS_SET_DEREVERB,&tmp);
		}
#else
		ms_error("No AGC possible, mediastreamer2 was compiled without libspeexdsp.");
#endif
	}
}



static void volume_process(MSFilter *f){
	mblk_t *m;
	Volume *v=(Volume*)f->data;
	float en=v->energy;

	if (v->agc_enabled){
		mblk_t *om;
		int nbytes=v->nsamples*2;
		ms_bufferizer_put_from_queue(v->buffer,f->inputs[0]);
		while(ms_bufferizer_get_avail(v->buffer)>=nbytes){
			om=allocb(nbytes,0);
			ms_bufferizer_read(v->buffer,om->b_wptr,nbytes);
			om->b_wptr+=nbytes;
			en=update_energy((int16_t*)om->b_rptr,v->nsamples,en);
			volume_agc_process(v,om);
	
			if (v->peer){
				volume_echo_avoider_process(v,f->ticker->time);
			}else v->target_gain=v->static_gain;

			if (v->noise_gate_enabled)
				volume_noise_gate_process(v,en,om);
			apply_gain(v,om);
			ms_queue_put(f->outputs[0],om);
		}
	}else{
		/*light processing: no agc. Work in place in the input buffer*/
		while((m=ms_queue_get(f->inputs[0]))!=NULL){
			en=update_energy((int16_t*)m->b_rptr,(m->b_wptr-m->b_rptr)/2,en);
			if (v->peer){
				volume_echo_avoider_process(v,f->ticker->time);	
			}else v->target_gain=v->static_gain;

			if (v->noise_gate_enabled)
				volume_noise_gate_process(v,en,m);
			apply_gain(v,m);
			ms_queue_put(f->outputs[0],m);
		}
	}
	v->energy=en;
}

static MSFilterMethod methods[]={
	{	MS_VOLUME_GET		,	volume_get		},
	{	MS_VOLUME_GET_LINEAR	, 	volume_get_linear	},
	{	MS_VOLUME_SET_GAIN	,	volume_set_gain		},
	{	MS_VOLUME_GET_EA_STATE	, 	volume_get_ea_state	},
	{	MS_VOLUME_SET_PEER	,	volume_set_peer		},
	{	MS_VOLUME_SET_EA_THRESHOLD , 	volume_set_ea_threshold	},
	{	MS_VOLUME_SET_EA_SPEED	,	volume_set_ea_speed	},
	{	MS_VOLUME_SET_EA_FORCE	, 	volume_set_ea_force	},
	{	MS_VOLUME_SET_EA_SUSTAIN,	volume_set_ea_sustain	},
	{	MS_FILTER_SET_SAMPLE_RATE,	volume_set_sample_rate	},
	{	MS_VOLUME_ENABLE_AGC	,	volume_set_agc		},
	{	MS_VOLUME_ENABLE_NOISE_GATE,	volume_enable_noise_gate},
	{	MS_VOLUME_SET_NOISE_GATE_THRESHOLD,	volume_set_noise_gate_threshold},
	{	MS_VOLUME_SET_NOISE_GATE_FLOORGAIN,	volume_set_noise_gate_floorgain},
	{	0			,	NULL			}
};

#ifndef _MSC_VER
MSFilterDesc ms_volume_desc={
	.name="MSVolume",
	.text=N_("A filter that controls and measure sound volume"),
	.id=MS_VOLUME_ID,
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=1,
	.init=volume_init,
	.uninit=volume_uninit,
	.preprocess=volume_preprocess,
	.process=volume_process,
	.methods=methods
};
#else
MSFilterDesc ms_volume_desc={
	MS_VOLUME_ID,
	"MSVolume",
	N_("A filter that controls and measure sound volume"),
	MS_FILTER_OTHER,
	NULL,
	1,
	1,
	volume_init,
	volume_preprocess,
	volume_process,
	NULL,
	volume_uninit,
	methods
};
#endif

MS_FILTER_DESC_EXPORT(ms_volume_desc)


