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

#include "msringplayer.h"
#include "mssync.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "waveheader.h"

#define WAVE_HEADER_OFFSET sizeof(wave_header_t)

enum { PLAY_RING, PLAY_SILENCE};

static int supported_freq[6]={8000,11025,16000,22050,32000,44100};

gint ms_ring_player_set_property(MSRingPlayer *f,MSFilterProperty prop, void *value);

gint freq_is_supported(gint freq){
	int i;
	for (i=0;i<6;i++){
		if (abs(supported_freq[i]-freq)<50) return supported_freq[i];
	}
	return 0;
}

static MSRingPlayerClass *ms_ring_player_class=NULL;

/**
 * ms_ring_player_new:
 * @name:  The path to the 16-bit 8khz raw file to be played as a ring.
 * @seconds: The number of seconds that separates two rings.
 *
 * Allocates a new MSRingPlayer object.
 *
 *
 * Returns: a pointer the the object, NULL if name could not be open.
 */
MSFilter * ms_ring_player_new(char *name, gint seconds)
{
	MSRingPlayer *r;
	int fd=-1;
	
	if ((name!=NULL) && (strlen(name)!=0))
	{
		fd=open(name,O_RDONLY);
		if (fd<0) 
		{
			g_warning("ms_ring_player_new: failed to open %s.\n",name);
			return NULL;
		}
		
	}else {
		g_warning("ms_ring_player_new: Bad file name");
		return NULL;
	}
	
	r=g_new(MSRingPlayer,1);
	ms_ring_player_init(r);
	if (ms_ring_player_class==NULL)
	{
		ms_ring_player_class=g_new(MSRingPlayerClass,1);
		ms_ring_player_class_init(ms_ring_player_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_ring_player_class);
	
	r->fd=fd;
	r->silence=seconds;
	r->freq=8000;
	if (strstr(name,".wav")!=NULL){
		wave_header_t header;
		int freq,freq2;
		/* read the header */
		size_t ret = read(fd,&header,sizeof(wave_header_t));
		assert( ret == sizeof(wave_header_t) );
		freq=wave_header_get_rate(&header);
		if ((freq2=freq_is_supported(freq))>0){
			r->freq=freq2;
		}else {
			g_warning("Unsupported sampling rate %i",freq);
			r->freq=8000;
		}
		r->channel=wave_header_get_channel(&header);
		lseek(fd,WAVE_HEADER_OFFSET,SEEK_SET);
#ifdef WORDS_BIGENDIAN
		r->need_swap=1;	
#else
		r->need_swap=0;
#endif
	}
	ms_ring_player_set_property(r, MS_FILTER_PROPERTY_FREQ,&r->freq);
	r->state=PLAY_RING;
	return(MS_FILTER(r));
}
	

/* FOR INTERNAL USE*/
void ms_ring_player_init(MSRingPlayer *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->outfifos=r->foutputs;
	MS_FILTER(r)->outqueues=r->qoutputs;
	memset(r->foutputs,0,sizeof(MSFifo*)*MS_RING_PLAYER_MAX_OUTPUTS);
	memset(r->qoutputs,0,sizeof(MSQueue*)*MS_RING_PLAYER_MAX_OUTPUTS);
	r->fd=-1;
	r->current_pos=0;
	r->need_swap=0;
	r->sync=NULL;
}

gint ms_ring_player_set_property(MSRingPlayer *f,MSFilterProperty prop, void *value)
{
	switch(prop){
		case MS_FILTER_PROPERTY_FREQ:
			f->rate=((gint*)value)[0]*2;
			f->silence_bytes=f->silence*f->rate;
			if (f->sync!=NULL)
				f->gran=(f->rate*f->sync->interval/1000)*2;
		break;
		default:
		break;
	}
	return 0;
}

gint ms_ring_player_get_property(MSRingPlayer *f,MSFilterProperty prop, void *value)
{
	switch(prop){
		case MS_FILTER_PROPERTY_FREQ:
			((gint*)value)[0]=f->freq;
			
		break;
		case MS_FILTER_PROPERTY_CHANNELS:
			((gint*)value)[0]=f->channel;
		break;
		default:
		break;
	}
	return 0;
}

gint ms_ring_player_get_sample_freq(MSRingPlayer *obj){
	return obj->freq;
}


void ms_ring_player_class_init(MSRingPlayerClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"ringplay");
	ms_filter_class_set_attr(MS_FILTER_CLASS(klass),FILTER_IS_SOURCE);
	MS_FILTER_CLASS(klass)->max_foutputs=MS_RING_PLAYER_MAX_OUTPUTS;
	MS_FILTER_CLASS(klass)->max_qoutputs=MS_RING_PLAYER_MAX_OUTPUTS;
	MS_FILTER_CLASS(klass)->w_maxgran=MS_RING_PLAYER_DEF_GRAN;
	MS_FILTER_CLASS(klass)->setup=(MSFilterSetupFunc)ms_ring_player_setup;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_ring_player_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_ring_player_process;
	MS_FILTER_CLASS(klass)->set_property=(MSFilterPropertyFunc)ms_ring_player_set_property;
	MS_FILTER_CLASS(klass)->get_property=(MSFilterPropertyFunc)ms_ring_player_get_property;
}
	
void ms_ring_player_process(MSRingPlayer *r)
{
	MSFifo *f;
	gint err;
	gint processed=0;
	gint gran=r->gran;
	char *p;
	
	g_return_if_fail(gran>0);
	/* process output fifos*/
	
	f=r->foutputs[0];
	ms_fifo_get_write_ptr(f,gran,(void**)&p);
	g_return_if_fail(p!=NULL);
	for (processed=0;processed<gran;){
		switch(r->state){
			case PLAY_RING:
				err=read(r->fd,&p[processed],gran-processed);
				if (err<0)
				{
					memset(&p[processed],0,gran-processed);
					processed=gran;
					g_warning("ms_ring_player_process: failed to read: %s.\n",strerror(errno));
					return;
				}
				else if (err<gran)
				{/* end of file */
					
					r->current_pos=r->silence_bytes;
					lseek(r->fd,WAVE_HEADER_OFFSET,SEEK_SET);
					r->state=PLAY_SILENCE;
          ms_filter_notify_event(MS_FILTER(r),MS_RING_PLAYER_END_OF_RING_EVENT,NULL);
				}
				if (r->need_swap) swap_buffer(&p[processed],err);
				processed+=err;
			break;
			case PLAY_SILENCE:
				err=gran-processed;
				if  (r->current_pos>err){
					memset(&p[processed],0,err);
					r->current_pos-=gran;
					processed=gran;
				}else{
					memset(&p[processed],0,r->current_pos);
					processed+=r->current_pos;
					r->state=PLAY_RING;
				}
			break;
		}
	}
}

/**
 * ms_ring_player_destroy:
 * @obj: A valid MSRingPlayer object.
 *
 * Destroy a MSRingPlayer object.
 *
 *
 */

void ms_ring_player_destroy( MSRingPlayer *obj)
{
	if (obj->fd!=0) close(obj->fd);
	g_free(obj);
}

void ms_ring_player_setup(MSRingPlayer *r,MSSync *sync)
{
	r->sync=sync;
	r->gran=(r->rate*r->sync->interval/1000)*r->channel;
}
