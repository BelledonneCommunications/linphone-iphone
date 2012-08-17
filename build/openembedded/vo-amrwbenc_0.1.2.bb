DESCRIPTION = "VisualOn AMR-WB encoder library"
SECTION = "libs"
PRIORITY = "optional"
LICENSE = "Apache"
DEPENDS = "opencore-amr"

PR = "r1"
SRC_URI = "${SOURCEFORGE_MIRROR}/opencore-amr/${PN}-${PV}.tar.gz" 

inherit autotools pkgconfig
SRC_URI[md5sum] = "588205f686adc23532e31fe3646ddcb6"
SRC_URI[sha256sum] = "dd8c33e57bc415754f31fbb1b1536563bf731fc14e55f8182564e4c0fbb26435"
LIC_FILES_CHKSUM = "file://COPYING;md5=dd2c2486aca02190153cf399e508c7e7"