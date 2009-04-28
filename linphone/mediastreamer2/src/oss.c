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

#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msfilter.h"

#include <sys/soundcard.h>

#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef HAVE_ALLOCA_H /*FreeBSD does not have alloca.h*/
#include <alloca.h>
#endif

MSFilter *ms_oss_read_new(MSSndCard *card);
MSFilter *ms_oss_write_new(MSSndCard *card);

static int oss_open(const char *devname, int bits,int stereo, int rate, int *minsz)
{
	int fd;
	int p=0,cond=0;
	int i=0;
	int min_size=0,blocksize=512;
	int err;
	int frag;
	audio_buf_info info;
  
	//g_message("opening sound device");
	fd=open(devname,O_RDWR|O_NONBLOCK);
	if (fd<0) return -EWOULDBLOCK;
	/* unset nonblocking mode */
	/* We wanted non blocking open but now put it back to normal ; thanks Xine !*/
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);

	/* reset is maybe not needed but takes time*/
	/*ioctl(fd, SNDCTL_DSP_RESET, 0); */

	/* This code is used to limit the internal buffer of the sound
	   card so that no internal delay can occur in the sound card */
	frag = ( ( 32767 << 16 ) | 7 );
	if( ioctl( fd, SNDCTL_DSP_SETFRAGMENT, &frag ) ) {
		ms_warning("oss_open: can't set fragment size:%s.",strerror(errno));
	}
	
	p=AFMT_S16_NE;
	
	err=ioctl(fd,SNDCTL_DSP_SETFMT,&p);
	if (err<0){
		ms_warning("oss_open: can't set sample format:%s.",strerror(errno));
	}

	
	p =  bits;  /* 16 bits */
	err=ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &p);
	if (err<0){
		ms_warning("oss_open: can't set sample size to %i:%s.",bits,strerror(errno));
	}

	p =  rate;  /* rate in khz*/
	err=ioctl(fd, SNDCTL_DSP_SPEED, &p);
	if (err<0){
		ms_warning("oss_open: can't set sample rate to %i:%s.",rate,strerror(errno));
	}
	
	p =  stereo;  /* stereo or not */
	err=ioctl(fd, SNDCTL_DSP_STEREO, &p);
	if (err<0){
		ms_warning("oss_open: can't set mono/stereo mode:%s.",strerror(errno));
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
		ms_warning("dsp block size set to %i.",min_size);
	}else{
		/* no need to access the card with less latency than needed*/
		min_size=blocksize;
	}

	ms_message("/dev/dsp opened: rate=%i,bits=%i,stereo=%i blocksize=%i.",
			rate,bits,stereo,min_size);
	
	if( ioctl( fd, SNDCTL_DSP_GETISPACE, &info ) == -1 ) {
		ms_warning("oss_open: can't get ispace:%s.",strerror(errno));
	}
	else{
		ms_warning("oss_open: audio buffer size: %i.", info.fragsize * sizeof( short ));
	}


	/* start recording !!! Alex */
	{
		int fl,res;
		
		fl=PCM_ENABLE_OUTPUT|PCM_ENABLE_INPUT;
		res=ioctl(fd, SNDCTL_DSP_SETTRIGGER, &fl);
		if (res<0) ms_warning("OSS_TRIGGER: %s",strerror(errno));
	} 
	*minsz=min_size;
	return fd;
}

typedef struct OssData{
	char *pcmdev;
	char *mixdev;
	int pcmfd;
	int rate;
	int bits;
	ms_thread_t thread;
	ms_mutex_t mutex;
	queue_t rq;
	MSBufferizer * bufferizer;
	bool_t read_started;
	bool_t write_started;
	bool_t stereo;
} OssData;

static void oss_set_level(MSSndCard *card, MSSndCardMixerElem e, int percent)
{
	OssData *d=(OssData*)card->data;
	int p,mix_fd;
	int osscmd;
	if (d->mixdev==NULL) return;
	switch(e){
		case MS_SND_CARD_MASTER:
			osscmd=SOUND_MIXER_VOLUME;
		break;
		case MS_SND_CARD_CAPTURE:
			osscmd=SOUND_MIXER_IGAIN;
		break;
		case MS_SND_CARD_PLAYBACK:
			osscmd=SOUND_MIXER_PCM;
		break;
		default:
			ms_warning("oss_card_set_level: unsupported command.");
			return;
	}
	p=(((int)percent)<<8 | (int)percent);
	mix_fd = open(d->mixdev, O_WRONLY);
	ioctl(mix_fd,MIXER_WRITE(osscmd), &p);
	close(mix_fd);
}

