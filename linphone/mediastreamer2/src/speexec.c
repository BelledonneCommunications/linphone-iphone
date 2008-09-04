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
	MSBufferizer in[2];
	int framesize;
	int filterlength;
	int samplerate;
	SpeexPreprocessState *den;
        int ref;
        int echo;
        int out;
}SpeexECState;

static void speex_ec_init(MSFilter *f){
	SpeexECState *s=(SpeexECState *)ms_new(SpeexECState,1);

	s->samplerate=8000;
	s->framesize=framesize;
	s->filterlength=filter_length;

	ms_bufferizer_init(&s->in[0]);
	ms_bufferizer_init(&s->in[1]);
	s->ecstate=speex_echo_state_init(s->framesize,s->filterlength);
	s->den = speex_preprocess_state_init(s->framesize, s->samplerate);

	f->data=s;
}

static void speex_ec_uninit(MSFilter *f){
	SpeexECState *s=(SpeexECState*)f->data;
	ms_bufferizer_uninit(&s->in[0]);
	ms_bufferizer_uninit(&s->in[1]);
	speex_echo_state_destroy(s->ecstate);
	if (s->den!=NULL)
	  speex_preprocess_state_destroy(s->den);

	ms_free(s);
}

/*	inputs[0]= reference signal (sent to soundcard)
	inputs[1]= echo signal	(read from soundcard)
*/


static void speex_ec_process(MSFilter *f){
	SpeexECState *s=(SpeexECState*)f->data;
	int nbytes=s->framesize*2;
	uint8_t *in1;
	mblk_t *om0,*om1;
#ifdef HAVE_SPEEX_NOISE
	spx_int32_t *noise=(spx_int32_t*)alloca(nbytes*sizeof(spx_int32_t)+1);
#else
	float *noise=NULL;
#endif
#ifdef AMD_WIN32_HACK
	static int count=0;
#endif

	/*read input and put in bufferizers*/
	ms_bufferizer_put_from_queue(&s->in[0],f->inputs[0]);
	ms_bufferizer_put_from_queue(&s->in[1],f->inputs[1]);
	
	in1=(uint8_t*)alloca(nbytes);

	ms_debug("speexec:  in0=%i, in1=%i",ms_bufferizer_get_avail(&s->in[0]),ms_bufferizer_get_avail(&s->in[1]));

	while (ms_bufferizer_get_avail(&s->in[0])>=nbytes && ms_bufferizer_get_avail(&s->in[1])>=nbytes){
		om0=allocb(nbytes,0);
		ms_bufferizer_read(&s->in[0],(uint8_t*)om0->b_wptr,nbytes);
		/* we have reference signal */
		/* the reference signal is sent through outputs[0]*/
		
		om0->b_wptr+=nbytes;
		ms_queue_put(f->outputs[0],om0);

		ms_bufferizer_read(&s->in[1],in1,nbytes);
		/* we have echo signal */
		om1=allocb(nbytes,0);
		speex_echo_cancel(s->ecstate,(short*)in1,(short*)om0->b_rptr,(short*)om1->b_wptr,(spx_int32_t*)noise);
		if (s->den!=NULL && noise!=NULL)
		  speex_preprocess(s->den, (short*)om1->b_wptr, (spx_int32_t*)noise);

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
	}

	if (ms_bufferizer_get_avail(&s->in[0])> 4*320*(s->samplerate/8000)) /* above 4*20ms -> useless */
	  {
	    /* reset evrything */
	    ms_warning("speexec: -reset of echo canceller- in0=%i, in1=%i",ms_bufferizer_get_avail(&s->in[0]),ms_bufferizer_get_avail(&s->in[1]));
	    flushq(&s->in[1].q,0);
	    flushq(&s->in[0].q,0);
	    ms_bufferizer_init(&s->in[0]);
	    ms_bufferizer_init(&s->in[1]);
	    speex_echo_state_reset(s->ecstate);
	  }

	while (ms_bufferizer_get_avail(&s->in[1])> 4*320*(s->samplerate/8000)){
		om1=allocb(nbytes,0);
		ms_bufferizer_read(&s->in[1],(uint8_t*)om1->b_wptr,nbytes);
		om1->b_wptr+=nbytes;
		ms_queue_put(f->outputs[1],om1);
		ms_message("too much echo signal, sending anyway.");
		speex_echo_state_reset(s->ecstate);
	}
	
}

