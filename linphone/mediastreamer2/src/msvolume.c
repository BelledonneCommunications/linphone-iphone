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

typedef struct Volume{
	float energy;
}Volume;

static void volume_init(MSFilter *f){
	f->data=ms_new0(Volume,1);
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
		ms_queue_put(f->outputs[0],m);
	}
	v->energy=en;
}

static MSFilterMethod methods[]={
	{	MS_VOLUME_GET	,	volume_get	},
	{	0		,	NULL		}
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
