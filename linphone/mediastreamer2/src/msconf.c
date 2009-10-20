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

#include "mediastreamer2/msfilter.h"
#include <math.h>

#if defined(_WIN32_WCE)
#define DISABLE_SPEEX
#endif

#ifndef HAVE_SPEEXDSP
#define DISABLE_SPEEX
#endif

#ifndef DISABLE_SPEEX
#include <speex/speex_preprocess.h>
#endif

#define CONF_NSAMPLES 160*4*4 /* (CONF_GRAN/2) */
#ifndef CONF_MAX_PINS
#define CONF_MAX_PINS 128
#endif

static const float max_e=(float)32767*32767;
static const float coef=(float)0.01;

typedef struct Channel{
	MSBufferizer buff;
	int16_t input[CONF_NSAMPLES];
	bool_t has_contributed;
	bool_t is_used;

	int is_speaking;
	int count_speaking;

	int count;
	int missed;

	int stat_discarded;
	int stat_missed;
	int stat_processed;

#ifndef DISABLE_SPEEX
	SpeexPreprocessState *speex_pp;
#endif

	float energy;
	double average_psd;

} Channel;

typedef struct ConfState{
	Channel channels[CONF_MAX_PINS];
	int sum[CONF_NSAMPLES];
	int enable_directmode;
	int enable_vad;

	int enable_halfduplex;
	int vad_prob_start;
	int vad_prob_continue;

	int agc_level;
	int max_gain;
	int mix_mode;
	int samplerate;

	int adaptative_msconf_buf;
	int conf_gran;
	int conf_nsamples;
} ConfState;


static void channel_init(ConfState *s, Channel *chan, int pos){
#ifndef DISABLE_SPEEX
	float f;
	int val;
#endif
	memset(chan, 0, sizeof(Channel));
	ms_bufferizer_init(&chan->buff);
#ifndef DISABLE_SPEEX
	chan->speex_pp = speex_preprocess_state_init(s->conf_gran/2, s->samplerate);
	if (chan->speex_pp==NULL)
		return;

	/* configure sound card input on pin 0 */
	val=0;
	if (pos==0)
		val=1;

	if (s->enable_halfduplex>0 && pos%2==1)
		val=1;

	speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_SET_DENOISE, &val);
	val = -30;
	speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &val);

	/* enable VAD only on incoming RTP stream */
	val=0;
	if (pos%2==1 || (pos==0 && s->enable_halfduplex>0))
	{
		val=1;
	}

	speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_SET_VAD, &val);
	if (s->vad_prob_start>0 && s->vad_prob_continue>0)
	{
		val = s->vad_prob_start; // xx%
		speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_SET_PROB_START, &val);
		val = s->vad_prob_continue; // xx%
		speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &val);
	}

	/* enable AGC only on local soundcard */
	val=0;
	f=12000;
	if (s->agc_level>0 && pos==0)
		val=1;
	else if (pos==0 && s->enable_halfduplex>0)
		val=1;
	else if ( pos%2==1 && s->enable_halfduplex>0)
		val=1; //1; should not do that
	if (s->agc_level>0)
		f=(float)s->agc_level;


	speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_SET_AGC, &val);
	speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_SET_AGC_LEVEL, &f);

	val=s->max_gain;
	if ( pos%2==1 && s->enable_halfduplex>0)
		val=1;

	speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &val);

	val=0;
#if 0
	val=1; // do more testing
#endif
	speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_SET_DEREVERB, &val);
	f=(float).4;
	speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_SET_DEREVERB_DECAY, &f);
	f=(float).3;
	speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, &f);

#endif
}

static void channel_uninit(Channel *chan){
	ms_bufferizer_uninit(&chan->buff);
	chan->is_speaking=0;
	chan->energy=0;
	chan->average_psd=0;
#ifndef DISABLE_SPEEX
	if (chan->speex_pp!=NULL)
	    speex_preprocess_state_destroy(chan->speex_pp);
	chan->speex_pp=NULL;
#endif
}

static void conf_init(MSFilter *f){
	ConfState *s=(ConfState *)ms_new0(ConfState,1);
	int i;
	s->samplerate=8000;
	s->conf_gran=((16 * s->samplerate) / 800) *2;
	s->conf_nsamples=s->conf_gran/2;
    for (i=0;i<CONF_MAX_PINS;i++)
		channel_init(s, &s->channels[i], i);
	s->enable_directmode=FALSE;
	s->enable_vad=TRUE;
	s->agc_level=0;
	s->max_gain=30;
	s->mix_mode=TRUE;
	s->adaptative_msconf_buf=2;
	f->data=s;
}

