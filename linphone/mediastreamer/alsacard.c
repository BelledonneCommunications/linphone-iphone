/*
  The mediastreamer library aims at providing modular media processing and I/O
	for linphone, but also for any telephony application.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org
  										
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "alsacard.h"

#ifdef HAVE_ALSA_ASOUNDLIB_H

static gchar *over_pcmdev=NULL;

#include "msossread.h"
#include "msosswrite.h"

#include <signal.h>

int __alsa_card_write(AlsaCard *obj,char *buf,int size);

int alsa_set_params(AlsaCard *obj, int rw, int bits, int stereo, int rate)
{
	snd_pcm_hw_params_t *hwparams=NULL;
	snd_pcm_sw_params_t *swparams=NULL;
	snd_pcm_t *pcm_handle;
	gint dir;
	guint exact_uvalue;
	gulong exact_ulvalue;
	gint channels;
//	gint fsize=0;
	gint periods=8;
	gint periodsize=256;
	gint err;
	int format;
	
	if (rw) {
		pcm_handle=obj->write_handle;
	}
	else pcm_handle=obj->read_handle;
	
	/* Allocate the snd_pcm_hw_params_t structure on the stack. */
    snd_pcm_hw_params_alloca(&hwparams);
	
	/* Init hwparams with full configuration space */
    if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
		g_warning("alsa_set_params: Cannot configure this PCM device.\n");
		return(-1);
    }
	
	if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
      g_warning("alsa_set_params: Error setting access.\n");
      return(-1);
    }
	/* Set sample format */
#ifdef WORDS_BIGENDIAN
	format=SND_PCM_FORMAT_S16_BE;
#else
	format=SND_PCM_FORMAT_S16_LE;
#endif
    if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, format) < 0) {
      g_warning("alsa_set_params: Error setting format.\n");
      return(-1);
    }
	/* Set number of channels */
	if (stereo) channels=2;
	else channels=1;
    if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, channels) < 0) {
      g_warning("alsa_set_params: Error setting channels.\n");
      return(-1);
    }
	/* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.         */ 
	exact_uvalue=rate;
	dir=0;
    if ((err=snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &exact_uvalue, &dir))<0){
		g_warning("alsa_set_params: Error setting rate to %i:%s",rate,snd_strerror(err));
 		return -1;
	}
    if (dir != 0) {
      g_warning("alsa_set_params: The rate %d Hz is not supported by your hardware.\n "
		"==> Using %d Hz instead.\n", rate, exact_uvalue);
    }
	/* choose greater period size when rate is high */
	periodsize=periodsize*(rate/8000);	
	
	/* Set buffer size (in frames). The resulting latency is given by */
    /* latency = periodsize * periods / (rate * bytes_per_frame)     */
	/*
	fsize=periodsize * periods;
	exact_value=fsize;
    if ((err=snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams,&exact_value)) < 0) {
      g_warning("alsa_set_params: Error setting buffer size:%s",snd_strerror(err));
      return(-1);
    }
	if (fsize!= exact_value) {
      g_warning("alsa_set_params: The buffer size %d is not supported by your hardware.\n "
		"==> Using %d instead.\n", fsize, exact_value);
    }
	*/
	/* set period size */
	exact_ulvalue=periodsize;
	dir=0;
    if (snd_pcm_hw_params_set_period_size_near(pcm_handle, hwparams, &exact_ulvalue, &dir) < 0) {
      g_warning("alsa_set_params: Error setting period size.\n");
      return(-1);
    }
	if (dir != 0) {
      g_warning("alsa_set_params: The period size %d is not supported by your hardware.\n "
		"==> Using %d instead.\n", periodsize, (int)exact_ulvalue);
    }
	periodsize=exact_ulvalue;
	/* Set number of periods. Periods used to be called fragments. */ 
	exact_uvalue=periods;
	dir=0;
    if (snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &exact_uvalue, &dir) < 0) {
      g_warning("alsa_set_params: Error setting periods.\n");
      return(-1);
    }
	if (dir != 0) {
      g_warning("alsa_set_params: The number of periods %d is not supported by your hardware.\n "
		"==> Using %d instead.\n", periods, exact_uvalue);
    }
	/* Apply HW parameter settings to */
    /* PCM device and prepare device  */
    if ((err=snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
      g_warning("alsa_set_params: Error setting HW params:%s",snd_strerror(err));
      return(-1);
    }
	/*prepare sw params */
	if (rw){
		snd_pcm_sw_params_alloca(&swparams);
		snd_pcm_sw_params_current(pcm_handle, swparams);
		if ((err=snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams,periodsize*2 ))<0){
			g_warning("alsa_set_params: Error setting start threshold:%s",snd_strerror(err));
			return -1;
		}
		if ((err=snd_pcm_sw_params(pcm_handle, swparams))<0){
			g_warning("alsa_set_params: Error setting SW params:%s",snd_strerror(err));
			return(-1);
		}
	}
	obj->frame_size=channels*(bits/8);
	SND_CARD(obj)->bsize=periodsize*obj->frame_size;
	//SND_CARD(obj)->bsize=4096;
	obj->frames=periodsize;
	g_message("alsa_set_params:  blocksize=%i.",SND_CARD(obj)->bsize);
	return SND_CARD(obj)->bsize;	
}

