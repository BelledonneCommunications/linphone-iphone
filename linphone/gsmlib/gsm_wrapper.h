#ifndef GSM_WRAPPER
#define GSM_WRAPPER

#include "../console/codec.h"
#include "gsm.h"

/* the following code has been added by Simon MORLAT to make lpc10 interface compatible with linphone*/

/*Class definition*/
typedef struct _GSMCodec
{
	Codec	baseclass;   /* Codec must be the first element of the structure in order to have the object mechanism to work*/
	gsm gsm_enc,gsm_dec;
} GSMCodec;

/* this the constructor for derivate class GSMCodec*/
Codec * GSMcodec_new();

extern struct codec_info gsm_codec_info;

/* these are the overrides for the base class 's functions*/
void wgsm_getinfo(Codec *codec, struct codec_info *info);
void wgsm_encode(Codec *codec, char *frame, char *data);
void wgsm_decode(Codec *codec, char *data, char *frame);
void wgsm_destroy(Codec *codec);


#endif
