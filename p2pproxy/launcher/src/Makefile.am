JAVA_LIBS=-L$(javadir)/jre/lib/amd64 -L$(javadir)/jre/lib/i386 \
			-L$(javadir)/jre/lib/amd64/server -L$(javadir)/jre/lib/i386/server -ljvm 

JAVA_LDFLAGS=\
	-Wl,-rpath,$(javadir)/jre/lib/amd64 -Wl,-rpath,$(javadir)/jre/lib/i386 \
	-Wl,-rpath,$(javadir)/jre/lib/amd64/server -Wl,-rpath,$(javadir)/jre/lib/i386/server

lib_LTLIBRARIES=libfonis.la

libfonis_la_SOURCES=p2pproxy.c p2pproxy.h

libfonis_la_LIBADD=$(JAVA_LIBS)

libfonis_la_LDFLAGS=$(JAVA_LDFLAGS)

noinst_PROGRAMS=launcher-tester

launcher_tester_SOURCES=launcher-tester.c

launcher_tester_LDADD=libfonis.la $(JAVA_LIBS)

launcher_tester_LDFLAGS=$(JAVA_LDFLAGS)

INCLUDES=-I$(javadir)/include -I$(javadir)/include/linux
