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


#include <errno.h>
#include "msfilter.h"



void ms_filter_init(MSFilter *filter)
{
	filter->finputs=0;
	filter->foutputs=0;
	filter->qinputs=0;
	filter->qoutputs=0;
	filter->infifos=NULL;
	filter->outfifos=NULL;
	filter->inqueues=NULL;
	filter->outqueues=NULL;
	filter->lock=g_mutex_new();
	filter->min_fifo_size=0x7fff;
	filter->notify_event=NULL;
	filter->userdata=NULL;
}

void ms_filter_uninit(MSFilter *filter)
{
	g_mutex_free(filter->lock);
}

void ms_filter_class_init(MSFilterClass *filterclass)
{
	filterclass->name=NULL;
	filterclass->max_finputs=0;
	filterclass->max_foutputs=0;
	filterclass->max_qinputs=0;
	filterclass->max_qoutputs=0;
	filterclass->r_maxgran=0;
	filterclass->w_maxgran=0;
	filterclass->r_offset=0;
	filterclass->w_offset=0;
	filterclass->set_property=NULL;
	filterclass->get_property=NULL;
	filterclass->setup=NULL;
	filterclass->unsetup=NULL;
	filterclass->process=NULL;
	filterclass->destroy=NULL;
	filterclass->attributes=0;
	filterclass->ref_count=0;
}

/* find output queue */
gint find_oq(MSFilter *m1,MSQueue *oq)
{
	gint i;
	
	for (i=0;i<MS_FILTER_GET_CLASS(m1)->max_qoutputs;i++){
			if (m1->outqueues[i]==oq) return i;
	}
	
	return -1;
}

/* find input queue */
gint find_iq(MSFilter *m1,MSQueue *iq)
{
	gint i;
	for (i=0;i<MS_FILTER_GET_CLASS(m1)->max_qinputs;i++){
		if (m1->inqueues[i]==iq) return i;
	}
	return -1;
}

/* find output fifo */
gint find_of(MSFilter *m1,MSFifo *of)
{
	gint i;
	for (i=0;i<MS_FILTER_GET_CLASS(m1)->max_foutputs;i++){
		if (m1->outfifos[i]==of) return i;
	}
	
	return -1;
}

/* find input fifo */
gint find_if(MSFilter *m1,MSFifo *inf)
{
	gint i;
	
	for (i=0;i<MS_FILTER_GET_CLASS(m1)->max_finputs;i++){
		if (m1->infifos[i]==inf) return i;
	}
	
	return -1;
}

#define find_free_iq(_m1)	find_iq(_m1,NULL)
#define find_free_oq(_m1)	find_oq(_m1,NULL)
#define find_free_if(_m1)	find_if(_m1,NULL)
#define find_free_of(_m1)	find_of(_m1,NULL)

int ms_filter_add_link(MSFilter *m1, MSFilter *m2)
{
	gint m1_q=-1;
	gint m1_f=-1;
	gint m2_q=-1;
	gint m2_f=-1;
	/* determine the type of link we can add */
	m1_q=find_free_oq(m1);
	m1_f=find_free_of(m1);
	m2_q=find_free_iq(m2);
	m2_f=find_free_if(m2);
	if ((m1_q!=-1) && (m2_q!=-1)){
		/* link with queues */
		ms_trace("m1_q=%i , m2_q=%i",m1_q,m2_q);
		return ms_filter_link(m1,m1_q,m2,m2_q,LINK_QUEUE);
	}
	if ((m1_f!=-1) && (m2_f!=-1)){
		/* link with queues */
		ms_trace("m1_f=%i , m2_f=%i",m1_f,m2_f);
		return ms_filter_link(m1,m1_f,m2,m2_f,LINK_FIFO);
	}
	g_warning("ms_filter_add_link: could not link.");
	return -1;
}
/**
 * ms_filter_link:
 * @m1:  A #MSFilter object.
 * @pin1:  The pin number on @m1.
 * @m2:  A #MSFilter object.
 * @pin2: The pin number on @m2.
 * @linktype: Type of connection, it may be #LINK_QUEUE, #LINK_FIFOS.
 *
 * This function links two MSFilter object between them. It must be used to make chains of filters.
 * All data outgoing from pin1 of m1 will go to the input pin2 of m2.
 * The way to communicate can be fifos or queues, depending of the nature of the filters. Filters can have
 * multiple queue pins and multiple fifo pins, but most of them have only one queue input/output or only one
 * fifo input/output. Fifos are usally used by filters doing audio processing, while queues are used by filters doing
 * video processing.
 *
 * Returns: 0 if successfull, a negative value reprensenting the errno.h error.
 */
