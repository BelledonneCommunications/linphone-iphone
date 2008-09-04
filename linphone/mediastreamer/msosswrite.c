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

#include "msosswrite.h"
#include "mssync.h"
#include <unistd.h>
#include <math.h>

MSFilterInfo oss_write_info={
	"OSS write",
	0,
	MS_FILTER_OTHER,
	ms_oss_write_new,
	NULL
};


static MSOssWriteClass *msosswriteclass=NULL;

MSFilter * ms_oss_write_new()
{
	MSOssWrite *w;
	
  if (msosswriteclass==NULL)
	{
		msosswriteclass=g_new(MSOssWriteClass,1);
		ms_oss_write_class_init( msosswriteclass );
	}
	w=g_new(MSOssWrite,1);
	MS_FILTER(w)->klass=MS_FILTER_CLASS(msosswriteclass);
	ms_oss_write_init(w);
	return(MS_FILTER(w));
}

/* FOR INTERNAL USE*/
void ms_oss_write_init(MSOssWrite *w)
{
	ms_sound_write_init(MS_SOUND_WRITE(w));
	MS_FILTER(w)->infifos=w->f_inputs;
	MS_FILTER(w)->infifos[0]=NULL;
	MS_FILTER(w)->r_mingran=512;  /* very few cards can do that...*/
	w->devid=0;
	w->sndcard=NULL;
	w->freq=8000;
	w->channels=1;
	w->dtmf_time=-1;
}
	
gint ms_oss_write_set_property(MSOssWrite *f,MSFilterProperty prop, void *value)
{
	switch(prop){
		case MS_FILTER_PROPERTY_FREQ:
			f->freq=((gint*)value)[0];
		break;
		case MS_FILTER_PROPERTY_CHANNELS:
			f->channels=((gint*)value)[0];
		break;
		default:
		break;
	}
	return 0;
}

void ms_oss_write_class_init(MSOssWriteClass *klass)
{
	ms_sound_write_class_init(MS_SOUND_WRITE_CLASS(klass));
	MS_FILTER_CLASS(klass)->max_finputs=1;  /* one fifo input only */
	MS_FILTER_CLASS(klass)->r_maxgran=MS_OSS_WRITE_DEF_GRAN;
	MS_FILTER_CLASS(klass)->process= (MSFilterProcessFunc)ms_oss_write_process;
	MS_FILTER_CLASS(klass)->destroy= (MSFilterDestroyFunc)ms_oss_write_destroy;
	MS_FILTER_CLASS(klass)->setup= (MSFilterSetupFunc)ms_oss_write_setup;
	MS_FILTER_CLASS(klass)->unsetup= (MSFilterSetupFunc)ms_oss_write_stop;
	MS_FILTER_CLASS(klass)->set_property=(MSFilterPropertyFunc)ms_oss_write_set_property;
	MS_FILTER_CLASS(klass)->info=&oss_write_info;
	MS_SOUND_WRITE_CLASS(klass)->set_device=(gint (*)(MSSoundWrite*,gint))ms_oss_write_set_device;
	MS_SOUND_WRITE_CLASS(klass)->start=(void (*)(MSSoundWrite*))ms_oss_write_start;
	MS_SOUND_WRITE_CLASS(klass)->stop=(void (*)(MSSoundWrite*))ms_oss_write_stop;
	MS_SOUND_WRITE_CLASS(klass)->set_level=(void (*)(MSSoundWrite*, gint))ms_oss_write_set_level;
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"OssWrite");
}

void ms_oss_write_destroy( MSOssWrite *obj)
{
	
	g_free(obj);
}

