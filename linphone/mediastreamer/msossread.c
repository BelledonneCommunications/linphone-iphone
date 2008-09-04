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

#include "msossread.h"
#include "mssync.h"
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>

MSFilterInfo oss_read_info={
	"OSS read",
	0,
	MS_FILTER_AUDIO_IO,
	ms_oss_read_new,
	NULL
};

static MSOssReadClass *msossreadclass=NULL;

MSFilter * ms_oss_read_new()
{
	MSOssRead *w;
	
	if (msossreadclass==NULL)
	{
		msossreadclass=g_new(MSOssReadClass,1);
		ms_oss_read_class_init( msossreadclass );
	}
	
	w=g_new(MSOssRead,1);
	MS_FILTER(w)->klass=MS_FILTER_CLASS(msossreadclass);
	ms_oss_read_init(w);
	
	return(MS_FILTER(w));
}

/* FOR INTERNAL USE*/
void ms_oss_read_init(MSOssRead *w)
{
	ms_sound_read_init(MS_SOUND_READ(w));
	MS_FILTER(w)->outfifos=w->f_outputs;
	MS_FILTER(w)->outfifos[0]=NULL;
	w->devid=0;
	w->sndcard=NULL;
	w->freq=8000;
}
	
gint ms_oss_read_set_property(MSOssRead *f,MSFilterProperty prop, void *value)
{
	switch(prop){
		case MS_FILTER_PROPERTY_FREQ:
			f->freq=((gint*)value)[0];
		break;
		default:
		break;
	}
	return 0;
}
void ms_oss_read_class_init(MSOssReadClass *klass)
{
	ms_sound_read_class_init(MS_SOUND_READ_CLASS(klass));
	MS_FILTER_CLASS(klass)->max_foutputs=1;  /* one fifo output only */
	MS_FILTER_CLASS(klass)->setup=(MSFilterSetupFunc)ms_oss_read_setup;
	MS_FILTER_CLASS(klass)->unsetup=(MSFilterSetupFunc)ms_oss_read_stop;
	MS_FILTER_CLASS(klass)->process= (MSFilterProcessFunc)ms_oss_read_process;
	MS_FILTER_CLASS(klass)->set_property=(MSFilterPropertyFunc)ms_oss_read_set_property;
	MS_FILTER_CLASS(klass)->destroy= (MSFilterDestroyFunc)ms_oss_read_destroy;
	MS_FILTER_CLASS(klass)->w_maxgran=MS_OSS_READ_MAX_GRAN;
	MS_FILTER_CLASS(klass)->info=&oss_read_info;
	MS_SOUND_READ_CLASS(klass)->set_device=(gint (*)(MSSoundRead*,gint))ms_oss_read_set_device;
	MS_SOUND_READ_CLASS(klass)->start=(void (*)(MSSoundRead*))ms_oss_read_start;
	MS_SOUND_READ_CLASS(klass)->stop=(void (*)(MSSoundRead*))ms_oss_read_stop;
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"OssRead");
	//ms_filter_class_set_attr( MS_FILTER_CLASS(klass),FILTER_CAN_SYNC|FILTER_IS_SOURCE);	
}

void ms_oss_read_destroy( MSOssRead *obj)
{
	g_free(obj);
}

void ms_oss_read_process(MSOssRead *f)
{
	MSFifo *fifo;
	char *p;
	fifo=f->f_outputs[0];
	
	g_return_if_fail(f->sndcard!=NULL);
	g_return_if_fail(f->gran>0);
	
	if (snd_card_can_read(f->sndcard)){
		int got;
		ms_fifo_get_write_ptr(fifo,f->gran,(void**)&p);
		g_return_if_fail(p!=NULL);
		got=snd_card_read(f->sndcard,p,f->gran);
		if (got>=0 && got!=f->gran) ms_fifo_update_write_ptr(fifo,got);
	}		
}


void ms_oss_read_start(MSOssRead *r)
{
	g_return_if_fail(r->devid!=-1);
	r->sndcard=snd_card_manager_get_card(snd_card_manager,r->devid);
	g_return_if_fail(r->sndcard!=NULL);
	/* open the device for an audio telephony signal with minimum latency */
	snd_card_open_r(r->sndcard,16,0,r->freq);
	r->gran=(512*r->freq)/8000;
	
}

void ms_oss_read_stop(MSOssRead *w)
{
	g_return_if_fail(w->devid!=-1);
	g_return_if_fail(w->sndcard!=NULL);
	snd_card_close_r(w->sndcard);
	w->sndcard=NULL;
}


void ms_oss_read_setup(MSOssRead *f, MSSync *sync)
{
	f->sync=sync;
	ms_oss_read_start(f);
}


gint ms_oss_read_set_device(MSOssRead *r,gint devid)
{
	r->devid=devid;
	return 0;
}
