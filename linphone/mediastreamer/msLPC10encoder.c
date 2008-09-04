/*
  The mediastreamer library aims at providing modular media processing and I/O
	for linphone, but also for any telephony application.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org
  										
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdlib.h>
#include "msLPC10encoder.h"
#include <lpc10.h>


extern MSCodecInfo LPC10info;

/* The return value of each of these calls is the same as that
   returned by fread/fwrite, which should be the number of samples
   successfully read/written, not the number of bytes. */

int
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



int
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

void
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

int
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




static MSLPC10EncoderClass *ms_LPC10encoder_class=NULL;

MSFilter * ms_LPC10encoder_new(void)
{
	MSLPC10Encoder *r;
	
	r=g_new(MSLPC10Encoder,1);
	ms_LPC10encoder_init(r);
	if (ms_LPC10encoder_class==NULL)
	{
		ms_LPC10encoder_class=g_new(MSLPC10EncoderClass,1);
		ms_LPC10encoder_class_init(ms_LPC10encoder_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_LPC10encoder_class);
	return(MS_FILTER(r));
}
	

/* FOR INTERNAL USE*/
void ms_LPC10encoder_init(MSLPC10Encoder *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->infifos=r->f_inputs;
	MS_FILTER(r)->outfifos=r->f_outputs;
	MS_FILTER(r)->r_mingran=LPC10_SAMPLES_PER_FRAME*2;
	memset(r->f_inputs,0,sizeof(MSFifo*)*MSLPC10ENCODER_MAX_INPUTS);
	memset(r->f_outputs,0,sizeof(MSFifo*)*MSLPC10ENCODER_MAX_INPUTS);
	r->lpc10_enc=create_lpc10_encoder_state();
}

void ms_LPC10encoder_class_init(MSLPC10EncoderClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"LPC10Enc");
	MS_FILTER_CLASS(klass)->max_finputs=MSLPC10ENCODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->max_foutputs=MSLPC10ENCODER_MAX_INPUTS;
	MS_FILTER_CLASS(klass)->r_maxgran=LPC10_SAMPLES_PER_FRAME*2;
	MS_FILTER_CLASS(klass)->w_maxgran=7;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_LPC10encoder_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_LPC10encoder_process;
	MS_FILTER_CLASS(klass)->info=(MSFilterInfo*)&LPC10info;
}
	
void ms_LPC10encoder_process(MSLPC10Encoder *r)
{
	MSFifo *fi,*fo;
	int err1;
	void *s,*d;
	float speech[LPC10_SAMPLES_PER_FRAME];
	INT32 bits[LPC10_BITS_IN_COMPRESSED_FRAME];
	
	/* process output fifos, but there is only one for this class of filter*/
	
	fi=r->f_inputs[0];
	fo=r->f_outputs[0];
	if (fi!=NULL)
	{
		err1=ms_fifo_get_read_ptr(fi,LPC10_SAMPLES_PER_FRAME*2,&s);
		if (err1>0)
		{
			err1=ms_fifo_get_write_ptr(fo,7,&d);
			if (d!=NULL)
			{
				read_16bit_samples((INT16*)s, speech, LPC10_SAMPLES_PER_FRAME);
				lpc10_encode(speech, bits, r->lpc10_enc);
				write_bits(d, bits, LPC10_BITS_IN_COMPRESSED_FRAME);
			}
		}
		
	}
}

void ms_LPC10encoder_uninit(MSLPC10Encoder *obj)
{
	free(obj->lpc10_enc);
}

void ms_LPC10encoder_destroy( MSLPC10Encoder *obj)
{
	ms_LPC10encoder_uninit(obj);
	g_free(obj);
}
