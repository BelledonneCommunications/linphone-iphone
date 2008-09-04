#include "../console/codec.h"
#include "lpc10.h"


/*
int read_16bit_samples(INT16 int16samples[], float speech[], int n);
int write_16bit_samples(INT16 int16samples[], float speech[], int n);


void write_bits(unsigned char *data, INT32 *bits, int len);
int read_bits(unsigned char *data, INT32 *bits, int len);
*/

/* the following code has been added by Simon MORLAT to make lpc10 interface compatible with linphone*/

/*Class definition*/

typedef struct _LPC10Codec
{
	Codec	baseclass;   /* Codec must be the first element of the structure in order to have the object mechanism to work*/
	struct lpc10_encoder_state *enc;
	struct lpc10_decoder_state *dec;
} LPC10Codec;

/* this the constructor for derivate class LPC10Codec*/
Codec *LPC10codec_new();

extern struct codec_info LPC10codec_info;

/* these are the overrides for the base class 's functions*/
void wlpc10_getinfo(Codec *codec,struct codec_info *info);
void wlpc10_encode(Codec *codec,char *frame, char *data);
void wlpc10_decode(Codec *codec,char *data, char *frame);
void wlpc10_destroy(Codec *codec);