int alsa_card_open_r(AlsaCard *obj,int bits,int stereo,int rate)
{
	int bsize;
	int err;
	snd_pcm_t *pcm_handle;
	gchar *pcmdev;
	if (over_pcmdev!=NULL) pcmdev=over_pcmdev;
	else pcmdev=obj->pcmdev;
	
	if (snd_pcm_open(&pcm_handle, pcmdev,SND_PCM_STREAM_CAPTURE,SND_PCM_NONBLOCK) < 0) {
		g_warning("alsa_card_open_r: Error opening PCM device %s\n",obj->pcmdev );
		return -1;
	}
	g_return_val_if_fail(pcm_handle!=NULL,-1);
	obj->read_handle=pcm_handle;
	if ((bsize=alsa_set_params(obj,0,bits,stereo,rate))<0){
		snd_pcm_close(pcm_handle);
		obj->read_handle=NULL;
		return -1;
	}
	obj->readbuf=g_malloc0(bsize);
	
	err=snd_pcm_start(obj->read_handle);
	if (err<0){
		g_warning("Cannot start read pcm: %s", snd_strerror(err));
	}
	obj->readpos=0;
	SND_CARD(obj)->bsize=bsize;
	SND_CARD(obj)->flags|=SND_CARD_FLAGS_OPENED;
	return 0;
}

int alsa_card_open_w(AlsaCard *obj,int bits,int stereo,int rate)
{
//	int err;
	int bsize;
	snd_pcm_t *pcm_handle;
	gchar *pcmdev;
	if (over_pcmdev!=NULL) pcmdev=over_pcmdev;
	else pcmdev=obj->pcmdev;
	
	if (snd_pcm_open(&pcm_handle, pcmdev,SND_PCM_STREAM_PLAYBACK,SND_PCM_NONBLOCK) < 0) {
      g_warning("alsa_card_open_w: Error opening PCM device %s\n", obj->pcmdev);
      return -1;
    }
	obj->write_handle=pcm_handle;
	if ((bsize=alsa_set_params(obj,1,bits,stereo,rate))<0){
		snd_pcm_close(pcm_handle);
		obj->write_handle=NULL;
		return -1;
	}
	obj->writebuf=g_malloc0(bsize);
	
	obj->writepos=0;
	SND_CARD(obj)->bsize=bsize;
	SND_CARD(obj)->flags|=SND_CARD_FLAGS_OPENED;
	return 0;
}


