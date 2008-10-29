EXTRA_DIST=dependencies dependencies-src src test-src

SUBDIRS=launcher

antbuild/dist/p2pproxy_0.1/p2pproxy.jar:	src/org/linphone/p2pproxy/*/*.java
	$(ANT) dist

jarsdir=$(datadir)/java/fonis

jars_DATA=antbuild/dist/p2pproxy_0.1/p2pproxy.jar antbuild/dist/p2pproxy_0.1/*.jar

