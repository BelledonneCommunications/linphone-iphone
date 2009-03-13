EXTRA_DIST=dependencies dependencies-src src test-src

SUBDIRS=launcher plugin-src

ALL_JARS= \
	antbuild/dist/p2pproxy_0.1/bcprov-jdk14.jar   antbuild/dist/p2pproxy_0.1/log4j.jar \
	antbuild/dist/p2pproxy_0.1/javax.servlet.jar  antbuild/dist/p2pproxy_0.1/org.mortbay.jetty.jar \
	antbuild/dist/p2pproxy_0.1/jstun-0.7.0.jar    antbuild/dist/p2pproxy_0.1/p2pproxy.jar \
	antbuild/dist/p2pproxy_0.1/junit-4.3.1.jar    antbuild/dist/p2pproxy_0.1/sip.jar

$(ALL_JARS):	src/org/linphone/p2pproxy/*/*.java
	$(ANT) dist

jarsdir=$(datadir)/java/fonis

jars_DATA=$(ALL_JARS)

clean-local:
	$(ANT) clean
