/* configuration options for the win32 ACM loader library.
 * Please edit these to correspond to what your setup requires.
 * So far it has only been tested on x86 Debian Linux (Sid).
 * If you discover you need to make changes, refer to config.h.bak
 * which is the original bloated version of this file which may
 * offer some hints.  We are trying to slim this code down as much as
 * possible which is why that file is deprecated.
 */

/* Define this if your system has the "malloc.h" header file */
#define HAVE_MALLOC_H 1


/* Define this if you have the elf dynamic linker -ldl library */
#define HAVE_LIBDL 1


/* Define this if your system has the "sys/mman.h" header file */
#define HAVE_SYS_MMAN_H 1

/* Define this if your system has vsscanf */
#define HAVE_VSSCANF 1

/* Define this if you have the kstat kernel statistics library */
#undef HAVE_LIBKSTAT

/* nanosleep support */
#define HAVE_NANOSLEEP 1

/* Win32 DLL support */
#define WIN32_PATH "/usr/local/lib/win32"

/* enables / disables QTX codecs */
#define USE_QTX_CODECS 1


/* Extension defines */
#define ARCH_X86 1

#define HAVE_3DNOW 1	// only define if you have 3DNOW (AMD k6-2, AMD Athlon, iDT WinChip, etc.)
#define HAVE_3DNOWEX 1	// only define if you have 3DNOWEX (AMD Athlon, etc.)
#define HAVE_MMX 1	// only define if you have MMX (newer x86 chips, not P54C/PPro)
#define HAVE_MMX2 1	// only define if you have MMX2 (Athlon/PIII/4/CelII)
#define HAVE_SSE 1	// only define if you have SSE (Intel Pentium III/4 or Celeron II)
#undef HAVE_SSE2	// only define if you have SSE2 (Intel Pentium 4)
#undef HAVE_ALTIVEC	// only define if you have Altivec (G4)

#ifdef HAVE_MMX
#define USE_MMX_IDCT 1
#endif

