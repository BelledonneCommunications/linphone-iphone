DESCRIPTION = "iLBC codec as published in IETF RFC 3951"
SECTION = "libs"
PRIORITY = "optional"
LICENSE = "LGPLv3"
PR = "r1"

SRC_URI = "git://git.linphone.org/libilbc-rfc3951.git;protocol=git"
SRCREV = "b9490e0cbdda6a4ec29f7c47d81d3997004fedba"
S = "${WORKDIR}/git"

LIC_FILES_CHKSUM = "file://COPYING;md5=586c8a6efdeabd095cc4206ce4d0699b"

inherit autotools pkgconfig
