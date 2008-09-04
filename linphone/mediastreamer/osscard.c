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

#include "osscard.h"

#include "msossread.h"
#include "msosswrite.h"

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if 0
void * oss_thread(OssCard *obj)
{
	gint i;
	gint err;
	g_message("oss_thread: starting **********");
	while(1){
		for(i=0;i<OSS_CARD_BUFFERS;i++){
			g_mutex_lock(obj->lock);
			if (obj->ref==0){
				g_cond_signal(obj->cond);
				g_mutex_unlock(obj->lock);
				g_thread_exit(NULL);
			}
			g_mutex_unlock(obj->lock);
			obj->readindex=i;
			
			err=read(obj->fd,obj->readbuf[i],SND_CARD(obj)->bsize);
			if (err<0) g_warning("oss_thread: read() error:%s.",strerror(errno));
			obj->writeindex=i;
			write(obj->fd,obj->writebuf[i],SND_CARD(obj)->bsize);
			memset(obj->writebuf[i],0,SND_CARD(obj)->bsize);
		}
	}
}
#endif
int oss_open(OssCard *obj, int bits,int stereo, int rate)
{
	int fd;
	int p=0,cond=0;
	int i=0;
	int min_size=0,blocksize=512;
	int err;
	
	//g_message("opening sound device");
	fd=open(obj->dev_name,O_RDWR|O_NONBLOCK);
	if (fd<0) return -EWOULDBLOCK;
	/* unset nonblocking mode */
	/* We wanted non blocking open but now put it back to normal ; thanks Xine !*/
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);

	/* reset is maybe not needed but takes time*/
	/*ioctl(fd, SNDCTL_DSP_RESET, 0); */

	
#ifdef WORDS_BIGENDIAN
	p=AFMT_U16_BE;
#else
	p=AFMT_U16_LE;
#endif
	
	err=ioctl(fd,SNDCTL_DSP_SETFMT,&p);
	if (err<0){
		g_warning("oss_open: can't set sample format:%s.",strerror(errno));
	}

	
	p =  bits;  /* 16 bits */
	err=ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &p);
	if (err<0){
		g_warning("oss_open: can't set sample size to %i:%s.",bits,strerror(errno));
	}

	p =  rate;  /* rate in khz*/
	err=ioctl(fd, SNDCTL_DSP_SPEED, &p);
	if (err<0){
		g_warning("oss_open: can't set sample rate to %i:%s.",rate,strerror(errno));
	}
	
	p =  stereo;  /* stereo or not */
	err=ioctl(fd, SNDCTL_DSP_STEREO, &p);
	if (err<0){
		g_warning("oss_open: can't set mono/stereo mode:%s.",strerror(errno));
	}
	
	if (rate==16000) blocksize=4096;	/* oss emulation is not very good at 16khz */
	else blocksize=blocksize*(rate/8000);
	ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &min_size);

	/* try to subdivide BLKSIZE to reach blocksize if necessary */
	if (min_size>blocksize)
  {
		cond=1;
    	p=min_size/blocksize;
    	while(cond)
    	{
			i=ioctl(fd, SNDCTL_DSP_SUBDIVIDE, &p);
			//printf("SUB_DIVIDE said error=%i,errno=%i\n",i,errno);
     	if ((i==0) || (p==1)) cond=0;
     	else p=p/2;
     }
	}
	ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &min_size);
	if (min_size>blocksize)
	{
		g_warning("dsp block size set to %i.",min_size);
	}else{
		/* no need to access the card with less latency than needed*/
		min_size=blocksize;
	}

	g_message("dsp blocksize is %i.",min_size);
	
	/* start recording !!! Alex */
    {
      int fl,res;

      fl=PCM_ENABLE_OUTPUT|PCM_ENABLE_INPUT;
      res=ioctl(fd, SNDCTL_DSP_SETTRIGGER, &fl);
      if (res<0) g_warning("OSS_TRIGGER: %s",strerror(errno));
    } 
	
	obj->fd=fd;
	obj->readpos=0;
	obj->writepos=0;
	SND_CARD(obj)->bits=bits;
	SND_CARD(obj)->stereo=stereo;
	SND_CARD(obj)->rate=rate;
	SND_CARD(obj)->bsize=min_size;
	return fd;
}

