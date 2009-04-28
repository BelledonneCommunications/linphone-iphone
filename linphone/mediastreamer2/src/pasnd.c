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

#include <speex/speex_preprocess.h>

#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msfilter.h"

#include "portaudio.h"

MSFilter *ms_pasnd_read_new(MSSndCard *card);
MSFilter *ms_pasnd_write_new(MSSndCard *card);

typedef struct PASndData{
  char *pcmdev;
  char *mixdev;
  int sound_err;
  char waveoutbuffer[30][3200];
  PaStream   *waveoutdev;
  
  PaStream   *waveindev;
  
  int rate;
  int bits;
  ms_thread_t thread;
  ms_mutex_t mutex;
  queue_t rq;
  MSBufferizer * bufferizer;
  bool_t read_started;
  bool_t write_started;
  bool_t stereo;

  SpeexPreprocessState *pst;
} PASndData;

int SpeakerCallback(  const void *inputBuffer, void *outputBuffer,
		      unsigned long framesPerBuffer,
		      const PaStreamCallbackTimeInfo* timeInfo,
		      PaStreamCallbackFlags statusFlags,
		      void *userData )
{
  PASndData *device = (PASndData*)userData;
  uint8_t *wtmpbuff=NULL;
  int err;
  int ovfl = (device->rate/8000)*320*6;

  memset(outputBuffer,0, framesPerBuffer*2);
  if (!device->read_started && !device->write_started)
    {
      return 0;
    }

  wtmpbuff=(uint8_t*)alloca(framesPerBuffer*2);

  memset(outputBuffer,0, framesPerBuffer*2);

  ms_mutex_lock(&device->mutex);

  /* remove extra buffer when latency is increasing:
     this often happen with USB device */
  if (device->bufferizer->size>=ovfl){
    ms_warning("Extra data for sound card (total:%i %ims)",
	       device->bufferizer->size, (device->bufferizer->size*20)/320);
    err=ms_bufferizer_read(device->bufferizer,wtmpbuff, framesPerBuffer*2);
    err=ms_bufferizer_read(device->bufferizer,wtmpbuff, framesPerBuffer*2);
    err=ms_bufferizer_read(device->bufferizer,wtmpbuff, framesPerBuffer*2);
    err=ms_bufferizer_read(device->bufferizer,wtmpbuff, framesPerBuffer*2);
    err=ms_bufferizer_read(device->bufferizer,wtmpbuff, framesPerBuffer*2);
    ms_warning("Extra data for sound card removed (total:%i %ims)",
	       device->bufferizer->size, (device->bufferizer->size*20)/320);
  }

  err=ms_bufferizer_read(device->bufferizer,wtmpbuff,framesPerBuffer*2);
  ms_mutex_unlock(&device->mutex);
  if (err==framesPerBuffer*2)
    {
      memcpy (outputBuffer, wtmpbuff, framesPerBuffer*2);
    }

  return 0;
}

int WaveInCallback(  const void *inputBuffer, void *outputBuffer,
                     unsigned long framesPerBuffer,
		     const PaStreamCallbackTimeInfo* timeInfo,
		     PaStreamCallbackFlags statusFlags,
		     void *userData )
{
  PASndData *device = (PASndData*)userData;

  if (!device->read_started && !device->write_started)
    {
      return 0;
    }

  ms_mutex_lock(&device->mutex);
  if (device->read_started)
    {
      int vad;
      mblk_t *rm=NULL;
      if (rm==NULL) rm=allocb(framesPerBuffer*2,0);
      memcpy(rm->b_wptr,inputBuffer, framesPerBuffer*2);
      
      if (device->pst!=NULL)
	{
	  vad = speex_preprocess(device->pst, (spx_int16_t *)rm->b_wptr, NULL);
#if 0
	  if (vad!=1)
	    ms_message("WaveInCallback : %d", vad);
#endif
	}
      
      rm->b_wptr+=framesPerBuffer*2;
      
      putq(&device->rq,rm);
      rm=NULL;
    }
  ms_mutex_unlock(&device->mutex);

  return 0;
}