void ms_oss_write_process(MSOssWrite *f)
{
	MSFifo *fifo;
	void *p;
	int i;
	gint gran=ms_filter_get_mingran(MS_FILTER(f));
	
	/* always consume something */
	fifo=f->f_inputs[0];
	ms_fifo_get_read_ptr(fifo,gran,&p);
	if (p==NULL) {
		g_warning("Not enough data: gran=%i.",gran);
		return;
	}
	g_return_if_fail(f->sndcard!=NULL);
	if (f->dtmf_time!=-1){
		gint16 *buf=(gint16*)p;
		/* generate a DTMF*/
		for(i=0;i<gran/2;i++){
			buf[i]=(gint16)(10000.0*sin(2*M_PI*(double)f->dtmf_time*f->lowfreq));
			buf[i]+=(gint16)(10000.0*sin(2*M_PI*(double)f->dtmf_time*f->highfreq));
			f->dtmf_time++;
			//printf("buf[%i]=%i\n",i,buf[i]);
		}
		if (f->dtmf_time>f->dtmf_duration) f->dtmf_time=-1; /*finished*/
	}
	snd_card_write(f->sndcard,p,gran);
}

void ms_oss_write_start(MSOssWrite *w)
{
	//gint bsize;
	g_return_if_fail(w->devid!=-1);
	w->sndcard=snd_card_manager_get_card(snd_card_manager,w->devid);
	g_return_if_fail(w->sndcard!=NULL);
	/* open the device for an audio telephony signal with minimum latency */
	snd_card_open_w(w->sndcard,16,w->channels==2,w->freq);
	w->bsize=snd_card_get_bsize(w->sndcard);
	//MS_FILTER(w)->r_mingran=w->bsize;
	//ms_sync_set_samples_per_tick(MS_FILTER(w)->sync,bsize);
}

void ms_oss_write_stop(MSOssWrite *w)
{
	g_return_if_fail(w->devid!=-1);
	g_return_if_fail(w->sndcard!=NULL);
	snd_card_close_w(w->sndcard);
	w->sndcard=NULL;
}

void ms_oss_write_set_level(MSOssWrite *w,gint a)
{
	
}

gint ms_oss_write_set_device(MSOssWrite *w, gint devid)
{
	w->devid=devid;
	return 0;
}

void ms_oss_write_setup(MSOssWrite *r)
{
	//g_message("starting MSOssWrite..");
	ms_oss_write_start(r);
}



void ms_oss_write_play_dtmf(MSOssWrite *w, char dtmf){
	
	w->dtmf_duration=0.1*w->freq;
	switch(dtmf){
		case '0':
			w->lowfreq=941;
			w->highfreq=1336;
			break;
		case '1':
			w->lowfreq=697;
			w->highfreq=1209;
			break;
		case '2':
			w->lowfreq=697;
			w->highfreq=1336;
			break;
		case '3':
			w->lowfreq=697;
			w->highfreq=1477;
			break;
		case '4':
			w->lowfreq=770;
			w->highfreq=1209;
			break;
		case '5':
			w->lowfreq=770;
			w->highfreq=1336;
			break;
		case '6':
			w->lowfreq=770;
			w->highfreq=1477;
			break;
		case '7':
			w->lowfreq=852;
			w->highfreq=1209;
			break;
		case '8':
			w->lowfreq=852;
			w->highfreq=1336;
			break;
		case '9':
			w->lowfreq=852;
			w->highfreq=1477;
			break;
		case '*':
			w->lowfreq=941;
			w->highfreq=1209;
			break;
		case '#':
			w->lowfreq=941;
			w->highfreq=1477;
			break;
		case 'A':
			w->lowfreq=697;
			w->highfreq=1633;
			break;
		case 'B':
			w->lowfreq=770;
			w->highfreq=1633;
			break;
		case 'C':
			w->lowfreq=852;
			w->highfreq=1633;
			break;
		case 'D':
			w->lowfreq=941;
			w->highfreq=1633;
			break;	
		default:
			g_warning("Not a dtmf key.");
			return;
	}
	w->lowfreq=w->lowfreq/w->freq;
	w->highfreq=w->highfreq/w->freq;
	w->dtmf_time=0;
}
