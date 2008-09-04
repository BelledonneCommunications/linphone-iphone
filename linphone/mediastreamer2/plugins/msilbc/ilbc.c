/*
	NO LICENSE
*/
#include <ilbc/iLBC_encode.h>
#include <ilbc/iLBC_decode.h>

#include "mediastreamer2/msfilter.h"

typedef struct EncState{
	int nsamples;
	int nbytes;
	int ms_per_frame;
	int ptime;
	uint32_t ts;
	MSBufferizer bufferizer;
	iLBC_Enc_Inst_t ilbc_enc;	
}EncState;

static void enc_init(MSFilter *f){
	EncState *s=ms_new(EncState,1);
	s->nsamples=BLOCKL_20MS;
	s->nbytes=NO_OF_BYTES_20MS;
	s->ms_per_frame=20;
	s->ptime=0;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);
	f->data=s;
}

static void enc_uninit(MSFilter *f){
	EncState *s=(EncState*)f->data;
	ms_bufferizer_uninit(&s->bufferizer);
	ms_free(f->data);
}

static void enc_preprocess(MSFilter *f){
	EncState *s=(EncState*)f->data;
	initEncode(&s->ilbc_enc,s->ms_per_frame);
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	EncState *s=(EncState*)f->data;

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "mode", buf, sizeof(buf));
	if (buf[0]=='\0'){
		ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	}
	else if (strstr(buf,"20")!=NULL){
		s->nsamples=BLOCKL_20MS;
		s->nbytes=NO_OF_BYTES_20MS;
		s->ms_per_frame=20;
	}else if (strstr(buf,"30")!=NULL){
		s->nsamples=BLOCKL_30MS;
		s->nbytes=NO_OF_BYTES_30MS;
		s->ms_per_frame=30;
	}
	return 0;
}

static int enc_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	EncState *s=(EncState*)f->data;
	if (strstr(fmtp,"ptime:20")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:30")!=NULL){
		s->ptime=30;
	}else if (strstr(fmtp,"ptime:40")!=NULL){
		s->ptime=40;
	}else if (strstr(fmtp,"ptime:60")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:80")!=NULL){
		s->ptime=80;
	}else if (strstr(fmtp,"ptime:90")!=NULL){
		s->ptime=90;
	}else if (strstr(fmtp,"ptime:100")!=NULL){
		s->ptime=100;
	}else if (strstr(fmtp,"ptime:120")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:140")!=NULL){
		s->ptime=140;
	}
	return 0;
}

static void enc_process(MSFilter *f){
	EncState *s=(EncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[1610]; /* BLOCKL_MAX * 7 is the largest size for ptime == 140 */
	float samples2[BLOCKL_MAX];
	int i;
	int frame_per_packet=1;

	if (s->ptime>=20 && s->ms_per_frame>0 && s->ptime%s->ms_per_frame==0)
	{
		frame_per_packet = s->ptime/s->ms_per_frame;
	}

	if (frame_per_packet<=0)
		frame_per_packet=1;
	if (frame_per_packet>7) /* 7*20 == 140 ms max */
		frame_per_packet=7;

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		ms_bufferizer_put(&s->bufferizer,im);
	}
	while(ms_bufferizer_read(&s->bufferizer,(uint8_t*)samples,size*frame_per_packet)==(size*frame_per_packet)){
		int k;
		om=allocb(s->nbytes*frame_per_packet,0);
		for (k=0;k<frame_per_packet;k++)
		{
			for (i=0;i<s->nsamples;i++){
				samples2[i]=samples[i+(s->nsamples*k)];
			}
			iLBC_encode((uint8_t*)om->b_wptr,samples2,&s->ilbc_enc);
			om->b_wptr+=s->nbytes;
			s->ts+=s->nsamples;
		}
		mblk_set_timestamp_info(om,s->ts-s->nsamples);
		ms_queue_put(f->outputs[0],om);
	}
}

