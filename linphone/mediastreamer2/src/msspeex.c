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

#include "mediastreamer2/msfilter.h"

#include <speex/speex.h>

#ifdef WIN32
#include <malloc.h> /* for alloca */
#endif

typedef struct SpeexEncState{
	int rate;
	int bitrate;
	int maxbitrate;
	int ptime;
	int vbr;
	int cng;
	int mode;
	int frame_size;
	void *state;
	uint32_t ts;
	MSBufferizer *bufferizer;
} SpeexEncState;

static void enc_init(MSFilter *f){
	SpeexEncState *s=(SpeexEncState *)ms_new(SpeexEncState,1);
	s->rate=8000;
	s->bitrate=-1;
	s->maxbitrate=-1;
	s->ptime=0;
	s->mode=-1;
	s->vbr=0;
	s->cng=0;
	s->frame_size=0;
	s->state=0;
	s->ts=0;
	s->bufferizer=ms_bufferizer_new();
	f->data=s;
}

static void enc_uninit(MSFilter *f){
	SpeexEncState *s=(SpeexEncState*)f->data;
	if (s==NULL)
		return;
	ms_bufferizer_destroy(s->bufferizer);
	if (s->state!=NULL)
		speex_encoder_destroy(s->state);
	ms_free(s);
}

static void enc_preprocess(MSFilter *f){
	SpeexEncState *s=(SpeexEncState*)f->data;
	const SpeexMode *mode=NULL;
	int _mode=0;

	switch(s->rate){
		case 8000:
	        _mode = SPEEX_MODEID_NB;    /* rate = 8000Hz */
			break;
		case 16000:
	        _mode = SPEEX_MODEID_WB;    /* rate = 16000Hz */
			break;
			/* should be supported in the future */
		case 32000:
	        _mode = SPEEX_MODEID_UWB;   /* rate = 32000Hz */
			break;
		default:
			ms_error("Unsupported rate for speex encoder (back to default rate=8000).");
			s->rate=8000;
	}
	/* warning: speex_lib_get_mode() is not available on speex<1.1.12 */
	mode = speex_lib_get_mode(_mode);

	if (mode==NULL)
		return;
	s->state=speex_encoder_init(mode);

	if (s->vbr==1)
	{
		if (speex_encoder_ctl(s->state,SPEEX_SET_VBR,&s->vbr)!=0){
			ms_error("Could not set vbr mode to speex encoder.");
		}
		/* implicit VAD */
		speex_encoder_ctl (s->state, SPEEX_SET_DTX, &s->vbr);
	}
	else if (s->vbr==2)
	{
		int vad=1;
		/* VAD */
		speex_encoder_ctl (s->state, SPEEX_SET_VAD, &vad);
		speex_encoder_ctl (s->state, SPEEX_SET_DTX, &vad);
	}
	else if (s->cng==1)
	{
		speex_encoder_ctl (s->state, SPEEX_SET_VAD, &s->cng);
	}

	if (s->rate==8000){
		//+------+---------------+-------------+
		//| mode | Speex quality |   bit-rate  |
		//+------+---------------+-------------+
		//|   1  |       0       | 2.15 kbit/s |
		//|   2  |       2       | 5.95 kbit/s |
		//|   3  |     3 or 4    | 8.00 kbit/s |
		//|   4  |     5 or 6    | 11.0 kbit/s |
		//|   5  |     7 or 8    | 15.0 kbit/s |
		//|   6  |       9       | 18.2 kbit/s |
		//|   7  |      10       | 24.6 kbit/s |
		//|   8  |       1       | 3.95 kbit/s |
		//+------+---------------+-------------+
		if (s->mode<=0 || s->mode>8)
			s->mode = 3; /* default mode */

		if (s->mode==1)
			s->bitrate = 2150;
		else if (s->mode==2)
			s->bitrate = 5950;
		else if (s->mode==3)
			s->bitrate = 8000;
		else if (s->mode==4)
			s->bitrate = 11000;
		else if (s->mode==5)
			s->bitrate = 15000;
		else if (s->mode==6)
			s->bitrate = 18200;
		else if (s->mode==7)
			s->bitrate = 24600;
		else if (s->mode==8)
			s->bitrate = 3950;

		if (s->bitrate!=-1){
			if (speex_encoder_ctl(s->state,SPEEX_SET_BITRATE,&s->bitrate)!=0){
				ms_error("Could not set bitrate %i to speex encoder.",s->bitrate);
			}
		}
	}
	else if (s->rate==16000 || s->rate==32000){
		//+------+---------------+-------------------+------------------------+
		//| mode | Speex quality | wideband bit-rate |     ultra wideband     |
		//|      |               |                   |        bit-rate        |
		//+------+---------------+-------------------+------------------------+
		//|   0  |       0       |    3.95 kbit/s    |       5.75 kbit/s      |
		//|   1  |       1       |    5.75 kbit/s    |       7.55 kbit/s      |
		//|   2  |       2       |    7.75 kbit/s    |       9.55 kbit/s      |
		//|   3  |       3       |    9.80 kbit/s    |       11.6 kbit/s      |
		//|   4  |       4       |    12.8 kbit/s    |       14.6 kbit/s      |
		//|   5  |       5       |    16.8 kbit/s    |       18.6 kbit/s      |
		//|   6  |       6       |    20.6 kbit/s    |       22.4 kbit/s      |
		//|   7  |       7       |    23.8 kbit/s    |       25.6 kbit/s      |
		//|   8  |       8       |    27.8 kbit/s    |       29.6 kbit/s      |
		//|   9  |       9       |    34.2 kbit/s    |       36.0 kbit/s      |
		//|  10  |       10      |    42.2 kbit/s    |       44.0 kbit/s      |
		//+------+---------------+-------------------+------------------------+
		int q=0;
		if (s->mode<0 || s->mode>10)
			s->mode = 8; /* default mode */
		q=s->mode;
		if (speex_encoder_ctl(s->state,SPEEX_SET_QUALITY,&q)!=0){
			ms_error("Could not set quality %i to speex encoder.",q);
		}
	}

	if (s->maxbitrate>0){
		/* convert from network bitrate to codec bitrate:*/
		/* ((nbr/(50*8)) -20-12-8)*50*8*/
		int cbr=(int)( ((((float)s->maxbitrate)/(50.0*8))-20-12-8)*50*8);
		ms_message("Setting maxbitrate=%i to speex encoder.",cbr);
		if (speex_encoder_ctl(s->state,SPEEX_SET_BITRATE,&cbr)!=0){
			ms_error("Could not set maxbitrate %i to speex encoder.",s->bitrate);
		}
	}
	if (speex_encoder_ctl(s->state,SPEEX_GET_BITRATE,&s->bitrate)!=0){
			ms_error("Could not get bitrate %i to speex encoder.",s->bitrate);
	}
	else ms_message("Using bitrate %i for speex encoder.",s->bitrate);

	speex_mode_query(mode,SPEEX_MODE_FRAME_SIZE,&s->frame_size);
}

