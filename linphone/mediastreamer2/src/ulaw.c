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
#include "g711common.h"

typedef struct _UlawEncData{
	MSBufferizer *bz;
	int ptime;
	uint32_t ts;
} UlawEncData;

static UlawEncData * ulaw_enc_data_new(){
	UlawEncData *obj=(UlawEncData *)ms_new(UlawEncData,1);
	obj->bz=ms_bufferizer_new();
	obj->ptime=0;
	obj->ts=0;
	return obj;
}

static void ulaw_enc_data_destroy(UlawEncData *obj){
	ms_bufferizer_destroy(obj->bz);
	ms_free(obj);
}

static void ulaw_enc_init(MSFilter *obj){
	obj->data=ulaw_enc_data_new();
}

static void ulaw_enc_uninit(MSFilter *obj){
	ulaw_enc_data_destroy((UlawEncData*)obj->data);
}

static void ulaw_enc_process(MSFilter *obj){
	UlawEncData *dt=(UlawEncData*)obj->data;
	MSBufferizer *bz=dt->bz;
	uint8_t buffer[2240];
	int frame_per_packet=2;
	int size_of_pcm=320;

	mblk_t *m;
	
	if (dt->ptime>=10)
	{
		frame_per_packet = dt->ptime/10;
	}

	if (frame_per_packet<=0)
		frame_per_packet=1;
	if (frame_per_packet>14) /* 7*20 == 140 ms max */
		frame_per_packet=14;

	size_of_pcm = 160*frame_per_packet; /* ex: for 20ms -> 160*2==320 */

	while((m=ms_queue_get(obj->inputs[0]))!=NULL){
		ms_bufferizer_put(bz,m);
	}

	while (ms_bufferizer_read(bz,buffer,size_of_pcm)==size_of_pcm){
		mblk_t *o=allocb(size_of_pcm/2,0);
		int i;
		for (i=0;i<size_of_pcm/2;i++){
			*o->b_wptr=s16_to_ulaw(((int16_t*)buffer)[i]);
			o->b_wptr++;
		}
		mblk_set_timestamp_info(o,dt->ts);
		dt->ts+=size_of_pcm/2;
		ms_queue_put(obj->outputs[0],o);
	}
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	UlawEncData *s=(UlawEncData*)f->data;
	char val[30];
	if (fmtp_get_value(fmtp,"ptime",val,sizeof(val))){
		s->ptime=atoi(val);
		ms_message("MSUlawEnc: got ptime=%i",s->ptime);
	}
	return 0;
}


static int enc_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	UlawEncData *s=(UlawEncData*)f->data;
	if (strstr(fmtp,"ptime:10")!=NULL){
		s->ptime=10;
	}else if (strstr(fmtp,"ptime:20")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:30")!=NULL){
		s->ptime=30;
	}else if (strstr(fmtp,"ptime:40")!=NULL){
		s->ptime=40;
	}else if (strstr(fmtp,"ptime:50")!=NULL){
		s->ptime=50;
	}else if (strstr(fmtp,"ptime:60")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:70")!=NULL){
		s->ptime=70;
	}else if (strstr(fmtp,"ptime:80")!=NULL){
		s->ptime=80;
	}else if (strstr(fmtp,"ptime:90")!=NULL){
		s->ptime=90;
	}else if (strstr(fmtp,"ptime:100")!=NULL){
		s->ptime=100;
	}else if (strstr(fmtp,"ptime:110")!=NULL){
		s->ptime=110;
	}else if (strstr(fmtp,"ptime:120")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:130")!=NULL){
		s->ptime=130;
	}else if (strstr(fmtp,"ptime:140")!=NULL){
		s->ptime=140;
	}
	return 0;
}

static MSFilterMethod enc_methods[]={
	{	MS_FILTER_ADD_ATTR		,	enc_add_attr},
	{	MS_FILTER_ADD_FMTP		,	enc_add_fmtp},
	{	0				,	NULL		}
};

#ifdef _MSC_VER

MSFilterDesc ms_ulaw_enc_desc={
	MS_ULAW_ENC_ID,
	"MSUlawEnc",
	N_("ITU-G.711 ulaw encoder"),
	MS_FILTER_ENCODER,
	"pcmu",
	1,
	1,
    ulaw_enc_init,
	NULL,
	ulaw_enc_process,
	NULL,
	ulaw_enc_uninit,
    enc_methods
};

#else

MSFilterDesc ms_ulaw_enc_desc={
	.id=MS_ULAW_ENC_ID,
	.name="MSUlawEnc",
	.text=N_("ITU-G.711 ulaw encoder"),
	.category=MS_FILTER_ENCODER,
	.enc_fmt="pcmu",
	.ninputs=1,
	.noutputs=1,
	.init=ulaw_enc_init,
	.process=ulaw_enc_process,
	.uninit=ulaw_enc_uninit,
	.methods=enc_methods
};

#endif

static void ulaw_dec_process(MSFilter *obj){
	mblk_t *m;
	while((m=ms_queue_get(obj->inputs[0]))!=NULL){
		mblk_t *o;
		msgpullup(m,-1);
		o=allocb((m->b_wptr-m->b_rptr)*2,0);
		for(;m->b_rptr<m->b_wptr;m->b_rptr++,o->b_wptr+=2){
			*((int16_t*)(o->b_wptr))=ulaw_to_s16(*m->b_rptr);
		}
		freemsg(m);
		ms_queue_put(obj->outputs[0],o);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_ulaw_dec_desc={
	MS_ULAW_DEC_ID,
	"MSUlawDec",
	N_("ITU-G.711 ulaw decoder"),
	MS_FILTER_DECODER,
	"pcmu",
	1,
	1,
	NULL,
    NULL,
    ulaw_dec_process,
    NULL,
    NULL,
    NULL
};

#else

MSFilterDesc ms_ulaw_dec_desc={
	.id=MS_ULAW_DEC_ID,
	.name="MSUlawDec",
	.text=N_("ITU-G.711 ulaw decoder"),
	.category=MS_FILTER_DECODER,
	.enc_fmt="pcmu",
	.ninputs=1,
	.noutputs=1,
	.process=ulaw_dec_process,
};

#endif

MS_FILTER_DESC_EXPORT(ms_ulaw_dec_desc)
MS_FILTER_DESC_EXPORT(ms_ulaw_enc_desc)