int ms_filter_link(MSFilter *m1, gint pin1, MSFilter *m2,gint pin2, int linktype)
{
	MSQueue *q;
	MSFifo *fifo;
	
	g_message("ms_filter_add_link: %s,%i -> %s,%i",m1->klass->name,pin1,m2->klass->name,pin2);
	switch(linktype)
	{
	case LINK_QUEUE:
		/* Are filter m1 and m2 able to accept more queues connections ?*/
		g_return_val_if_fail(m1->qoutputs<MS_FILTER_GET_CLASS(m1)->max_qoutputs,-EMLINK);
		g_return_val_if_fail(m2->qinputs<MS_FILTER_GET_CLASS(m2)->max_qinputs,-EMLINK);
		/* Are filter m1 and m2 valid with their inputs and outputs ?*/
		g_return_val_if_fail(m1->outqueues!=NULL,-EFAULT);
		g_return_val_if_fail(m2->inqueues!=NULL,-EFAULT);
		/* are the requested pins exists ?*/
		g_return_val_if_fail(pin1<MS_FILTER_GET_CLASS(m1)->max_qoutputs,-EINVAL);
		g_return_val_if_fail(pin2<MS_FILTER_GET_CLASS(m2)->max_qinputs,-EINVAL);
		/* are the requested pins free ?*/
		g_return_val_if_fail(m1->outqueues[pin1]==NULL,-EBUSY);
		g_return_val_if_fail(m2->inqueues[pin2]==NULL,-EBUSY);
		
		q=ms_queue_new();
		m1->outqueues[pin1]=m2->inqueues[pin2]=q;
		m1->qoutputs++;
		m2->qinputs++;
		q->prev_data=(void*)m1;
		q->next_data=(void*)m2;
		break;
	case LINK_FIFO:
		/* Are filter m1 and m2 able to accept more fifo connections ?*/
		g_return_val_if_fail(m1->foutputs<MS_FILTER_GET_CLASS(m1)->max_foutputs,-EMLINK);
		g_return_val_if_fail(m2->finputs<MS_FILTER_GET_CLASS(m2)->max_finputs,-EMLINK);
		/* Are filter m1 and m2 valid with their inputs and outputs ?*/
		g_return_val_if_fail(m1->outfifos!=NULL,-EFAULT);
		g_return_val_if_fail(m2->infifos!=NULL,-EFAULT);
		/* are the requested pins exists ?*/
		g_return_val_if_fail(pin1<MS_FILTER_GET_CLASS(m1)->max_foutputs,-EINVAL);
		g_return_val_if_fail(pin2<MS_FILTER_GET_CLASS(m2)->max_finputs,-EINVAL);
		/* are the requested pins free ?*/
		g_return_val_if_fail(m1->outfifos[pin1]==NULL,-EBUSY);
		g_return_val_if_fail(m2->infifos[pin2]==NULL,-EBUSY);
		
		if (MS_FILTER_GET_CLASS(m1)->attributes & FILTER_IS_SOURCE)
		{
			/* configure min_fifo_size */
			fifo=ms_fifo_new_with_buffer(MS_FILTER_GET_CLASS(m2)->r_maxgran,
										MS_FILTER_GET_CLASS(m1)->w_maxgran,
										MS_FILTER_GET_CLASS(m2)->r_offset,
										MS_FILTER_GET_CLASS(m1)->w_offset,
										MS_FILTER_GET_CLASS(m1)->w_maxgran);
			m2->min_fifo_size=MS_FILTER_GET_CLASS(m1)->w_maxgran;
		}
		else
		{
			gint next_size;
			ms_trace("ms_filter_add_link: min_fifo_size=%i",m1->min_fifo_size);
			fifo=ms_fifo_new_with_buffer(MS_FILTER_GET_CLASS(m2)->r_maxgran,
										MS_FILTER_GET_CLASS(m1)->w_maxgran,
										MS_FILTER_GET_CLASS(m2)->r_offset,
										MS_FILTER_GET_CLASS(m1)->w_offset,
										m1->min_fifo_size);
			if (MS_FILTER_GET_CLASS(m2)->r_maxgran>0){
				next_size=(m1->min_fifo_size*
										(MS_FILTER_GET_CLASS(m2)->w_maxgran)) /
										(MS_FILTER_GET_CLASS(m2)->r_maxgran);
			}else next_size=m1->min_fifo_size;
			ms_trace("ms_filter_add_link: next_size=%i",next_size);
			m2->min_fifo_size=next_size;
		}
		
		
		m1->outfifos[pin1]=m2->infifos[pin2]=fifo;						
		m1->foutputs++;
		m2->finputs++;							
		fifo->prev_data=(void*)m1;
		fifo->next_data=(void*)m2;
		break;
	}
	return 0;
}
/**
 * ms_filter_unlink:
 * @m1:  A #MSFilter object.
 * @pin1:  The pin number on @m1.
 * @m2:  A #MSFilter object.
 * @pin2: The pin number on @m2.
 * @linktype: Type of connection, it may be #LINK_QUEUE, #LINK_FIFOS.
 *
 * Unlink @pin1 of filter @m1 from @pin2 of filter @m2. @linktype specifies what type of connection is removed.
 *
 * Returns: 0 if successfull, a negative value reprensenting the errno.h error.
 */