static MSFilterMethod enc_methods[]={
	{	MS_FILTER_ADD_FMTP,		enc_add_fmtp },
	{	MS_FILTER_ADD_ATTR,		enc_add_attr},
	{	0								,		NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_ilbc_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSIlbcEnc",
	"iLBC encoder",
	MS_FILTER_ENCODER,
	"iLBC",
	1,
	1,
	enc_init,
    enc_preprocess,
	enc_process,
    NULL,
	enc_uninit,
	enc_methods
};

#else

MSFilterDesc ms_ilbc_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSIlbcEnc",
	.text="iLBC encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="iLBC",
	.ninputs=1,
	.noutputs=1,
	.init=enc_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.uninit=enc_uninit,
	.methods=enc_methods
};

#endif

typedef struct DecState{
	int nsamples;
	int nbytes;
	int ms_per_frame;
	iLBC_Dec_Inst_t ilbc_dec;	
}DecState;


static void dec_init(MSFilter *f){
	DecState *s=ms_new(DecState,1);
	s->nsamples=0;
	s->nbytes=0;
	s->ms_per_frame=0;
	f->data=s;
}

static void dec_uninit(MSFilter *f){
	ms_free(f->data);
}

static void dec_process(MSFilter *f){
	DecState *s=(DecState*)f->data;
	mblk_t *im,*om;
	int nbytes;
	float samples[BLOCKL_MAX];
	int i;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		nbytes=msgdsize(im);
		if (nbytes<=0)
			return;
		if (nbytes%38!=0 && nbytes%50!=0)
			return;
		if (nbytes%38==0 && s->nbytes!=NO_OF_BYTES_20MS)
		{
			/* not yet configured, or misconfigured */
			s->ms_per_frame=20;
			s->nbytes=NO_OF_BYTES_20MS;
			s->nsamples=BLOCKL_20MS;
			initDecode(&s->ilbc_dec,s->ms_per_frame,0);
		}
		else if (nbytes%50==0 && s->nbytes!=NO_OF_BYTES_30MS)
		{
			/* not yet configured, or misconfigured */
			s->ms_per_frame=30;
			s->nbytes=NO_OF_BYTES_30MS;
			s->nsamples=BLOCKL_30MS;
			initDecode(&s->ilbc_dec,s->ms_per_frame,0);
		}
		if (s->nbytes>0 && nbytes>=s->nbytes){
			int frame_per_packet = nbytes/s->nbytes;
			int k;

			for (k=0;k<frame_per_packet;k++)
			{
				om=allocb(s->nsamples*2,0);
				iLBC_decode(samples,(uint8_t*)im->b_rptr+(k*s->nbytes),&s->ilbc_dec,1);
				for (i=0;i<s->nsamples;i++,om->b_wptr+=2){
					*((int16_t*)om->b_wptr)=samples[i];
				}
				ms_queue_put(f->outputs[0],om);
			}
		}else{
			ms_warning("bad iLBC frame !");
		}
		freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_ilbc_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSIlbcDec",
	"iLBC decoder",
	MS_FILTER_DECODER,
	"iLBC",
	1,
	1,
	dec_init,
    NULL,
	dec_process,
    NULL,
	dec_uninit,
    NULL
};

#else

MSFilterDesc ms_ilbc_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSIlbcDec",
	.text="iLBC decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="iLBC",
	.ninputs=1,
	.noutputs=1,
	.init=dec_init,
	.process=dec_process,
	.uninit=dec_uninit
};

#endif

#ifdef _MSC_VER
#define MS_PLUGIN_DECLARE(type) __declspec(dllexport) type
#else
#define MS_PLUGIN_DECLARE(type) type
#endif

MS_PLUGIN_DECLARE(void) libmsilbc_init(){
	ms_filter_register(&ms_ilbc_enc_desc);
	ms_filter_register(&ms_ilbc_dec_desc);
}