int oss_card_probe(OssCard *obj,int bits,int stereo,int rate)
{
	
	int fd;
	int p=0,cond=0;
	int i=0;
	int min_size=0,blocksize=512;
	
	if (obj->fd>0) return SND_CARD(obj)->bsize;
	fd=open(obj->dev_name,O_RDWR|O_NONBLOCK);
	if (fd<0) {
		g_warning("oss_card_probe: can't open %s: %s.",obj->dev_name,strerror(errno));
		return -1;
	}
	ioctl(fd, SNDCTL_DSP_RESET, 0);

	p =  bits;  /* 16 bits */
	ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &p);

	p =  stereo;  /* number of channels */
	ioctl(fd, SNDCTL_DSP_CHANNELS, &p);

	p =  rate;  /* rate in khz*/
	ioctl(fd, SNDCTL_DSP_SPEED, &p);

	ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &min_size);

	/* try to subdivide BLKSIZE to reach blocksize if necessary */
	if (min_size>blocksize)
  {
		cond=1;
    	p=min_size/blocksize;
    	while(cond)
    	{
			i=ioctl(fd, SNDCTL_DSP_SUBDIVIDE, &p);
			//printf("SUB_DIVIDE said error=%i,errno=%i\n",i,errno);
     	if ((i==0) || (p==1)) cond=0;
     	else p=p/2;
     }
	}
	ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &min_size);
	if (min_size>blocksize)
	{
		g_warning("dsp block size set to %i.",min_size);
	}else{
		/* no need to access the card with less latency than needed*/
		min_size=blocksize;
	}
	close(fd);
	return min_size;
}


int oss_card_open(OssCard *obj,int bits,int stereo,int rate)
{
	int fd;
	obj->ref++;
	if (obj->fd==0){
		fd=oss_open(obj,bits,stereo,rate);
		if (fd<0) {
			obj->fd=0;
			obj->ref--;
			return -1;
		}
	}
	
	obj->readbuf=g_malloc0(SND_CARD(obj)->bsize);
	obj->writebuf=g_malloc0(SND_CARD(obj)->bsize);
	
	SND_CARD(obj)->flags|=SND_CARD_FLAGS_OPENED;
	return 0;
}

void oss_card_close(OssCard *obj)
{
	obj->ref--;
	if (obj->ref==0) {
		close(obj->fd);
		obj->fd=0;
		SND_CARD(obj)->flags&=~SND_CARD_FLAGS_OPENED;
		g_free(obj->readbuf);
		obj->readbuf=NULL;
		g_free(obj->writebuf);
		obj->writebuf=NULL;
		
	}
}

void oss_card_destroy(OssCard *obj)
{
	snd_card_uninit(SND_CARD(obj));
	g_free(obj->dev_name);
	g_free(obj->mixdev_name);
	if (obj->readbuf!=NULL) g_free(obj->readbuf);
	if (obj->writebuf!=NULL) g_free(obj->writebuf);
}

gboolean oss_card_can_read(OssCard *obj)
{
	struct timeval tout={0,0};
	int err;
	fd_set fdset;
	if (obj->readpos!=0) return TRUE;
	FD_ZERO(&fdset);
	FD_SET(obj->fd,&fdset);
	err=select(obj->fd+1,&fdset,NULL,NULL,&tout);
	if (err>0) return TRUE;
	else return FALSE;
}