static void conf_uninit(MSFilter *f){
	ConfState *s=(ConfState*)f->data;
	int i;
	for (i=0;i<CONF_MAX_PINS;i++)
		channel_uninit(&s->channels[i]);
	ms_free(f->data);
}

static void conf_preprocess(MSFilter *f){
	ConfState *s=(ConfState*)f->data;
	int i;
	for (i=0;i<CONF_MAX_PINS;i++)
	  {
	    s->channels[i].is_used=FALSE;
	    s->channels[i].missed=0;
	    s->channels[i].stat_discarded=0;
	    s->channels[i].stat_missed=0;
	    s->channels[i].stat_processed=0;
	  }
}

static bool_t should_process(MSFilter *f, ConfState *s){
	Channel *chan;
	int active_channel=0;
	int i;

	if (ms_bufferizer_get_avail(&(&s->channels[0])->buff)>s->conf_gran
	    && s->channels[0].is_used==FALSE)
	  {
	    /* soundread has just started */
	    s->channels[0].is_used=TRUE;
	  }
	else if (s->channels[0].is_used==FALSE)
	  {
	    return FALSE;
	  }

	/* count active channel */
	for (i=1;i<CONF_MAX_PINS;++i){
		chan=&s->channels[i];
		if (chan->is_used == TRUE)
		{
			active_channel++;
		}
	}

	if (active_channel<=1) /* disable mix mode when it's not needed */
		s->mix_mode = FALSE;
	else
		s->mix_mode = TRUE;

	if (s->enable_directmode==FALSE)
	{
		s->mix_mode = TRUE;
	}

	if (s->mix_mode == FALSE)
		return FALSE;

	if (ms_bufferizer_get_avail(&(&s->channels[0])->buff)>=s->conf_gran)
	{
		return TRUE;
	}
	return FALSE;
}

#ifndef DISABLE_SPEEX
static double powerspectrum_stat_beyond8K(struct Channel *chan){
	spx_int32_t ps_size = 0;
	spx_int32_t *ps = NULL;
	double mystat = 0;
	float fftmul = 1.0 / (32768.0);
	int i;
	
	speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_GET_PSD_SIZE, &ps_size);
	ps = (spx_int32_t*)ortp_malloc(sizeof(spx_int32_t)*ps_size);
	speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_GET_PSD, ps);

	mystat = 0;
	for (i=ps_size/2;i < ps_size; i++) {
		double yp;
#if defined(__cplusplus)
		yp = sqrtf(sqrtf(static_cast<float>(ps[i]))) - 1.0f;
#else
		yp = sqrtf(sqrtf((float)(ps[i]))) - 1.0f;
#endif
		yp = yp * fftmul;
		yp = MIN(yp * 3000.0, 1.0);
		yp = (1 - yp) * (100 - 1.0f);
		mystat = yp + mystat;
	}

	mystat = (mystat*2)/ps_size;

	ortp_free(ps);

	/* values:
	Maximum: 108,064 low volume on high frequency.
	Decrease when volume increase. */

	/* return value between 0 and 108,064? */
	mystat = 108.064-mystat;

	chan->average_psd=(mystat*mystat*coef) + (1.0-coef)*chan->average_psd;
	//ms_message("average power spectrum on half highest values ONLY: stat=%.3lf", chan->average_psd);

	return mystat;
}
#endif

