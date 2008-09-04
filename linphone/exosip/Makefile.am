noinst_LTLIBRARIES = libeXosip.la


AM_CFLAGS = $(EXOSIP_CFLAGS)

libeXosip_la_SOURCES =       eXosip.c  \
jrequest.c jresponse.c    jcallback.c  jdialog.c   udp.c  \
jcall.c jreg.c jfreinds.c jidentity.c  eXutils.c          \
jnotify.c      jsubscribers.c          jsubscribe.c    jevents.c \
sdp_offans.c   misc.c                  eXosip2.h         \
jpipe.c        jpipe.h                 jauth.c     jpublish.c \
eXosip.h eXosip_cfg.h

libeXosip_la_LIBADD = $(EXOSIP_LIBS)

INCLUDES = -I$(top_srcdir)