static void enc_process(MSFilter *f){
	SpeexEncState *s=(SpeexEncState*)f->data;
	mblk_t *im;
	int nbytes;
	uint8_t *buf;
	int frame_per_packet=1;

	if (s->frame_size<=0)
		return;

	if (s->ptime>=20)
	{
		frame_per_packet = s->ptime/20;
	}

	if (frame_per_packet<=0)
		frame_per_packet=1;
	if (frame_per_packet>7) /* 7*20 == 140 ms max */
		frame_per_packet=7;

	nbytes=s->frame_size*2;
	buf=(uint8_t*)alloca(nbytes*frame_per_packet);

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		ms_bufferizer_put(s->bufferizer,im);
	}
	while(ms_bufferizer_read(s->bufferizer,buf,nbytes*frame_per_packet)==nbytes*frame_per_packet){
		mblk_t *om=allocb(nbytes*frame_per_packet,0);//too large...
		int k;
		SpeexBits bits;
		speex_bits_init(&bits);
		for (k=0;k<frame_per_packet;k++)
		{
			speex_encode_int(s->state,(int16_t*)(buf + (k*s->frame_size*2)),&bits);
			s->ts+=s->frame_size;
		}
		speex_bits_insert_terminator(&bits);
		k=speex_bits_write(&bits, (char*)om->b_wptr, nbytes*frame_per_packet);
		om->b_wptr+=k;

		mblk_set_timestamp_info(om,s->ts-s->frame_size);
		ms_queue_put(f->outputs[0],om);
		speex_bits_destroy(&bits);
	}
}