void alsa_card_set_blocking_mode(AlsaCard *obj, gboolean yesno){
	if (obj->read_handle!=NULL) snd_pcm_nonblock(obj->read_handle,!yesno);
	if (obj->write_handle!=NULL) snd_pcm_nonblock(obj->write_handle,!yesno);
}

void alsa_card_close_r(AlsaCard *obj)
{
	if (obj->read_handle!=NULL){
		snd_pcm_close(obj->read_handle);
		obj->read_handle=NULL;
		g_free(obj->readbuf);
		obj->readbuf=NULL;
	}
}

void alsa_card_close_w(AlsaCard *obj)
{
	if (obj->write_handle!=NULL){
		snd_pcm_close(obj->write_handle);
		obj->write_handle=NULL;
		g_free(obj->writebuf);
		obj->writebuf=NULL;
	}
}

int alsa_card_probe(AlsaCard *obj,int bits,int stereo,int rate)
{
	int ret;
	ret=alsa_card_open_w(obj,bits,stereo,rate);
	if (ret<0) return -1;
	ret=SND_CARD(obj)->bsize;
	alsa_card_close_w(obj);
	return ret;
}


void alsa_card_destroy(AlsaCard *obj)
{
	snd_card_uninit(SND_CARD(obj));
	g_free(obj->pcmdev);
	if (obj->readbuf!=0) g_free(obj->readbuf);
	if (obj->writebuf!=0) g_free(obj->writebuf);	
}

gboolean alsa_card_can_read(AlsaCard *obj)
{
	int frames;
	g_return_val_if_fail(obj->read_handle!=NULL,0);
	if (obj->readpos!=0) return TRUE;
	if ((frames=snd_pcm_avail_update(obj->read_handle)>=obj->frames)) return 1;
	//g_message("frames=%i",frames);
	return 0;
}



int __alsa_card_read(AlsaCard *obj,char *buf,int bsize)
{
	int err;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set,SIGALRM);
	sigprocmask(SIG_BLOCK,&set,NULL);
	err=snd_pcm_readi(obj->read_handle,buf,bsize/obj->frame_size);
	if (err<0) {
		if (err!=-EPIPE){
			g_warning("alsa_card_read: snd_pcm_readi() failed:%s.",snd_strerror(err));
		}
		snd_pcm_prepare(obj->read_handle);
		err=snd_pcm_readi(obj->read_handle,buf,bsize/obj->frame_size);
		if (err<0) g_warning("alsa_card_read: snd_pcm_readi() failed:%s.",snd_strerror(err));
	}
	sigprocmask(SIG_UNBLOCK,&set,NULL);
	return err*obj->frame_size;
}

int alsa_card_read(AlsaCard *obj,char *buf,int size)
{
	int err;
	gint bsize=SND_CARD(obj)->bsize;
	g_return_val_if_fail(obj->read_handle!=NULL,-1);
	if (size<bsize){
		gint canread=MIN(bsize-obj->readpos,size);
		
		if (obj->readpos==0){
			err=__alsa_card_read(obj,obj->readbuf,bsize);
		}
			
		memcpy(buf,&obj->readbuf[obj->readpos],canread);
		obj->readpos+=canread;
		if (obj->readpos>=bsize) obj->readpos=0;
		return canread;
	}else{
		err=__alsa_card_read(obj,buf,size);
		return err;
	}
	
}

int __alsa_card_write(AlsaCard *obj,char *buf,int size)
{
	int err;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set,SIGALRM);
	sigprocmask(SIG_BLOCK,&set,NULL);
	if ((err=snd_pcm_writei(obj->write_handle,buf,size/obj->frame_size))<0){
		if (err!=-EPIPE){
			g_warning("alsa_card_write: snd_pcm_writei() failed:%s.",snd_strerror(err));
		}
		snd_pcm_prepare(obj->write_handle);
		err=snd_pcm_writei(obj->write_handle,buf,size/obj->frame_size);
		if (err<0) g_warning("alsa_card_write: Error writing sound buffer (size=%i):%s",size,snd_strerror(err));
		
	}
	sigprocmask(SIG_UNBLOCK,&set,NULL);
	return err;
}