static void conf_sum(MSFilter *f, ConfState *s){
	int i,j;
	Channel *chan;
	memset(s->sum,0,s->conf_nsamples*sizeof(int));

	chan=&s->channels[0];
	if (s->adaptative_msconf_buf*s->conf_gran<ms_bufferizer_get_avail(&chan->buff))
	{
		i = ms_bufferizer_get_avail(&chan->buff)/s->conf_gran;
		if (i>5)
			ms_message("Increasing buffer because sound card is late. (nb_buf=%i /old=%i)", i, s->adaptative_msconf_buf);
		s->adaptative_msconf_buf=i;
		if (s->adaptative_msconf_buf>10)
		{
			while (ms_bufferizer_get_avail(&chan->buff)> s->conf_gran*6)
			{
				ms_bufferizer_read(&chan->buff,(uint8_t*)chan->input,s->conf_gran);
				ms_message("Deleting extra sound card data %i", ms_bufferizer_get_avail(&chan->buff));
			}
		}
	}
	else if (s->adaptative_msconf_buf*s->conf_gran>ms_bufferizer_get_avail(&chan->buff))
	{
		if (s->adaptative_msconf_buf>3)
		{
			s->adaptative_msconf_buf--;
			s->adaptative_msconf_buf=ms_bufferizer_get_avail(&chan->buff)/s->conf_gran;
			//ms_message("decreasing buffer because sound card is in advance. (nb_buf=%i)", s->adaptative_msconf_buf);
		}
	}

	if (s->adaptative_msconf_buf>6)
		s->adaptative_msconf_buf=6;

	for (i=0;i<CONF_MAX_PINS;++i){
		chan=&s->channels[i];

		/* skip soundread and short buffer entry */
		if (i>0 
			&& ms_bufferizer_get_avail(&chan->buff)> s->conf_gran
			&& ms_bufferizer_get_avail(&chan->buff)> (ms_bufferizer_get_avail(&s->channels[0].buff)+s->conf_gran*6) )
		{
#if 0
			int loudness;
#endif
			while (ms_bufferizer_get_avail(&chan->buff)> s->conf_gran
				&& ms_bufferizer_get_avail(&chan->buff)> (ms_bufferizer_get_avail(&s->channels[0].buff)) )
			{
				ms_bufferizer_read(&chan->buff,(uint8_t*)chan->input,s->conf_gran);
#if 0
				speex_preprocess_ctl(chan->speex_pp, SPEEX_PREPROCESS_GET_AGC_LOUDNESS, &loudness);
#endif
				/* we want to remove 4 packets (40ms) in a near future: */
#ifndef DISABLE_SPEEX
				if (chan->speex_pp!=NULL && s->enable_vad==TRUE)
				{
					int vad=0;
					vad = speex_preprocess(chan->speex_pp, (short*)chan->input, NULL);
					if (vad==1)
						break; /* voice detected: process as usual */
					if (ms_bufferizer_get_avail(&chan->buff)<s->conf_gran)
						break; /* no more data to remove */
					ms_message("No voice detected: discarding sample. (idx=%i - bufsize=%i sncardbufsize=%i)",
						i, ms_bufferizer_get_avail(&chan->buff), ms_bufferizer_get_avail(&s->channels[0].buff));
				}
				if (ms_bufferizer_get_avail(&chan->buff) == (ms_bufferizer_get_avail(&s->channels[0].buff)))
					ms_message("same data in soundcard and incoming rtp. (idx=%i - bufsize=%i sncardbufsize=%i)",
						i, ms_bufferizer_get_avail(&chan->buff), ms_bufferizer_get_avail(&s->channels[0].buff));
#endif
				chan->stat_discarded++;
			}

			for(j=0;j<s->conf_nsamples;++j){
				s->sum[j]+=chan->input[j];
			}
			chan->has_contributed=TRUE;

			chan->stat_processed++;
		}
		else if (ms_bufferizer_get_avail(&chan->buff)>=s->conf_gran)
		{
			struct channel_volume {
				float energy;
				int channel;
			};
			struct channel_volume vol;
			float en;

			ms_bufferizer_read(&chan->buff,(uint8_t*)chan->input,s->conf_gran);

			en=chan->energy;
			for(j=0;j<s->conf_nsamples;++j){
				float s=chan->input[j];
				en=(s*s*coef) + ((float)1.0-coef)*en;
			}
			chan->energy=en;
			vol.energy = chan->energy; //10*log10f(chan->energy/max_e);
			vol.channel = i;
			ms_filter_notify(f, MS_CONF_CHANNEL_VOLUME, (void*)&vol);

			if (i>0) /* not for MIC */
			{
				if (chan->energy>65)
					chan->count_speaking++;
				else
					chan->count_speaking=0;
			}

#ifndef DISABLE_SPEEX
			if (chan->speex_pp!=NULL && s->enable_vad==TRUE && i==0)
			{
				int vad;
				vad = speex_preprocess(chan->speex_pp, (short*)chan->input, NULL);
				ms_filter_notify(f, MS_CONF_SPEEX_PREPROCESS_MIC, (void*)chan->speex_pp);

				if (s->enable_halfduplex>0)
				{
					powerspectrum_stat_beyond8K(chan);
					if (chan->average_psd>s->vad_prob_start)
					{
						if (s->channels[0].is_speaking<0)
							ms_message("MIC is turned on");
						//ms_message("is_speaking (chan=%i) -> on/stat=%.3lf", i, chan->average_psd);
						s->channels[0].is_speaking=20; /* keep RTP unmuted for the next few ms */
					}
					else if (chan->average_psd<s->vad_prob_continue)
					{
						if (s->channels[0].is_speaking==0)
						{
							ms_message("MIC is turned off");
							s->channels[0].count_speaking=0;
						}
						s->channels[0].is_speaking--;
					}
					if (s->channels[0].is_speaking>0)
					{
						s->channels[0].count_speaking++;
					}
				}
			}
			else if (chan->speex_pp!=NULL && s->enable_vad==TRUE)
			{
				int vad;

				if (s->enable_halfduplex>0)
				{
					vad = speex_preprocess(chan->speex_pp, (short*)chan->input, NULL);
				}
				else
				{
					vad = speex_preprocess(chan->speex_pp, (short*)chan->input, NULL);
					//speex_preprocess_estimate_update(chan->speex_pp, (short*)chan->input);
				}
			}
#endif

			for(j=0;j<s->conf_nsamples;++j){
				s->sum[j]+=chan->input[j];
			}
			chan->has_contributed=TRUE;

			chan->stat_processed++;
		} else {
			chan->stat_missed++;
			if (i>0 && chan->is_used == TRUE)
			{
				chan->missed++;
				/* delete stream if data is missing since a long time */
				if (chan->missed>15)
				{
					chan->is_used=FALSE;
					ms_message("msconf: deleted contributing stream (pin=%i)", i);
				}
				/* couldn't we add confort noise for those outputs? */
			}
			chan->has_contributed=FALSE;
		}
	}
	return;
}