int ms_filter_unlink(MSFilter *m1, gint pin1, MSFilter *m2,gint pin2,gint linktype)
{
	switch(linktype)
	{
	case LINK_QUEUE:
		/* Are filter m1 and m2 valid with their inputs and outputs ?*/
		g_return_val_if_fail(m1->outqueues!=NULL,-EFAULT);
		g_return_val_if_fail(m2->inqueues!=NULL,-EFAULT);
		/* are the requested pins exists ?*/
		g_return_val_if_fail(pin1<MS_FILTER_GET_CLASS(m1)->max_qoutputs,-EINVAL);
		g_return_val_if_fail(pin2<MS_FILTER_GET_CLASS(m2)->max_qinputs,-EINVAL);
		/* are the requested pins busy ?*/
		g_return_val_if_fail(m1->outqueues[pin1]!=NULL,-ENOENT);
		g_return_val_if_fail(m2->inqueues[pin2]!=NULL,-ENOENT);
		/* are the two pins connected together ?*/
		g_return_val_if_fail(m1->outqueues[pin1]==m2->inqueues[pin2],-EINVAL);
		
		ms_queue_destroy(m1->outqueues[pin1]);
		m1->outqueues[pin1]=m2->inqueues[pin2]=NULL;
		m1->qoutputs--;
		m2->qinputs--;
		
		break;
	case LINK_FIFO:
		/* Are filter m1 and m2 valid with their inputs and outputs ?*/
		g_return_val_if_fail(m1->outfifos!=NULL,-EFAULT);
		g_return_val_if_fail(m2->infifos!=NULL,-EFAULT);
		/* are the requested pins exists ?*/
		g_return_val_if_fail(pin1<MS_FILTER_GET_CLASS(m1)->max_foutputs,-EINVAL);
		g_return_val_if_fail(pin2<MS_FILTER_GET_CLASS(m2)->max_finputs,-EINVAL);
		/* are the requested pins busy ?*/
		g_return_val_if_fail(m1->outfifos[pin1]!=NULL,-ENOENT);
		g_return_val_if_fail(m2->infifos[pin2]!=NULL,-ENOENT);
		/* are the two pins connected together ?*/
		g_return_val_if_fail(m1->outfifos[pin1]==m2->infifos[pin2],-EINVAL);
		ms_fifo_destroy_with_buffer(m1->outfifos[pin1]);
		m1->outfifos[pin1]=m2->infifos[pin2]=NULL;			
		m1->foutputs--;
		m2->finputs--;								
		break;
	}
	return 0;
}

