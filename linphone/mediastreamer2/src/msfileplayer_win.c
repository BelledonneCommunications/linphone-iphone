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

#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/waveheader.h"
#include "mediastreamer2/msticker.h"

typedef enum {
	CLOSED,
	STARTED,
	STOPPED
} PlayerState;

struct _PlayerData{
	HANDLE fd;
	PlayerState state;
	int rate;
	int nchannels;
	int hsize;
	int loop_after;
	int pause_time;
	bool_t swap;

	int stat;
	int big_buffer; /* ouput less & bigger buffer. (default => no change) */
};

typedef struct _PlayerData PlayerData;

static void player_init(MSFilter *f){
	PlayerData *d=(PlayerData *)ms_new(PlayerData,1);
	d->fd=INVALID_HANDLE_VALUE;
	d->state=CLOSED;
	d->swap=FALSE;
	d->rate=8000;
	d->nchannels=1;
	d->hsize=0;
	d->loop_after=-1;
	d->pause_time=0;
	d->stat=-1;
	d->big_buffer=1;
	f->data=d;	
}

static int read_wav_header(PlayerData *d){

  char header1[sizeof(riff_t)];
  char header2[sizeof(format_t)];
  char header3[sizeof(data_t)];
  int count;

  riff_t *riff_chunk=(riff_t*)header1;
	format_t *format_chunk=(format_t*)header2;
	data_t *data_chunk=(data_t*)header3;

  unsigned long len=0;
  BOOL res;
    
  res = ReadFile(d->fd, header1, sizeof(header1), &len, NULL) ;
  if (!res ||  len != sizeof(header1)){
		ms_warning("Wrong wav header: cannot read file");
		return -1;
	}
	
  if (0!=strncmp(riff_chunk->riff, "RIFF", 4) || 0!=strncmp(riff_chunk->wave, "WAVE", 4)){	
		ms_warning("Wrong wav header (not RIFF/WAV)");
		return -1;
	}

  res = ReadFile(d->fd, header2, sizeof(header2), &len, NULL) ;            
  if (!res ||  len != sizeof(header2)){
		ms_warning("Wrong wav header: cannot read file");
		return -1;
	}

  d->rate=le_uint32(format_chunk->rate);
	d->nchannels=le_uint16(format_chunk->channel);

  if (format_chunk->len-0x10>0)
  {
    SetFilePointer(d->fd, (format_chunk->len-0x10), NULL, FILE_CURRENT);
  }

  d->hsize=sizeof(wave_header_t)-0x10+format_chunk->len;

  res = ReadFile(d->fd, header3, sizeof(header3), &len, NULL) ;
  if (!res ||  len != sizeof(header3)){
		ms_warning("Wrong wav header: cannot read file");
		return -1;
	}
  count=0;
  while (strncmp(data_chunk->data, "data", 4)!=0 && count<30)
  {
    SetFilePointer(d->fd, data_chunk->len, NULL, FILE_CURRENT);
    count++;
    d->hsize=d->hsize+len+data_chunk->len;

    res = ReadFile(d->fd, header3, sizeof(header3), &len, NULL) ;
    if (!res ||  len != sizeof(header3)){
		  ms_warning("Wrong wav header: cannot read file");
		  return -1;
	  }
  }
#ifdef WORDS_BIGENDIAN
	d->swap=TRUE;
#endif
	return 0;
}

static int player_open(MSFilter *f, void *arg){
	PlayerData *d=(PlayerData*)f->data;
	HANDLE fd;
	const char *file=(const char*)arg;
	WCHAR wUnicode[1024];
	MultiByteToWideChar(CP_UTF8, 0, file, -1, wUnicode, 1024);
    fd = CreateFile(wUnicode, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, 0, NULL);
	if (fd==INVALID_HANDLE_VALUE){
		ms_warning("Failed to open %s",file);
		return -1;
	}
	d->state=STOPPED;
	d->fd=fd;
	if (strstr(file,".wav")!=NULL) read_wav_header(d);
	return 0;
}

static int player_close(MSFilter *f, void *arg){
	PlayerData *d=(PlayerData*)f->data;
	if (d->fd!=INVALID_HANDLE_VALUE)	CloseHandle(d->fd);
	d->fd=NULL;
	d->state=CLOSED;
	d->stat=-1;
	return 0;
}

static int player_start(MSFilter *f, void *arg){
	PlayerData *d=(PlayerData*)f->data;
	if (d->state==STOPPED)
		d->state=STARTED;
	return 0;
}

static int player_stop(MSFilter *f, void *arg){
	PlayerData *d=(PlayerData*)f->data;
	if (d->state==STARTED){
		d->state=STOPPED;
		d->stat=-1;
    SetFilePointer(d->fd, d->hsize, NULL, FILE_BEGIN);
    //read_wav_header(d);
	}
	return 0;
}

static void player_uninit(MSFilter *f){
	PlayerData *d=(PlayerData*)f->data;
	if (d->fd!=INVALID_HANDLE_VALUE) player_close(f,NULL);
	ms_free(d);
}

