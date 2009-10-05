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

#include "mediastreamer2/msspeexec.h"

#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#ifdef WIN32
#include <malloc.h> /* for alloca */
#endif

static const int framesize=128;
static const int filter_length=2048; /*250 ms*/

typedef struct SpeexECState{
	SpeexEchoState *ecstate;
	MSBufferizer speak_delay;
	int size_delay;
	int playback_delay;
	MSBufferizer in[2];
	int framesize;
	int filterlength;
	int samplerate;
	SpeexPreprocessState *den;
        int ref;
        int echo;
        int out;
	int delay_ms;
	int tail_length_ms;
}SpeexECState;

static void speex_ec_init(MSFilter *f){
	SpeexECState *s=(SpeexECState *)ms_new(SpeexECState,1);

	s->samplerate=8000;
	s->framesize=framesize;
	s->filterlength=filter_length;

	ms_bufferizer_init(&s->speak_delay);
	s->size_delay=0;
	s->delay_ms=0;
	s->playback_delay=0;
	s->tail_length_ms=250;

	ms_bufferizer_init(&s->in[0]);
	ms_bufferizer_init(&s->in[1]);
	s->ecstate=NULL;
	s->den = NULL;

	f->data=s;
}

static void speex_ec_uninit(MSFilter *f){
	SpeexECState *s=(SpeexECState*)f->data;
	ms_bufferizer_uninit(&s->speak_delay);
	ms_bufferizer_uninit(&s->in[0]);
	ms_bufferizer_uninit(&s->in[1]);
	if (s->ecstate!=NULL)
		speex_echo_state_destroy(s->ecstate);
	if (s->den!=NULL)
		speex_preprocess_state_destroy(s->den);

	ms_free(s);
}


static void speex_ec_preprocess(MSFilter *f){
	SpeexECState *s=(SpeexECState*)f->data;
	if (s->ecstate!=NULL)
		speex_echo_state_destroy(s->ecstate);
	if (s->den!=NULL)
		speex_preprocess_state_destroy(s->den);

	if (s->tail_length_ms!=0)
		s->filterlength=(s->tail_length_ms*s->samplerate)/1000;
	if (s->delay_ms!=0)
		s->playback_delay=s->delay_ms*s->samplerate/1000;
	ms_message("Initializing speex echo canceler with framesize=%i, filterlength=%i, playback_delay=%i",
		s->framesize,s->filterlength,s->playback_delay);
	s->ecstate=speex_echo_state_init(s->framesize,s->filterlength);
	s->den = speex_preprocess_state_init(s->framesize, s->samplerate);
	speex_echo_ctl(s->ecstate, SPEEX_ECHO_SET_SAMPLING_RATE, &s->samplerate);
	speex_preprocess_ctl(s->den, SPEEX_PREPROCESS_SET_ECHO_STATE, s->ecstate);
}

/*	inputs[0]= reference signal (sent to soundcard)
	inputs[1]= echo signal	(read from soundcard)
*/