static inline int16_t saturate(int sample){
	if (sample>32000)
		sample=32000;
	else if (sample<-32000)
		sample=-32000;
	return (int16_t)sample;
}

static mblk_t * conf_output(ConfState *s, Channel *chan, int16_t attenuation){
	mblk_t *m=allocb(s->conf_gran,0);
	int i;
	int tmp;
	if (chan->has_contributed==TRUE){
		for (i=0;i<s->conf_nsamples;++i){
			tmp=s->sum[i]-(int)chan->input[i];
			*((int16_t*)m->b_wptr)=saturate(tmp)/attenuation;
			m->b_wptr+=2;
		}
	}else{
		for (i=0;i<s->conf_nsamples;++i){
			tmp=s->sum[i];
			*((int16_t*)m->b_wptr)=saturate(tmp)/attenuation;
			m->b_wptr+=2;
		}
	}
	return m;
}

static void conf_dispatch(MSFilter *f, ConfState *s){
	int i;
	Channel *chan;
	mblk_t *m;

	for (i=0;i<CONF_MAX_PINS;++i){
		if (f->outputs[i]!=NULL){
			chan=&s->channels[i];
			if (s->channels[0].is_speaking>0 // if MIC is speaking 
				&& s->channels[0].count_speaking<75 // if MIC was silence a few time ago
				&& chan->count_speaking>0 // if RTP has started to talk
				&& chan->count_speaking<75 // if RTP was silence a few ms ago
				&& i%2==1) // false detection of MIC speaking
			{
				ms_message("false detection of MIC speaking: turned off");
				m=conf_output(s,chan, 32000);
			}
			else if (s->channels[0].is_speaking<0 && i%2==1) // MIC is NOT speaking -> send silence on RTP
				m=conf_output(s,chan, 32000);
			else
				m=conf_output(s,chan, 1);
			ms_queue_put(f->outputs[i],m);
		}
	}
}

