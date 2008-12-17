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

#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/waveheader.h"
#include "mediastreamer2/msticker.h"


static int player_close(MSFilter *f, void *arg);

typedef enum {
	CLOSED,
	STARTED,
	STOPPED
} PlayerState;

struct _PlayerData{
	int fd;
	PlayerState state;
	int rate;
	int nchannels;
	int hsize;
	int loop_after;
	int pause_time;
	bool_t swap;
};

typedef struct _PlayerData PlayerData;

static void player_init(MSFilter *f){
	PlayerData *d=ms_new(PlayerData,1);
	d->fd=-1;
	d->state=CLOSED;
	d->swap=FALSE;
	d->rate=8000;
	d->nchannels=1;
	d->hsize=0;
	d->loop_after=-1; /*by default, don't loop*/
	d->pause_time=0;
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
    
  len = read(d->fd, header1, sizeof(header1)) ;
  if (len != sizeof(header1)){
    ms_warning("Wrong wav header: cannot read file");
    return -1;
  }
  
  if (0!=strncmp(riff_chunk->riff, "RIFF", 4) || 0!=strncmp(riff_chunk->wave, "WAVE", 4)){	
    ms_warning("Wrong wav header (not RIFF/WAV)");
    return -1;
  }
  
  len = read(d->fd, header2, sizeof(header2)) ;            
  if (len != sizeof(header2)){
    ms_warning("Wrong wav header: cannot read file");
    return -1;
  }
  
  d->rate=le_uint32(format_chunk->rate);
  d->nchannels=le_uint16(format_chunk->channel);
  
  if (format_chunk->len-0x10>0)
    {
      lseek(d->fd,(format_chunk->len-0x10),SEEK_CUR);
    }
  
  d->hsize=sizeof(wave_header_t)-0x10+format_chunk->len;
  
  len = read(d->fd, header3, sizeof(header3)) ;
  if (len != sizeof(header3)){
    ms_warning("Wrong wav header: cannot read file");
    return -1;
  }
  count=0;
  while (strncmp(data_chunk->data, "data", 4)!=0 && count<30)
    {
      ms_warning("skipping chunk=%s len=%i", data_chunk->data, data_chunk->len);
      lseek(d->fd,data_chunk->len,SEEK_CUR);
      count++;
      d->hsize=d->hsize+len+data_chunk->len;
      
      len = read(d->fd, header3, sizeof(header3)) ;
      if (len != sizeof(header3)){
	ms_warning("Wrong wav header: cannot read file");
	return -1;
      }
    }
#ifdef WORDS_BIGENDIAN
  if (le_uint16(format_chunk->blockalign)==le_uint16(format_chunk->channel) * 2)
    d->swap=TRUE;
#endif
  return 0;
}

static int player_open(MSFilter *f, void *arg){
	PlayerData *d=(PlayerData*)f->data;
	int fd;
	const char *file=(const char*)arg;

	if (d->fd>=0){
		player_close(f,NULL);
	}
	if ((fd=open(file,O_RDONLY))==-1){
		ms_warning("Failed to open %s",file);
		return -1;
	}
	d->state=STOPPED;
	d->fd=fd;
	if (strstr(file,".wav")!=NULL) read_wav_header(d);
	ms_message("%s opened: rate=%i,channel=%i",file,d->rate,d->nchannels);
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
	ms_filter_lock(f);
	if (d->state==STARTED){
		d->state=STOPPED;
		lseek(d->fd,d->hsize,SEEK_SET);
	}
	ms_filter_unlock(f);
	return 0;
}

static int player_close(MSFilter *f, void *arg){
	PlayerData *d=(PlayerData*)f->data;
	player_stop(f,NULL);
	if (d->fd>=0)	close(d->fd);
	d->fd=-1;
	d->state=CLOSED;
	return 0;
}

static void player_uninit(MSFilter *f){
	PlayerData *d=(PlayerData*)f->data;
	if (d->fd>=0) player_close(f,NULL);
	ms_free(d);
}

static void swap_bytes(unsigned char *bytes, int len){
	int i;
	unsigned char tmp;
	for(i=0;i<len;i+=2){
		tmp=bytes[i];
		bytes[i]=bytes[i+1];
		bytes[i+1]=tmp;
	}
}

static void player_process(MSFilter *f){
	PlayerData *d=(PlayerData*)f->data;
	int bytes=2*(f->ticker->interval*d->rate*d->nchannels)/1000;
	ms_filter_lock(f);
	if (d->state==STARTED){
		int err;
		mblk_t *om=allocb(bytes,0);
		if (d->pause_time>0){
			err=bytes;
			memset(om->b_wptr,0,bytes);
			d->pause_time-=f->ticker->interval;
		}else{
			err=read(d->fd,om->b_wptr,bytes);
			if (d->swap) swap_bytes(om->b_wptr,bytes);
		}
		if (err>=0){
			if (err!=0){
				om->b_wptr+=bytes;
				ms_queue_put(f->outputs[0],om);
			}else freemsg(om);
			if (err<bytes){
				ms_filter_notify_no_arg(f,MS_FILE_PLAYER_EOF);
				lseek(d->fd,d->hsize,SEEK_SET);

				/* special value for playing file only once */
				if (d->loop_after==-2)
				{
					d->state=STOPPED;
					ms_filter_unlock(f);
					return;
				}

				if (d->loop_after>=0){
					d->pause_time=d->loop_after;
				}
			}
		}else{
			ms_warning("Fail to read %i bytes: %s",bytes,strerror(errno));
		}
	}
	ms_filter_unlock(f);
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

static int player_eof(MSFilter *f, void *arg){
	PlayerData *d=(PlayerData*)f->data;
	if (d->fd<0 && d->state==CLOSED)
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
	{	0,			NULL		}
};

#ifdef WIN32

MSFilterDesc ms_file_player_desc={
	MS_FILE_PLAYER_ID,
	"MSFilePlayer",
	N_("Raw files and wav reader"),
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
	.text=N_("Raw files and wav reader"),
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