/**
 *ms_filter_remove_links:
 *@m1: a filter
 *@m2: another filter.
 *
 * Removes all links between m1 and m2.
 *
 *Returns: 0 if one more link have been removed, -1 if not.
**/
gint ms_filter_remove_links(MSFilter *m1, MSFilter *m2)
{
	int i,j;
	int removed=-1;
	MSQueue *qo;
	MSFifo *fo;
	/* takes all outputs of m1, and removes the one that goes to m2 */
	if (m1->outqueues!=NULL){
		for (i=0;i<MS_FILTER_GET_CLASS(m1)->max_qoutputs;i++)
		{
			qo=m1->outqueues[i];
			if (qo!=NULL){
				MSFilter *rmf;
				/* test if the queue connects to m2 */
				rmf=(MSFilter*)qo->next_data;
				if (rmf==m2){
					j=find_iq(rmf,qo);
					if (j==-1) g_error("Could not find input queue: impossible case.");
					ms_filter_unlink(m1,i,m2,j,LINK_QUEUE);
					removed=0;
				}
			}
		}
	}
	if (m1->outfifos!=NULL){
		for (i=0;i<MS_FILTER_GET_CLASS(m1)->max_foutputs;i++)
		{
			fo=m1->outfifos[i];
			if (fo!=NULL){
				MSFilter *rmf;
				/* test if the queue connects to m2 */
				rmf=(MSFilter*)fo->next_data;
				if (rmf==m2){
					j=find_if(rmf,fo);
					if (j==-1) g_error("Could not find input fifo: impossible case.");
					ms_filter_unlink(m1,i,m2,j,LINK_FIFO);
					removed=0;
				}
			}
		}
	}
	return removed;
}

/**
 * ms_filter_fifos_have_data:
 * @f: a #MSFilter object.
 *
 * Tells if the filter has enough data in its input fifos in order to be executed succesfully.
 *
 * Returns: 1 if it can be executed, 0 else.
 */
gint ms_filter_fifos_have_data(MSFilter *f)
{
	gint i,j;
	gint max_inputs=f->klass->max_finputs;
	gint con_inputs=f->finputs;
	MSFifo *fifo;
	/* test fifos */
	for(i=0,j=0; (i<max_inputs) && (j<con_inputs);i++)
	{
		fifo=f->infifos[i];
		if (fifo!=NULL)
		{
			j++;
    		if (fifo->readsize==0) return 0;
			if (fifo->readsize>=f->r_mingran) return 1;
		}
	}
	return 0;  
}

/**
 * ms_filter_queues_have_data:
 * @f: a #MSFilter object.
 *
 * Tells if the filter has enough data in its input queues in order to be executed succesfully.
 *
 * Returns: 1 if it can be executed, 0 else.
 */
gint ms_filter_queues_have_data(MSFilter *f)
{
	gint i,j;
	gint max_inputs=f->klass->max_qinputs;
	gint con_inputs=f->qinputs;
	MSQueue *q;
	/* test queues */
	for(i=0,j=0; (i<max_inputs) && (j<con_inputs);i++)
	{
		q=f->inqueues[i];
		if (q!=NULL)
		{
			j++;
			if (ms_queue_can_get(q)) return 1;
		}
	}
	return 0;  
}



