#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include "lpc10_wrapper.h"
#include <netinet/in.h>

/* the public codec_info_t*/

struct codec_info LPC10codec_info=
{
	"LPC10-1.5",
	LPC10_SAMPLES_PER_FRAME*2,    /* size in bytes of the uncompressed frame*/
	((LPC10_BITS_IN_COMPRESSED_FRAME/8) + 1), /* 7 bytes */   /* size in bytes of compressed frame*/
	2400, /* bit rate*/
	{{
		8000
	}},               /* audio sampling freq*/
	LPC10codec_new,   /* codec constructor*/
	115,				/* payload type */
	"lpc10-15/8000/1", 	/*description */
	CODEC_AUDIO,          /* type*/
	0,										/*usable, set later*/	
	1										/*usable for user, default value*/
};

/* The return value of each of these calls is the same as that
   returned by fread/fwrite, which should be the number of samples
   successfully read/written, not the number of bytes. */

static int
read_16bit_samples(INT16 int16samples[], float speech[], int n)
{
    int i;

    /* Convert 16 bit integer samples to floating point values in the
       range [-1,+1]. */

    for (i = 0; i < n; i++) {
        speech[i] = ((float) int16samples[i]) / 32768.0;
    }

    return (n);
}



static int
write_16bit_samples(INT16 int16samples[], float speech[], int n)
{
	int i;
	float real_sample;

	/* Convert floating point samples in range [-1,+1] to 16 bit
	integers. */
	for (i = 0; i < n; i++) {
		real_sample = 32768.0 * speech[i];
		if (real_sample < -32768.0) {
			int16samples[i] = -32768;
		} else if (real_sample > 32767.0) {
		int16samples[i] = 32767;
		} else {
			int16samples[i] = real_sample;
		}
	}
	return (n);
}

/*

Write the bits in bits[0] through bits[len-1] to file f, in "packed"
format.

bits is expected to be an array of len integer values, where each
integer is 0 to represent a 0 bit, and any other value represents a 1
bit.  This bit string is written to the file f in the form of several
8 bit characters.  If len is not a multiple of 8, then the last
character is padded with 0 bits -- the padding is in the least
significant bits of the last byte.  The 8 bit characters are "filled"
in order from most significant bit to least significant.

*/

static void
write_bits(unsigned char *data, INT32 *bits, int len)
{
    int             i;		/* generic loop variable */
    unsigned char   mask;	/* The next bit position within the
				   variable "data" to place the next
				   bit. */


    /* Fill in the array bits.
     * The first compressed output bit will be the most significant
     * bit of the byte, so initialize mask to 0x80.  The next byte of
     * compressed data is initially 0, and the desired bits will be
     * turned on below.
     */
    mask = 0x80;
    *data = 0;

    for (i = 0; i < len; i++) {
	/* Turn on the next bit of output data, if necessary. */
	if (bits[i]) {
	    (*data) |= mask;
	}
	/*
	 * If the byte data is full, determined by mask becoming 0,
	 * then write the byte to the output file, and reinitialize
	 * data and mask for the next output byte.  Also add the byte
	 * if (i == len-1), because if len is not a multiple of 8,
	 * then mask won't yet be 0.  */
	mask >>= 1;
	if ((mask == 0) || (i == len-1)) {
	    data++;
	    *data = 0;
	    mask = 0x80;
	}
    }
}



/*

Read bits from file f into bits[0] through bits[len-1], in "packed"
format.

Read ceiling(len/8) characters from file f, if that many are available
to read, otherwise read to the end of the file.  The first character's
8 bits, in order from MSB to LSB, are used to fill bits[0] through
bits[7].  The second character's bits are used to fill bits[8] through
bits[15], and so on.  If ceiling(len/8) characters are available to
read, and len is not a multiple of 8, then some of the least
significant bits of the last character read are completely ignored.
Every entry of bits[] that is modified is changed to either a 0 or a
1.

The number of bits successfully read is returned, and is always in the
range 0 to len, inclusive.  If it is less than len, it will always be
a multiple of 8.

*/

static int
read_bits(unsigned char *data, INT32 *bits, int len)
{
	int             i,ind=0;		/* generic loop variable */
	int             c=0;

	/* Unpack the array bits into coded_frame. */
	for (i = 0; i < len; i++) {
		if ((i % 8) == 0) {
			c = (int)(data[ind]);
			ind++;
		}
		if (c & (0x80 >> (i & 7))) {
			bits[i] = 1;
			} else {
			bits[i] = 0;
		}
	}
	return (len);
}




Codec *LPC10codec_new()
{
	LPC10Codec *obj;
	
	obj=(LPC10Codec*)malloc(sizeof(LPC10Codec));
	obj->baseclass._getinfo=&wlpc10_getinfo;
	obj->baseclass._encode=&wlpc10_encode;
	obj->baseclass._decode=&wlpc10_decode;
	obj->baseclass._destroy=&wlpc10_destroy;

	obj->enc = create_lpc10_encoder_state();
  if (obj->enc == NULL) {
		fprintf(stderr, "Couldn't allocate %d bytes for encoder state.\n",
		sizeof(struct lpc10_encoder_state));
		return(NULL);
	}
	obj->dec = create_lpc10_decoder_state();
	if (obj->dec == NULL) {
		fprintf(stderr, "Couldn't allocate %d bytes for decoder state.\n",
					sizeof(struct lpc10_decoder_state));
		return(NULL);
	}
	return((Codec*)obj);
}


  	
	
void wlpc10_getinfo(Codec *codec, struct codec_info *info)
{
	 memcpy(info,&LPC10codec_info,sizeof(struct codec_info));
}
void wlpc10_encode(Codec *codec, char *frame, char *data)
{
	float speech[LPC10_SAMPLES_PER_FRAME];
	INT32 bits[LPC10_BITS_IN_COMPRESSED_FRAME];
	LPC10Codec *obj=(LPC10Codec*)codec;  /* we should make a few check to see if this codec is a LPC10...*/
	
	read_16bit_samples((INT16*)frame, speech, LPC10_SAMPLES_PER_FRAME);
	lpc10_encode(speech, bits, obj->enc);
	write_bits(data, bits, LPC10_BITS_IN_COMPRESSED_FRAME);
}

void wlpc10_decode(Codec *codec,char *data, char *frame)
{
	float speech[LPC10_SAMPLES_PER_FRAME];
	INT32 bits[LPC10_BITS_IN_COMPRESSED_FRAME];
	LPC10Codec *obj=(LPC10Codec*)codec;  /* we should make a few check to see if this codec is a LPC10...*/
	
	read_bits(data, bits, LPC10_BITS_IN_COMPRESSED_FRAME);
	lpc10_decode(bits,speech, obj->dec);
	write_16bit_samples((INT16*)frame, speech, LPC10_SAMPLES_PER_FRAME);
}

void wlpc10_destroy(Codec *codec)
{
	LPC10Codec *obj=(LPC10Codec*)codec;  /* we should make a few check to see if this codec is a LPC10...*/
	
	free(obj->enc);
	free(obj->dec);
	free(obj);
}