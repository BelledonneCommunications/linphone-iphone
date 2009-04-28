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

#include <kde/artsc/artsc.h>

#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msfilter.h"

extern MSSndCardDesc arts_card_desc;

static int arts_users=0;
static void check_arts_init(){
	if (arts_users==0){
		arts_init();
	}
	arts_users++;
}

static void check_arts_uninit(){
	arts_users--;
	if (arts_users==0){
		arts_free();
	}
}


typedef struct ArtsState{
	int rate;
	int nchannels;
	int bits;
	int bsize;
	arts_stream_t stream;
	mblk_t *msg;
} ArtsState;

static void reader_init(MSFilter *f){
	ArtsState *s=ms_new(ArtsState,1);
	s->rate=8000;
	s->nchannels=1;
	s->bits=16;
	s->bsize=512;
	s->stream=NULL;
	s->msg=NULL;
	f->data=s;
}

static void reader_uninit(MSFilter *f){
	ArtsState *s=(ArtsState *)f->data;
	ms_free(s);
}

static void configure(arts_stream_t stream){
	int ret;
	int latency=50;
	ret=arts_stream_set(stream,ARTS_P_BUFFER_TIME,latency);
	if (ret!=latency) ms_message("Arts set latency to %i",ret);
	arts_stream_set(stream,ARTS_P_BLOCKING,0);
}

static void reader_preprocess(MSFilter *f){
	ArtsState *s=(ArtsState *)f->data;
	check_arts_init();
	s->stream=arts_record_stream(s->rate,s->bits,s->nchannels, "linphone");
	s->bsize=512*s->rate/8000;
	if (s->stream!=NULL) configure(s->stream);
}

static void reader_process(MSFilter *f){
	int err;
	ArtsState *s=(ArtsState *)f->data;
	if (s->stream!=NULL){
		mblk_t *om=s->msg;
		if (om==NULL) om=allocb(s->bsize,0);
		err=arts_read(s->stream,om->b_wptr,s->bsize);
		if (err>0){
			om->b_wptr+=err;
			ms_queue_put(f->outputs[0],om);
			om=NULL;
		}
		s->msg=om;
	}
}

static void reader_postprocess(MSFilter *f){
	ArtsState *s=(ArtsState *)f->data;
	if (s->stream) arts_close_stream(s->stream);
	s->stream=NULL;
	check_arts_uninit();
}

static void writer_preprocess(MSFilter *f){
	ArtsState *s=(ArtsState *)f->data;
	check_arts_init();
	s->stream=arts_play_stream(s->rate,s->bits,s->nchannels, "linphone");
	s->bsize=512*s->rate/8000;
	if (s->stream!=NULL) configure(s->stream);
}

static void writer_process(MSFilter *f){
	ArtsState *s=(ArtsState *)f->data;
	int err;
	mblk_t *im;
	
	if (s->stream==NULL){
		ms_queue_flush(f->inputs[0]);
		return;
	}
	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		err=arts_write(s->stream,im->b_rptr,im->b_wptr-im->b_rptr);
		if (err<0){
			ms_warning("arts_write error");
		}
		freemsg(im);
	}
}

static int reader_set_sr(MSFilter *f, void *arg){
	ArtsState *s=(ArtsState *)f->data;
	s->rate=*(int*)arg;
	return 0;
}

static int reader_set_nchannels(MSFilter *f, void *arg){
	ArtsState *s=(ArtsState *)f->data;
	s->nchannels=*(int*)arg;
	return 0;
}

static MSFilterMethod methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE	,	reader_set_sr	},
	{	MS_FILTER_SET_NCHANNELS		,	reader_set_nchannels	},
	{	0												,	NULL			}
};

MSFilterDesc ms_arts_read_desc={
	.id=MS_ARTS_READ_ID,
	.name="MSArtsRead",
	.category=MS_FILTER_OTHER,
	.ninputs=0,
	.noutputs=1,
	.init=reader_init,
	.preprocess=reader_preprocess,
	.process=reader_process,
	.postprocess=reader_postprocess,
	.uninit=reader_uninit,
	.methods=methods
};

MSFilterDesc ms_arts_write_desc={
	.id=MS_ARTS_WRITE_ID,
	.name="MSArtsWrite",
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=0,
	.init=reader_init, /*the read and the write method do the same*/
	.preprocess=writer_preprocess,
	.process=writer_process,
	.postprocess=reader_postprocess,/*the read and the write method do the same*/
	.uninit=reader_uninit,/*the read and the write method do the same*/
	.methods=methods /*the read and the write method do the same*/
};

static void arts_card_detect(MSSndCardManager *m){
	if (arts_init()==0){
		MSSndCard *card=ms_snd_card_new(&arts_card_desc);
		card->name=ms_strdup("arts driver");
		ms_snd_card_manager_add_card(ms_snd_card_manager_get(),card);
		arts_free();
	}
}

static MSFilter * arts_card_create_reader(MSSndCard *card){
	return ms_filter_new(MS_ARTS_READ_ID);
}

static MSFilter * arts_card_create_writer(MSSndCard *card){
	return ms_filter_new(MS_ARTS_WRITE_ID);
}

MSSndCardDesc arts_card_desc={
	.driver_type="aRts",
	.detect=arts_card_detect,
	.set_control=NULL,
	.get_control=NULL,
	.create_reader=arts_card_create_reader,
	.create_writer=arts_card_create_writer,
	.duplicate=NULL
};

MS_FILTER_DESC_EXPORT(ms_arts_read_desc)
MS_FILTER_DESC_EXPORT(ms_arts_write_desc)