void ms_filter_destroy(MSFilter *f)
{
	/* first check if the filter is disconnected from any others */
	g_return_if_fail(f->finputs==0);
	g_return_if_fail(f->foutputs==0);
	g_return_if_fail(f->qinputs==0);
	g_return_if_fail(f->qoutputs==0);
	f->klass->destroy(f);
}

GList *filter_list=NULL;

void ms_filter_register(MSFilterInfo *info)
{
	gpointer tmp;
	tmp=g_list_find(filter_list,info);
	if (tmp==NULL) filter_list=g_list_append(filter_list,(gpointer)info);
}

void ms_filter_unregister(MSFilterInfo *info)
{
	filter_list=g_list_remove(filter_list,(gpointer)info);
}

static gint compare_names(gpointer info, gpointer name)
{
	MSFilterInfo *i=(MSFilterInfo*) info;
	return (strcmp(i->name,name));
}

MSFilterInfo * ms_filter_get_by_name(const gchar *name)
{
	GList *elem=g_list_find_custom(filter_list,
						(gpointer)name,(GCompareFunc)compare_names);
	if (elem!=NULL){
		return (MSFilterInfo*)elem->data;
	}
	return NULL;
}



MSFilter * ms_filter_new_with_name(const gchar *name)
{
	MSFilterInfo *info=ms_filter_get_by_name(name);
	if (info!=NULL) return info->constructor();
	g_warning("ms_filter_new_with_name: no filter named %s found.",name);
	return NULL;
}


/* find the first codec in the left part of the stream */
MSFilter * ms_filter_search_upstream_by_type(MSFilter *f,MSFilterType type)
{
	MSFilter *tmp=f;
	MSFilterInfo *info;
	
	if ((tmp->infifos!=NULL) && (tmp->infifos[0]!=NULL)){
		tmp=(MSFilter*) tmp->infifos[0]->prev_data;
		while(1){
			info=MS_FILTER_GET_CLASS(tmp)->info;
			if (info!=NULL){
				if ( (info->type==type) ){
					return tmp;
				}
			}
			if ((tmp->infifos!=NULL) && (tmp->infifos[0]!=NULL))
				tmp=(MSFilter*) tmp->infifos[0]->prev_data;
			else break;
		}
	}
	tmp=f;
	if ((tmp->inqueues!=NULL) && (tmp->inqueues[0]!=NULL)){
		tmp=(MSFilter*) tmp->inqueues[0]->prev_data;
		while(1){
		
			info=MS_FILTER_GET_CLASS(tmp)->info;
			if (info!=NULL){
				if ( (info->type==type)){
					return tmp;
				}
			}else g_warning("ms_filter_search_upstream_by_type: filter %s has no info."
							,MS_FILTER_GET_CLASS(tmp)->name);
			if ((tmp->inqueues!=NULL) && (tmp->inqueues[0]!=NULL))
				tmp=(MSFilter*) tmp->inqueues[0]->prev_data;
			else break;
		}	
	}
	return NULL;
}


int ms_filter_set_property(MSFilter *f, MSFilterProperty prop,void *value)
{
	if (f->klass->set_property!=NULL){
		return f->klass->set_property(f,prop,value);
	}
	return 0;
}

int ms_filter_get_property(MSFilter *f, MSFilterProperty prop,void *value)
{
	if (f->klass->get_property!=NULL){
		return f->klass->get_property(f,prop,value);
	}
	return -1;
}

void ms_filter_set_notify_func(MSFilter* filter,MSFilterNotifyFunc func, gpointer userdata)
{
	filter->notify_event=func;
	filter->userdata=userdata;
}

void ms_filter_notify_event(MSFilter *filter,gint event, gpointer arg)
{
	if (filter->notify_event!=NULL){
		filter->notify_event(filter,event,arg,filter->userdata);
	}
}

void swap_buffer(gchar *buffer, gint len)
{
	int i;
	gchar tmp;
	for (i=0;i<len;i+=2){
		tmp=buffer[i];
		buffer[i]=buffer[i+1];
		buffer[i+1]=tmp;
	}
}