static void enc_postprocess(MSFilter *f){
	SpeexEncState *s=(SpeexEncState*)f->data;
	speex_encoder_destroy(s->state);
	s->state=NULL;
}

static int enc_set_sr(MSFilter *f, void *arg){
	SpeexEncState *s=(SpeexEncState*)f->data;
	/* TODO: should be done with fmtp parameter */
	s->rate=((int*)arg)[0];
	return 0;
}

static int enc_set_br(MSFilter *f, void *arg){
	SpeexEncState *s=(SpeexEncState*)f->data;
	s->maxbitrate=((int*)arg)[0];
	return 0;
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	SpeexEncState *s=(SpeexEncState*)f->data;

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "vbr", buf, sizeof(buf));
	if (buf[0]=='\0'){
	}
	else if (strstr(buf,"off")!=NULL){
		s->vbr=0;
	}
	else if (strstr(buf,"on")!=NULL){
		s->vbr=1;
	}
	else if (strstr(buf,"vad")!=NULL){
		s->vbr=2;
	}

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "cng", buf, sizeof(buf));
	if (buf[0]=='\0'){
	}
	else if (strstr(buf,"off")!=NULL){
		s->cng=0;
	}
	else if (strstr(buf,"on")!=NULL){
		s->cng=1;
	}

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "mode", buf, sizeof(buf));
	if (buf[0]=='\0' || buf[1]=='\0'){
	}
	else if (buf[0]=='0' || (buf[0]=='"' && buf[1]=='0')){
		s->mode=0;
	}
	else if (buf[0]=='"' && atoi(buf+1)>=0){
		s->mode=atoi(buf+1);
	}
	else if (buf[0]!='"' && atoi(buf)>=0){
		s->mode=atoi(buf);
	}
	else {
		s->mode = -1; /* deault mode */
	}
	return 0;
}

static int enc_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	SpeexEncState *s=(SpeexEncState*)f->data;
	if (strstr(fmtp,"ptime:10")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:20")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:30")!=NULL){
		s->ptime=40;
	}else if (strstr(fmtp,"ptime:40")!=NULL){
		s->ptime=40;
	}else if (strstr(fmtp,"ptime:50")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:60")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:70")!=NULL){
		s->ptime=80;
	}else if (strstr(fmtp,"ptime:80")!=NULL){
		s->ptime=80;
	}else if (strstr(fmtp,"ptime:90")!=NULL){
		s->ptime=100; /* not allowed */
	}else if (strstr(fmtp,"ptime:100")!=NULL){
		s->ptime=100;
	}else if (strstr(fmtp,"ptime:110")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:120")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:130")!=NULL){
		s->ptime=140;
	}else if (strstr(fmtp,"ptime:140")!=NULL){
		s->ptime=140;
	}
	return 0;
}

static MSFilterMethod enc_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE	,	enc_set_sr	},
	{	MS_FILTER_SET_BITRATE		,	enc_set_br	},
	{	MS_FILTER_ADD_FMTP		,	enc_add_fmtp },
	{	MS_FILTER_ADD_ATTR		,	enc_add_attr},
	{	0				,	NULL		}
};

#ifdef _MSC_VER

MSFilterDesc ms_speex_enc_desc={
	MS_SPEEX_ENC_ID,
	"MSSpeexEnc",
	N_("The free and wonderful speex codec"),
	MS_FILTER_ENCODER,
	"speex",
	1,
	1,
	enc_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	enc_methods
};

#else

MSFilterDesc ms_speex_enc_desc={
	.id=MS_SPEEX_ENC_ID,
	.name="MSSpeexEnc",
	.text=N_("The free and wonderful speex codec"),
	.category=MS_FILTER_ENCODER,
	.enc_fmt="speex",
	.ninputs=1,
	.noutputs=1,
	.init=enc_init,
	.preprocess=enc_preprocess,
	.postprocess=enc_postprocess,
	.process=enc_process,
	.uninit=enc_uninit,
	.methods=enc_methods
};

#endif

typedef struct DecState{
	int rate;
	int penh;
	int frsz;
	void *state;
} DecState;

