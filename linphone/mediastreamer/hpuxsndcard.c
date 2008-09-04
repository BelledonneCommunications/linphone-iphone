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

#include "sndcard.h"
#include "osscard.h"

#ifdef HAVE_SYS_AUDIO_H
#include <sys/audio.h>


#include "msossread.h"
#include "msosswrite.h"

#include <errno.h>
#include <fcntl.h>


int hpuxsnd_open(HpuxSndCard *obj, int bits,int stereo, int rate)
{
	int fd;
	int p=0,cond=0;
	int i=0;
	int min_size=0,blocksize=512;
	/* do a quick non blocking open to be sure that we are not going to be blocked here
		for the eternity */
	fd=open(obj->dev_name,O_RDWR|O_NONBLOCK);
	if (fd<0) return -EWOULDBLOCK;
	close(fd);
	/* open the device */
	fd=open(obj->dev_name,O_RDWR);

	g_return_val_if_fail(fd>0,-errno);

	ioctl(fd,AUDIO_RESET,0);
	ioctl(fd,AUDIO_SET_SAMPLE_RATE,rate);
	ioctl(fd,AUDIO_SET_CHANNELS,stereo);
	p=AUDIO_FORMAT_LINEAR16BIT;
	ioctl(fd,AUDIO_SET_DATA_FORMAT,p); 
	/* ioctl(fd,AUDIO_GET_RXBUFSIZE,&min_size); does not work ? */
	min_size=2048;

	g_message("dsp blocksize is %i.",min_size);
	obj->fd=fd;
	obj->readpos=0;
	obj->writepos=0;
	SND_CARD(obj)->bits=bits;
	SND_CARD(obj)->stereo=stereo;
	SND_CARD(obj)->rate=rate;
	SND_CARD(obj)->bsize=min_size;
	return fd;
}

int hpux_snd_card_probe(HpuxSndCard *obj,int bits,int stereo,int rate)
{
	return 2048;
}


int hpux_snd_card_open(HpuxSndCard *obj,int bits,int stereo,int rate)
{
	int fd;
	obj->ref++;
	if (obj->fd==0){
		fd=hpuxsnd_open(obj,bits,stereo,rate);
		if (fd<0) {
			obj->fd=0;
			obj->ref--;
			return -1;
		}
	}
	SND_CARD(obj)->flags|=SND_CARD_FLAGS_OPENED;
	return 0;
}

void hpux_snd_card_close(HpuxSndCard *obj)
{
	int i;
	obj->ref--;
	if (obj->ref==0) {
		close(obj->fd);
		obj->fd=0;
		SND_CARD(obj)->flags&=~SND_CARD_FLAGS_OPENED;
		
	}
}

void hpux_snd_card_destroy(HpuxSndCard *obj)
{
	snd_card_uninit(SND_CARD(obj));
	g_free(obj->dev_name);
	g_free(obj->mixdev_name);
}

gboolean hpux_snd_card_can_read(HpuxSndCard *obj)
{
	struct timeval tout={0,0};
	int err;
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(obj->fd,&fdset);
	err=select(obj->fd+1,&fdset,NULL,NULL,&tout);
	if (err>0) return TRUE;
	else return FALSE;
}

int hpux_snd_card_read(HpuxSndCard *obj,char *buf,int size)
{
	int err;
	gint bsize=SND_CARD(obj)->bsize;
	if (size<bsize){
		gint canread=MIN(bsize-obj->readpos,size);
		if (obj->readbuf==NULL) obj->readbuf=g_malloc0(bsize);
		if (obj->readpos==0){
			err=read(obj->fd,obj->readbuf,bsize);
			if (err<0) {
				g_warning("hpux_snd_card_read: read() failed:%s.",strerror(errno));
				return -1;
			}
		}
			
		memcpy(buf,&obj->readbuf[obj->readpos],canread);
		obj->readpos+=canread;
		if (obj->readpos>=bsize) obj->readpos=0;
		return canread;
	}else{
		err=read(obj->fd,buf,size);
		if (err<0) {
			g_warning("hpux_snd_card_read: read-2() failed:%s.",strerror(errno));
		}
		return err;
	}
	
}

int hpux_snd_card_write(HpuxSndCard *obj,char *buf,int size)
{
	int err;
	gint bsize=SND_CARD(obj)->bsize;
	if (size<bsize){
		gint canwrite=MIN(bsize-obj->writepos,size);
		if (obj->writebuf==NULL) obj->writebuf=g_malloc0(bsize);
		
		memcpy(&obj->writebuf[obj->writepos],buf,canwrite);
		obj->writepos+=canwrite;
		if (obj->writepos>=bsize){
			err=write(obj->fd,obj->writebuf,bsize);
		}
		return canwrite;
	}else{
		return write(obj->fd,buf,bsize);
	}
}