static int pasnd_open(PASndData *device, int devnumber, int bits,int stereo, int rate, int *minsz)
{
    PaStreamParameters outputParameters;
    PaStreamParameters inputParameters;
    PaError err;

    const PaHostApiInfo *pa_hai = Pa_GetHostApiInfo(Pa_GetDefaultHostApi());
    
    ms_warning("pasnd_open : opening default input device: name=%s (%i)",
	       pa_hai->name, pa_hai->defaultInputDevice);
    ms_warning("pasnd_open : opening default output device name=%s (%i)",
	       pa_hai->name, pa_hai->defaultOutputDevice);

    outputParameters.device = devnumber; /* default output device */
    outputParameters.device = pa_hai->defaultOutputDevice;
    outputParameters.channelCount = 1;       /* stereo output */
    outputParameters.sampleFormat = paInt16; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
			&device->waveoutdev,	/* stream */
			NULL,                   /* no input */
			&outputParameters,	//
			rate,			// double sampleRate
			160*(rate/8000),	//unsigned long framesPerBuffer
			paClipOff,
			SpeakerCallback,	//PortAudioCallback *callback
			(void *) device);	//void *userData

    if (err != paNoError)
    {
        ms_warning("Failed to open out device. (Pa_OpenDefaultStream:0x%i)", err);
        return -1;
    }

    inputParameters.device = devnumber; /* default input device */
    inputParameters.device = pa_hai->defaultInputDevice;
    inputParameters.channelCount = 1;       /* stereo input */
    inputParameters.sampleFormat = paInt16; /* 32 bit floating point input */
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
			&device->waveindev,	//PortAudioStream** stream
			&inputParameters,	/* input param*/
			NULL,                   /* output param */
			rate,			// double sampleRate
			160*(rate/8000),	//unsigned long framesPerBuffer
			paClipOff,
			WaveInCallback,		//PortAudioCallback *callback
			(void *) device);	//void *userData

	
    if (err != paNoError)
    {
        ms_warning("Failed to open in device. (Pa_OpenDefaultStream:0x%i)", err);
        return -1;
    }

    err = Pa_StartStream( device->waveoutdev );
    if( err != paNoError )
      {
        ms_warning("Failed to start out device. (Pa_StartStream:0x%i)", err);
        return -1;
      }

	device->pst = speex_preprocess_state_init((device->rate/8000 * 320)/2, device->rate);
	if (device->pst!=NULL) {
		float f;
		int i=1;
		speex_preprocess_ctl(device->pst, SPEEX_PREPROCESS_SET_VAD, &i);
		i=1;
		speex_preprocess_ctl(device->pst, SPEEX_PREPROCESS_SET_DENOISE, &i);
		i=0;
		speex_preprocess_ctl(device->pst, SPEEX_PREPROCESS_SET_AGC, &i);
		f=8000;
		speex_preprocess_ctl(device->pst, SPEEX_PREPROCESS_SET_AGC_LEVEL, &f);
		i=0;
		speex_preprocess_ctl(device->pst, SPEEX_PREPROCESS_SET_DEREVERB, &i);
		f=.4;
		speex_preprocess_ctl(device->pst, SPEEX_PREPROCESS_SET_DEREVERB_DECAY, &f);
		f=.3;
		speex_preprocess_ctl(device->pst, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, &f);
	}

    err = Pa_StartStream( device->waveindev );
    if( err != paNoError )
      {
        ms_warning("Failed to start in device: trying default device. (Pa_StartStream:0x%i)", err);
        return -1;
      }

	*minsz=device->rate/8000 * 320;
	return 0;
}

static void pasnd_set_level(MSSndCard *card, MSSndCardMixerElem e, int percent)
{
	PASndData *d=(PASndData*)card->data;

	if (d->mixdev==NULL) return;
	switch(e){
	case MS_SND_CARD_MASTER:
	  return;
	  break;
        case MS_SND_CARD_CAPTURE:
	  break;
	case MS_SND_CARD_PLAYBACK:
	  break;
        default:
	  ms_warning("pasnd_card_set_level: unsupported command.");
	  return;
	}
}

static int pasnd_get_level(MSSndCard *card, MSSndCardMixerElem e)
{
	PASndData *d=(PASndData*)card->data;

	if (d->mixdev==NULL) return -1;
	switch(e){
	case MS_SND_CARD_MASTER:
	  return 60;
	  break;
        case MS_SND_CARD_CAPTURE:
	  break;
	case MS_SND_CARD_PLAYBACK:
	  break;
	default:
	  ms_warning("pasnd_card_get_level: unsupported command.");
	  return -1;
	}
	return -1;
}

static void pasnd_set_source(MSSndCard *card, MSSndCardCapture source)
{
	PASndData *d=(PASndData*)card->data;
	if (d->mixdev==NULL) return;

	switch(source){
		case MS_SND_CARD_MIC:
		break;
		case MS_SND_CARD_LINE:
		break;
	}	
}

