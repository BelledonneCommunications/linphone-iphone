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


#include "msqdispatcher.h"

static MSQdispatcherClass *ms_qdispatcher_class=NULL;

MSFilter * ms_qdispatcher_new(void)
{
	MSQdispatcher *obj;
	obj=g_malloc(sizeof(MSQdispatcher));
	if (ms_qdispatcher_class==NULL){
		ms_qdispatcher_class=g_malloc0(sizeof(MSQdispatcherClass));
		ms_qdispatcher_class_init(ms_qdispatcher_class);
	}
	MS_FILTER(obj)->klass=MS_FILTER_CLASS(ms_qdispatcher_class);
	ms_qdispatcher_init(obj);
	return MS_FILTER(obj);
}


void ms_qdispatcher_init(MSQdispatcher *obj)
{
	ms_filter_init(MS_FILTER(obj));
	
	MS_FILTER(obj)->inqueues=obj->q_inputs;
	MS_FILTER(obj)->outqueues=obj->q_outputs;
	memset(obj->q_inputs,0,sizeof(MSQueue*)*MS_QDISPATCHER_MAX_INPUTS);
	memset(obj->q_outputs,0,sizeof(MSQueue*)*MS_QDISPATCHER_MAX_OUTPUTS);
}



void ms_qdispatcher_class_init(MSQdispatcherClass *klass)
{
	MSFilterClass *parent_class=MS_FILTER_CLASS(klass);
	ms_filter_class_init(parent_class);
	ms_filter_class_set_name(parent_class,"qdispatcher");
	parent_class->max_qinputs=MS_QDISPATCHER_MAX_INPUTS;
	parent_class->max_qoutputs=MS_QDISPATCHER_MAX_OUTPUTS;
	
	parent_class->destroy=(MSFilterDestroyFunc)ms_qdispatcher_destroy;
	parent_class->process=(MSFilterProcessFunc)ms_qdispatcher_process;
}


void ms_qdispatcher_destroy( MSQdispatcher *obj)
{
	g_free(obj);
}

void ms_qdispatcher_process(MSQdispatcher *obj)
{
	gint i;
	MSQueue *inq=obj->q_inputs[0];
	
	if (inq!=NULL){
		MSQueue *outq;
		MSMessage *m1,*m2;
		while ( (m1=ms_queue_get(inq))!=NULL){
			/* dispatch incoming messages to output queues */
			for (i=0;i<MS_QDISPATCHER_MAX_OUTPUTS;i++){
				outq=obj->q_outputs[i];
				if (outq!=NULL){
					m2=ms_message_dup(m1);
					ms_queue_put(outq,m2);
				}
			}
			ms_message_destroy(m1);
		}
	}
	
}
