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

#include <gsm/gsm.h>

typedef struct EncState{
	gsm state;
	uint32_t ts;
	MSBufferizer *bufferizer;
} EncState;

static void enc_init(MSFilter *f){
	EncState *s=(EncState *)ms_new(EncState,1);
	s->state=gsm_create();
	s->ts=0;
	s->bufferizer=ms_bufferizer_new();
	f->data=s;
}

static void enc_uninit(MSFilter *f){
	EncState *s=(EncState*)f->data;
	gsm_destroy(s->state);
	ms_bufferizer_destroy(s->bufferizer);
	ms_free(s);
}



static void enc_process(MSFilter *f){
	EncState *s=(EncState*)f->data;
	mblk_t *im;
	int16_t buf[160];
	
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		ms_bufferizer_put(s->bufferizer,im);
	}
	while(ms_bufferizer_read(s->bufferizer,(uint8_t*)buf,sizeof(buf))==sizeof(buf)) {
		mblk_t *om=allocb(33,0);
		gsm_encode(s->state,(gsm_signal*)buf,(gsm_byte*)om->b_wptr);
		om->b_wptr+=33;
		mblk_set_timestamp_info(om,s->ts);
		ms_queue_put(f->outputs[0],om);
		s->ts+=sizeof(buf)/2;
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_gsm_enc_desc={
	MS_GSM_ENC_ID,
	"MSGsmEnc",
	"The GSM full-rate codec",
	MS_FILTER_ENCODER,
	"gsm",
	1,
	1,
	enc_init,
	NULL,
	enc_process,
	NULL,
	enc_uninit,
	NULL
};

#else

MSFilterDesc ms_gsm_enc_desc={
	.id=MS_GSM_ENC_ID,
	.name="MSGsmEnc",
	.text="The GSM full-rate codec",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="gsm",
	.ninputs=1,
	.noutputs=1,
	.init=enc_init,
	.process=enc_process,
	.uninit=enc_uninit,
};

#endif

static void dec_init(MSFilter *f){
	f->data=gsm_create();
}

static void dec_uninit(MSFilter *f){
	gsm s=(gsm)f->data;
	gsm_destroy(s);
}


static void dec_process(MSFilter *f){
	gsm s=(gsm)f->data;
	mblk_t *im;
	mblk_t *om;
	const int frsz=160*2;

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		om=allocb(frsz,0);
		if (gsm_decode(s,(gsm_byte*)im->b_rptr,(gsm_signal*)om->b_wptr)<0){
			ms_warning("gsm_decode error!");
			freemsg(om);
		}else{
			om->b_wptr+=frsz;
			ms_queue_put(f->outputs[0],om);
		}
		freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_gsm_dec_desc={
	MS_GSM_DEC_ID,
	"MSGsmDec",
	"The GSM codec",
	MS_FILTER_DECODER,
	"gsm",
	1,
	1,
	dec_init,
	NULL,
	dec_process,
	NULL,
	dec_uninit,
	NULL
};

#else

MSFilterDesc ms_gsm_dec_desc={
	.id=MS_GSM_DEC_ID,
	.name="MSGsmDec",
	.text="The GSM codec",
	.category=MS_FILTER_DECODER,
	.enc_fmt="gsm",
	.ninputs=1,
	.noutputs=1,
	.init=dec_init,
	.process=dec_process,
	.uninit=dec_uninit
};

#endif

MS_FILTER_DESC_EXPORT(ms_gsm_dec_desc)
MS_FILTER_DESC_EXPORT(ms_gsm_enc_desc)