static int oss_get_level(MSSndCard *card, MSSndCardMixerElem e)
{
	OssData *d=(OssData*)card->data;
	int p=0,mix_fd;
	int osscmd;
	if (d->mixdev==NULL) return -1;
	switch(e){
		case MS_SND_CARD_MASTER:
			osscmd=SOUND_MIXER_VOLUME;
		break;
		case MS_SND_CARD_CAPTURE:
			osscmd=SOUND_MIXER_IGAIN;
		break;
		case MS_SND_CARD_PLAYBACK:
			osscmd=SOUND_MIXER_PCM;
		break;
		default:
			ms_warning("oss_card_get_level: unsupported command.");
			return -1;
	}
	mix_fd = open(d->mixdev, O_RDONLY);
	ioctl(mix_fd,MIXER_READ(osscmd), &p);
	close(mix_fd);
	return p>>8;
}

static void oss_set_source(MSSndCard *card, MSSndCardCapture source)
{
	OssData *d=(OssData*)card->data;
	int p=0;
	int mix_fd;
	if (d->mixdev==NULL) return;

	switch(source){
		case MS_SND_CARD_MIC:
			p = 1 << SOUND_MIXER_MIC;
		break;
		case MS_SND_CARD_LINE:
			p = 1 << SOUND_MIXER_LINE;
		break;
	}
	
	mix_fd = open(d->mixdev, O_WRONLY);
	ioctl(mix_fd, SOUND_MIXER_WRITE_RECSRC, &p);
	close(mix_fd);
}

static void oss_init(MSSndCard *card){
	OssData *d=ms_new(OssData,1);
	d->pcmdev=NULL;
	d->mixdev=NULL;
	d->pcmfd=-1;
	d->read_started=FALSE;
	d->write_started=FALSE;
	d->bits=16;
	d->rate=8000;
	d->stereo=FALSE;
	qinit(&d->rq);
	d->bufferizer=ms_bufferizer_new();
	ms_mutex_init(&d->mutex,NULL);
	card->data=d;
}

static void oss_uninit(MSSndCard *card){
	OssData *d=(OssData*)card->data;
	if (d->pcmdev!=NULL) ms_free(d->pcmdev);
	if (d->mixdev!=NULL) ms_free(d->mixdev);
	ms_bufferizer_destroy(d->bufferizer);
	flushq(&d->rq,0);
	ms_mutex_destroy(&d->mutex);
	ms_free(d);
}

#define DSP_NAME "/dev/dsp"
#define MIXER_NAME "/dev/mixer"

static void oss_detect(MSSndCardManager *m);
static MSSndCard *oss_duplicate(MSSndCard *obj);

MSSndCardDesc oss_card_desc={
	.driver_type="OSS",
	.detect=oss_detect,
	.init=oss_init,
	.set_level=oss_set_level,
	.get_level=oss_get_level,
	.set_capture=oss_set_source,
	.set_control=NULL,
	.get_control=NULL,
	.create_reader=ms_oss_read_new,
	.create_writer=ms_oss_write_new,
	.uninit=oss_uninit,
	.duplicate=oss_duplicate
};

static MSSndCard *oss_duplicate(MSSndCard *obj){
	MSSndCard *card=ms_snd_card_new(&oss_card_desc);
	OssData *dcard=(OssData*)card->data;
	OssData *dobj=(OssData*)obj->data;
	dcard->pcmdev=ms_strdup(dobj->pcmdev);
	dcard->mixdev=ms_strdup(dobj->mixdev);
	card->name=ms_strdup(obj->name);
	return card;
}

static MSSndCard *oss_card_new(const char *pcmdev, const char *mixdev){
	MSSndCard *card=ms_snd_card_new(&oss_card_desc);
	OssData *d=(OssData*)card->data;
	d->pcmdev=ms_strdup(pcmdev);
	d->mixdev=ms_strdup(mixdev);
	card->name=ms_strdup(pcmdev);
	return card;
}

static void oss_detect(MSSndCardManager *m){
	int i;
	char pcmdev[sizeof(DSP_NAME)+3];
	char mixdev[sizeof(MIXER_NAME)+3];
	if (access(DSP_NAME,F_OK)==0){
		MSSndCard *card=oss_card_new(DSP_NAME,MIXER_NAME);
		ms_snd_card_manager_add_card(m,card);
	}
	for(i=0;i<10;i++){
		snprintf(pcmdev,sizeof(pcmdev),"%s%i",DSP_NAME,i);
		snprintf(mixdev,sizeof(mixdev),"%s%i",MIXER_NAME,i);
		if (access(pcmdev,F_OK)==0){
		  MSSndCard *card=oss_card_new(pcmdev,mixdev);
		  ms_snd_card_manager_add_card(m,card);
		}
	}
}