static int speex_ec_set_sr(MSFilter *f, void *arg){
#ifdef SPEEX_ECHO_SET_SAMPLING_RATE
	SpeexECState *s=(SpeexECState*)f->data;

	s->samplerate = *(int*)arg;

	if (s->ecstate==NULL)
		speex_echo_state_destroy(s->ecstate);
	if (s->den!=NULL)
	  speex_preprocess_state_destroy(s->den);

	s->ecstate=speex_echo_state_init(s->framesize,s->filterlength);
	speex_echo_ctl(s->ecstate, SPEEX_ECHO_SET_SAMPLING_RATE, &s->samplerate);
	s->den = speex_preprocess_state_init(s->framesize, s->samplerate);
#else
	ms_error("Speex echocanceler does not support 16Khz sampling rate in this version!");
#endif
	return 0;
}

static int speex_ec_set_framesize(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
	s->framesize = *(int*)arg;

	if (s->ecstate==NULL)
		speex_echo_state_destroy(s->ecstate);
	if (s->den!=NULL)
	  speex_preprocess_state_destroy(s->den);

	s->ecstate=speex_echo_state_init(s->framesize,s->filterlength);
#ifdef SPEEX_ECHO_SET_SAMPLING_RATE
	speex_echo_ctl(s->ecstate, SPEEX_ECHO_SET_SAMPLING_RATE, &s->samplerate);
#endif
	s->den = speex_preprocess_state_init(s->framesize, s->samplerate);
	return 0;
}

static int speex_ec_set_filterlength(MSFilter *f, void *arg){
	SpeexECState *s=(SpeexECState*)f->data;
	s->filterlength = *(int*)arg;

	if (s->ecstate==NULL)
		speex_echo_state_destroy(s->ecstate);
	if (s->den!=NULL)
	  speex_preprocess_state_destroy(s->den);

	s->ecstate=speex_echo_state_init(s->framesize,s->filterlength);
#ifdef SPEEX_ECHO_SET_SAMPLING_RATE
	speex_echo_ctl(s->ecstate, SPEEX_ECHO_SET_SAMPLING_RATE, &s->samplerate);
#endif
	s->den = speex_preprocess_state_init(s->framesize, s->samplerate);

	return 0;
}

static MSFilterMethod speex_ec_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE, speex_ec_set_sr },
	{	MS_FILTER_SET_FRAMESIZE, speex_ec_set_framesize },
	{	MS_FILTER_SET_FILTERLENGTH, speex_ec_set_filterlength },
	{	0			, NULL}
};

#ifdef _MSC_VER

MSFilterDesc ms_speex_ec_desc={
	MS_SPEEX_EC_ID,
	"MSSpeexEC",
	"Echo canceler using speex library",
	MS_FILTER_OTHER,
	NULL,
	2,
	2,
	speex_ec_init,
	NULL,
	speex_ec_process,
	NULL,
	speex_ec_uninit,
	speex_ec_methods
};

#else

MSFilterDesc ms_speex_ec_desc={
	.id=MS_SPEEX_EC_ID,
	.name="MSSpeexEC",
	.text="Echo canceler using speex library",
	.category=MS_FILTER_OTHER,
	.ninputs=2,
	.noutputs=2,
	.init=speex_ec_init,
	.process=speex_ec_process,
	.uninit=speex_ec_uninit,
	.methods=speex_ec_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_speex_ec_desc)
