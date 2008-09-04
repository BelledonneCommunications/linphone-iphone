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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ms.h"
#include "sndcard.h"
#include "mscodec.h"

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef HAVE_DLOPEN
#include <dlfcn.h>
#endif

#ifdef HAVE_GLIB
#include "gmodule.h"			/* g_module_open() */
#endif

#define MS_PLUGINS_DIR PACKAGE_PLUGINS_DIR "/mediastreamer"


#ifdef VIDEO_ENABLED
extern void ms_video_source_register_all();
#endif

/**
 * ms_init:
 *
 *
 * Initialize the mediastreamer. This must be the first function called in a program
 * using the mediastreamer library.
 *
 *
 */
void ms_init()
{
	if (!g_thread_supported()) g_thread_init (NULL);

	/* initialize the oss subsystem */
	snd_card_manager_init(snd_card_manager);
	/* register the statically linked codecs */
	ms_codec_register_all();
#ifdef VIDEO_ENABLED
	ms_video_source_register_all();
#endif
	ms_load_plugins(MS_PLUGINS_DIR);
}


static gint compare(gconstpointer a, gconstpointer b)
{
	MSFilter *f1=(MSFilter*)a,*f2=(MSFilter*)b;
	if (f1->klass<f2->klass) return -1;
	if (f1->klass==f2->klass) return 0;
	/* if f1->klass>f2->klass ....*/
	return 1;
}

static GList *g_list_append_if_new(GList *l,gpointer data)
{
	GList *res=l;
	if (g_list_find(res,data)==NULL)
		res=g_list_append(res,data);
	return(res);
}

static GList *get_nexts(MSFilter *f,GList *l)
{
	int i;
	MSFifo *fifo;
	MSQueue *q;
	GList *res=l;
	
	/* check fifos*/
	for (i=0;i	<f->klass->max_foutputs;i++)
	{
		fifo=f->outfifos[i];
		if (fifo!=NULL) res=g_list_append_if_new(res,(gpointer)fifo->next_data);
	}
	/* check queues*/
	for (i=0;i	<f->klass->max_qoutputs;i++)
	{
		q=f->outqueues[i];
		if (q!=NULL) res=g_list_append_if_new(res,(gpointer)q->next_data);
	}
	return(res);
}
	
/* compile graphs attached to a sync source*/
int ms_compile(MSSync *sync)
{
	int i;
	GList *list1=NULL,*list2=NULL,*elem;
	GList *proc_chain=NULL;
	MSFilter *f;
	
	/* first free the old list if we are just updating*/
	if (sync->execution_list!=NULL) g_list_free(sync->execution_list);
	/* get the list of filters attached to this sync*/
	for (i=0;i<sync->filters;i++)
	{
		//printf("found filter !\n");
		list1=g_list_append(list1,sync->attached_filters[i]);
	}
	/* find the processing chain */
	while (list1!=NULL)
	{
		list2=NULL;
		/* sort the list by types of filter*/
		list1=g_list_sort(list1,compare);
		/* save into the processing chain list*/
		//printf("list1 :%i elements\n",g_list_length(list1));
		proc_chain=g_list_concat(proc_chain,list1);
		/* get all following filters. They are appended to list2*/
		elem=list1;
		while (elem!=NULL)
		{
			f=(MSFilter*)(elem->data);
			/* check if filter 's status */
			if (f->klass->attributes & FILTER_CAN_SYNC)
			{
				sync->samples_per_tick=0;
			}
			list2=get_nexts(f,list2);
			elem=g_list_next(elem);
		}
		list1=list2;
	}
	sync->execution_list=proc_chain;
	sync->flags&=~MS_SYNC_NEED_UPDATE;
	ms_trace("%i filters successfully compiled in a processing chain.",g_list_length(sync->execution_list));
	return 0;
}

/*execute the processing chain attached to a sync source. It is called as a thread by ms_main()*/
void *ms_thread_run(void *sync_ptr)
{
	MSSync *sync=(MSSync*) sync_ptr;
	GList *filter;
	MSFilter *f;
	
	
	ms_sync_lock(sync);  
	while(sync->run)
	{
		//g_message("sync->run=%i",sync->run);
		if (sync->samples_per_tick==0) ms_sync_suspend(sync);
		if (sync->flags & MS_SYNC_NEED_UPDATE){
			ms_compile(sync);
			ms_sync_setup(sync);
		}
		filter=sync->execution_list;
		ms_sync_unlock(sync);
		//ms_trace("Calling synchronisation");
		ms_sync_synchronize(sync);
		while(filter!=NULL)
		{
			f=(MSFilter*)filter->data;
			if (MS_FILTER_GET_CLASS(f)->attributes & FILTER_IS_SOURCE)
			{
				/* execute it once */
				ms_trace("Running source filter %s.",f->klass->name);
				ms_filter_process(f);
			}
			else
			{
				/* make the filter process its input data until it has no more */
				while ( ms_filter_fifos_have_data(f) || ms_filter_queues_have_data(f) )
				{
					ms_trace("Running filter %s.",f->klass->name);
					ms_filter_process(f);
				}
			}
			filter=g_list_next(filter);
		}
		ms_sync_lock(sync);  
	}
	g_cond_signal(sync->stop_cond);	/* signal that the sync thread has finished */
	ms_sync_unlock(sync);
	g_message("Mediastreamer processing thread is exiting.");
	return NULL;
}

