/* test_truespeech.c
 *
 * Copyright 2003 Robert W. Brewer <rbrewer at op.net>
 *  and based on some initial code to load the
 *  TrueSpeech DLL with these libraries by
 *  Piotr P. Karwasz <karwasz at clipper.en.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include "loader.h"
#include "driver.h"
#include "wine/windef.h"
#include "wineacm.h"
#include "string.h"

#define PCM_INFILE  "pcmin.raw"
#define TS_OUTFILE  "truespeechout.raw"
#define PCM_OUTFILE "pcmout.raw"

int verbose=5;
char* get_path(char* x){  return strdup(x);}

#define TRUESPEECH_FORMAT 0x22
#define TRUESPEECH_CBSIZE 32

// Looks like entire .wav header for a TrueSpeech
// file is 90 bytes long.  This means if we assume
// we have a TrueSpeech input file we can just skip
// the first 90 bytes instead of deleting them first
// with a hex editor to create a .raw file.



void* xmalloc(size_t s)
{
  void* p = malloc(s);
  if (!p)
  {
    printf("out of memory\n");
    exit(1);
  }

  return p;
}


// infilepcm: 1 if infile is pcm, outfile is truespeech
//            0 if infile is truespeech, outfile is pcm
// Returns:
//   0: success
//   1: error
int convertfile(char* infile, char* outfile, int infilepcm)
{
  WAVEFORMATEX *ts_wf = 0;
  WAVEFORMATEX pcm_wf;
  WAVEFORMATEX *i_wf = 0;
  WAVEFORMATEX *o_wf = 0;
  HACMSTREAM handle;
  MMRESULT ret;
  DWORD srcsize = 0;
  DWORD destsize = 0;
  long* iptr = 0;
  PWINE_ACMDRIVERID acmDriver1 = 0;
  PWINE_ACMDRIVERID acmDriver2 = 0;
  ACMSTREAMHEADER header;
  FILE* i_file;
  FILE* o_file;
  

  // TrueSpeech format
  ts_wf = xmalloc(sizeof(WAVEFORMATEX) + TRUESPEECH_CBSIZE);
  memset(ts_wf, 0, sizeof(*ts_wf) + TRUESPEECH_CBSIZE);
  
  ts_wf->wFormatTag      = TRUESPEECH_FORMAT;
  ts_wf->nChannels       = 1;
  ts_wf->nSamplesPerSec  = 8000;
  ts_wf->wBitsPerSample  = 1;
  ts_wf->nBlockAlign     = 32;
  ts_wf->nAvgBytesPerSec = 1067;
  ts_wf->cbSize          = TRUESPEECH_CBSIZE;

  // write extra data needed by TrueSpeech codec found
  // from examining a TrueSpeech .wav file header
  iptr = (long*)(ts_wf + 1);
  *iptr = 0x00f00001;

//  print_wave_header(in_fmt);

  // Typical PCM format, 16-bit signed samples at 8 KHz.
  memset(&pcm_wf, 0, sizeof(pcm_wf));
  
  pcm_wf.wFormatTag      = WAVE_FORMAT_PCM;
  pcm_wf.nChannels       = 1;
  pcm_wf.nSamplesPerSec  = 8000;
  pcm_wf.wBitsPerSample  = 16;
  pcm_wf.nBlockAlign     = pcm_wf.nChannels * pcm_wf.wBitsPerSample / 8;
  pcm_wf.nAvgBytesPerSec = pcm_wf.nSamplesPerSec * pcm_wf.nBlockAlign;
    
//  print_wave_header(priv->o_wf);

  // decide which way to perform the conversion
  if (infilepcm)
  {
    i_wf = &pcm_wf;
    o_wf = ts_wf;
  }
  else
  {
    i_wf = ts_wf;
    o_wf = &pcm_wf;
  }
  
  // from here on out, everything is pretty generic as long as we
  // use i_wf and o_wf

  acmDriver1 = MSACM_RegisterDriver("tssoft32.acm", TRUESPEECH_FORMAT, 0);
  if (!acmDriver1)
  {
    printf("error registering TrueSpeech ACM driver for TrueSpeech\n");
    return 1;
  }
  
  printf("register driver complete for truespeech\n");


  acmDriver2 = MSACM_RegisterDriver("tssoft32.acm", WAVE_FORMAT_PCM, 0);
  if (!acmDriver2)
  {
    printf("error registering TrueSpeech ACM driver for PCM\n");
    return 1;
  }
  
  printf("register driver complete for pcm\n");

  // TrueSpeech isn't confident it can do realtime
  // compression.  Tell it we don't care and it's happy.
  ret = acmStreamOpen(&handle, 0, i_wf, o_wf, 0, 0, 0,
                      ACM_STREAMOPENF_NONREALTIME);
  if (ret)
  {
    if (ret == ACMERR_NOTPOSSIBLE)
    {
      printf("invalid codec\n");
    }
    else
    {
      printf("acmStreamOpen error %d\n", ret);
    }

    return 1;
  }
  
  printf("audio codec opened\n");

  // we assume that the format with the largest block alignment is
  // the most picky.  And it basically turns out that TrueSpeech has
  // the largest block alignment.  Anyway, we use that block alignment
  // to ask for the preferred size of the "other" format's buffer.
  // Then we turn around and ask for the first one's preferred buffer
  // size based on the other's buffer size, which seems to turn out
  // to be its block alignment.  Or something like that.
  if (i_wf->nBlockAlign > o_wf->nBlockAlign)
  {
    ret = acmStreamSize(handle, i_wf->nBlockAlign, &destsize, ACM_STREAMSIZEF_SOURCE);
    if (ret)
    {
      printf("acmStreamSize error %d\n", ret);
      return 1;
    }
  
    printf("Audio ACM output buffer min. size: %ld\n", destsize);
    if (!destsize)
    {
      printf("ACM codec reports destsize=0\n");
      return 1;
    }

    ret = acmStreamSize(handle, destsize, &srcsize, ACM_STREAMSIZEF_DESTINATION);
    if (ret)
    {
      printf("acmStreamSize error\n");
      return 1;
    }

    printf("Audio ACM input buffer min. size: %ld\n", srcsize);
  }
  else
  {
    ret = acmStreamSize(handle, o_wf->nBlockAlign, &srcsize, ACM_STREAMSIZEF_DESTINATION);
    if (ret)
    {
      printf("acmStreamSize error %d\n", ret);
      return 1;
    }
  
    printf("Audio ACM input buffer min. size: %ld\n", srcsize);
    if (!srcsize)
    {
      printf("ACM codec reports srcsize=0\n");
      return 1;
    }

    ret = acmStreamSize(handle, srcsize, &destsize, ACM_STREAMSIZEF_SOURCE);
    if (ret)
    {
      printf("acmStreamSize error\n");
      return 1;
    }

    printf("Audio ACM input buffer min. size: %ld\n", destsize);
  }


  // set up conversion buffers

  memset(&header, 0, sizeof(header));
  header.cbStruct    = sizeof(header);
  header.cbSrcLength = srcsize;
  header.pbSrc       = xmalloc(header.cbSrcLength);
  header.cbDstLength = destsize;
  header.pbDst       = xmalloc(header.cbDstLength);
  
  ret = acmStreamPrepareHeader(handle, &header, 0);
  if (ret)
  {
    printf("error preparing header\n");
    return 1;
  }

  printf("header prepared %lx\n", header.fdwStatus);
  
  
  // open files
  i_file = fopen(infile, "rb");
  if (!i_file)
  {
    printf("error opening input file %s\n", infile);
    return 1;
  }

  printf("input file opened\n");
  
  
  o_file = fopen(outfile, "wb");
  if (!o_file)
  {
    printf("error opening output file %s\n", outfile);
    return 1;
  }


  printf("output file opened\n");

  int inbytes = 0;
  
  while (1)
  {
    inbytes = fread(header.pbSrc, 1, srcsize, i_file);

    if (inbytes != srcsize)
    {
      if (feof(i_file))
      {
        printf("end of input file, last byte count %d\n", inbytes);
        break;
      }
      
      printf("read error\n");
      return 1;
    }

    ret = acmStreamConvert(handle, &header, ACM_STREAMCONVERTF_BLOCKALIGN);
    if (ret)
    {
      printf("conversion error\n");
      return 1;
    }

    if (!(header.fdwStatus & ACMSTREAMHEADER_STATUSF_DONE))
    {
      printf("header not marked done %lx\n", header.fdwStatus);
      return 1;
    }

    if (header.cbSrcLengthUsed != header.cbSrcLength)
    {
      printf("didn't use all of source data\n");
      return 1;
    }

    printf("converted %d bytes to %d bytes\n",
           header.cbSrcLengthUsed,
           header.cbDstLengthUsed);

    if (fwrite(header.pbDst, 1, header.cbDstLengthUsed, o_file) !=
        header.cbDstLengthUsed)
    {
      printf("error writing file\n");
      return 1;
    }      
  }

  // now convert the remaining bit of the file
  // and ask for any leftover stuff

  header.cbSrcLength = inbytes;
  while (1)
  {
    if (header.cbSrcLength)
    {
      // not a full block, but not quite the "end" either
      ret = acmStreamConvert(handle, &header, 0);
    }
    else
    {
      // now we're at the end, let's see if anything is left
      ret = acmStreamConvert(handle, &header, ACM_STREAMCONVERTF_END);
    }
    
    if (ret)
    {
      printf("conversion error\n");
      return 1;
    }
    
    if (!(header.fdwStatus & ACMSTREAMHEADER_STATUSF_DONE))
    {
      printf("header not marked done %lx\n", header.fdwStatus);
      return 1;
    }
    
    if (header.cbSrcLengthUsed != header.cbSrcLength)
    {
      printf("didn't use all of source data\n");
      return 1;
    }
    
    printf("converted %d bytes to %d bytes\n",
           header.cbSrcLengthUsed,
           header.cbDstLengthUsed);

    if (!header.cbDstLengthUsed)
    {
      printf("nothing given to output, must be done\n");
      break;
    }
    
    if (fwrite(header.pbDst, 1, header.cbDstLengthUsed, o_file) !=
        header.cbDstLengthUsed)
    {
      printf("error writing file\n");
      return 1;
    }

    // ensure we are getting the remaining stuff on the next time around
    header.cbSrcLength = 0;
  }
  
    
  printf("finished converting file\n");
  

  // now close up shop

  fclose(i_file);
  fclose(o_file);

  ret = acmStreamUnprepareHeader(handle, &header, 0);
  if (ret)
  {
    printf("error unpreparing header\n");
    return 1;
  }

  free(header.pbSrc);
  free(header.pbDst);
  
    
  ret = acmStreamClose(handle, 0);
  if (ret)
  {
    printf("error closing acm stream\n");
    return 1;
  }

  printf("acm stream closed\n");

  MSACM_UnregisterDriver(acmDriver1);

  printf("acm driver unregistered for truespeech\n");

  MSACM_UnregisterDriver(acmDriver2);

  printf("acm driver unregistered for pcm\n");

  free(ts_wf);
  
  return 0;
}


int main (int argc, char **argv)
{
  printf("converting pcm to truespeech\n");

  if (convertfile(PCM_INFILE, TS_OUTFILE, 1))
  {
    printf("error converting pcm -> truespeech\n");
    return 1;
  }

  printf("converting truespeech to pcm\n");
  
  if (convertfile(TS_OUTFILE, PCM_OUTFILE, 0))
  {
    printf("error converting truespeech -> pcm\n");
    return 1;
  }

  printf("all done\n");

  return 0;
}
