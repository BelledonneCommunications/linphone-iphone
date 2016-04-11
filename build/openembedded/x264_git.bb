DESCRIPTION = "x264 is a free software library and application for encoding video streams into the H.264/MPEG-4 AVC format."
SECTION = "libs/multimedia"
PRIORITY = "optional"
LICENSE = "GPLv2"
HOMEPAGE = "http://www.videolan.org/developers/x264.html"
PR = "r1"

SRC_URI = "git://git.videolan.org/x264.git;protocol=git"
SRCREV = "e89c4cfc9f37d0b7684507974b333545b5bcc37a"
S = "${WORKDIR}/git"

EXTRA_OECONF += "--disable-lavf --enable-pic"
EXTRA_OEMAKE = ""
AS = "${TARGET_PREFIX}gcc"

LIC_FILES_CHKSUM = "file://COPYING;md5=94d55d512a9ba36caa9b7df079bae19f"

inherit autotools pkgconfig
