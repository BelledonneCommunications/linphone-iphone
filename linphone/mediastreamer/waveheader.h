/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@free.fr)

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

/* the following code was taken from a free software utility that I don't remember the name. */
/* sorry */



#include "ms.h"
#ifndef waveheader_h
#define waveheader_h

typedef struct uint16scheme
{
    unsigned char lo_byte;
    unsigned char hi_byte;
} uint16scheme_t;

typedef struct uint32scheme
{
    guint16 lo_int;
    guint16 hi_int;
} uint32scheme_t;


/* all integer in wav header must be read in least endian order */
inline guint16 _readuint16(guint16 a)
{
    guint16 res;
    uint16scheme_t *tmp1=(uint16scheme_t*)&a;

    ((uint16scheme_t *)(&res))->lo_byte=tmp1->hi_byte;
    ((uint16scheme_t *)(&res))->hi_byte=tmp1->lo_byte;
    return res;
}

inline guint32 _readuint32(guint32 a)
{
    guint32 res;
    uint32scheme_t *tmp1=(uint32scheme_t*)&a;

    ((uint32scheme_t *)(&res))->lo_int=_readuint16(tmp1->hi_int);
    ((uint32scheme_t *)(&res))->hi_int=_readuint16(tmp1->lo_int);
    return res;
}

#ifdef WORDS_BIGENDIAN
#define le_uint32(a) (_readuint32((a)))
#define le_uint16(a) (_readuint16((a)))
#define le_int16(a) ( (gint16) _readuint16((guint16)((a))) )
#else
#define le_uint32(a) (a)
#define le_uint16(a) (a)
#define le_int16(a) (a)
#endif

typedef struct _riff_t {
	char riff[4] ;	/* "RIFF" (ASCII characters) */
	guint32  len ;	/* Length of package (binary, little endian) */
	char wave[4] ;	/* "WAVE" (ASCII characters) */
} riff_t;

/* The FORMAT chunk */

typedef struct _format_t {
	char  fmt[4] ;	/* "fmt_" (ASCII characters) */
	guint32   len ;	/* length of FORMAT chunk (always 0x10) */
	guint16 que ;	/* Always 0x01 */
	guint16 channel ;	/* Channel numbers (0x01 = mono, 0x02 = stereo) */
	guint32   rate ;	/* Sample rate (binary, in Hz) */
	guint32   bps ;	/* Bytes Per Second */
	guint16 bpsmpl ;	/* bytes per sample: 1 = 8 bit Mono,
					     2 = 8 bit Stereo/16 bit Mono,
					     4 = 16 bit Stereo */
	guint16 bitpspl ;	/* bits per sample */
} format_t;

/* The DATA chunk */

typedef struct _data_t {
	char data[4] ;	/* "data" (ASCII characters) */
	int  len ;	/* length of data */
} data_t;

typedef struct _wave_header_t
{
	riff_t riff_chunk;
	format_t format_chunk;
	data_t data_chunk;
} wave_header_t;

#define wave_header_get_rate(header)		le_uint32((header)->format_chunk.rate)
#define wave_header_get_channel(header)		le_uint16((header)->format_chunk.channel)

#endif
