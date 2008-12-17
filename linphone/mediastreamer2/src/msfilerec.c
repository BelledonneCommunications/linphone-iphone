/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "mediastreamer2/msfilerec.h"
#include "mediastreamer2/waveheader.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static int rec_close(MSFilter *f, void *arg);

typedef enum{
	Closed,
	Stopped,
	Started
} State;

typedef struct RecState{
	int fd;
	int rate;
	int size;
	State state;
} RecState;

static void rec_init(MSFilter *f){
	RecState *s=ms_new(RecState,1);
	s->fd=-1;
	s->rate=8000;
	s->size=0;
	s->state=Closed;
	f->data=s;
}

static void rec_process(MSFilter *f){
	RecState *s=(RecState*)f->data;
	mblk_t *m;
	int err;
	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		mblk_t *it=m;
		ms_mutex_lock(&f->lock);
		if (s->state==Started){
			while(it!=NULL){
				int len=it->b_wptr-it->b_rptr;
				if ((err=write(s->fd,it->b_rptr,len))!=len){
					if (err<0)
						ms_warning("MSFileRec: fail to write %i bytes: %s",len,strerror(errno));
				}
				it=it->b_cont;
				s->size+=len;
			}
		}
		ms_mutex_unlock(&f->lock);
		freemsg(m);
	}
}

static int rec_open(MSFilter *f, void *arg){
	RecState *s=(RecState*)f->data;
	const char *filename=(const char*)arg;
	if (s->fd>=0) rec_close(f,NULL);
	ms_mutex_lock(&f->lock);
	s->fd=open(filename,O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	if (s->fd<0){
		ms_warning("Cannot open %s: %s",filename,strerror(errno));
		ms_mutex_unlock(&f->lock);
		return -1;
	}
	s->state=Stopped;
	ms_mutex_unlock(&f->lock);
	return 0;
}

static int rec_start(MSFilter *f, void *arg){
	RecState *s=(RecState*)f->data;
	ms_mutex_lock(&f->lock);
	s->state=Started;
	ms_mutex_unlock(&f->lock);
	return 0;
}

static int rec_stop(MSFilter *f, void *arg){
	RecState *s=(RecState*)f->data;
	ms_mutex_lock(&f->lock);
	s->state=Stopped;
	ms_mutex_unlock(&f->lock);
	return 0;
}

static void write_wav_header(int fd, int rate,int size){
	wave_header_t header;
	memcpy(&header.riff_chunk.riff,"RIFF",4);
	header.riff_chunk.len=le_uint32(size+32);
	memcpy(&header.riff_chunk.wave,"WAVE",4);

	memcpy(&header.format_chunk.fmt,"fmt ",4);
	header.format_chunk.len=le_uint32(0x10);
	header.format_chunk.type=le_uint16(0x1);
	header.format_chunk.channel=le_uint16(0x1);
	header.format_chunk.rate=le_uint32(rate);
	header.format_chunk.bps=le_uint32(rate*2);
	header.format_chunk.blockalign=le_uint16(2);
	header.format_chunk.bitpspl=le_uint16(16);

	memcpy(&header.data_chunk.data,"data",4);
	header.data_chunk.len=le_uint32(size);
	lseek(fd,0,SEEK_SET);
	if (write(fd,&header,sizeof(header))!=sizeof(header)){
		ms_warning("Fail to write wav header.");
	}
}

static int rec_close(MSFilter *f, void *arg){
	RecState *s=(RecState*)f->data;
	ms_mutex_lock(&f->lock);
	s->state=Closed;
	if (s->fd>=0)	{
		write_wav_header(s->fd,s->rate, s->size);
		close(s->fd);
		s->fd=-1;
	}
	ms_mutex_unlock(&f->lock);
	return 0;
}

static int rec_set_sr(MSFilter *f, void *arg){
	RecState *s=(RecState*)f->data;
	ms_mutex_lock(&f->lock);
	s->rate=*((int*)arg);
	ms_mutex_unlock(&f->lock);
	return 0;
}

static void rec_uninit(MSFilter *f){
	RecState *s=(RecState*)f->data;
	if (s->fd>=0)	rec_close(f,NULL);
	ms_free(s);
}

static MSFilterMethod rec_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE,	rec_set_sr	},
	{	MS_FILE_REC_OPEN	,	rec_open	},
	{	MS_FILE_REC_START	,	rec_start	},
	{	MS_FILE_REC_STOP	,	rec_stop	},
	{	MS_FILE_REC_CLOSE	,	rec_close	},
	{	0			,	NULL		}
};

#ifdef WIN32

MSFilterDesc ms_file_rec_desc={
	MS_FILE_REC_ID,
	"MSFileRec",
	N_("Wav file recorder"),
	MS_FILTER_OTHER,
	NULL,
    1,
	0,
	rec_init,
	NULL,
    rec_process,
	NULL,
    rec_uninit,
	rec_methods
};

#else

MSFilterDesc ms_file_rec_desc={
	.id=MS_FILE_REC_ID,
	.name="MSFileRec",
	.text=N_("Wav file recorder"),
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=0,
	.init=rec_init,
	.process=rec_process,
	.uninit=rec_uninit,
	.methods=rec_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_file_rec_desc)
