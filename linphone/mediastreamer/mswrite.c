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

#include "mswrite.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

static MSWriteClass *ms_write_class=NULL;

#ifdef _WIN32
#	define FILEPERMS (S_IRUSR|S_IWUSR)
#else
#	define FILEPERMS (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#endif

MSFilter * ms_write_new(char *name)
{
	MSWrite *r;
	int fd=-1;
	
	r=g_new(MSWrite,1);
	ms_write_init(r);
	if (ms_write_class==NULL)
	{
		ms_write_class=g_new(MSWriteClass,1);
		ms_write_class_init(ms_write_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_write_class);
	if ((name!=NULL) && (strlen(name)!=0))
	{
		fd=open(name,O_WRONLY | O_CREAT | O_TRUNC,FILEPERMS);
		if (fd<0) g_error("ms_write_new: failed to open %s.\n",name);
	}
	r->fd=fd;
	return(MS_FILTER(r));
}
	

/* FOR INTERNAL USE*/
void ms_write_init(MSWrite *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->infifos=r->f_inputs;
	MS_FILTER(r)->inqueues=r->q_inputs;
	MS_FILTER(r)->r_mingran=MSWRITE_MIN_GRAN;
	memset(r->f_inputs,0,sizeof(MSFifo*)*MSWRITE_MAX_INPUTS);
	memset(r->q_inputs,0,sizeof(MSQueue*)*MSWRITE_MAX_INPUTS);
	r->fd=-1;
}

void ms_write_class_init(MSWriteClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"dskwriter");
	MS_FILTER_CLASS(klass)->max_finputs=MSWRITE_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->max_qinputs=MSWRITE_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->r_maxgran=MSWRITE_DEF_GRAN;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_write_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_write_process;
}
	
void ms_write_process(MSWrite *r)
{
	MSFifo *f;
	MSQueue *q;
	MSMessage *buf=NULL;
	int i,j,err1,err2;
	gint gran=ms_filter_get_mingran(MS_FILTER(r));
	void *p;
	
	/* process output fifos*/
	for (i=0,j=0;(i<MS_FILTER(r)->klass->max_finputs)&&(j<MS_FILTER(r)->finputs);i++)
	{
		f=r->f_inputs[i];
		if (f!=NULL)
		{
			if ( (err1=ms_fifo_get_read_ptr(f,gran,&p))>0 )
			{
			
				err2=write(r->fd,p,gran);
				if (err2<0) g_warning("ms_write_process: failed to write: %s.\n",strerror(errno));
			}
			j++;
		}
	}
	/* process output queues*/
	for (i=0,j=0;(i<MS_FILTER(r)->klass->max_qinputs)&&(j<MS_FILTER(r)->qinputs);i++)
	{
		q=r->q_inputs[i];
		if (q!=NULL)
		{
			while ( (buf=ms_queue_get(q))!=NULL ){
				int ret = write(r->fd,buf->data,buf->size);
				assert( ret == buf->size );
				j++;	
				ms_message_destroy(buf);
			}
		}				
	}	
}

void ms_write_destroy( MSWrite *obj)
{
	if (obj->fd!=0) close(obj->fd);
	g_free(obj);
}