static void conf_process(MSFilter *f){
	int i;
	ConfState *s=(ConfState*)f->data;
	Channel *chan;
	Channel *chan0;
	/*read from all inputs and put into bufferizers*/
	for (i=0;i<CONF_MAX_PINS;++i){
		if (f->inputs[i]!=NULL){
			chan=&s->channels[i];
			ms_bufferizer_put_from_queue(&chan->buff,f->inputs[i]);
			if (ms_bufferizer_get_avail(&chan->buff)>0)
			{
				chan->missed=0; /* reset counter of missed packet */
				if (i>0 && chan->is_used==FALSE)
				{
					chan->is_used=TRUE;
					ms_message("msconf: new contributing stream (chan=%i) %i", ms_bufferizer_get_avail(&chan->buff), i);
				}
			}
		}
	}

	/*do the job */
	while(should_process(f,s)==TRUE){
		conf_sum(f, s);
		conf_dispatch(f,s);
	}

	/* mixer is disabled! -> copy A->B and B->A*/
	if (s->mix_mode == FALSE)
	{
		/* get the soundread data and copy it to pinX */
		for (i=1;i<CONF_MAX_PINS;i=i+2){
			if (f->inputs[i]!=NULL){
				chan0=&s->channels[0];
				chan=&s->channels[i];
				if (chan->is_used==TRUE)
				{
					while (ms_bufferizer_read(&chan->buff,(uint8_t*)chan->input,s->conf_gran)==s->conf_gran)
					{
						if (f->outputs[0]!=NULL)
						{
							/* send in pin0 */
							mblk_t *m=allocb(s->conf_gran,0);
							memcpy(m->b_wptr, chan->input, s->conf_gran);
							m->b_wptr+=s->conf_gran;
							ms_queue_put(f->outputs[0],m);
						}
					}
				}

				if (chan0->is_used==TRUE)
				{
					while (ms_bufferizer_read(&chan0->buff,(uint8_t*)chan0->input,s->conf_gran)==s->conf_gran)
					{
						if (f->outputs[i]!=NULL)
						{
							/* send in pinI */
							mblk_t *m=allocb(s->conf_gran,0);
							memcpy(m->b_wptr, chan0->input, s->conf_gran);
							m->b_wptr+=s->conf_gran;
							ms_queue_put(f->outputs[i],m);
						}
					}
				}
				break;
			}
		}
	}

}

static void conf_postprocess(MSFilter *f){
	int i;
	ConfState *s=(ConfState*)f->data;
	for (i=0;i<CONF_MAX_PINS;i++)
		channel_uninit(&s->channels[i]);
    for (i=0;i<CONF_MAX_PINS;i++)
		channel_init(s, &s->channels[i], i);
}

static int msconf_set_sr(MSFilter *f, void *arg){
	ConfState *s=(ConfState*)f->data;
	int i;

	s->samplerate = *(int*)arg;
	s->conf_gran = ((16 * s->samplerate) / 800) *2;
	s->conf_nsamples=s->conf_gran/2;
	for (i=0;i<CONF_MAX_PINS;i++)
		channel_uninit(&s->channels[i]);
    for (i=0;i<CONF_MAX_PINS;i++)
		channel_init(s, &s->channels[i], i);
	return 0;
}

static int msconf_enable_directmode(MSFilter *f, void *arg){
	ConfState *s=(ConfState*)f->data;
	s->enable_directmode = *(int*)arg;
	return 0;
}

static int msconf_enable_agc(MSFilter *f, void *arg){
	ConfState *s=(ConfState*)f->data;
	int i;
	s->agc_level = *(int*)arg;

	for (i=0;i<CONF_MAX_PINS;i++)
		channel_uninit(&s->channels[i]);
    for (i=0;i<CONF_MAX_PINS;i++)
		channel_init(s, &s->channels[i], i);
	return 0;
}

static int msconf_set_max_gain(MSFilter *f, void *arg){
	ConfState *s=(ConfState*)f->data;
	int i;
	s->max_gain = *(int*)arg;

	for (i=0;i<CONF_MAX_PINS;i++)
		channel_uninit(&s->channels[i]);
    for (i=0;i<CONF_MAX_PINS;i++)
		channel_init(s, &s->channels[i], i);
	return 0;
}


static int msconf_enable_halfduplex(MSFilter *f, void *arg){
	ConfState *s=(ConfState*)f->data;
	int i;
	s->enable_halfduplex = *(int*)arg;

	for (i=0;i<CONF_MAX_PINS;i++)
		channel_uninit(&s->channels[i]);
    for (i=0;i<CONF_MAX_PINS;i++)
		channel_init(s, &s->channels[i], i);
	return 0;
}

static int msconf_set_vad_prob_start(MSFilter *f, void *arg){
	ConfState *s=(ConfState*)f->data;
	int i;
	s->vad_prob_start = *(int*)arg;

	for (i=0;i<CONF_MAX_PINS;i++)
		channel_uninit(&s->channels[i]);
    for (i=0;i<CONF_MAX_PINS;i++)
		channel_init(s, &s->channels[i], i);
	return 0;
}

