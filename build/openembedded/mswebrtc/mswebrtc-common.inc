DESCRIPTION = "Mediastreamer2 plugin adding support for WebRTC features (iSAC codec, AEC...)"
SECTION = "libs"
PRIORITY = "optional"
LICENSE = "GPLv2"
DEPENDS = "linphone"

EXTRA_OECONF = "--disable-isac"

FILES_${PN} = "${libdir}/mediastreamer/plugins/*.so.*"
FILES_${PN}-dev = "${libdir}/mediastreamer/plugins/*.la ${libdir}/mediastreamer/plugins/*.so"
FILES_${PN}-dbg = "${libdir}/mediastreamer/plugins/.debug/*.so.* /usr/src/debug"

LIC_FILES_CHKSUM = "file://COPYING;md5=59530bdf33659b29e73d4adb9f9f6552"

inherit autotools pkgconfig