/* stop the processing chain attached to a sync source.*/
void ms_thread_stop(MSSync *sync)
{
	if (sync->thread!=NULL)
	{
		if (sync->samples_per_tick==0)
		{
			/* to wakeup the thread */
			//g_cond_signal(sync->thread_cond);
		}
		g_mutex_lock(sync->lock);
		sync->run=0;
		sync->thread=NULL;
		g_cond_wait(sync->stop_cond,sync->lock);
		g_mutex_unlock(sync->lock);
	}
	//g_message("ms_thread_stop() finished.");
}

/**
 * ms_start:
 * @sync: A synchronisation source to be started.
 *
 * Starts a thread that will shedule all processing chains attached to the synchronisation source @sync.
 *
 *
 */
void ms_start(MSSync *sync)
{
	if (sync->run==1) return; /*already running*/
	ms_compile(sync);  
	ms_sync_setup(sync);
	/* this is to avoid race conditions, for example:
							ms_start(sync);
							ms_oss_write_start(ossw);
							here tge ossw filter need to be compiled to run ms_oss_write_start()
							*/
	ms_trace("ms_start: creating new thread.");
	sync->run=1;
	sync->thread=g_thread_create((GThreadFunc)ms_thread_run,(gpointer)sync,TRUE,NULL);
	if (sync->thread==NULL){
		g_warning("Could not create thread !");
	}
}

/**
 * ms_stop:
 * @sync: A synchronisation source to be stopped.
 *
 * Stop the thread that was sheduling the processing chains attached to the synchronisation source @sync.
 * The processing chains are kept unchanged, no object is freed. The synchronisation source can be restarted using ms_start().
 *
 *
 */
void ms_stop(MSSync *sync)
{
	ms_thread_stop(sync);
	ms_sync_unsetup(sync);
}


gint ms_load_plugin(gchar *path)
{
#ifdef HAVE_GLIB
	g_module_open(path,0);
#endif
	return 0;
}

gchar * ms_proc_get_param(gchar *parameter)
{
	gchar *file;
	int fd;
	int err,len;
	gchar *p,*begin,*end;
	gchar *ret;
	fd=open("/proc/cpuinfo",O_RDONLY);
	if (fd<0){
		g_warning("Could not open /proc/cpuinfo.");
		return NULL;
	}
	file=g_malloc(1024);
	err=read(fd,file,1024);
	file[err-1]='\0';
	/* find the parameter */
	p=strstr(file,parameter);
	if (p==NULL){
		/* parameter not found */
		g_free(file);
		return NULL;		
	}
	/* find the following ':' */
	p=strchr(p,':');
	if (p==NULL){
		g_free(file);
		return NULL;
	}
	/* find the value*/
	begin=p+2;
	end=strchr(begin,'\n');
	if (end==NULL) end=strchr(begin,'\0');
	len=end-begin+1;
	ret=g_malloc(len+1);
	snprintf(ret,len,"%s",begin);
	//printf("%s=%s\n",parameter,ret);
	g_free(file);
	return ret;
}

gint ms_proc_get_type()
{
	static int proc_type=0;
	gchar *value;
	if (proc_type==0){
		value=ms_proc_get_param("cpu family");
		if (value!=NULL) {
			proc_type=atoi(value);
			g_free(value);
		}else return -1;
	}
	return proc_type;
}

gint ms_proc_get_speed()
{
	char *value;
	static int proc_speed=0;
	if (proc_speed==0){
		value=ms_proc_get_param("cpu MHz");
		if (value!=NULL){
			proc_speed=atoi(value);
			g_free(value);
		}else return -1;
	}
	//printf("proc_speed=%i\n",proc_speed);
	return proc_speed;
}

#define PLUGINS_EXT ".so"

typedef void (*init_func_t)(void);

void ms_load_plugins(const char *dir){
#ifdef HAVE_DLOPEN
	DIR *ds;
	struct dirent *de;
	char *fullpath;
	ds=opendir(dir);	
	if (ds==NULL){
		g_warning("Cannot open directory %s: %s",dir,strerror(errno));
		return;
	}
	while( (de=readdir(ds))!=NULL){
		if (de->d_type==DT_REG && strstr(de->d_name,PLUGINS_EXT)!=NULL){
			void *handle;
			fullpath=g_strdup_printf("%s/%s",dir,de->d_name);
			g_message("Loading plugin %s...",fullpath);
			
			if ( (handle=dlopen(fullpath,RTLD_NOW))==NULL){
				g_warning("Fail to load plugin %s : %s",fullpath,dlerror());
			}else {
				char *initroutine_name=g_malloc0(strlen(de->d_name)+10);
				char *p;
				void *initroutine;
				strcpy(initroutine_name,de->d_name);
				p=strstr(initroutine_name,PLUGINS_EXT);
				strcpy(p,"_init");
				initroutine=dlsym(handle,initroutine_name);
				if (initroutine!=NULL){
					init_func_t func=(init_func_t)initroutine;
					func();
					g_message("Plugin loaded.");
				}else{
					g_warning("Could not locate init routine of plugin %s",de->d_name);
				}
				g_free(initroutine_name);
			}
			g_free(fullpath);
		}
	}
	closedir(ds);
#else
	g_warning("no loadable plugin support: plugins cannot be loaded.");
#endif
}
