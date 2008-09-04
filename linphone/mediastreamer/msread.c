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

#include "msread.h"
#include "mssync.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

static MSReadClass *ms_read_class=NULL;

gint ms_read_open(MSRead *r, gchar *name);

MSFilter * ms_read_new(char *name)
{
	MSRead *r;
	
	r=g_new(MSRead,1);
	ms_read_init(r);
	if (ms_read_class==NULL)
	{
		ms_read_class=g_new(MSReadClass,1);
		ms_read_class_init(ms_read_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_read_class);
	r->fd=-1;
	if (name!=NULL) ms_read_open(r,name);
	return(MS_FILTER(r));
}
	


gint ms_read_open(MSRead *r, gchar *name)
{
	gint fd;
	fd=open(name,O_RDONLY);
	if (fd<0) {
		r->fd=-1;
		g_warning("ms_read_new: cannot open %s : %s",name,strerror(errno));
		return -1;
	}
	r->fd=fd;
	if (strstr(name,".wav")!=NULL){
		/* skip the header */
		lseek(fd,20,SEEK_SET);
#ifdef WORDS_BIGENDIAN
		r->need_swap=1;	
#else
		r->need_swap=0;
#endif
	}
	r->state=MS_READ_STATE_STARTED;
	return 0;
}

/* FOR INTERNAL USE*/
void ms_read_init(MSRead *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->outfifos=r->foutputs;
	MS_FILTER(r)->outqueues=r->qoutputs;
	memset(r->foutputs,0,sizeof(MSFifo*)*MSREAD_MAX_OUTPUTS);
	memset(r->qoutputs,0,sizeof(MSQueue*)*MSREAD_MAX_OUTPUTS);
	r->fd=-1;
	r->gran=320;
	r->state=MS_READ_STATE_STOPPED;
	r->need_swap=0;
	r->rate=8000;
}

gint ms_read_set_property(MSRead *f,MSFilterProperty prop, void *value)
{
	switch(prop){
		case MS_FILTER_PROPERTY_FREQ:
			f->rate=((gint*)value)[0];
		break;
		default:
		break;
	}
	return 0;
}

void ms_read_class_init(MSReadClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"dskreader");
	ms_filter_class_set_attr(MS_FILTER_CLASS(klass),FILTER_IS_SOURCE);
	MS_FILTER_CLASS(klass)->max_foutputs=MSREAD_MAX_OUTPUTS;
	MS_FILTER_CLASS(klass)->max_qoutputs=MSREAD_MAX_OUTPUTS;
	MS_FILTER_CLASS(klass)->w_maxgran=MSREAD_DEF_GRAN;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_read_destroy;
	MS_FILTER_CLASS(klass)->setup=(MSFilterSetupFunc)ms_read_setup;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_read_process;
	MS_FILTER_CLASS(klass)->set_property=(MSFilterPropertyFunc)ms_read_set_property;
}

void ms_read_process(MSRead *r)
{
	MSFifo *f;
	MSQueue *q;
	MSMessage *msg=NULL;
	int err;
	gint gran=r->gran;
	void *p;
	
	f=r->foutputs[0];
	if ((f!=NULL) && (r->state==MS_READ_STATE_STARTED))
	{
		ms_fifo_get_write_ptr(f,gran,&p);
		if (p!=NULL)
		{
			err=read(r->fd,p,gran);
			if (err<0)
			{
				/* temp: */
				g_warning("ms_read_process: failed to read: %s.\n",strerror(errno));
			}
			else if (err<gran){
				ms_trace("ms_read_process: end of file.");
				ms_filter_notify_event(MS_FILTER(r),MS_READ_EVENT_EOF,NULL);
				r->state=MS_READ_STATE_STOPPED;
				close(r->fd);
				r->fd=-1;
			}
			if (r->need_swap) swap_buffer(p,gran);
		}
	}
	/* process output queues*/
	q=r->qoutputs[0];
	if ((q!=NULL) && (r->fd>0))
	{
		msg=ms_message_new(r->gran);
		err=read(r->fd,msg->data,r->gran);
		if (err>0){
			msg->size=err;
			ms_queue_put(q,msg);
			if (r->need_swap) swap_buffer(msg->data,r->gran);
		}else{
			ms_filter_notify_event(MS_FILTER(r),MS_READ_EVENT_EOF,NULL);
			ms_trace("End of file reached.");
			r->state=MS_READ_STATE_STOPPED;
		}				
	}	
}

void ms_read_destroy( MSRead *obj)
{
	if (obj->fd!=0) close(obj->fd);
	g_free(obj);
}

gint ms_read_close(MSRead *obj)
{
	if (obj->fd!=0) {
		close(obj->fd);
		obj->fd=-1;
		obj->state=MS_READ_STATE_STOPPED;
	}
	return 0;
}


void ms_read_setup(MSRead *r, MSSync *sync)
{
	r->sync=sync;
	r->gran=(r->rate*sync->interval/1000)*2;
}
