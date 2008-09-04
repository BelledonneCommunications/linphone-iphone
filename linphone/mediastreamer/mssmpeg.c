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


#include "mssmpeg.h"
#include <unistd.h>
#include <fcntl.h>


static MSSmpegClass *ms_smpeg_class=NULL;
	
	
int seek_mpeg_data(struct SDL_RWops *context, int offset, int whence)
{
	MSSmpeg *obj=context->hidden.unknown.data1;
	g_message("Entering seek function offset=%i whence=%i",offset,whence);
	switch (whence)
	{
		case SEEK_SET:
			obj->pos=offset;
		break;
		case SEEK_CUR:
			return obj->pos;
		break;
		case SEEK_END:
		return obj->end_pos;	
		break;
	}
	return 0;
}

int read_mpeg_data(struct SDL_RWops *context, void *ptr, int size, int maxnum)
{
	MSSmpeg *obj=context->hidden.unknown.data1;
	gint bytes;
	g_message("Entering read function: size=%i maxnum=%i",size,maxnum);
	if (obj->pos>=obj->end_pos) 
	{
		g_message("End of file");
		return 0;
	}
	if (obj->current==NULL) 
	{
		g_message("Nothing to read.");
		return 0;
	}
	bytes=MIN(maxnum,obj->current->size);
	memcpy(ptr,obj->current->data,bytes);
	obj->pos+=bytes;
	//obj->current;
	return bytes;
}

void ms_smpeg_init(MSSmpeg *obj)
{
	gint error;
	ms_filter_init(MS_FILTER(obj));
	MS_FILTER(obj)->inqueues=obj->input;
	obj->surface = SDL_SetVideoMode ( 400, 400,0 , SDL_HWSURFACE );
	if (obj->surface==NULL)
	{
		g_error("Could not create a SDL surface");
	}
	/*
	error=pipe(obj->fd);
	if (error<0)
	{
		g_error("Could not create pipe !");
	}
	fcntl(obj->fd[1],F_SETFL,O_NONBLOCK);
	*/
	obj->rwops=SDL_AllocRW();
	obj->rwops->read=read_mpeg_data;
	obj->rwops->seek=seek_mpeg_data;
	obj->rwops->hidden.unknown.data1=(void*)obj;
	obj->pos=0;
	obj->end_pos=0;
}

void ms_smpeg_class_init(MSSmpegClass *klass)
{
	int error;
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	MS_FILTER_CLASS(klass)->max_qinputs=1;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_smpeg_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_smpeg_process;
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"MSSmpeg");
	error=SDL_Init(SDL_INIT_VIDEO);
	if (error<0){
		g_error("Could not initialize SDL !");
	}
	
}

void ms_smpeg_uninit(MSSmpeg *obj)
{
	
}

MSFilter * ms_smpeg_new()
{
	MSSmpeg *obj=g_malloc(sizeof(MSSmpeg));
	
	if (ms_smpeg_class==NULL)
	{
		ms_smpeg_class=g_malloc(sizeof(MSSmpegClass));
		ms_smpeg_class_init(ms_smpeg_class);
	}
	MS_FILTER(obj)->klass=MS_FILTER_CLASS(ms_smpeg_class);
	ms_smpeg_init(obj);
	return MS_FILTER(obj);
}

void ms_smpeg_start(MSSmpeg *obj)
{
	//SMPEG_play(obj->handle);
	obj->first_time=1;
	obj->run_cond=1;
}

void ms_smpeg_stop(MSSmpeg *obj)
{
	SMPEG_stop(obj->handle);
	obj->run_cond=0;
}

void ms_smpeg_process(MSSmpeg *obj)
{
	MSQueue *q=obj->input[0];
	MSMessage *m;
	SMPEG_Info info;
	
	while((m=ms_queue_get(q))!=NULL)
	{
		g_message("Getting new buffer");
		if (obj->run_cond)
		{
			obj->current=m;
			obj->end_pos+=m->size;
			if (obj->first_time)
			{
				obj->handle=SMPEG_new_rwops(obj->rwops,NULL,0);
				if (obj->handle==NULL){
					g_error("Could not create smpeg object.");
				}
				SMPEG_setdisplay(obj->handle,obj->surface,NULL,NULL);
				obj->first_time=0;
				//SMPEG_play(obj->handle);
			}
			SMPEG_getinfo(obj->handle, &info );
			g_message("Current frame is %i",info.current_frame);
			SMPEG_renderFrame(obj->handle, info.current_frame+1);
		}
		ms_message_destroy(m);
		obj->current=NULL;
	}
}

void ms_smpeg_destroy(MSSmpeg *obj)
{
	ms_smpeg_uninit(obj);
	g_free(obj);
}

