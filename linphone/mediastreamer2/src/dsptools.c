/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2009  Simon MORLAT (simon.morlat@linphone.org)

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

/* This file contains useful DSP routines from the speex project.
*/

/* Copyright (C) 2002-2006 Jean-Marc Valin 
   File: filters.c
   Various analysis/synthesis filters

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <mediastreamer2/dsptools.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else
#if !defined(__APPLE__)
#include <malloc.h>
#endif
#endif

#define ALLOC(var,size,type) var = alloca(sizeof(type)*(size))

#if 0
//#ifdef ARCH_BFIN

static void filter_mem16(const ms_word16_t *_x, const ms_coef_t *num, const ms_coef_t *den, ms_word16_t *_y, int N, int ord, ms_mem_t *mem)
{
   VARDECL(ms_word32_t *xy2);
   VARDECL(ms_word32_t *numden_a);
   ms_word32_t *xy;
   ms_word16_t *numden;
   int i;

   ALLOC(xy2, (N+1), ms_word32_t);
   ALLOC(numden_a, (2*ord+2), ms_word32_t);
   xy = xy2+1;  
   numden = (ms_word16_t*) numden_a;

   for (i=0;i<ord;i++)
   {
      numden[2*i] = num[i];
      numden[2*i+1] = den[i];
   }
   __asm__ __volatile__
   (
   /* Register setup */
   "R0 = %5;\n\t"      /*ord */
   
   "P0 = %3;\n\t"
   "I0 = P0;\n\t"
   "B0 = P0;\n\t" /* numden */
   "L0 = 0;\n\t"
      
   "P2 = %0;\n\t" /* Fused xy */
   "I2 = P2;\n\t"
   "L2 = 0;\n\t"
   
   "P4 = %6;\n\t" /* mem */
   "P0 = %1;\n\t" /* _x */
   "P1 = %2;\n\t" /* _y */
   
   /* First sample */
   "R1 = [P4++];\n\t"
   "R1 <<= 3;\n\t" /* shift mem */
   "R1.L = R1 (RND);\n\t"
   "R2 = W[P0++];\n\t" /* load x[0] */
   "R1.L = R1.L + R2.L;\n\t"
   "W[P1++] = R1;\n\t" /* store y[0] */
   "R2 = PACK(R1.L, R2.L);\n\t" /* pack x16 and y16 */
   "[P2] = R2;\n\t"
               
   /* Samples 1 to ord-1 (using memory) */
   "R0 += -1;\n\t"
   "R3 = 0;\n\t"
   "LC0 = R0;\n\t"
   "LOOP filter_start%= LC0;\n\t"
   "LOOP_BEGIN filter_start%=;\n\t"
      "R3 += 1;\n\t"
      "LC1 = R3;\n\t"
      
      "R1 = [P4++];\n\t"
      "A1 = R1;\n\t"
      "A0 = 0;\n\t"
      "I0 = B0;\n\t"
      "I2 = P2;\n\t"
      "P2 += 4;\n\t"
      "R4 = [I0++] || R5 = [I2--];\n\t"
      "LOOP filter_start_inner%= LC1;\n\t"
      "LOOP_BEGIN filter_start_inner%=;\n\t"
         "A1 -= R4.H*R5.H, A0 += R4.L*R5.L (IS) || R4 = [I0++] || R5 = [I2--];\n\t"
      "LOOP_END filter_start_inner%=;\n\t"
      "A0 += A1;\n\t"
      "R4 = A0;\n\t"
      "R4 <<= 3;\n\t" /* shift mem */
      "R4.L = R4 (RND);\n\t"
      "R2 = W[P0++];\n\t" /* load x */
      "R4.L = R4.L + R2.L;\n\t"
      "W[P1++] = R4;\n\t" /* store y */
      //"R4 <<= 2;\n\t"
      //"R2 <<= 2;\n\t"
      "R2 = PACK(R4.L, R2.L);\n\t" /* pack x16 and y16 */
      "[P2] = R2;\n\t"

   "LOOP_END filter_start%=;\n\t"

   /* Samples ord to N*/   
   "R0 = %5;\n\t"
   "R0 <<= 1;\n\t"
   "I0 = B0;\n\t" /* numden */
   "R0 <<= 1;\n\t"   
   "L0 = R0;\n\t"
   
   "R0 = %5;\n\t" /* org */
   "R2 = %4;\n\t" /* N */
   "R2 = R2 - R0;\n\t"
   "R4 = [I0++];\n\t" /* numden */
   "LC0 = R2;\n\t"
   "P3 = R0;\n\t"
   "R0 <<= 2;\n\t"
   "R0 += 8;\n\t"
   "I2 = P2;\n\t"
   "M0 = R0;\n\t"
   "A1 = A0 = 0;\n\t"
   "R5 = [I2--];\n\t" /* load xy */
   "LOOP filter_mid%= LC0;\n\t"
   "LOOP_BEGIN filter_mid%=;\n\t"
      "LOOP filter_mid_inner%= LC1=P3;\n\t"
      "LOOP_BEGIN filter_mid_inner%=;\n\t"
         "A1 -= R4.H*R5.H, A0 += R4.L*R5.L (IS) || R4 = [I0++] || R5 = [I2--];\n\t"
      "LOOP_END filter_mid_inner%=;\n\t"
      "R0 = (A0 += A1) || I2 += M0;\n\t"
      "R0 = R0 << 3 || R5 = W[P0++];\n\t" /* load x */
      "R0.L = R0 (RND);\n\t"
      "R0.L = R0.L + R5.L;\n\t"
      "R5 = PACK(R0.L, R5.L) || W[P1++] = R0;\n\t" /* shift y | store y */
      "A1 = A0 = 0 || [I2--] = R5\n\t"
      "LOOP_END filter_mid%=;\n\t"
   "I2 += 4;\n\t"
   "P2 = I2;\n\t"
   /* Update memory */
   "P4 = %6;\n\t"
   "R0 = %5;\n\t"
   "LC0 = R0;\n\t"
   "P0 = B0;\n\t"
   "A1 = A0 = 0;\n\t"
   "LOOP mem_update%= LC0;\n\t"
   "LOOP_BEGIN mem_update%=;\n\t"
      "I2 = P2;\n\t"
      "I0 = P0;\n\t"
      "P0 += 4;\n\t"
      "R0 = LC0;\n\t"
      "LC1 = R0;\n\t"
      "R5 = [I2--] || R4 = [I0++];\n\t"
      "LOOP mem_accum%= LC1;\n\t"
      "LOOP_BEGIN mem_accum%=;\n\t"
         "A1 -= R4.H*R5.H, A0 += R4.L*R5.L (IS) || R4 = [I0++] || R5 = [I2--];\n\t"
      "LOOP_END mem_accum%=;\n\t"
      "R0 = (A0 += A1);\n\t"
      "A1 = A0 = 0 || [P4++] = R0;\n\t"
   "LOOP_END mem_update%=;\n\t"
   "L0 = 0;\n\t"
   : : "m" (xy), "m" (_x), "m" (_y), "m" (numden), "m" (N), "m" (ord), "m" (mem)
   : "A0", "A1", "R0", "R1", "R2", "R3", "R4", "R5", "P0", "P1", "P2", "P3", "P4", "B0", "I0", "I2", "L0", "L2", "M0", "memory"
   );

}


