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

#include "mediastreamer2/msvolume.h"
#include <math.h>

static const float max_e=32767*32767;
static const float coef=0.1;
static const float gain_k=0.02;
static const float en_weight=4.0;
static const float noise_thres=0.1;


typedef struct Volume{
	float energy;
	float norm_en;
	float gain;
	float static_gain;
	float gain_k;
	float thres;
	float force;
	MSFilter *peer;
	bool_t ea_active;
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
	f->data=v;
}

static void volume_uninit(MSFilter *f){
	ms_free(f->data);
}

static int volume_get(MSFilter *f, void *arg){
	float *farg=(float*)arg;
	Volume *v=(Volume*)f->data;
	*farg=10*log10f((v->energy+1)/max_e);
	return 0;
}

static int volume_get_linear(MSFilter *f, void *arg){
	float *farg=(float*)arg;
	Volume *v=(Volume*)f->data;
	*farg=(v->energy+1)/max_e;
	return 0;
}

static inline float compute_gain(float static_gain, float energy, float weight){
	float ret=static_gain*(1 - (energy*weight));
	if (ret<0) ret=0;
	return ret;
}

static void volume_echo_avoider_process(Volume *v){
	float peer_e;
	float gain;
	float gain_k2;
	ms_filter_call_method(v->peer,MS_VOLUME_GET_LINEAR,&peer_e);
	peer_e=sqrt(peer_e);
	if (v->ea_active){
		if (peer_e>v->thres){
			/*lower our output*/
			gain=compute_gain(v->static_gain,peer_e,v->force);
		}else {
			gain=v->static_gain;
			v->ea_active=FALSE;
		}
	}else{
		int peer_active=FALSE;
		ms_filter_call_method(v->peer,MS_VOLUME_GET_EA_STATE,&peer_active);
		if (peer_e>v->thres && ! peer_active){
			/*lower our output*/
			gain=compute_gain(v->static_gain,peer_e,v->force);
			v->ea_active=TRUE;
		}else gain=v->static_gain;
	}
	if (v->ea_active){
		gain_k2=5*v->gain_k;
	}else gain_k2=v->gain_k;
	v->gain=(v->gain*(1-gain_k2)) + (gain_k2*gain);
	ms_message("ea_active=%i, peer_e=%f gain=%f gain_k=%f",v->ea_active,peer_e,v->gain, v->gain_k);
}

static int volume_set_gain(MSFilter *f, void *arg){
	float *farg=(float*)arg;
	Volume *v=(Volume*)f->data;
	v->gain=v->static_gain=*farg;
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

static inline int16_t saturate(float val){
	return (val>32767) ? 32767 : ( (val<-32767) ? -32767 : val);
}

static void volume_process(MSFilter *f){
	mblk_t *m;
	int16_t *sample;
	Volume *v=(Volume*)f->data;
	float en=v->energy;
	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		for (	sample=(int16_t*)m->b_rptr;
			sample<(int16_t*)m->b_wptr;
			++sample){
			float s=*sample;
			en=(s*s*coef) + (1.0-coef)*en;
		}
		if (v->peer){
			volume_echo_avoider_process(v);	
		}
		if (v->gain!=1){
			for (	sample=(int16_t*)m->b_rptr;
				sample<(int16_t*)m->b_wptr;
				++sample){
				float s=*sample;
				*sample=saturate(s*v->gain);
			}
		}
		ms_queue_put(f->outputs[0],m);
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
	{	0			,	NULL			}
};

#ifndef _MSC_VER
MSFilterDesc ms_volume_desc={
	.name="MSVolume",
	.text=N_("A filter to make level measurements on 16 bits pcm audio stream"),
	.id=MS_VOLUME_ID,
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=1,
	.init=volume_init,
	.uninit=volume_uninit,
	.process=volume_process,
	.methods=methods
};
#else
MSFilterDesc ms_volume_desc={
	MS_VOLUME_ID,
	"MSVolume",
	N_("A filter to make level measurements on 16 bits pcm audio stream"),
	MS_FILTER_OTHER,
	NULL,
	1,
	1,
	volume_init,
	NULL,
	volume_process,
	NULL,
	volume_uninit,
	methods
};
#endif

MS_FILTER_DESC_EXPORT(ms_volume_desc)