int alsa_card_write(AlsaCard *obj,char *buf,int size){
	int err;
	gint bsize=SND_CARD(obj)->bsize;
	g_return_val_if_fail(obj->write_handle!=NULL,-1);
	if (size!=bsize || obj->writepos!=0)
	{
		gint canwrite;
		gint totalwrite=0;
		
		while(1){
			canwrite=MIN(bsize-obj->writepos,size);
			if (canwrite==0)
				break;
			memcpy(&obj->writebuf[obj->writepos],buf,canwrite);
			obj->writepos+=canwrite;
			if (obj->writepos>=bsize){
				err=__alsa_card_write(obj,obj->writebuf,bsize);
				obj->writepos=0;	
			}
			size-=canwrite;
			buf+=canwrite;
			totalwrite+=canwrite;
		}
		return totalwrite;
		
	}else{
		return __alsa_card_write(obj,buf,bsize);
	}
}

snd_mixer_t *alsa_mixer_open(AlsaCard *obj){
	snd_mixer_t *mixer=NULL;
	int err;
	err=snd_mixer_open(&mixer,0);
	if (err<0){
		g_warning("Could not open alsa mixer: %s",snd_strerror(err));
		return NULL;
	}
	if ((err = snd_mixer_attach (mixer, obj->mixdev)) < 0){
		g_warning("Could not attach mixer to card: %s",snd_strerror(err));
		snd_mixer_close(mixer);
		return NULL;
	}
	if ((err = snd_mixer_selem_register (mixer, NULL, NULL)) < 0){
		g_warning("snd_mixer_selem_register: %s",snd_strerror(err));
		snd_mixer_close(mixer);
		return NULL;
	}
	if ((err = snd_mixer_load (mixer)) < 0){
		g_warning("snd_mixer_load: %s",snd_strerror(err));
		snd_mixer_close(mixer);
		return NULL;
	}
	obj->mixer=mixer;
	return mixer;
}

void alsa_mixer_close(AlsaCard *obj){
	snd_mixer_close(obj->mixer);
	obj->mixer=NULL;
}

typedef enum {CAPTURE, PLAYBACK, CAPTURE_SWITCH, PLAYBACK_SWITCH} MixerAction;

static gint get_mixer_element(snd_mixer_t *mixer,const char *name, MixerAction action){
	long value=0;
	const char *elemname;
	snd_mixer_elem_t *elem;
	int err;
	long sndMixerPMin;
	long sndMixerPMax;
	long newvol;
	elem=snd_mixer_first_elem(mixer);
	while (elem!=NULL){
		elemname=snd_mixer_selem_get_name(elem);
		//g_message("Found alsa mixer element %s.",elemname);
		if (strcmp(elemname,name)==0){
			switch (action){
				case CAPTURE:
				if (snd_mixer_selem_has_capture_volume(elem)){
					snd_mixer_selem_get_playback_volume_range(elem, &sndMixerPMin, &sndMixerPMax);
					err=snd_mixer_selem_get_capture_volume(elem,SND_MIXER_SCHN_UNKNOWN,&newvol);
					newvol-=sndMixerPMin;
					value=(100*newvol)/(sndMixerPMax-sndMixerPMin);
					if (err<0) g_warning("Could not get capture volume for %s:%s",name,snd_strerror(err));
					//else g_message("Succesfully get capture level for %s.",elemname);
					break;
				}
				break;
				case PLAYBACK:
				if (snd_mixer_selem_has_playback_volume(elem)){
					snd_mixer_selem_get_playback_volume_range(elem, &sndMixerPMin, &sndMixerPMax);
					err=snd_mixer_selem_get_playback_volume(elem,SND_MIXER_SCHN_FRONT_LEFT,&newvol);
					newvol-=sndMixerPMin;
					value=(100*newvol)/(sndMixerPMax-sndMixerPMin);
					if (err<0) g_warning("Could not get playback volume for %s:%s",name,snd_strerror(err));
					//else g_message("Succesfully get playback level for %s.",elemname);
					break;
				}
				break;
				case CAPTURE_SWITCH:
				
				break;
				case PLAYBACK_SWITCH:

				break;
			}
		}
		elem=snd_mixer_elem_next(elem);
	}
	
	return value;
}