static void player_process(MSFilter *f){
	PlayerData *d=(PlayerData*)f->data;
	int bytes =d->big_buffer * 2*(f->ticker->interval*d->rate*d->nchannels)/1000;

	if (d->big_buffer>1)
	{
		/* when starting reading a file: prepare more data
		so that sound card buffer will never remain empty.
		*/
		d->stat++;
		if (d->stat>3)
		{
			if (d->stat%(d->big_buffer)!=0)
				return;
		}
	}

	if (d->state==STARTED){
		unsigned long err;
		mblk_t *om=allocb(bytes,0);
		if (d->pause_time>0){
			err=bytes;
			memset(om->b_wptr,0,bytes);
			d->pause_time-=f->ticker->interval;
		}else{
            BOOL res;
            err=0;
            res = ReadFile(d->fd, om->b_wptr, bytes, &err, NULL) ;            
		}
		if (err>=0){
			if (err==bytes){
				om->b_wptr+=err;
				ms_queue_put(f->outputs[0],om);
			}
			else if (err>0){
				BOOL res;

				om->b_wptr+=err;

				ms_filter_notify_no_arg(f,MS_FILE_PLAYER_EOF);
				SetFilePointer(d->fd, d->hsize, NULL, FILE_BEGIN);
        //read_wav_header(d);

				/* special value for playing file only once */
				if (d->loop_after==-2)
				{
  				freemsg(om);
					player_close(f,NULL);
					return;
				}

				if (d->loop_after>0)
				{
					d->stat=-1;
					d->pause_time=d->loop_after;
				}
				else
				{
					bytes=bytes-err;
					err=0;
					res = ReadFile(d->fd, om->b_wptr, bytes, &err, NULL);
					if (err>0){
						om->b_wptr+=err;
					}
				}

				ms_queue_put(f->outputs[0],om);
			}
			else if (err==0){
				BOOL res;
				ms_filter_notify_no_arg(f,MS_FILE_PLAYER_EOF);
				SetFilePointer(d->fd, d->hsize, NULL, FILE_BEGIN);

				if (d->loop_after==-2)
				{
  				freemsg(om);
					player_close(f,NULL);
					return;
				}

				if (d->loop_after>0)
				{
					d->stat=-1;
					d->pause_time=d->loop_after;
				}
				else
				{
					bytes=bytes-err;
					err=0;
					res = ReadFile(d->fd, om->b_wptr, bytes, &err, NULL);
					if (err>0){
						om->b_wptr+=err;
						ms_queue_put(f->outputs[0],om);
						return;
					}
				}
				freemsg(om);

			}else freemsg(om);
		}else{
#if !defined(_WIN32_WCE)
			ms_warning("Fail to read %i bytes: %s",bytes,strerror(errno));
#else
			ms_warning("Fail to read %i bytes: %i",bytes,WSAGetLastError());
#endif
		}
	}
}

static int player_get_sr(MSFilter *f, void*arg){
	PlayerData *d=(PlayerData*)f->data;
	*((int*)arg)=d->rate;
	return 0;
}

static int player_loop(MSFilter *f, void *arg){
	PlayerData *d=(PlayerData*)f->data;
	d->loop_after=*((int*)arg);
	return 0;
}

static int player_set_big_buffer(MSFilter *f, void *arg){
	PlayerData *d=(PlayerData*)f->data;
	d->big_buffer=*((int*)arg);
	return 0;
}

static int player_eof(MSFilter *f, void *arg){
	PlayerData *d=(PlayerData*)f->data;
	if (d->fd==NULL && d->state==CLOSED)
		*((int*)arg) = TRUE; /* 1 */
	else
		*((int*)arg) = FALSE; /* 0 */
	return 0;
}

static int player_get_nch(MSFilter *f, void *arg){
	PlayerData *d=(PlayerData*)f->data;
	*((int*)arg)=d->nchannels;
	return 0;
}

static MSFilterMethod player_methods[]={
	{	MS_FILE_PLAYER_OPEN,	player_open	},
	{	MS_FILE_PLAYER_START,	player_start	},
	{	MS_FILE_PLAYER_STOP,	player_stop	},
	{	MS_FILE_PLAYER_CLOSE,	player_close	},
	{	MS_FILTER_GET_SAMPLE_RATE, player_get_sr},
	{	MS_FILTER_GET_NCHANNELS, player_get_nch	},
	{	MS_FILE_PLAYER_LOOP,	player_loop	},
	{	MS_FILE_PLAYER_DONE,	player_eof	},
	{	MS_FILE_PLAYER_BIG_BUFFER,	player_set_big_buffer	},
	{	0,			NULL		}
};

#ifdef _MSC_VER

MSFilterDesc ms_file_player_desc={
	MS_FILE_PLAYER_ID,
	"MSFilePlayer",
	"Raw files and wav reader",
	MS_FILTER_OTHER,
	NULL,
    0,
	1,
	player_init,
	NULL,
    player_process,
	NULL,
    player_uninit,
	player_methods
};

#else

MSFilterDesc ms_file_player_desc={
	.id=MS_FILE_PLAYER_ID,
	.name="MSFilePlayer",
	.text="Raw files and wav reader",
	.category=MS_FILTER_OTHER,
	.ninputs=0,
	.noutputs=1,
	.init=player_init,
	.process=player_process,
	.uninit=player_uninit,
	.methods=player_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_file_player_desc)
