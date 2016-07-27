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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef waveheader_h
#define waveheader_h

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#	include <io.h>
#	ifndef R_OK
#		define R_OK 0x2
#	endif
#	ifndef W_OK
#		define W_OK 0x6
#	endif
#   ifndef F_OK
#       define F_OK 0x0
#   endif

#	ifndef S_IRUSR
#	define S_IRUSR S_IREAD
#	endif

#	ifndef S_IWUSR
#	define S_IWUSR S_IWRITE
#	endif

#	define open _open
#	define read _read
#	define write _write
#	define close _close
#	define access _access
#	define lseek _lseek
#else /*_WIN32*/

#	ifndef O_BINARY
#	define O_BINARY 0
#	endif

#endif /*!_WIN32*/

#ifdef swap16
#else
/* all integer in wav header must be read in least endian order */
static MS2_INLINE uint16_t swap16(uint16_t a)
{
	return ((a & 0xFF) << 8) | ((a & 0xFF00) >> 8);
}
#endif

#ifdef swap32
#else
static MS2_INLINE uint32_t swap32(uint32_t a)
{
	return ((a & 0xFF) << 24) | ((a & 0xFF00) << 8) |
		((a & 0xFF0000) >> 8) | ((a & 0xFF000000) >> 24);
}
#endif

#ifdef WORDS_BIGENDIAN
#define le_uint32(a) (swap32((a)))
#define le_uint16(a) (swap16((a)))
#define le_int16(a) ( (int16_t) swap16((uint16_t)((a))) )
#else
#define le_uint32(a) (a)
#define le_uint16(a) (a)
#define le_int16(a) (a)
#endif

typedef struct _riff_t {
	char riff[4] ;	/* "RIFF" (ASCII characters) */
	uint32_t  len ;	/* Length of package (binary, little endian) */
	char wave[4] ;	/* "WAVE" (ASCII characters) */
} riff_t;

/* The FORMAT chunk */

typedef struct _format_t {
	char  fmt[4] ;		/* "fmt_" (ASCII characters) */
	uint32_t   len ;	/* length of FORMAT chunk (always 0x10) */
	uint16_t  type;		/* codec type*/
	uint16_t channel ;	/* Channel numbers (0x01 = mono, 0x02 = stereo) */
	uint32_t   rate ;	/* Sample rate (binary, in Hz) */
	uint32_t   bps ;	/* Average Bytes Per Second */
	uint16_t blockalign ;	/*number of bytes per sample */
	uint16_t bitpspl ;	/* bits per sample */
} format_t;

/* The DATA chunk */

typedef struct _data_t {
	char data[4] ;	/* "data" (ASCII characters) */
	uint32_t  len ;	/* length of data */
} data_t;

typedef struct _wave_header_t
{
	riff_t riff_chunk;
	format_t format_chunk;
	data_t data_chunk;
} wave_header_t;

#ifndef _WIN32
#define WAVE_FORMAT_PCM			0x0001
#define WAVE_FORMAT_IEEE_FLOAT	0x0003
#define WAVE_FORMAT_ALAW		0x0006
#define WAVE_FORMAT_MULAW		0x0007
#define WAVE_FORMAT_EXTENSIBLE	0xFFFE
#endif

#define wave_header_get_format_type(header)	le_uint16((header)->format_chunk.type)
#define wave_header_get_rate(header)		le_uint32((header)->format_chunk.rate)
#define wave_header_get_channel(header)		le_uint16((header)->format_chunk.channel)
#define wave_header_get_bpsmpl(header) \
	le_uint16((header)->format_chunk.blockalign)

int ms_read_wav_header_from_fd(wave_header_t *header,int fd);

#endif