static void * oss_thread(void *p){
	MSSndCard *card=(MSSndCard*)p;
	OssData *d=(OssData*)card->data;
	int bsize=0;
	uint8_t *rtmpbuff=NULL;
	uint8_t *wtmpbuff=NULL;
	int err;
	mblk_t *rm=NULL;
	d->pcmfd=oss_open(d->pcmdev,d->bits,d->stereo,d->rate,&bsize);
	if (d->pcmfd>=0){
		rtmpbuff=(uint8_t*)malloc(bsize);
		wtmpbuff=(uint8_t*)malloc(bsize);
		if(rtmpbuff == NULL || wtmpbuff == NULL) {
			free(rtmpbuff);
			free(wtmpbuff);
			return NULL;
		}
	}
	while(d->read_started || d->write_started){
		if (d->pcmfd>=0){
			if (d->read_started){
				struct timeval timeout;
				fd_set read_fds;
				audio_buf_info info;
				if (rm==NULL) rm=allocb(bsize,0);

				timeout.tv_sec = 0;
				timeout.tv_usec = 0;
				FD_ZERO( &read_fds );
				FD_SET( d->pcmfd, &read_fds );
				if( select( d->pcmfd + 1, &read_fds, NULL, NULL, &timeout ) == -1 ) {
				}
				if (FD_ISSET( d->pcmfd, &read_fds ) &&  ioctl( d->pcmfd, SNDCTL_DSP_GETISPACE, &info ) != -1)
				{
					if (info.bytes>=bsize)
					{
						err=read(d->pcmfd,rm->b_wptr,bsize);
						if (err<0){
							ms_warning("Fail to read %i bytes from soundcard: %s",
								   bsize,strerror(errno));
						}else{
							rm->b_wptr+=err;
							ms_mutex_lock(&d->mutex);
							putq(&d->rq,rm);
							ms_mutex_unlock(&d->mutex);
							rm=NULL;
						}
					}
					else
					  {
					    timeout.tv_sec = 0;
					    timeout.tv_usec = 5000;
					    select(0, 0, NULL, NULL, &timeout );
					  }
				}
				else
				  {
				    timeout.tv_sec = 0;
				    timeout.tv_usec = 5000;
				    select(0, 0, NULL, NULL, &timeout );
				  }
			}else {
				int sz = read(d->pcmfd,rtmpbuff,bsize);
				if( sz!=bsize) ms_warning("sound device read returned %i !",sz);
			}
			if (d->write_started){

				audio_buf_info info;
				if( ms_bufferizer_get_avail(d->bufferizer)>=bsize && ioctl( d->pcmfd, SNDCTL_DSP_GETOSPACE, &info ) == 0 ) {
					if( info.fragstotal - info.fragments > 15 ) {
						static int c=0;
						/* drop the fragment if the buffer starts to fill up */
						/* we got too much data: I prefer to empty the incoming buffer */
						while (ms_bufferizer_get_avail(d->bufferizer)>bsize*4){
							ms_mutex_lock(&d->mutex);
							err=ms_bufferizer_read(d->bufferizer,wtmpbuff,bsize);
							err=ms_bufferizer_read(d->bufferizer,wtmpbuff,bsize);
							err=ms_bufferizer_read(d->bufferizer,wtmpbuff,bsize);
							err=ms_bufferizer_read(d->bufferizer,wtmpbuff,bsize);
							ms_mutex_unlock(&d->mutex);
							c=c+err*4;
							ms_warning("drop fragment when buffer gets too much data (%i - discarded:%i)", info.fragstotal - info.fragments, c);
							if (err==0)
							  break;
						}

					}else {
						ms_mutex_lock(&d->mutex);
						err=ms_bufferizer_read(d->bufferizer,wtmpbuff,bsize);
						ms_mutex_unlock(&d->mutex);
						err=write(d->pcmfd,wtmpbuff,bsize);
						if (err<0){
							ms_warning("Fail to write %i bytes from soundcard: %s",
								   bsize,strerror(errno));
						}
					}
				}

			}else {
				int sz;
				memset(wtmpbuff,0,bsize);
				sz = write(d->pcmfd,wtmpbuff,bsize);
				if( sz!=bsize) ms_warning("sound device write returned %i !",sz);
			}
		}else usleep(20000);
	}
	if (d->pcmfd>=0) {
		close(d->pcmfd);
		d->pcmfd=-1;
	}
	free(rtmpbuff);
	free(wtmpbuff);
	if (rm!=NULL) freemsg(rm);
	/*reset to default parameters */
	//d->bits=16;
	//d->rate=8000;
	//d->stereo=FALSE;
	return NULL;
}