static void pasnd_init(MSSndCard *card){
	PASndData *d=ms_new(PASndData,1);
	memset(d, 0, sizeof(PASndData));
	d->pcmdev=NULL;
	d->mixdev=NULL;
	d->sound_err=-1; /* not opened */
	d->read_started=FALSE;
	d->write_started=FALSE;
	d->bits=16;
	d->rate=8000;
	d->stereo=FALSE;
	qinit(&d->rq);
	d->bufferizer=ms_bufferizer_new();
	ms_mutex_init(&d->mutex,NULL);
	card->data=d;
	d->pst=0;
}

static void pasnd_uninit(MSSndCard *card){
	PASndData *d=(PASndData*)card->data;
	if (d==NULL)
		return;
	if (d->pcmdev!=NULL) ms_free(d->pcmdev);
	if (d->mixdev!=NULL) ms_free(d->mixdev);
	ms_bufferizer_destroy(d->bufferizer);
	flushq(&d->rq,0);

	ms_mutex_destroy(&d->mutex);

	if (d->pst!=NULL)
	    speex_preprocess_state_destroy(d->pst);

	ms_free(d);
}

#define DSP_NAME "/dev/dsp"
#define MIXER_NAME "/dev/mixer"

static void pasnd_detect(MSSndCardManager *m);
static MSSndCard *pasnd_duplicate(MSSndCard *obj);

MSSndCardDesc pasnd_card_desc={
	"PASND",
	pasnd_detect,
	pasnd_init,
	pasnd_set_level,
	pasnd_get_level,
	pasnd_set_source,
	NULL,
	NULL,
	ms_pasnd_read_new,
	ms_pasnd_write_new,
	pasnd_uninit,
	pasnd_duplicate
};

static MSSndCard *pasnd_duplicate(MSSndCard *obj){
	MSSndCard *card=ms_snd_card_new(&pasnd_card_desc);
	PASndData *dcard=(PASndData*)card->data;
	PASndData *dobj=(PASndData*)obj->data;
	dcard->pcmdev=ms_strdup(dobj->pcmdev);
	dcard->mixdev=ms_strdup(dobj->mixdev);
	card->name=ms_strdup(obj->name);
	return card;
}

static MSSndCard *pasnd_card_new(const char *pcmdev, const char *mixdev){
	MSSndCard *card=ms_snd_card_new(&pasnd_card_desc);
	PASndData *d=(PASndData*)card->data;
	d->pcmdev=ms_strdup(pcmdev);
	d->mixdev=ms_strdup(mixdev);
	card->name=ms_strdup(pcmdev);
	return card;
}

static void pasnd_detect(MSSndCardManager *m){
    int err = 0;
    unsigned int numDevices;
    const PaDeviceInfo *pdi;
    char pcmdev[1024];
    char mixdev[1024];
    int i;

    err = Pa_Initialize();
    if( err != paNoError )
      {
	ms_warning("PortAudio error: %s\n", Pa_GetErrorText( err ) );
	return;
      }

    numDevices = Pa_GetDeviceCount();

    for( i=0; i<numDevices; i++ ) {
      pdi = Pa_GetDeviceInfo( i );
      if (pdi!=NULL)
	{
	  MSSndCard *card;
	  snprintf(pcmdev,sizeof(pcmdev),"%s",pdi->name);
	  snprintf(mixdev,sizeof(mixdev),"%s",pdi->name);
	  if (i == 0)
            {
	      card=pasnd_card_new(pcmdev,mixdev);
	      ms_snd_card_manager_add_card(m,card);
            }
	  card=pasnd_card_new(pcmdev,mixdev);
	  ms_snd_card_manager_add_card(m,card);
	}
    }
}

static void pasnd_closedriver(PASndData *d)
{
	if (d->sound_err==0) {

	  int err = Pa_StopStream( d->waveindev );
	  if( err != paNoError )
	    {
	      ms_warning("Failed to stop device. (Pa_StopStream:0x%i)", err);
	    }
	  
	  err = Pa_CloseStream( d->waveindev);
	  if( err != paNoError )
	    {
	      ms_warning("failed to close recording sound card (Pa_CloseStream:0x%i)", err);
	    }
	  else
	    {
	      ms_message("successfully closed recording sound card");
	    }
	  
	  err = Pa_StopStream( d->waveoutdev );
	  if( err != paNoError )
	    {
	      ms_warning("Failed to stop device. (Pa_StopStream:0x%i)", err);
	    }
	  
	  err = Pa_CloseStream( d->waveoutdev );
	  if( err != paNoError ) 
	    {
	      ms_error("failed to stop recording sound card (Pa_CloseStream:0x%i)", err);
	    }
	  else
	    {
	      ms_message("successfully stopped recording sound card");
	    }
	  
	  
	  d->sound_err=-1;
	}
}

