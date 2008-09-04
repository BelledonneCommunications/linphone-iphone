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


#include "mscopy.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

static MSCopyClass *ms_copy_class=NULL;

MSFilter * ms_copy_new(void)
{
	MSCopy *r;
	
	r=g_new(MSCopy,1);
	ms_copy_init(r);
	if (ms_copy_class==NULL)
	{
		ms_copy_class=g_new(MSCopyClass,1);
		ms_copy_class_init(ms_copy_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_copy_class);
	return(MS_FILTER(r));
}
	

/* FOR INTERNAL USE*/
void ms_copy_init(MSCopy *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->infifos=r->f_inputs;
	MS_FILTER(r)->outfifos=r->f_outputs;
	MS_FILTER(r)->r_mingran=MSCOPY_DEF_GRAN;
	memset(r->f_inputs,0,sizeof(MSFifo*)*MSCOPY_MAX_INPUTS);
	memset(r->f_outputs,0,sizeof(MSFifo*)*MSCOPY_MAX_INPUTS);
}

void ms_copy_class_init(MSCopyClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"fifocopier");
	MS_FILTER_CLASS(klass)->max_finputs=MSCOPY_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->max_foutputs=MSCOPY_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->r_maxgran=MSCOPY_DEF_GRAN;
	MS_FILTER_CLASS(klass)->w_maxgran=MSCOPY_DEF_GRAN;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_copy_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_copy_process;
}
	
void ms_copy_process(MSCopy *r)
{
	MSFifo *fi,*fo;
	int err1;
	gint gran=MS_FILTER(r)->klass->r_maxgran;
	void *s,*d;
	
	/* process output fifos, but there is only one for this class of filter*/
	
	fi=r->f_inputs[0];
	fo=r->f_outputs[0];
	if (fi!=NULL)
	{
		err1=ms_fifo_get_read_ptr(fi,gran,&s);
		if (err1>0) err1=ms_fifo_get_write_ptr(fo,gran,&d);
		if (err1>0)
		{
			memcpy(d,s,gran);
		}
	}
}

void ms_copy_destroy( MSCopy *obj)
{
	g_free(obj);
}
