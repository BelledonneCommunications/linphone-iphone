
OBJEXT=o
AR = ar
RANLIB = ranlib
DEFS= -DG_LOG_DOMAIN=\"MediaStreamer\"
INCLUDES=-I/usr/local/include/glib-2.0   -I/usr/local/lib/glib-2.0/include/ \
                  -I../gsmlib/   -I../lpc10-1.5  -I../oRTP
COMPILE= gcc $(DEFS)   $(INCLUDES)
LIBTOOL=libtool
LDFLAGS=-L/usr/local/lib/ -lglib-1.3 -lgthread-1.3 -lpthread
LINK = $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

libmediastreamer_a_OBJECTS =  msfilter.$(OBJEXT) msbuffer.$(OBJEXT) \
msqueue.$(OBJEXT) msfifo.$(OBJEXT) ms.$(OBJEXT) mssync.$(OBJEXT) \
msnosync.$(OBJEXT) msread.$(OBJEXT) mswrite.$(OBJEXT) mscopy.$(OBJEXT) \
msv4lsource.$(OBJEXT) msoss.$(OBJEXT) msosswrite.$(OBJEXT) \
msossread.$(OBJEXT) msringplayer.$(OBJEXT) msGSMencoder.$(OBJEXT) \
msGSMdecoder.$(OBJEXT) msLPC10encoder.$(OBJEXT) \
msLPC10decoder.$(OBJEXT)

all:  libmediastreamer.a mstest


.c.o:
	$(COMPILE) -c $<

libmediastreamer.a: $(libmediastreamer_a_OBJECTS)
	-rm -f libmediastreamer.a
	$(AR) cru libmediastreamer.a $(libmediastreamer_a_OBJECTS)
	$(RANLIB) libmediastreamer.a


mstest:	test.o libmediastreamer.a
	gcc -o mstest test.o libmediastreamer.a $(LDFLAGS) -Wl,-rpath /usr/local/lib