static void speex_ec_process(MSFilter *f){
	SpeexECState *s=(SpeexECState*)f->data;
	int nbytes=s->framesize*2;
	uint8_t *in1;
	mblk_t *om0,*om1;
#ifdef AMD_WIN32_HACK
	static int count=0;
#endif
	mblk_t *m;
	mblk_t *md;	

	if (s->size_delay<s->playback_delay){
		while((m=ms_queue_get(f->inputs[0]))!=NULL && s->size_delay<s->playback_delay){
			// Duplicate queue : one to write to the output speaker, the other will be delayed for AEC
			int size=msgdsize(m);
			md = copyb(m);
			s->size_delay = s->size_delay + size;
			ms_bufferizer_put(&s->speak_delay,md);
			ms_bufferizer_put(&s->in[0],m);
		}

		if (s->size_delay<=s->playback_delay)
		{
			/* make sure we always send block with same size */
			while (ms_bufferizer_get_avail(&s->speak_delay)>=nbytes)
			{
				om0=allocb(nbytes,0);
				ms_bufferizer_read(&s->speak_delay,(uint8_t*)om0->b_wptr,nbytes);
				om0->b_wptr+=nbytes;
				ms_queue_put(f->outputs[0],om0);
			}

			/* make sure we always send block with same size */
			ms_bufferizer_put_from_queue(&s->in[1],f->inputs[1]);
			while (ms_bufferizer_get_avail(&s->in[1])>=nbytes)
			{
				om0=allocb(nbytes,0);
				ms_bufferizer_read(&s->in[1],(uint8_t*)om0->b_wptr,nbytes);
				om0->b_wptr+=nbytes;
    				ms_queue_put(f->outputs[1],om0);
			}
			/* we are now equal and speaker is delayed */
			return;
		}
	}

	ms_bufferizer_put_from_queue(&s->in[1],f->inputs[1]);

	/*read input and put in bufferizers*/
	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		md = copyb(m);
		// Duplicate queue : one to write to the output speaker, the other will be delayed for AEC
		ms_bufferizer_put(&s->in[0],m);
		ms_bufferizer_put(&s->speak_delay,md);	
	}
	

	in1=(uint8_t*)alloca(nbytes);

	//ms_debug("speexec:  in0=%i, in1=%i",ms_bufferizer_get_avail(&s->in[0]),ms_bufferizer_get_avail(&s->in[1]));

	while (ms_bufferizer_get_avail(&s->speak_delay)>=nbytes && ms_bufferizer_get_avail(&s->in[1])>=nbytes){
		om0=allocb(nbytes,0);
		ms_bufferizer_read(&s->speak_delay,(uint8_t*)om0->b_wptr,nbytes);
		om0->b_wptr+=nbytes;
		ms_queue_put(f->outputs[0],om0);

		om0=allocb(nbytes,0);
		ms_bufferizer_read(&s->in[0],(uint8_t*)om0->b_wptr,nbytes);
		/* we have reference signal */
		/* the reference signal is sent through outputs[0]*/
		
		om0->b_wptr+=nbytes;
		//ms_queue_put(f->outputs[0],om0);

		ms_bufferizer_read(&s->in[1],in1,nbytes);
		/* we have echo signal */
		om1=allocb(nbytes,0);
		speex_echo_cancel(s->ecstate,(short*)in1,(short*)om0->b_rptr,(short*)om1->b_wptr,NULL);
		speex_preprocess_run(s->den, (short*)om1->b_wptr);
		ms_filter_notify(f, MS_SPEEX_EC_ECHO_STATE, (void*)s->ecstate);
		ms_filter_notify(f, MS_SPEEX_EC_PREPROCESS_MIC, (void*)s->den);

		om1->b_wptr+=nbytes;
		ms_queue_put(f->outputs[1],om1);
#ifdef AMD_WIN32_HACK
		count++;
		if (count==100*3)
		{
			ms_message("periodic reset of echo canceller.");
			speex_echo_state_reset(s->ecstate);
			count=0;
		}		
#endif
		freeb(om0);
	}

	if (ms_bufferizer_get_avail(&s->speak_delay)> 5*320*(s->samplerate/8000)) /* above 4*20ms -> useless */
	{
		/* reset evrything */
		ms_warning("speexec: -reset of echo canceller- in0=%i, in1=%i",ms_bufferizer_get_avail(&s->in[0]),ms_bufferizer_get_avail(&s->in[1]));
		flushq(&s->in[1].q,0);
		flushq(&s->in[0].q,0);
		flushq(&s->speak_delay.q,0);
		ms_bufferizer_init(&s->in[0]);
		ms_bufferizer_init(&s->in[1]);
		ms_bufferizer_init(&s->speak_delay);
		s->size_delay=0;
		speex_echo_state_reset(s->ecstate);
	}

	while (ms_bufferizer_get_avail(&s->in[1])> 5*320*(s->samplerate/8000)){
		om1=allocb(nbytes,0);
		ms_bufferizer_read(&s->in[1],(uint8_t*)om1->b_wptr,nbytes);
		om1->b_wptr+=nbytes;
		ms_queue_put(f->outputs[1],om1);
		ms_message("too much echo signal, sending anyway.");
		speex_echo_state_reset(s->ecstate);
	}
	
}

static void speex_ec_postprocess(MSFilter *f){
	SpeexECState *s=(SpeexECState*)f->data;
	ms_bufferizer_uninit(&s->in[0]);
	ms_bufferizer_uninit(&s->in[1]);
	ms_bufferizer_uninit(&s->speak_delay);
	ms_bufferizer_init(&s->in[0]);
	ms_bufferizer_init(&s->in[1]);
	ms_bufferizer_init(&s->speak_delay);
	s->size_delay=0;

	if (s->ecstate!=NULL){
		speex_echo_state_destroy(s->ecstate);
		s->ecstate=NULL;
	}
	if (s->den!=NULL){
		speex_preprocess_state_destroy(s->den);
		s->den=NULL;
	}
}

