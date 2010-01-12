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

#define UNICODE

#include "mediastreamer2/msfilerec.h"
#include "mediastreamer2/waveheader.h"

#if !defined(_WIN32_WCE)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif


typedef enum{
	Closed,
	Stopped,
	Started
} State;

typedef struct RecState{
	HANDLE fd;
	int rate;
	int size;
	State state;
	char filename[256];
} RecState;

static void rec_init(MSFilter *f){
	RecState *s=(RecState *)ms_new(RecState,1);
	s->fd=INVALID_HANDLE_VALUE;
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
			    DWORD byte_written=0;
				if ((err=WriteFile(s->fd,it->b_rptr,len, &byte_written, NULL))!=len){
					if (err<0)
					{
#if !defined(_WIN32_WCE)
						ms_warning("MSFileRec: fail to write %i bytes: %s",len,strerror(errno));
#else
						ms_warning("MSFileRec: fail to write %i bytes: %i",len,WSAGetLastError());
#endif
					}
				}
				it=it->b_cont;
				s->size+=len;
			}
		}
		ms_mutex_unlock(&f->lock);
		freemsg(m);
	}
}

static void write_wav_header(int rate,int size, char *filename){
	wave_header_t header;
	DWORD bytes_written=0;
	HANDLE fd;
	WCHAR wUnicode[1024];
	MultiByteToWideChar(CP_UTF8, 0, filename, -1, wUnicode, 1024);

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

	/* TODO: replace with "lseek" equivalent for windows */
	fd=CreateFile(wUnicode, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (fd==INVALID_HANDLE_VALUE){
#if !defined(_WIN32_WCE)
		ms_warning("Cannot open %s: %s",filename,strerror(errno));
#else
		ms_warning("Cannot open %s: %i",filename,WSAGetLastError());
#endif
		return;
	}
	WriteFile(fd,&header,sizeof(header), &bytes_written, NULL);
	if (bytes_written!=sizeof(header)){
		ms_warning("Fail to write wav header.");
	}
	CloseHandle(fd);
}

static int rec_open(MSFilter *f, void *arg){
	wave_header_t header;
	DWORD bytes_written=0;

	RecState *s=(RecState*)f->data;
	const char *filename=(const char*)arg;
	WCHAR wUnicode[1024];
	MultiByteToWideChar(CP_UTF8, 0, filename, -1, wUnicode, 1024);

	ms_mutex_lock(&f->lock);
	snprintf(s->filename, sizeof(s->filename), "%s", filename);
	s->fd=CreateFile(wUnicode, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (s->fd==INVALID_HANDLE_VALUE){
#if !defined(_WIN32_WCE)
		ms_warning("Cannot open %s: %s",filename,strerror(errno));
#else
		ms_warning("Cannot open %s: %i",filename,WSAGetLastError());
#endif
		ms_mutex_unlock(&f->lock);
		return -1;
	}

	memset(&header ,0,sizeof(header));
	WriteFile(s->fd,&header,sizeof(header), &bytes_written, NULL);
	if (bytes_written!=sizeof(header)){
		ms_warning("Fail to write wav header.");
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

static int rec_close(MSFilter *f, void *arg){
	RecState *s=(RecState*)f->data;
	ms_mutex_lock(&f->lock);
	s->state=Closed;
	if (s->fd!=INVALID_HANDLE_VALUE) {
		CloseHandle(s->fd);
		write_wav_header(s->rate, s->size, s->filename);
		s->fd=INVALID_HANDLE_VALUE;
		s->size=0;
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
	if (s->fd!=INVALID_HANDLE_VALUE)	rec_close(f,NULL);
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