static void set_mixer_element(snd_mixer_t *mixer,const char *name, gint level,MixerAction action){
	const char *elemname;
	snd_mixer_elem_t *elem;
	long sndMixerPMin;
	long sndMixerPMax;
	long newvol;
	
	elem=snd_mixer_first_elem(mixer);
	
	while (elem!=NULL){
		elemname=snd_mixer_selem_get_name(elem);
		//g_message("Found alsa mixer element %s.",elemname);
		if (strcmp(elemname,name)==0){
			switch(action){
				case CAPTURE:
				if (snd_mixer_selem_has_capture_volume(elem)){
					snd_mixer_selem_get_playback_volume_range(elem, &sndMixerPMin, &sndMixerPMax);
					newvol=(((sndMixerPMax-sndMixerPMin)*level)/100)+sndMixerPMin;
					snd_mixer_selem_set_capture_volume_all(elem,newvol);
					//g_message("Succesfully set capture level for %s.",elemname);
					return;
				}
				break;
				case PLAYBACK:
				if (snd_mixer_selem_has_playback_volume(elem)){
					snd_mixer_selem_get_playback_volume_range(elem, &sndMixerPMin, &sndMixerPMax);
					newvol=(((sndMixerPMax-sndMixerPMin)*level)/100)+sndMixerPMin;
					snd_mixer_selem_set_playback_volume_all(elem,newvol);
					//g_message("Succesfully set playback level for %s.",elemname);
					return;
				}
				break;
				case CAPTURE_SWITCH:
				if (snd_mixer_selem_has_capture_switch(elem)){
					snd_mixer_selem_set_capture_switch_all(elem,level);
					//g_message("Succesfully set capture switch for %s.",elemname);
				}
				break;
				case PLAYBACK_SWITCH:
				if (snd_mixer_selem_has_playback_switch(elem)){
					snd_mixer_selem_set_playback_switch_all(elem,level);
					//g_message("Succesfully set capture switch for %s.",elemname);
				}
				break;

			}
		}
		elem=snd_mixer_elem_next(elem);
	}

	return ;
}


void alsa_card_set_level(AlsaCard *obj,gint way,gint a)
{	
	snd_mixer_t *mixer;
	mixer=alsa_mixer_open(obj);
	if (mixer==NULL) return ;
	switch(way){
		case SND_CARD_LEVEL_GENERAL:
			set_mixer_element(mixer,"Master",a,PLAYBACK);
		break;
		case SND_CARD_LEVEL_INPUT:
			set_mixer_element(mixer,"Capture",a,CAPTURE);
		break;
		case SND_CARD_LEVEL_OUTPUT:
			set_mixer_element(mixer,"PCM",a,PLAYBACK);
		break;
		default:
			g_warning("oss_card_set_level: unsupported command.");
	}
	alsa_mixer_close(obj);
}

gint alsa_card_get_level(AlsaCard *obj,gint way)
{
	snd_mixer_t *mixer;
	gint value = -1;
	mixer=alsa_mixer_open(obj);
	if (mixer==NULL) return 0;
	switch(way){
		case SND_CARD_LEVEL_GENERAL:
			value=get_mixer_element(mixer,"Master",PLAYBACK);
		break;
		case SND_CARD_LEVEL_INPUT:
			value=get_mixer_element(mixer,"Capture",CAPTURE);
		break;
		case SND_CARD_LEVEL_OUTPUT:
			value=get_mixer_element(mixer,"PCM",PLAYBACK);
		break;
		default:
			g_warning("oss_card_set_level: unsupported command.");
	}
	alsa_mixer_close(obj);
	return value;
}