#define SND_CARD_LEVEL_TO_HPUX_LEVEL(a)	 (((a)*2) - 100)
#define HPUX_LEVEL_TO_SND_CARD_LEVEL(a)		(((a)+200)/2)
void hpux_snd_card_set_level(HpuxSndCard *obj,gint way,gint a)
{
	struct audio_gain gain;
	int error,mix_fd;
		
	g_return_if_fail(obj->mixdev_name!=NULL);
	memset(&gain,0,sizeof(struct audio_gain));
	switch(way){
		case SND_CARD_LEVEL_GENERAL:
			gain.cgain[0].monitor_gain=SND_CARD_LEVEL_TO_HPUX_LEVEL(a);
			gain.cgain[1].monitor_gain=SND_CARD_LEVEL_TO_HPUX_LEVEL(a);
		break;
		case SND_CARD_LEVEL_INPUT:
			gain.cgain[0].receive_gain=SND_CARD_LEVEL_TO_HPUX_LEVEL(a);
			gain.cgain[1].receive_gain=SND_CARD_LEVEL_TO_HPUX_LEVEL(a);
		break;
		case SND_CARD_LEVEL_OUTPUT:
			gain.cgain[0].transmit_gain=SND_CARD_LEVEL_TO_HPUX_LEVEL(a);
			gain.cgain[1].transmit_gain=SND_CARD_LEVEL_TO_HPUX_LEVEL(a);
		break;
		default:
			g_warning("hpux_snd_card_set_level: unsupported command.");
			return;
	}
	gain.channel_mask=AUDIO_CHANNEL_RIGHT|AUDIO_CHANNEL_LEFT;
	mix_fd = open(obj->mixdev_name, O_WRONLY);
	g_return_if_fail(mix_fd>0);
	error=ioctl(mix_fd,AUDIO_SET_GAINS,&gain);
  	if (error<0){
    	g_warning("hpux_snd_card_set_level: Could not set gains: %s",strerror(errno));
	}
	close(mix_fd);
}

gint hpux_snd_card_get_level(HpuxSndCard *obj,gint way)
{
	struct audio_gain gain;
	int p=0,mix_fd,error;
	g_return_if_fail(obj->mixdev_name!=NULL);
	
	gain.channel_mask=AUDIO_CHANNEL_RIGHT|AUDIO_CHANNEL_LEFT;
	mix_fd = open(obj->mixdev_name, O_RDONLY);
	g_return_if_fail(mix_fd>0);
	error=ioctl(mix_fd,AUDIO_GET_GAINS,&gain);
  	if (error<0){
    	g_warning("hpux_snd_card_set_level: Could not get gains: %s",strerror(errno));
	}
	close(mix_fd);
	
	switch(way){
		case SND_CARD_LEVEL_GENERAL:
			p=gain.cgain[0].monitor_gain;
		break;
		case SND_CARD_LEVEL_INPUT:
			p=gain.cgain[0].receive_gain;
		break;
		case SND_CARD_LEVEL_OUTPUT:
			p=gain.cgain[0].transmit_gain;
		break;
		default:
			g_warning("hpux_snd_card_get_level: unsupported command.");
			return -1;
	}
	return HPUX_LEVEL_TO_SND_CARD_LEVEL(p);
}

void hpux_snd_card_set_source(HpuxSndCard *obj,int source)
{
	gint p=0;
	gint mix_fd;
	gint error=0;
	g_return_if_fail(obj->mixdev_name!=NULL);
	
	mix_fd=open("/dev/audio",O_WRONLY);
	g_return_if_fail(mix_fd>0);
	switch(source){
		case 'm':
			error=ioctl(mix_fd,AUDIO_SET_INPUT,AUDIO_IN_MIKE);
		break;
		case 'l':
			error=ioctl(mix_fd,AUDIO_SET_INPUT,AUDIO_IN_LINE);
		break;
		default:
			g_warning("hpux_snd_card_set_source: unsupported source.");
	}
	close(mix_fd);
}

MSFilter *hpux_snd_card_create_read_filter(HpuxSndCard *card)
{
	MSFilter *f=ms_oss_read_new();
	ms_oss_read_set_device(MS_OSS_READ(f),SND_CARD(card)->index);
	return f;
}

MSFilter *hpux_snd_card_create_write_filter(HpuxSndCard *card)
{
	MSFilter *f=ms_oss_write_new();
	ms_oss_write_set_device(MS_OSS_WRITE(f),SND_CARD(card)->index);
	return f;
}


SndCard * hpux_snd_card_new(char *devname, char *mixdev_name)
{
	HpuxSndCard * obj= g_new0(HpuxSndCard,1);
	SndCard *base= SND_CARD(obj);
	snd_card_init(base);
	obj->dev_name=g_strdup(devname);
	obj->mixdev_name=g_strdup( mixdev_name);
	base->card_name=g_strdup(devname);
	base->_probe=(SndCardOpenFunc)hpux_snd_card_probe;
	base->_open_r=(SndCardOpenFunc)hpux_snd_card_open;
	base->_open_w=(SndCardOpenFunc)hpux_snd_card_open;
	base->_can_read=(SndCardPollFunc)hpux_snd_card_can_read;
	base->_read=(SndCardIOFunc)hpux_snd_card_read;
	base->_write=(SndCardIOFunc)hpux_snd_card_write;
	base->_close_r=(SndCardCloseFunc)hpux_snd_card_close;
	base->_close_w=(SndCardCloseFunc)hpux_snd_card_close;
	base->_set_rec_source=(SndCardMixerSetRecSourceFunc)hpux_snd_card_set_source;
	base->_set_level=(SndCardMixerSetLevelFunc)hpux_snd_card_set_level;
	base->_get_level=(SndCardMixerGetLevelFunc)hpux_snd_card_get_level;
	base->_destroy=(SndCardDestroyFunc)hpux_snd_card_destroy;
	base->_create_read_filter=(SndCardCreateFilterFunc)hpux_snd_card_create_read_filter;
	base->_create_write_filter=(SndCardCreateFilterFunc)hpux_snd_card_create_write_filter;
	return base;
}

#endif
