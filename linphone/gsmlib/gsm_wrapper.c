#include "gsm_wrapper.h"
#include <stdlib.h>

/* the following code has been added by Simon MORLAT to make GSM interface compatible with linphone*/

/* the public codec_info structure*/

struct codec_info gsm_codec_info=
{
	"GSM",
	160*2,    /* size of the uncompressed frame*/
	33,   /* size of compressed frame*/
	13000, /* bit rate*/
	{{
		8000
	}},               /* audio sampling freq*/
	GSMcodec_new,   /* codec constructor*/
	3,									/* payload type*/
	"gsm/8000/1",
	CODEC_AUDIO,          /* type*/
	0,										/*usable, set later*/
	1										/*usable for user, default value*/	
};

	
Codec *GSMcodec_new()
{
	GSMCodec *obj;
	
	obj=(GSMCodec*)malloc(sizeof(GSMCodec));/* we should make a few check to see if this codec is a GSM...*/
	
	obj->baseclass._getinfo=&wgsm_getinfo;
	obj->baseclass._encode=&wgsm_encode;
	obj->baseclass._decode=&wgsm_decode;
	obj->baseclass._destroy=&wgsm_destroy;
	obj->gsm_enc=gsm_create();
	obj->gsm_dec=gsm_create();
	return((Codec*)obj);
}


void wgsm_getinfo(Codec *codec,struct codec_info *info)
{
	if (info==NULL) return;
	memcpy(info,&gsm_codec_info,sizeof(codec_info_t));
}

void wgsm_encode(Codec *codec,char *frame, char *data)
{
	GSMCodec *obj=(GSMCodec*)codec; /* we should make a few check to see if this codec is a GSM...*/
	gsm_encode(obj->gsm_enc,(gsm_signal*)frame,data);
}

void wgsm_decode(Codec *codec,char *data, char *frame)
{
	GSMCodec *obj=(GSMCodec*)codec; /* we should make a few check to see if this codec is a GSM...*/
	gsm_decode(obj->gsm_dec,data,(gsm_signal*)frame);
}

void wgsm_destroy(Codec *codec)
{
	GSMCodec *obj=(GSMCodec*)codec; /* we should make a few check to see if this codec is a GSM...*/
	gsm_destroy(obj->gsm_enc);
	gsm_destroy(obj->gsm_dec);
	free(obj);
}