static void dec_init(MSFilter *f){
	DecState *s=(DecState *)ms_new(DecState,1);
	s->rate=8000;
	s->frsz=0;
	s->state=NULL;
	s->penh=1;
	f->data=s;
}

static void dec_uninit(MSFilter *f){
	DecState *s=(DecState*)f->data;
    if (s==NULL)
		return;
    if (s->state!=NULL)
		speex_decoder_destroy(s->state);
	ms_free(s);
}

static void dec_preprocess(MSFilter *f){
	DecState *s=(DecState*)f->data;
	const SpeexMode *mode=NULL;
	int modeid;
	switch(s->rate){
		case 8000:
	        modeid = SPEEX_MODEID_NB;    /* rate = 8000Hz */
			break;
		case 16000:
	        modeid = SPEEX_MODEID_WB;    /* rate = 16000Hz */
			break;
			/* should be supported in the future */
		case 32000:
	        modeid = SPEEX_MODEID_UWB;   /* rate = 32000Hz */
			break;
		default:
			ms_error("Unsupported rate for speex decoder (back to default rate=8000).");
			modeid=SPEEX_MODEID_NB;
	}
	/* warning: speex_lib_get_mode() is not available on speex<1.1.12 */
	mode = speex_lib_get_mode(modeid);
	s->state=speex_decoder_init(mode);
	speex_mode_query(mode,SPEEX_MODE_FRAME_SIZE,&s->frsz);
	if (s->penh==1)
		speex_decoder_ctl (s->state, SPEEX_SET_ENH, &s->penh);
}

static void dec_postprocess(MSFilter *f){
	DecState *s=(DecState*)f->data;
	speex_decoder_destroy(s->state);
	s->state=NULL;
}

static int dec_set_sr(MSFilter *f, void *arg){
	DecState *s=(DecState*)f->data;
	s->rate=((int*)arg)[0];
	return 0;
}

static void dec_process(MSFilter *f){
	DecState *s=(DecState*)f->data;
	mblk_t *im;
	mblk_t *om;
	int err;
	int frame_per_packet;
	SpeexBits bits;
	int bytes=s->frsz*2;
	speex_bits_init(&bits);
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		speex_bits_reset(&bits);
		speex_bits_read_from(&bits,(char*)im->b_rptr,im->b_wptr-im->b_rptr);
		om=allocb(bytes*7,0);
		/* support for multiple frame (max=7 frames???) in one RTP packet */
        for (frame_per_packet=0;frame_per_packet<7;frame_per_packet++)
        {
            int i;
			err=speex_decode_int(s->state,&bits,(int16_t*)(om->b_wptr+(frame_per_packet*320)));
            
            i = speex_bits_remaining(&bits);
			if (i<10) /* this seems to work: don't know why. */
                break;
        }
		if (err==0){
			om->b_wptr+=bytes*(frame_per_packet+1);
			ms_queue_put(f->outputs[0],om);
		}else {
			if (err==-1)
				ms_warning("speex end of stream");
			else if (err==-2)
				ms_warning("speex corrupted stream");
			freemsg(om);
		}
		freemsg(im);
	}
	speex_bits_destroy(&bits);
}

static MSFilterMethod dec_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE	,	dec_set_sr	},
	{	0				,	NULL		}
};

#ifdef _MSC_VER

MSFilterDesc ms_speex_dec_desc={
	MS_SPEEX_DEC_ID,
	"MSSpeexDec",
	N_("The free and wonderful speex codec"),
	MS_FILTER_DECODER,
	"speex",
	1,
	1,
	dec_init,
	dec_preprocess,
	dec_process,
	dec_postprocess,
	dec_uninit,
	dec_methods
};

#else

MSFilterDesc ms_speex_dec_desc={
	.id=MS_SPEEX_DEC_ID,
	.name="MSSpeexDec",
	.text=N_("The free and wonderful speex codec"),
	.category=MS_FILTER_DECODER,
	.enc_fmt="speex",
	.ninputs=1,
	.noutputs=1,
	.init=dec_init,
	.preprocess=dec_preprocess,
	.postprocess=dec_postprocess,
	.process=dec_process,
	.uninit=dec_uninit,
	.methods=dec_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_speex_dec_desc)
MS_FILTER_DESC_EXPORT(ms_speex_enc_desc)
