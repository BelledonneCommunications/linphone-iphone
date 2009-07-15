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

#ifndef ms_dsptools_h
#define ms_dsptools_h

#include <mediastreamer2/mscommon.h>

typedef int32_t ms_int32_t;

#ifdef MS_FIXED_POINT

typedef short ms_word16_t;
typedef int   ms_word32_t;
typedef int ms_mem_t;
typedef short ms_coef_t;

#define QCONST16(x,bits) ((ms_word16_t)(.5+(x)*(((ms_word32_t)1)<<(bits))))
#define QCONST32(x,bits) ((ms_word32_t)(.5+(x)*(((ms_word32_t)1)<<(bits))))

#define NEG16(x) (-(x))
#define NEG32(x) (-(x))
#define EXTRACT16(x) ((ms_word16_t)(x))
#define EXTEND32(x) ((ms_word32_t)(x))
#define SHR16(a,shift) ((a) >> (shift))
#define SHL16(a,shift) ((a) << (shift))
#define SHR32(a,shift) ((a) >> (shift))
#define SHL32(a,shift) ((a) << (shift))
#define PSHR16(a,shift) (SHR16((a)+((1<<((shift))>>1)),shift))
#define PSHR32(a,shift) (SHR32((a)+((EXTEND32(1)<<((shift))>>1)),shift))
#define VSHR32(a, shift) (((shift)>0) ? SHR32(a, shift) : SHL32(a, -(shift)))
#define SATURATE16(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))
#define SATURATE32(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))

#define SHR(a,shift) ((a) >> (shift))
#define SHL(a,shift) ((ms_word32_t)(a) << (shift))
#define PSHR(a,shift) (SHR((a)+((EXTEND32(1)<<((shift))>>1)),shift))
#define SATURATE(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))


#define ADD16(a,b) ((ms_word16_t)((ms_word16_t)(a)+(ms_word16_t)(b)))
#define SUB16(a,b) ((ms_word16_t)(a)-(ms_word16_t)(b))
#define ADD32(a,b) ((ms_word32_t)(a)+(ms_word32_t)(b))
#define SUB32(a,b) ((ms_word32_t)(a)-(ms_word32_t)(b))


/* result fits in 16 bits */
#define MULT16_16_16(a,b)     ((((ms_word16_t)(a))*((ms_word16_t)(b))))

/* (ms_word32_t)(ms_word16_t) gives TI compiler a hint that it's 16x16->32 multiply */
#define MULT16_16(a,b)     (((ms_word32_t)(ms_word16_t)(a))*((ms_word32_t)(ms_word16_t)(b)))

#define MAC16_16(c,a,b) (ADD32((c),MULT16_16((a),(b))))
#define MULT16_32_Q12(a,b) ADD32(MULT16_16((a),SHR((b),12)), SHR(MULT16_16((a),((b)&0x00000fff)),12))
#define MULT16_32_Q13(a,b) ADD32(MULT16_16((a),SHR((b),13)), SHR(MULT16_16((a),((b)&0x00001fff)),13))
#define MULT16_32_Q14(a,b) ADD32(MULT16_16((a),SHR((b),14)), SHR(MULT16_16((a),((b)&0x00003fff)),14))

#define MULT16_32_Q11(a,b) ADD32(MULT16_16((a),SHR((b),11)), SHR(MULT16_16((a),((b)&0x000007ff)),11))
#define MAC16_32_Q11(c,a,b) ADD32(c,ADD32(MULT16_16((a),SHR((b),11)), SHR(MULT16_16((a),((b)&0x000007ff)),11)))

#define MULT16_32_P15(a,b) ADD32(MULT16_16((a),SHR((b),15)), PSHR(MULT16_16((a),((b)&0x00007fff)),15))
#define MULT16_32_Q15(a,b) ADD32(MULT16_16((a),SHR((b),15)), SHR(MULT16_16((a),((b)&0x00007fff)),15))
#define MAC16_32_Q15(c,a,b) ADD32(c,ADD32(MULT16_16((a),SHR((b),15)), SHR(MULT16_16((a),((b)&0x00007fff)),15)))


#define MAC16_16_Q11(c,a,b)     (ADD32((c),SHR(MULT16_16((a),(b)),11)))
#define MAC16_16_Q13(c,a,b)     (ADD32((c),SHR(MULT16_16((a),(b)),13)))
#define MAC16_16_P13(c,a,b)     (ADD32((c),SHR(ADD32(4096,MULT16_16((a),(b))),13)))

#define MULT16_16_Q11_32(a,b) (SHR(MULT16_16((a),(b)),11))
#define MULT16_16_Q13(a,b) (SHR(MULT16_16((a),(b)),13))
#define MULT16_16_Q14(a,b) (SHR(MULT16_16((a),(b)),14))
#define MULT16_16_Q15(a,b) (SHR(MULT16_16((a),(b)),15))

