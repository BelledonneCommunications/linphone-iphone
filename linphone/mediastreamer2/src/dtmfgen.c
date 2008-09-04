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

#include "mediastreamer2/dtmfgen.h"


#include <math.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

struct DtmfGenState{
	int rate;
	int dur;
	int pos;
	float highfreq;
	float lowfreq;
	char dtmf;
};

typedef struct DtmfGenState DtmfGenState;

static void dtmfgen_init(MSFilter *f){
	DtmfGenState *s=(DtmfGenState *)ms_new(DtmfGenState,1);
	s->rate=8000;
	s->dur=s->rate/10;
	s->pos=0;
	s->dtmf=0;
	f->data=s;
}

static void dtmfgen_uninit(MSFilter *f){
	ms_free(f->data);
}

static int dtmfgen_put(MSFilter *f, void *arg){
	DtmfGenState *s=(DtmfGenState*)f->data;
	const char *dtmf=(char*)arg;
	s->pos=0;
	switch(dtmf[0]){
		case '0':
			s->lowfreq=941;
			s->highfreq=1336;
			break;
		case '1':
			s->lowfreq=697;
			s->highfreq=1209;
			break;
		case '2':
			s->lowfreq=697;
			s->highfreq=1336;
			break;
		case '3':
			s->lowfreq=697;
			s->highfreq=1477;
			break;
		case '4':
			s->lowfreq=770;
			s->highfreq=1209;
			break;
		case '5':
			s->lowfreq=770;
			s->highfreq=1336;
			break;
		case '6':
			s->lowfreq=770;
			s->highfreq=1477;
			break;
		case '7':
			s->lowfreq=852;
			s->highfreq=1209;
			break;
		case '8':
			s->lowfreq=852;
			s->highfreq=1336;
			break;
		case '9':
			s->lowfreq=852;
			s->highfreq=1477;
			break;
		case '*':
			s->lowfreq=941;
			s->highfreq=1209;
			break;
		case '#':
			s->lowfreq=941;
			s->highfreq=1477;
			break;
		case 'A':
			s->lowfreq=697;
			s->highfreq=1633;
			break;
		case 'B':
			s->lowfreq=770;
			s->highfreq=1633;
			break;
		case 'C':
			s->lowfreq=852;
			s->highfreq=1633;
			break;
		case 'D':
			s->lowfreq=941;
			s->highfreq=1633;
			break;	
		default:
			ms_warning("Not a dtmf key.");
			return -1;
	}
	s->lowfreq=s->lowfreq/s->rate;
	s->highfreq=s->highfreq/s->rate;

	s->dtmf=dtmf[0];
	return 0;
}

static int dtmfgen_set_rate(MSFilter *f, void *arg){
	DtmfGenState *s=(DtmfGenState*)f->data;
	s->rate=*((int*)arg);
	s->dur=s->rate/10;
	return 0;
}

static void dtmfgen_process(MSFilter *f){
	mblk_t *m;
	DtmfGenState *s=(DtmfGenState*)f->data;

	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		if (s->dtmf!=0){
			int nsamples=(m->b_wptr-m->b_rptr)/2;
			int i;
			int16_t *sample=(int16_t*)m->b_rptr;
			for (i=0;i<nsamples && s->pos<s->dur;i++,s->pos++){
				sample[i]=(int16_t)(10000.0*sin(2*M_PI*(float)s->pos*s->lowfreq));
				sample[i]+=(int16_t)(10000.0*sin(2*M_PI*(float)s->pos*s->highfreq));
			}
			if (s->pos==s->dur){
				s->pos=0;
				s->dtmf=0;
			}
		}
		ms_queue_put(f->outputs[0],m);
	}
}

MSFilterMethod dtmfgen_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE	,	dtmfgen_set_rate	},
	{	MS_DTMF_GEN_PUT			,	dtmfgen_put		},
	{	0				,	NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_dtmf_gen_desc={
	MS_DTMF_GEN_ID,
	"MSDtmfGen",
	"DTMF generator",
	MS_FILTER_OTHER,
	NULL,
    1,
	1,
	dtmfgen_init,
	NULL,
    dtmfgen_process,
	NULL,
    dtmfgen_uninit,
	dtmfgen_methods
};

#else

MSFilterDesc ms_dtmf_gen_desc={
	.id=MS_DTMF_GEN_ID,
	.name="MSDtmfGen",
	.text="DTMF generator",
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=1,
	.init=dtmfgen_init,
	.process=dtmfgen_process,
	.uninit=dtmfgen_uninit,
	.methods=dtmfgen_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_dtmf_gen_desc)

