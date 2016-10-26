DESCRIPTION = "Mediastreamer2 plugin adding support for H264 codec"
SECTION = "libs"
PRIORITY = "optional"
LICENSE = "GPLv3"

LIC_FILES_CHKSUM = "file://COPYING;md5=59530bdf33659b29e73d4adb9f9f6552"

DEPENDS = "linphone x264"

FILES_${PN} = "${libdir}/mediastreamer/plugins/*.so.*"
FILES_${PN}-dev = "${libdir}/mediastreamer/plugins/*.la ${libdir}/mediastreamer/plugins/*.so"
inherit autotools pkgconfig
