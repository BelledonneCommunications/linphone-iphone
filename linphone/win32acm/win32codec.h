#ifndef _WIN32CODEC_H_
#define _WIN32CODEC_H_

/*
  Copyright 2003 Robert W. Brewer <rbrewer at op.net>

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

/*
  Wrapper for Windows ACM functions mainly designed to make it easier
  to write Linphone codecs, but may be useful in other applications
  too.
*/


#include "wine/msacm.h"
#include "wineacm.h"

typedef struct _Win32Codec
{
  HACMSTREAM handle;
  unsigned int min_insize;
  unsigned int min_outsize;
} Win32Codec;


typedef struct _Win32CodecDriver
{
  PWINE_ACMDRIVERID id;
} Win32CodecDriver;


/* Register an ACM DLL for the corresponding formatTag.
   This should be called twice for each type of codec
   needed, once with the encode flag set to encoding
   and once for decoding (assuming both will be used).
   Under the hood a PCM registration is made when encoding
   is requested.  Many instances
   of a Win32Codec in the corresponding encode/decode mode can
   be created after the DLL is registered with this call.
  Returns 0 if driver could not be created.
*/
Win32CodecDriver* win32codec_create_driver(const char* dllName,
                                           short formatTag,
                                           int encodeFlag);

void win32codec_destroy_driver(Win32CodecDriver* d);
                               



/* Create a codec that can handle 1 stream of data
   in a single direction only.  If encode is 1, the
   data will originate in 16-bit signed linear PCM format at the
   sampling rate and number of channels given in wf and be converted to wf.
   If encode is 0, it will originate in the codec specified
   by wf and be converted to 16-bit signed linear PCM at the same
   sampling rate and number of channels.

   Returns 0 if codec cannot be created correctly.
*/
Win32Codec* win32codec_create(const WAVEFORMATEX* wf,
                              int encodeFlag);
void win32codec_destroy(Win32Codec* c);


/* Best to use a multiple of min_insize and min_outsize
   for inlen and outlen.
  Returns:
      -1: conversion failure
    >= 0: number of bytes used in outbuf
*/
signed long win32codec_convert(Win32Codec* c, const void* inbuf, long inlen,
                               void* outbuf, long outlen);




#endif /* _WIN32CODEC_H_ */