void ms_fir_mem16(const ms_word16_t *x, const ms_coef_t *num, ms_word16_t *y, int N, int ord, ms_mem_t *mem)
{
   int i;
   ms_coef_t den2[12];
   ms_coef_t *den;
   den = (ms_coef_t*)((((int)den2)+4)&0xfffffffc);
   for (i=0;i<10;i++)
      den[i] = 0;
   filter_mem16(x, num, den, y, N, ord, mem, stack);
}


#else
#if 0
/* this one comes from speex but unfortunately does not make the expected result, maybe it is mis-used.*/
void ms_fir_mem16(const ms_word16_t *x, const ms_coef_t *num, ms_word16_t *y, int N, int ord, ms_mem_t *mem)
{
   int i,j;
   ms_word16_t xi,yi;

   for (i=0;i<N;i++)
   {
      xi=x[i];
      yi = EXTRACT16(SATURATE(ADD32(EXTEND32(xi),PSHR32(mem[0],LPC_SHIFT)),32767));
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_16(mem[j+1], num[j],xi);
      }
      mem[ord-1] = MULT16_16(num[ord-1],xi);
      y[i] = yi;
   }
}
#else

#ifndef MS_FIXED_POINT

void ms_fir_mem16(const ms_word16_t *x, const ms_coef_t *num, ms_word16_t *y, int N, int ord, ms_mem_t *mem){
	int i,j;
	ms_word16_t xi;
	ms_word32_t acc;
	for(i=0;i<N;++i){
		xi=x[i];
		mem[0]=xi;
		/* accumulate and shift within the same loop*/
		acc=mem[ord-1]*num[ord-1];
		for(j=ord-2;j>=0;--j){
			acc+=num[j]*mem[j];
			mem[j+1]=mem[j];
		}
		y[i]=acc;
	}
}