#define MULT16_16_P13(a,b) (SHR(ADD32(4096,MULT16_16((a),(b))),13))
#define MULT16_16_P14(a,b) (SHR(ADD32(8192,MULT16_16((a),(b))),14))
#define MULT16_16_P15(a,b) (SHR(ADD32(16384,MULT16_16((a),(b))),15))

#define MUL_16_32_R15(a,bh,bl) ADD32(MULT16_16((a),(bh)), SHR(MULT16_16((a),(bl)),15))

#define DIV32_16(a,b) ((ms_word16_t)(((ms_word32_t)(a))/((ms_word16_t)(b))))
#define PDIV32_16(a,b) ((ms_word16_t)(((ms_word32_t)(a)+((ms_word16_t)(b)>>1))/((ms_word16_t)(b))))
#define DIV32(a,b) (((ms_word32_t)(a))/((ms_word32_t)(b)))
#define PDIV32(a,b) (((ms_word32_t)(a)+((ms_word16_t)(b)>>1))/((ms_word32_t)(b)))

#ifdef ARM5E_ASM
#error "Fix me"
#elif defined (ARM4_ASM)
#error "Fix me"
#elif defined (BFIN_ASM)

#undef PDIV32_16
static inline ms_word16_t PDIV32_16(ms_word32_t a, ms_word16_t b)
{
   ms_word32_t res, bb;
   bb = b;
   a += b>>1;
   __asm__  (
         "P0 = 15;\n\t"
         "R0 = %1;\n\t"
         "R1 = %2;\n\t"
         //"R0 = R0 + R1;\n\t"
         "R0 <<= 1;\n\t"
         "DIVS (R0, R1);\n\t"
         "LOOP divide%= LC0 = P0;\n\t"
         "LOOP_BEGIN divide%=;\n\t"
            "DIVQ (R0, R1);\n\t"
         "LOOP_END divide%=;\n\t"
         "R0 = R0.L;\n\t"
         "%0 = R0;\n\t"
   : "=m" (res)
   : "m" (a), "m" (bb)
   : "P0", "R0", "R1", "cc");
   return res;
}

#undef DIV32_16
static inline ms_word16_t DIV32_16(ms_word32_t a, ms_word16_t b)
{
   ms_word32_t res, bb;
   bb = b;
   /* Make the roundinf consistent with the C version 
      (do we need to do that?)*/
   if (a<0) 
      a += (b-1);
   __asm__  (
         "P0 = 15;\n\t"
         "R0 = %1;\n\t"
         "R1 = %2;\n\t"
         "R0 <<= 1;\n\t"
         "DIVS (R0, R1);\n\t"
         "LOOP divide%= LC0 = P0;\n\t"
         "LOOP_BEGIN divide%=;\n\t"
            "DIVQ (R0, R1);\n\t"
         "LOOP_END divide%=;\n\t"
         "R0 = R0.L;\n\t"
         "%0 = R0;\n\t"
   : "=m" (res)
   : "m" (a), "m" (bb)
   : "P0", "R0", "R1", "cc");
   return res;
}

#undef MAX16
static inline ms_word16_t MAX16(ms_word16_t a, ms_word16_t b)
{
   ms_word32_t res;
   __asm__  (
         "%1 = %1.L (X);\n\t"
         "%2 = %2.L (X);\n\t"
         "%0 = MAX(%1,%2);"
   : "=d" (res)
   : "%d" (a), "d" (b)
   );
   return res;
}

#undef MULT16_32_Q15
static inline ms_word32_t MULT16_32_Q15(ms_word16_t a, ms_word32_t b)
{
   ms_word32_t res;
   __asm__
   (
         "A1 = %2.L*%1.L (M);\n\t"
         "A1 = A1 >>> 15;\n\t"
         "%0 = (A1 += %2.L*%1.H) ;\n\t"
   : "=&W" (res), "=&d" (b)
   : "d" (a), "1" (b)
   : "A1"
   );
   return res;
}

#undef MAC16_32_Q15
static inline ms_word32_t MAC16_32_Q15(ms_word32_t c, ms_word16_t a, ms_word32_t b)
{
   ms_word32_t res;
   __asm__
         (
         "A1 = %2.L*%1.L (M);\n\t"
         "A1 = A1 >>> 15;\n\t"
         "%0 = (A1 += %2.L*%1.H);\n\t"
         "%0 = %0 + %4;\n\t"
   : "=&W" (res), "=&d" (b)
   : "d" (a), "1" (b), "d" (c)
   : "A1"
         );
   return res;
}

#undef MULT16_32_Q14
static inline ms_word32_t MULT16_32_Q14(ms_word16_t a, ms_word32_t b)
{
   ms_word32_t res;
   __asm__
         (
         "%2 <<= 1;\n\t"
         "A1 = %1.L*%2.L (M);\n\t"
         "A1 = A1 >>> 15;\n\t"
         "%0 = (A1 += %1.L*%2.H);\n\t"
   : "=W" (res), "=d" (a), "=d" (b)
   : "1" (a), "2" (b)
   : "A1"
         );
   return res;
}

