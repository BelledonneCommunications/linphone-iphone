DESCRIPTION = "Mediastreamer2 plugin adding support for AMR codec"
SECTION = "libs"
PRIORITY = "optional"
LICENSE = "GPLv3"

LIC_FILES_CHKSUM = "file://COPYING;md5=d32239bcb673463ab874e80d47fae504"

DEPENDS = "linphone opencore-amr"
DEPENDS_append_wideband = " vo-amrwbenc"

MSAMR_WIDEBAND = ""
MSAMR_WIDEBAND_wideband = "--enable-wideband"

EXTRA_OECONF =        "\
                ${MSAMR_WIDEBAND} \
                "

FILES_${PN} = "${libdir}/mediastreamer/plugins/*.so.*"
FILES_${PN}-dev = "${libdir}/mediastreamer/plugins/*.la ${libdir}/mediastreamer/plugins/*.so"
inherit autotools pkgconfig
