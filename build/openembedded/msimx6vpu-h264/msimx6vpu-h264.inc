SECTION = "libs"

DEPENDS_${PN} = "libmediastreamer imx-lib imx-vpu"
DEPENDS = "libmediastreamer imx-lib imx-vpu"

PROVIDES = "msimx6vpu-h264 msimx6vpu-h264-dbg"

inherit autotools gettext

FILES_${PN} = "${libdir}/mediastreamer/plugins/*.so.*"
FILES_${PN}-dev = "${libdir}/mediastreamer/plugins/*.la ${libdir}/mediastreamer/plugins/*.so"
FILES_${PN}-dbg = "${libdir}/mediastreamer/plugins/.debug/*.so.* /usr/src/debug"