#else

void ms_fir_mem16(const ms_word16_t *x, const ms_coef_t *num, ms_word16_t *y, int N, int ord, ms_mem_t *mem){
	int i,j;
	ms_word16_t xi;
	ms_word32_t acc;
	int shift=0; /* REVISIT: empiric value...*/

	for(i=0;i<N;++i){
		xi=x[i];
		mem[0]=xi;
		/* accumulate and shift memories within the same loop*/
		acc=(mem[ord-1]*(ms_word32_t)num[ord-1])>>shift;
		for(j=ord-2;j>=0;--j){
			acc+=(((ms_word32_t)num[j])*mem[j])>>shift;
			mem[j+1]=mem[j];
		}
		y[i]=(ms_word16_t)SATURATE16(acc>>14,32767);
	}
}


#endif

#endif

#endif

#ifdef MS_FIXED_POINT
static int maximize_range(ms_word16_t *in, ms_word16_t *out, ms_word16_t bound, int len)
{
   int i, shift;
   ms_word16_t max_val = 0;
   for (i=0;i<len;i++)
   {
      if (in[i]>max_val)
         max_val = in[i];
      if (-in[i]>max_val)
         max_val = -in[i];
   }
   shift=0;
   while (max_val <= (bound>>1) && max_val != 0)
   {
      max_val <<= 1;
      shift++;
   }
   for (i=0;i<len;i++)
   {
      out[i] = SHL16(in[i], shift);
   }   
   return shift;
}

static void renorm_range(ms_word16_t *in, ms_word16_t *out, int shift, int len)
{
   int i;
   for (i=0;i<len;i++)
   {
      out[i] = PSHR16(in[i], shift);
   }
}
#endif

#include "kiss_fftr.h"
#include "kiss_fft.h"

struct kiss_config {
   kiss_fftr_cfg forward;
   kiss_fftr_cfg backward;
   int N;
};

void *ms_fft_init(int size)
{
	struct kiss_config *table;
	table = (struct kiss_config*)ms_malloc(sizeof(struct kiss_config));
	table->forward = kiss_fftr_alloc(size,0,NULL,NULL);
	table->backward = kiss_fftr_alloc(size,1,NULL,NULL);
	table->N = size;
	return table;
}

void ms_fft_destroy(void *table)
{
	struct kiss_config *t = (struct kiss_config *)table;
	kiss_fftr_free(t->forward);
	kiss_fftr_free(t->backward);
	ms_free(table);
}

#ifdef MS_FIXED_POINT

void ms_fft(void *table, ms_word16_t *in, ms_word16_t *out)
{
	int shift;
	struct kiss_config *t = (struct kiss_config *)table;
	shift = maximize_range(in, in, 32000, t->N);
	kiss_fftr2(t->forward, in, out);
	renorm_range(in, in, shift, t->N);
	renorm_range(out, out, shift, t->N);
}

#else

void ms_fft(void *table, ms_word16_t *in, ms_word16_t *out)
{
	int i;
	float scale;
	struct kiss_config *t = (struct kiss_config *)table;
	scale = 1./t->N;
	kiss_fftr2(t->forward, in, out);
	for (i=0;i<t->N;i++)
		out[i] *= scale;
}
#endif

void ms_ifft(void *table, ms_word16_t *in, ms_word16_t *out)
{
	struct kiss_config *t = (struct kiss_config *)table;
	kiss_fftri2(t->backward, in, out);
}