static void oss_start_r(MSSndCard *card){
	OssData *d=(OssData*)card->data;
	ms_mutex_lock(&d->mutex);
	if (d->read_started==FALSE && d->write_started==FALSE){
		d->read_started=TRUE;
		ms_thread_create(&d->thread,NULL,oss_thread,card);
	}else d->read_started=TRUE;
	flushq(&d->rq,0);
	ms_mutex_unlock(&d->mutex);
}

static void oss_stop_r(MSSndCard *card){
	OssData *d=(OssData*)card->data;
	d->read_started=FALSE;
	if (d->write_started==FALSE){
		ms_thread_join(d->thread,NULL);
	}
}

static void _flush_buffer(MSBufferizer *obj){
	flushq(&obj->q,0);
	obj->size=0;
}

static void oss_start_w(MSSndCard *card){
	OssData *d=(OssData*)card->data;
	ms_mutex_lock(&d->mutex);
	if (d->read_started==FALSE && d->write_started==FALSE){
		d->write_started=TRUE;
		ms_thread_create(&d->thread,NULL,oss_thread,card);
	}else{
		d->write_started=TRUE;
	}
	_flush_buffer(d->bufferizer);
	ms_mutex_unlock(&d->mutex);
}

static void oss_stop_w(MSSndCard *card){
	OssData *d=(OssData*)card->data;
	d->write_started=FALSE;
	if (d->read_started==FALSE){
		ms_thread_join(d->thread,NULL);
	}
}

static mblk_t *oss_get(MSSndCard *card){
	OssData *d=(OssData*)card->data;
	mblk_t *m;
	ms_mutex_lock(&d->mutex);
	m=getq(&d->rq);
	ms_mutex_unlock(&d->mutex);
	return m;
}

static void oss_put(MSSndCard *card, mblk_t *m){
	OssData *d=(OssData*)card->data;
	ms_mutex_lock(&d->mutex);
	ms_bufferizer_put(d->bufferizer,m);
	ms_mutex_unlock(&d->mutex);
}


static void oss_read_preprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	oss_start_r(card);
}

static void oss_read_postprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	oss_stop_r(card);
}

static void oss_read_process(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	mblk_t *m;
	while((m=oss_get(card))!=NULL){
		ms_queue_put(f->outputs[0],m);
	}
}

static void oss_write_preprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	oss_start_w(card);
}

static void oss_write_postprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	oss_stop_w(card);
}

static void oss_write_process(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	mblk_t *m;
	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		oss_put(card,m);
	}
}

static int get_rate(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	OssData *d=(OssData*)card->data;
	*((int*)arg)=d->rate;
	return 0;
}

static int set_rate(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	OssData *d=(OssData*)card->data;
	d->rate=*((int*)arg);
	return 0;
}

static int set_nchannels(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	OssData *d=(OssData*)card->data;
	d->stereo=(*((int*)arg)==2);
	return 0;
}

static MSFilterMethod oss_methods[]={
	{	MS_FILTER_GET_SAMPLE_RATE	, get_rate	},
	{	MS_FILTER_SET_SAMPLE_RATE	, set_rate	},
	{	MS_FILTER_SET_NCHANNELS		, set_nchannels	},
	{	0				, NULL		}
};

MSFilterDesc oss_read_desc={
	.id=MS_OSS_READ_ID,
	.name="MSOssRead",
	.text=N_("Sound capture filter for OSS drivers"),
	.category=MS_FILTER_OTHER,
	.ninputs=0,
	.noutputs=1,
	.preprocess=oss_read_preprocess,
	.process=oss_read_process,
	.postprocess=oss_read_postprocess,
	.methods=oss_methods
};


MSFilterDesc oss_write_desc={
	.id=MS_OSS_WRITE_ID,
	.name="MSOssWrite",
	.text=N_("Sound playback filter for OSS drivers"),
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=0,
	.preprocess=oss_write_preprocess,
	.process=oss_write_process,
	.postprocess=oss_write_postprocess,
	.methods=oss_methods
};

MSFilter *ms_oss_read_new(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&oss_read_desc);
	f->data=card;
	return f;
}


MSFilter *ms_oss_write_new(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&oss_write_desc);
	f->data=card;
	return f;
}

MS_FILTER_DESC_EXPORT(oss_read_desc)
MS_FILTER_DESC_EXPORT(oss_write_desc)
