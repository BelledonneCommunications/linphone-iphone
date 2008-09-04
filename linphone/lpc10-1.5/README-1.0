              U.S. Department of Defense
             LPC-10 2400 bps Voice Coder
                   Release 1.0
                   October 1993


Contents:
   README               - This file
   FAQ                  - Frequently Asked Questions items for LPC and CELP
   lpc55                - Fortran code for LPC-10 Version 55
   lpc55-C              - C code for LPC-10
   data                 - Sample speech files, containing:
     dam9.spd             - Sample input speech file
     dam9_lpc55.spd       - Speech processed by LPC-10
   abtool_1.2		- Sun GUI tool for playing listening comparisons
			   between speech files (this is a beta version)

  The distribution file has been compressed with the GNU compression
program gzip version 1.2.3, available from archives such as ftp.uu.net and
wuarchive.wustl.edu.  To unpack it, use the commands:

  gunzip lpc-1.0.tar        (uncompress the archive file)
  tar xvf lpc-1.0.tar       (extract the contents of the archive)

(For distribution on pcfs floppy disks (IBM PC format), the file names may
 not appear exactly as shown above.)

  Documentation on using the above programs is included in README files
in each directory.  Documentation describing the internal operation of
LPC-10 is currently available only in hardcopy format.  We will include
more extensive documentation with future releases of this package.

  All development of LPC-10 has been done in Fortran, so the Fortran
code has been written for flexible and easy use in a research environment.
The C version was translated from the Fortran to assist in porting LPC
to Digital Signal Processors (DSPs), and does not have the debugging
features or I/O flexiblility of the Fortran version.  Both versions
have been tested on a Sun SPARCstation-10 running Solaris 2.2 and
SunOS 4.1.2.

  The speech files are in 16 bit linear format, sampled at 8000 Hz.  There
are no headers on these files, so some manipulation may be required to
play them on your system.  The following alias will play speech files on
the Sun speakerbox:

  alias play 'audioconvert -i pcm16,mono,raw,r=8k -f sun \!* | audioplay'
  play dam9.spd