int oss_card_read(OssCard *obj,char *buf,int size)
{
	int err;
	gint bsize=SND_CARD(obj)->bsize;
	if (size<bsize){
		gint canread=MIN(bsize-obj->readpos,size);
		if (obj->readpos==0){
			err=read(obj->fd,obj->readbuf,bsize);
			if (err<0) {
				g_warning("oss_card_read: read() failed:%s.",strerror(errno));
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
			g_warning("oss_card_read: read-2() failed:%s.",strerror(errno));
		}
		return err;
	}
	
}

int oss_card_write(OssCard *obj,char *buf,int size)
{
	int err;
	gint bsize=SND_CARD(obj)->bsize;
	
	if (size<bsize){
		gint canwrite;
		canwrite=MIN(bsize-obj->writepos,size);
		memcpy(&obj->writebuf[obj->writepos],buf,canwrite);
		obj->writepos+=canwrite;
		if (obj->writepos>=bsize){
			err=write(obj->fd,obj->writebuf,bsize);
			obj->writepos=0;
		}
		return canwrite;
	}else{
		return write(obj->fd,buf,bsize);
	}
}

void oss_card_set_level(OssCard *obj,gint way,gint a)
{
	int p,mix_fd;
	int osscmd;
	g_return_if_fail(obj->mixdev_name!=NULL);
#ifdef HAVE_SYS_SOUNDCARD_H
	switch(way){
		case SND_CARD_LEVEL_GENERAL:
			osscmd=SOUND_MIXER_VOLUME;
		break;
		case SND_CARD_LEVEL_INPUT:
			osscmd=SOUND_MIXER_IGAIN;
		break;
		case SND_CARD_LEVEL_OUTPUT:
			osscmd=SOUND_MIXER_PCM;
		break;
		default:
			g_warning("oss_card_set_level: unsupported command.");
			return;
	}
	p=(((int)a)<<8 | (int)a);
	mix_fd = open(obj->mixdev_name, O_WRONLY);
	ioctl(mix_fd,MIXER_WRITE(osscmd), &p);
	close(mix_fd);
#endif
}

gint oss_card_get_level(OssCard *obj,gint way)
{
	int p=0,mix_fd;
	int osscmd;
	g_return_val_if_fail(obj->mixdev_name!=NULL,-1);
#ifdef HAVE_SYS_SOUNDCARD_H
	switch(way){
		case SND_CARD_LEVEL_GENERAL:
			osscmd=SOUND_MIXER_VOLUME;
		break;
		case SND_CARD_LEVEL_INPUT:
			osscmd=SOUND_MIXER_IGAIN;
		break;
		case SND_CARD_LEVEL_OUTPUT:
			osscmd=SOUND_MIXER_PCM;
		break;
		default:
			g_warning("oss_card_get_level: unsupported command.");
			return -1;
	}
	mix_fd = open(obj->mixdev_name, O_RDONLY);
	ioctl(mix_fd,MIXER_READ(SOUND_MIXER_VOLUME), &p);
	close(mix_fd);
#endif
	return p>>8;
}

void oss_card_set_source(OssCard *obj,int source)
{
	gint p=0;
	gint mix_fd;
	g_return_if_fail(obj->mixdev_name!=NULL);
#ifdef HAVE_SYS_SOUNDCARD_H	
	if (source == 'c')
		p = 1 << SOUND_MIXER_CD;
	if (source == 'l')
		p = 1 << SOUND_MIXER_LINE;
	if (source == 'm')
		p = 1 << SOUND_MIXER_MIC;

	
	mix_fd = open(obj->mixdev_name, O_WRONLY);
	ioctl(mix_fd, SOUND_MIXER_WRITE_RECSRC, &p);
	close(mix_fd);
#endif
}

MSFilter *oss_card_create_read_filter(OssCard *card)
{
	MSFilter *f=ms_oss_read_new();
	ms_oss_read_set_device(MS_OSS_READ(f),SND_CARD(card)->index);
	return f;
}

MSFilter *oss_card_create_write_filter(OssCard *card)
{
	MSFilter *f=ms_oss_write_new();
	ms_oss_write_set_device(MS_OSS_WRITE(f),SND_CARD(card)->index);
	return f;
}


SndCard * oss_card_new(char *devname, char *mixdev_name)
{
	OssCard * obj= g_new0(OssCard,1);
	SndCard *base= SND_CARD(obj);
	snd_card_init(base);
	obj->dev_name=g_strdup(devname);
	obj->mixdev_name=g_strdup( mixdev_name);
#ifdef HAVE_GLIB
	base->card_name=g_strdup_printf("%s (Open Sound System)",devname);
#else
	base->card_name=malloc(100);
	snprintf(base->card_name, 100, "%s (Open Sound System)",devname);
#endif
	base->_probe=(SndCardOpenFunc)oss_card_probe;
	base->_open_r=(SndCardOpenFunc)oss_card_open;
	base->_open_w=(SndCardOpenFunc)oss_card_open;
	base->_can_read=(SndCardPollFunc)oss_card_can_read;
	base->_read=(SndCardIOFunc)oss_card_read;
	base->_write=(SndCardIOFunc)oss_card_write;
	base->_close_r=(SndCardCloseFunc)oss_card_close;
	base->_close_w=(SndCardCloseFunc)oss_card_close;
	base->_set_rec_source=(SndCardMixerSetRecSourceFunc)oss_card_set_source;
	base->_set_level=(SndCardMixerSetLevelFunc)oss_card_set_level;
	base->_get_level=(SndCardMixerGetLevelFunc)oss_card_get_level;
	base->_destroy=(SndCardDestroyFunc)oss_card_destroy;
	base->_create_read_filter=(SndCardCreateFilterFunc)oss_card_create_read_filter;
	base->_create_write_filter=(SndCardCreateFilterFunc)oss_card_create_write_filter;
	return base;
}

#define DSP_NAME "/dev/dsp"
#define MIXER_NAME "/dev/mixer"

gint oss_card_manager_init(SndCardManager *manager, gint tabindex)
{
	gchar *devname;
	gchar *mixername;
	gint devindex=0;
	gint found=0;

	/* search for /dev/dsp and /dev/mixer */
#ifdef HAVE_GLIB
	if (g_file_test(DSP_NAME,G_FILE_TEST_EXISTS)){
		tabindex++;
		devindex++;
		manager->cards[0]=oss_card_new(DSP_NAME,MIXER_NAME);
		manager->cards[0]->index=0;
		found++;
		g_message("Found /dev/dsp.");
	}
	for (;tabindex<MAX_SND_CARDS && devindex<MAX_SND_CARDS ;devindex++){
		devname=g_strdup_printf("%s%i",DSP_NAME,devindex);
		mixername=g_strdup_printf("%s%i",MIXER_NAME,devindex);
		if (g_file_test(devname,G_FILE_TEST_EXISTS)){
			manager->cards[tabindex]=oss_card_new(devname,mixername);
			manager->cards[tabindex]->index=tabindex;
			tabindex++;
			found++;
		}
		g_free(devname);
		g_free(mixername);
	}
#else
	if (access(DSP_NAME,F_OK)==0){
		tabindex++;
		devindex++;
		manager->cards[0]=oss_card_new(DSP_NAME,MIXER_NAME);
		manager->cards[0]->index=0;
		found++;
		g_message("Found /dev/dsp.");
	}
	for (;tabindex<MAX_SND_CARDS && devindex<MAX_SND_CARDS ;devindex++){
		devname=malloc(100);
		snprintf(devname, 100, "%s%i",DSP_NAME,devindex);
		mixername=malloc(100);
		snprintf(mixername, 100, "%s%i",MIXER_NAME,devindex);

		if (access(devname,F_OK)==0){
			manager->cards[tabindex]=oss_card_new(devname,mixername);
			manager->cards[tabindex]->index=tabindex;
			tabindex++;
			found++;
		}
		g_free(devname);
		g_free(mixername);
	}
#endif
	if (tabindex==0) g_warning("No sound cards found !");
	return found;
}


#endif