#undef MAC16_32_Q14
static inline ms_word32_t MAC16_32_Q14(ms_word32_t c, ms_word16_t a, ms_word32_t b)
{
   ms_word32_t res;
   __asm__
         (
         "%1 <<= 1;\n\t"
         "A1 = %2.L*%1.L (M);\n\t"
         "A1 = A1 >>> 15;\n\t"
         "%0 = (A1 += %2.L*%1.H);\n\t"
         "%0 = %0 + %4;\n\t"
   : "=&W" (res), "=&d" (b)
   : "d" (a), "1" (b), "d" (c)
   : "A1"
         );
   return res;
}



#endif



#else

typedef float ms_mem_t;
typedef float ms_coef_t;
typedef float ms_lsp_t;
typedef float ms_sig_t;
typedef float ms_word16_t;
typedef float ms_word32_t;

#define Q15ONE 1.0f
#define LPC_SCALING  1.f
#define SIG_SCALING  1.f
#define LSP_SCALING  1.f
#define GAMMA_SCALING 1.f
#define GAIN_SCALING 1.f
#define GAIN_SCALING_1 1.f


#define VERY_SMALL 1e-15f
#define VERY_LARGE32 1e15f
#define VERY_LARGE16 1e15f
#define Q15_ONE ((ms_word16_t)1.f)

#define QCONST16(x,bits) (x)
#define QCONST32(x,bits) (x)

#define NEG16(x) (-(x))
#define NEG32(x) (-(x))
#define EXTRACT16(x) (x)
#define EXTEND32(x) (x)
#define SHR16(a,shift) (a)
#define SHL16(a,shift) (a)
#define SHR32(a,shift) (a)
#define SHL32(a,shift) (a)
#define PSHR16(a,shift) (a)
#define PSHR32(a,shift) (a)
#define VSHR32(a,shift) (a)
#define SATURATE16(x,a) (x)
#define SATURATE32(x,a) (x)

#define PSHR(a,shift)       (a)
#define SHR(a,shift)       (a)
#define SHL(a,shift)       (a)
#define SATURATE(x,a) (x)

#define ADD16(a,b) ((a)+(b))
#define SUB16(a,b) ((a)-(b))
#define ADD32(a,b) ((a)+(b))
#define SUB32(a,b) ((a)-(b))
#define MULT16_16_16(a,b)     ((a)*(b))
#define MULT16_16(a,b)     ((ms_word32_t)(a)*(ms_word32_t)(b))
#define MAC16_16(c,a,b)     ((c)+(ms_word32_t)(a)*(ms_word32_t)(b))

#define MULT16_32_Q11(a,b)     ((a)*(b))
#define MULT16_32_Q13(a,b)     ((a)*(b))
#define MULT16_32_Q14(a,b)     ((a)*(b))
#define MULT16_32_Q15(a,b)     ((a)*(b))
#define MULT16_32_P15(a,b)     ((a)*(b))

#define MAC16_32_Q11(c,a,b)     ((c)+(a)*(b))
#define MAC16_32_Q15(c,a,b)     ((c)+(a)*(b))

#define MAC16_16_Q11(c,a,b)     ((c)+(a)*(b))
#define MAC16_16_Q13(c,a,b)     ((c)+(a)*(b))
#define MAC16_16_P13(c,a,b)     ((c)+(a)*(b))
#define MULT16_16_Q11_32(a,b)     ((a)*(b))
#define MULT16_16_Q13(a,b)     ((a)*(b))
#define MULT16_16_Q14(a,b)     ((a)*(b))
#define MULT16_16_Q15(a,b)     ((a)*(b))
#define MULT16_16_P15(a,b)     ((a)*(b))
#define MULT16_16_P13(a,b)     ((a)*(b))
#define MULT16_16_P14(a,b)     ((a)*(b))

#define DIV32_16(a,b)     (((ms_word32_t)(a))/(ms_word16_t)(b))
#define PDIV32_16(a,b)     (((ms_word32_t)(a))/(ms_word16_t)(b))
#define DIV32(a,b)     (((ms_word32_t)(a))/(ms_word32_t)(b))
#define PDIV32(a,b)     (((ms_word32_t)(a))/(ms_word32_t)(b))


#endif

#define MIN16(a,b) ((a) < (b) ? (a) : (b))   /**< Maximum 16-bit value.   */

#ifdef __cplusplus
extern "C"{
#endif

/*abstraction layer over kiss fft, taken from speex as well*/

/** Compute tables for an FFT */
void *ms_fft_init(int size);

/** Destroy tables for an FFT */
void ms_fft_destroy(void *table);

/** Forward (real to half-complex) transform */
void ms_fft(void *table, ms_word16_t *in, ms_word16_t *out);

/** Backward (half-complex to real) transform */
void ms_ifft(void *table, ms_word16_t *in, ms_word16_t *out);

/** digital filtering api*/
void ms_fir_mem16(const ms_word16_t *x, const ms_coef_t *num, ms_word16_t *y, int N, int ord, ms_mem_t *mem);

#ifdef __cplusplus
}
#endif

#endif