static int msconf_set_vad_prob_continue(MSFilter *f, void *arg){
	ConfState *s=(ConfState*)f->data;
	int i;
	s->vad_prob_continue = *(int*)arg;

	for (i=0;i<CONF_MAX_PINS;i++)
		channel_uninit(&s->channels[i]);
    for (i=0;i<CONF_MAX_PINS;i++)
		channel_init(s, &s->channels[i], i);
	return 0;
}

static int msconf_enable_vad(MSFilter *f, void *arg){
	ConfState *s=(ConfState*)f->data;
	int i;
	s->enable_vad = *(int*)arg;

	for (i=0;i<CONF_MAX_PINS;i++)
		channel_uninit(&s->channels[i]);
    for (i=0;i<CONF_MAX_PINS;i++)
		channel_init(s, &s->channels[i], i);
	return 0;
}

static int msconf_get_stat_discarded(MSFilter *f, void *arg){
	ConfState *s=(ConfState*)f->data;
	Channel *chan;
	int i;
	i = *(int*)arg;
	/*read from all inputs and put into bufferizers*/
	if (i<0 || i>CONF_MAX_PINS)
	  return -1;

	if (f->inputs[i]!=NULL){
		chan=&s->channels[i];
		return chan->stat_discarded;
	}
	return -1;
}

static int msconf_get_stat_missed(MSFilter *f, void *arg){
	ConfState *s=(ConfState*)f->data;
	Channel *chan;
	int i;
	i = *(int*)arg;
	/*read from all inputs and put into bufferizers*/
	if (i<0 || i>CONF_MAX_PINS)
	  return -1;

	if (f->inputs[i]!=NULL){
		chan=&s->channels[i];
		return chan->stat_missed;
	}
	return -1;
}

static int msconf_get_stat_processed(MSFilter *f, void *arg){
	ConfState *s=(ConfState*)f->data;
	Channel *chan;
	int i;
	i = *(int*)arg;
	/*read from all inputs and put into bufferizers*/
	if (i<0 || i>CONF_MAX_PINS)
	  return -1;

	if (f->inputs[i]!=NULL){
		chan=&s->channels[i];
		return chan->stat_processed;
	}
	return -1;
}

static MSFilterMethod msconf_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE, msconf_set_sr },
	{	MS_FILTER_ENABLE_DIRECTMODE, msconf_enable_directmode },
	{	MS_FILTER_ENABLE_VAD, msconf_enable_vad },
	{	MS_FILTER_ENABLE_AGC, msconf_enable_agc },
	{	MS_FILTER_SET_MAX_GAIN, msconf_set_max_gain },
	
	{	MS_FILTER_GET_STAT_DISCARDED, msconf_get_stat_discarded },
	{	MS_FILTER_GET_STAT_MISSED, msconf_get_stat_missed },
	{	MS_FILTER_GET_STAT_OUTPUT, msconf_get_stat_processed },

	{	MS_FILTER_ENABLE_HALFDUPLEX, msconf_enable_halfduplex },
	{	MS_FILTER_SET_VAD_PROB_START, msconf_set_vad_prob_start },
	{	MS_FILTER_SET_VAD_PROB_CONTINUE, msconf_set_vad_prob_continue },
	{	0			, NULL}
};

#ifdef _MSC_VER

MSFilterDesc ms_conf_desc={
	MS_CONF_ID,
	"MSConf",
	N_("A filter to make conferencing"),
	MS_FILTER_OTHER,
	NULL,
	CONF_MAX_PINS,
	CONF_MAX_PINS,
	conf_init,
	conf_preprocess,
	conf_process,
	conf_postprocess,
	conf_uninit,
	msconf_methods
};

#else

MSFilterDesc ms_conf_desc={
	.id=MS_CONF_ID,
	.name="MSConf",
	.text=N_("A filter to make conferencing"),
	.category=MS_FILTER_OTHER,
	.ninputs=CONF_MAX_PINS,
	.noutputs=CONF_MAX_PINS,
	.init=conf_init,
	.preprocess=conf_preprocess,
	.process=conf_process,
	.postprocess=conf_postprocess,
	.uninit=conf_uninit,
	.methods=msconf_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_conf_desc)

