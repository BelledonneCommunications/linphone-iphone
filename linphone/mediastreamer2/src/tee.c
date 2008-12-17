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

#include "mediastreamer2/mstee.h"

#define MS_TEE_NOUTPUTS 10

typedef struct _TeeData{
	bool_t muted[MS_TEE_NOUTPUTS];
}TeeData;

static void tee_init(MSFilter *f){
	f->data=ms_new0(TeeData,1);
}

static void tee_uninit(MSFilter *f){
	ms_free(f->data);
}

static void tee_process(MSFilter *f){
	TeeData *d=(TeeData*)f->data;
	mblk_t *im;
	int i;
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		for(i=0;i<f->desc->noutputs;i++){
			if (f->outputs[i]!=NULL && !d->muted[i])
				ms_queue_put(f->outputs[i],dupmsg(im));
		}
		freemsg(im);
	}
}

static int tee_mute(MSFilter *f, void *arg){
	TeeData *d=(TeeData*)f->data;
	int pin=((int*)arg)[0];
	if (pin>=0 && pin<MS_TEE_NOUTPUTS){
		d->muted[pin]=TRUE;
		return 0;
	}
	return -1;
}

static int tee_unmute(MSFilter *f, void *arg){
	TeeData *d=(TeeData*)f->data;
	int pin=((int*)arg)[0];
	if (pin>=0 && pin<MS_TEE_NOUTPUTS){
		d->muted[pin]=FALSE;
		return 0;
	}
	return -1;
}

static MSFilterMethod tee_methods[]={
	{	MS_TEE_MUTE	,	tee_mute	},
	{	MS_TEE_UNMUTE	,	tee_unmute	},
	{	0		,	NULL		}
};

#ifdef _MSC_VER

MSFilterDesc ms_tee_desc={
	MS_TEE_ID,
	"MSTee",
	N_("A filter that reads from input and copy to its multiple outputs."),
	MS_FILTER_OTHER,
	NULL,
	1,
	MS_TEE_NOUTPUTS,
    	tee_init,
	NULL,
	tee_process,
	NULL,
	tee_uninit,
    	tee_methods
};

#else

MSFilterDesc ms_tee_desc={
	.id=MS_TEE_ID,
	.name="MSTee",
	.text=N_("A filter that reads from input and copy to its multiple outputs."),
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=MS_TEE_NOUTPUTS,
	.init=tee_init,
	.process=tee_process,
	.uninit=tee_uninit,
	.methods=tee_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_tee_desc)
