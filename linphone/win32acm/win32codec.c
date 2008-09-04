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

#include <string.h>
#include <stdlib.h>

#include "win32codec.h"
#include "loader.h"
#include "driver.h"
#include "wine/windef.h"
#include "wineacm.h"

/* FIXME figure out a better way to deal with these two things. */
int verbose=5;
char* get_path(char* x){  return strdup(x);}


Win32CodecDriver* win32codec_create_driver(const char* dllName,
                                           short formatTag,
                                           int encodeFlag)
{
  Win32CodecDriver* d = 0;
  short format = formatTag;

  d = malloc(sizeof(Win32CodecDriver));
  if (!d)
  {
    return 0;
  }

  if (encodeFlag)
  {
    format = WAVE_FORMAT_PCM;
  }  
  
  d->id = MSACM_RegisterDriver(dllName, format, 0);
  if (!d->id)
  {
    free(d);
    return 0;
  }

  return d;
}


void win32codec_destroy_driver(Win32CodecDriver* d)
{
  MSACM_UnregisterDriver(d->id);
  free(d);
}



Win32Codec* win32codec_create(const WAVEFORMATEX* wf,
                              int encode)
{
  Win32Codec* c = 0;
  WAVEFORMATEX pcm_wf;
  WAVEFORMATEX* i_wf = 0;
  WAVEFORMATEX* o_wf = 0;
  MMRESULT ret;
  
  c = malloc(sizeof(Win32Codec));
  if (!c)
  {
    return 0;
  }

  /* Typical PCM format, 16-bit signed linear samples at
     the same rate and number of channels as the desired
     codec.
  */
  memset(&pcm_wf, 0, sizeof(pcm_wf));
  
  pcm_wf.wFormatTag      = WAVE_FORMAT_PCM;
  pcm_wf.nChannels       = wf->nChannels;
  pcm_wf.nSamplesPerSec  = wf->nSamplesPerSec;
  pcm_wf.wBitsPerSample  = 16;
  pcm_wf.nBlockAlign     = pcm_wf.nChannels * pcm_wf.wBitsPerSample / 8;
  pcm_wf.nAvgBytesPerSec = pcm_wf.nSamplesPerSec * pcm_wf.nBlockAlign;

  /* decide which way to perform the conversion */
  if (encode)
  {
    i_wf = &pcm_wf;
    o_wf = wf;
  }
  else
  {
    i_wf = wf;
    o_wf = &pcm_wf;
  }

  /* Some codecs (like TrueSpeech) aren't confident they
     can do realtime compression.  
     Tell it we don't care and it's happy.
  */
  ret = acmStreamOpen(&c->handle, 0, i_wf, o_wf, 0, 0, 0,
                      ACM_STREAMOPENF_NONREALTIME);
  if (ret)
  {
    free(c);
    return 0;
  }

  /* calculate minimum sizes to use for conversions */
  /* we assume that the format with the largest block alignment is
     the most picky.  And it basically turns out that TrueSpeech has
     the largest block alignment.  Anyway, we use that block alignment
     to ask for the preferred size of the "other" format's buffer.
     Then we turn around and ask for the first one's preferred buffer
     size based on the other's buffer size, which seems to turn out
     to be its block alignment.  Or something like that.
  */
  if (i_wf->nBlockAlign > o_wf->nBlockAlign)
  {
    ret = acmStreamSize(c->handle, i_wf->nBlockAlign, &c->min_outsize,
                        ACM_STREAMSIZEF_SOURCE);
    if (ret)
    {
      acmStreamClose(c->handle, 0);
      free(c);
      return 0;
    }
  
    if (!c->min_outsize)
    {
      acmStreamClose(c->handle, 0);
      free(c);
      return 0;
    }

    ret = acmStreamSize(c->handle, c->min_outsize, &c->min_insize,
                        ACM_STREAMSIZEF_DESTINATION);
    if (ret)
    {
      acmStreamClose(c->handle, 0);
      free(c);
      return 0;
    }
  }
  else
  {
    ret = acmStreamSize(c->handle, o_wf->nBlockAlign, &c->min_insize,
                        ACM_STREAMSIZEF_DESTINATION);
    if (ret)
    {
      acmStreamClose(c->handle, 0);
      free(c);
      return 0;
    }
  
    if (!c->min_insize)
    {
      acmStreamClose(c->handle, 0);
      free(c);
      return 0;
    }

    ret = acmStreamSize(c->handle, c->min_insize, &c->min_outsize,
                        ACM_STREAMSIZEF_SOURCE);
    if (ret)
    {
      acmStreamClose(c->handle, 0);
      free(c);
      return 0;
    }
  }

  return c;
}



void win32codec_destroy(Win32Codec* c)
{
  /* FIXME need to put this back. See TODO file
     for ideas.
  */
  printf("Warning: win32codec_destroy() stubbed out\n");
/*   acmStreamClose(c->handle, 0); */
  free(c);
}



signed long win32codec_convert(Win32Codec* c,
                               const void* inbuf, long inlen,
                               void* outbuf, long outlen)
{
  ACMSTREAMHEADER header;
  MMRESULT ret;
  signed long rval;

  /* set up conversion buffers */
  memset(&header, 0, sizeof(header));
  header.cbStruct    = sizeof(header);
  header.cbSrcLength = inlen;
  header.pbSrc       = inbuf;
  header.cbDstLength = outlen;
  header.pbDst       = outbuf;
  
  ret = acmStreamPrepareHeader(c->handle, &header, 0);
  if (ret)
  {
    return -1;
  }

  /* convert */
  ret = acmStreamConvert(c->handle, &header, 0);
  if (ret)
  {
    return -1;
  }

  if (!(header.fdwStatus & ACMSTREAMHEADER_STATUSF_DONE))
  {
    return -1;
  }

  if (header.cbSrcLengthUsed != header.cbSrcLength)
  {
    return -1;
  }

  /* not sure if the header is still valid after unpreparing,
     so let's save this off
  */
  rval = header.cbDstLengthUsed;

  /* seemed to succeed, now unprepare the header */
  ret = acmStreamUnprepareHeader(c->handle, &header, 0);
  if (ret)
  {
    return -1;
  }
  
  return rval;  
}