static int speex_ec_set_sr(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;

	s->samplerate = *(int*)arg;

	if (s->ecstate!=NULL){
		speex_echo_state_destroy(s->ecstate);
		if (s->den!=NULL)
			speex_preprocess_state_destroy(s->den);
	
		s->ecstate=speex_echo_state_init(s->framesize,s->filterlength);
		s->den = speex_preprocess_state_init(s->framesize, s->samplerate);
		speex_echo_ctl(s->ecstate, SPEEX_ECHO_SET_SAMPLING_RATE, &s->samplerate);
		speex_preprocess_ctl(s->den, SPEEX_PREPROCESS_SET_ECHO_STATE, s->ecstate);
	}
	return 0;
}

static int speex_ec_set_framesize(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
	s->framesize = *(int*)arg;

	if (s->ecstate!=NULL){
		speex_echo_state_destroy(s->ecstate);
		if (s->den!=NULL)
			speex_preprocess_state_destroy(s->den);
	
		s->ecstate=speex_echo_state_init(s->framesize,s->filterlength);
		s->den = speex_preprocess_state_init(s->framesize, s->samplerate);
		speex_echo_ctl(s->ecstate, SPEEX_ECHO_SET_SAMPLING_RATE, &s->samplerate);
		speex_preprocess_ctl(s->den, SPEEX_PREPROCESS_SET_ECHO_STATE, s->ecstate);
	}
	return 0;
}

static int speex_ec_set_filterlength(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
	s->filterlength = (*(int*)arg)*(s->samplerate/8000);
	s->tail_length_ms=0;/*trust the length in sample, not the length in milliseconds*/
	if (s->ecstate!=NULL)
		speex_echo_state_destroy(s->ecstate);
	if (s->den!=NULL)
	  speex_preprocess_state_destroy(s->den);

	s->ecstate=speex_echo_state_init(s->framesize,s->filterlength);
	s->den = speex_preprocess_state_init(s->framesize, s->samplerate);
	speex_echo_ctl(s->ecstate, SPEEX_ECHO_SET_SAMPLING_RATE, &s->samplerate);
	speex_preprocess_ctl(s->den, SPEEX_PREPROCESS_SET_ECHO_STATE, s->ecstate);
	return 0;
}

static int speex_ec_set_delay2(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
	s->delay_ms = *(int*)arg;
	return 0;
}

static int speex_ec_set_tail_length2(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
	s->tail_length_ms=*(int*)arg;
	return 0;
}

static int speex_ec_set_playbackdelay(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;	
	s->playback_delay = *(int*)arg;

	flushq(&s->in[1].q,0);
	flushq(&s->in[0].q,0);
	flushq(&s->speak_delay.q,0);
	ms_bufferizer_init(&s->in[0]);
	ms_bufferizer_init(&s->in[1]);
	ms_bufferizer_init(&s->speak_delay);
	s->size_delay=0;
	speex_echo_state_reset(s->ecstate);
	return 0;
}

static MSFilterMethod speex_ec_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE, speex_ec_set_sr },
	{	MS_SPEEX_EC_SET_TAIL_LENGTH	,	speex_ec_set_tail_length2	},
	{	MS_SPEEX_EC_SET_DELAY		,	speex_ec_set_delay2		},
	{	MS_SPEEX_EC_SET_FRAME_SIZE	,	speex_ec_set_framesize		},
/*these are kept for backward compatibility */
	{	MS_FILTER_SET_FRAMESIZE, speex_ec_set_framesize },
	{	MS_FILTER_SET_FILTERLENGTH, speex_ec_set_filterlength },
	{	MS_FILTER_SET_PLAYBACKDELAY, speex_ec_set_playbackdelay },
	{	0			, NULL}
};

#ifdef _MSC_VER

MSFilterDesc ms_speex_ec_desc={
	MS_SPEEX_EC_ID,
	"MSSpeexEC",
	N_("Echo canceler using speex library"),
	MS_FILTER_OTHER,
	NULL,
	2,
	2,
	speex_ec_init,
	speex_ec_preprocess,
	speex_ec_process,
	speex_ec_postprocess,
	speex_ec_uninit,
	speex_ec_methods
};

#else

MSFilterDesc ms_speex_ec_desc={
	.id=MS_SPEEX_EC_ID,
	.name="MSSpeexEC",
	.text=N_("Echo canceler using speex library"),
	.category=MS_FILTER_OTHER,
	.ninputs=2,
	.noutputs=2,
	.init=speex_ec_init,
	.preprocess=speex_ec_preprocess,
	.process=speex_ec_process,
	.postprocess=speex_ec_postprocess,
	.uninit=speex_ec_uninit,
	.methods=speex_ec_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_speex_ec_desc)
