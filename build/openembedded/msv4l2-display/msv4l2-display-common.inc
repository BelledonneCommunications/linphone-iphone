SECTION = "libs"

DEPENDS_${PN} = "libmediastreamer"
DEPENDS = "libmediastreamer"

PROVIDES = "msv4l2-display"

EXTRA_OECONF += ' CFLAGS="-DOUTPUT_VIDEO_DEVICE=17"'

inherit autotools gettext

FILES_${PN} = "${libdir}/mediastreamer/plugins/*.so.*"
FILES_${PN}-dev = "${libdir}/mediastreamer/plugins/*.la ${libdir}/mediastreamer/plugins/*.so"
FILES_${PN}-dbg = "${libdir}/mediastreamer/plugins/.debug/*.so.* /usr/src/debug"
