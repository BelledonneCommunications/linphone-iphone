#!/bin/sh
if [ -z "$P2P_JMX_PORT"] ; then
	P2P_JMX_PORT=6789
fi
exec ${JAVA_HOME}/bin/java -Dcom.sun.management.jmxremote \
 		-Dcom.sun.management.jmxremote.port=${P2P_JMX_PORT} \
 		-Dcom.sun.management.jmxremote.authenticate=false \
 		-Dcom.sun.management.jmxremote.ssl=false \
 		-jar p2pproxy.jar $*