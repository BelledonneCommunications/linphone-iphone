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

  You should have received a dispatcher of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "msfdispatcher.h"

static MSFdispatcherClass *ms_fdispatcher_class=NULL;

MSFilter * ms_fdispatcher_new(void)
{
	MSFdispatcher *obj;
	obj=g_malloc(sizeof(MSFdispatcher));
	if (ms_fdispatcher_class==NULL){
		ms_fdispatcher_class=g_malloc(sizeof(MSFdispatcherClass));
		ms_fdispatcher_class_init(ms_fdispatcher_class);
	}
	MS_FILTER(obj)->klass=MS_FILTER_CLASS(ms_fdispatcher_class);
	ms_fdispatcher_init(obj);
	return MS_FILTER(obj);
}


void ms_fdispatcher_init(MSFdispatcher *obj)
{
	ms_filter_init(MS_FILTER(obj));
	MS_FILTER(obj)->infifos=obj->f_inputs;
	MS_FILTER(obj)->outfifos=obj->f_outputs;
	MS_FILTER(obj)->r_mingran=MS_FDISPATCHER_DEF_GRAN;
	memset(obj->f_inputs,0,sizeof(MSFifo*)*MS_FDISPATCHER_MAX_INPUTS);
	memset(obj->f_outputs,0,sizeof(MSFifo*)*MS_FDISPATCHER_MAX_OUTPUTS);
}



void ms_fdispatcher_class_init(MSFdispatcherClass *klass)
{
	MSFilterClass *parent_class=MS_FILTER_CLASS(klass);
	ms_filter_class_init(parent_class);
	ms_filter_class_set_name(parent_class,"fdispatcher");
	parent_class->max_finputs=MS_FDISPATCHER_MAX_INPUTS;
	parent_class->max_foutputs=MS_FDISPATCHER_MAX_OUTPUTS;
	parent_class->r_maxgran=MS_FDISPATCHER_DEF_GRAN;
	parent_class->w_maxgran=MS_FDISPATCHER_DEF_GRAN;
	parent_class->destroy=(MSFilterDestroyFunc)ms_fdispatcher_destroy;
	parent_class->process=(MSFilterProcessFunc)ms_fdispatcher_process;
}


void ms_fdispatcher_destroy( MSFdispatcher *obj)
{
	g_free(obj);
}

void ms_fdispatcher_process(MSFdispatcher *obj)
{
	gint i;
	MSFifo *inf=obj->f_inputs[0];
	
	
	if (inf!=NULL){
		void *s,*d;
		/* dispatch fifos */
		while ( ms_fifo_get_read_ptr(inf,MS_FDISPATCHER_DEF_GRAN,&s) >0 ){
			for (i=0;i<MS_FDISPATCHER_MAX_OUTPUTS;i++){
				MSFifo *outf=obj->f_outputs[i];
			
				if (outf!=NULL)
				{
					ms_fifo_get_write_ptr(outf,MS_FDISPATCHER_DEF_GRAN,&d);
					if (d!=NULL) memcpy(d,s,MS_FDISPATCHER_DEF_GRAN);
				}
			}	
		}
	}
}