void alsa_card_set_source(AlsaCard *obj,int source)
{
	snd_mixer_t *mixer;
	mixer=alsa_mixer_open(obj);
	if (mixer==NULL) return;
	switch (source){
		case 'm':
			set_mixer_element(mixer,"Mic",1,CAPTURE_SWITCH);
			set_mixer_element(mixer,"Capture",1,CAPTURE_SWITCH);
			break;
		case 'l':
			set_mixer_element(mixer,"Line",1,CAPTURE_SWITCH);
			set_mixer_element(mixer,"Capture",1,CAPTURE_SWITCH);
			break;
	}
}

MSFilter *alsa_card_create_read_filter(AlsaCard *card)
{
	MSFilter *f=ms_oss_read_new();
	ms_oss_read_set_device(MS_OSS_READ(f),SND_CARD(card)->index);
	return f;
}

MSFilter *alsa_card_create_write_filter(AlsaCard *card)
{
	MSFilter *f=ms_oss_write_new();
	ms_oss_write_set_device(MS_OSS_WRITE(f),SND_CARD(card)->index);
	return f;
}


SndCard * alsa_card_new(gint devid)
{
	AlsaCard * obj;
	SndCard *base;
	int err;
	gchar *name=NULL;
	
	/* carefull: this is an alsalib call despite its name! */
	err=snd_card_get_name(devid,&name);
	if (err<0) {
		return NULL;
	}
	obj= g_new0(AlsaCard,1);
	base= SND_CARD(obj);
	snd_card_init(base);
	
	base->card_name=g_strdup_printf("%s (Advanced Linux Sound Architecture)",name);
	base->_probe=(SndCardOpenFunc)alsa_card_probe;
	base->_open_r=(SndCardOpenFunc)alsa_card_open_r;
	base->_open_w=(SndCardOpenFunc)alsa_card_open_w;
	base->_can_read=(SndCardPollFunc)alsa_card_can_read;
	base->_set_blocking_mode=(SndCardSetBlockingModeFunc)alsa_card_set_blocking_mode;
	base->_read=(SndCardIOFunc)alsa_card_read;
	base->_write=(SndCardIOFunc)alsa_card_write;
	base->_close_r=(SndCardCloseFunc)alsa_card_close_r;
	base->_close_w=(SndCardCloseFunc)alsa_card_close_w;
	base->_set_rec_source=(SndCardMixerSetRecSourceFunc)alsa_card_set_source;
	base->_set_level=(SndCardMixerSetLevelFunc)alsa_card_set_level;
	base->_get_level=(SndCardMixerGetLevelFunc)alsa_card_get_level;
	base->_destroy=(SndCardDestroyFunc)alsa_card_destroy;
	base->_create_read_filter=(SndCardCreateFilterFunc)alsa_card_create_read_filter;
	base->_create_write_filter=(SndCardCreateFilterFunc)alsa_card_create_write_filter;
	
	
	obj->pcmdev=g_strdup_printf("plughw:%i,0",devid);
	obj->mixdev=g_strdup_printf("hw:%i",devid);
	obj->readbuf=NULL;
	obj->writebuf=NULL;
	return base;
}


gint alsa_card_manager_init(SndCardManager *m, gint index)
{
	gint devindex;
	gint found=0;
	gchar *name=NULL;
	for(devindex=0;index<MAX_SND_CARDS && devindex<MAX_SND_CARDS ;devindex++){
		if (snd_card_get_name(devindex,&name)==0){
			g_message("Found ALSA device: %s",name);
			m->cards[index]=alsa_card_new(devindex);
			m->cards[index]->index=index;
			found++;
			index++;
		}
	}
	return found;
}

void alsa_card_manager_set_default_pcm_device(const gchar *pcmdev){
	if (over_pcmdev!=NULL){
		g_free(over_pcmdev);	
	}
	over_pcmdev=g_strdup(pcmdev);
}

#endif