static void pasnd_start_r(MSSndCard *card){
	PASndData *d=(PASndData*)card->data;
	if (d->read_started==FALSE && d->write_started==FALSE){
		int bsize=0;
		d->read_started=TRUE;
		d->sound_err=pasnd_open(d, 0, d->bits,d->stereo,d->rate,&bsize);
	}else d->read_started=TRUE;
}

static void pasnd_stop_r(MSSndCard *card){
	PASndData *d=(PASndData*)card->data;
	d->read_started=FALSE;
	if (d->write_started==FALSE){
	  /* ms_thread_join(d->thread,NULL); */
	  pasnd_closedriver(d);
	}
}

static void pasnd_start_w(MSSndCard *card){
	PASndData *d=(PASndData*)card->data;
	if (d->read_started==FALSE && d->write_started==FALSE){
		int bsize=0;
		d->write_started=TRUE;
		d->sound_err=pasnd_open(d, 0, d->bits,d->stereo,d->rate,&bsize);
	}else{
		d->write_started=TRUE;
	}
}

static void pasnd_stop_w(MSSndCard *card){
	PASndData *d=(PASndData*)card->data;
	d->write_started=FALSE;
	if (d->read_started==FALSE){
	  /* ms_thread_join(d->thread,NULL); */
	  pasnd_closedriver(d);
	}
}

static mblk_t *pasnd_get(MSSndCard *card){
	PASndData *d=(PASndData*)card->data;
	mblk_t *m;
	ms_mutex_lock(&d->mutex);
	m=getq(&d->rq);
	ms_mutex_unlock(&d->mutex);
	return m;
}

static void pasnd_put(MSSndCard *card, mblk_t *m){
	PASndData *d=(PASndData*)card->data;
	ms_mutex_lock(&d->mutex);
	ms_bufferizer_put(d->bufferizer,m);
	ms_mutex_unlock(&d->mutex);
}


static void pasnd_read_preprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	pasnd_start_r(card);
}

static void pasnd_read_postprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	pasnd_stop_r(card);
}

static void pasnd_read_process(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	mblk_t *m;
	while((m=pasnd_get(card))!=NULL){
		ms_queue_put(f->outputs[0],m);
	}
}

static void pasnd_write_preprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	pasnd_start_w(card);
}

static void pasnd_write_postprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	pasnd_stop_w(card);
}

static void pasnd_write_process(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	mblk_t *m;
	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		pasnd_put(card,m);
	}
}

static int set_rate(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	PASndData *d=(PASndData*)card->data;
	d->rate=*((int*)arg);
	return 0;
}

static int set_nchannels(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	PASndData *d=(PASndData*)card->data;
	d->stereo=(*((int*)arg)==2);
	return 0;
}

static MSFilterMethod pasnd_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE	, set_rate	},
	{	MS_FILTER_SET_NCHANNELS		, set_nchannels	},
	{	0				, NULL		}
};

MSFilterDesc pasnd_read_desc={
	MS_PASND_READ_ID,
	"MSPasndRead",
	"Sound capture filter for Port Audio Sound drivers",
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	NULL,
	pasnd_read_preprocess,
	pasnd_read_process,
	pasnd_read_postprocess,
	NULL,
	pasnd_methods
};


MSFilterDesc pasnd_write_desc={
	MS_PASND_WRITE_ID,
	"MSPasndWrite",
	"Sound playback filter for Port Audio Sound drivers",
	MS_FILTER_OTHER,
	NULL,
	1,
	0,
	NULL,
	pasnd_write_preprocess,
	pasnd_write_process,
	pasnd_write_postprocess,
	NULL,
	pasnd_methods
};

MSFilter *ms_pasnd_read_new(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&pasnd_read_desc);
	f->data=card;
	return f;
}


MSFilter *ms_pasnd_write_new(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&pasnd_write_desc);
	f->data=card;
	return f;
}

MS_FILTER_DESC_EXPORT(pasnd_read_desc)
MS_FILTER_DESC_EXPORT(pasnd_write_desc)
