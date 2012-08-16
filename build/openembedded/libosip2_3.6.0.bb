SECTION = "libs"
DESCRIPTION = "Session Initiation Protocol (SIP) library"
LEAD_SONAME = "libosip2\..*"
PR = "r0"
LICENSE = "LGPL"
SRC_URI = "${GNU_MIRROR}/osip/libosip2-${PV}.tar.gz"

inherit autotools pkgconfig
DEFAULT_PREFERENCE = "3"
SRC_URI[md5sum] = "92fd1c1698235a798497887db159c9b3"
SRC_URI[sha256sum] = "c9a18b0c760506d150017cdb1fa5c1cefe12b8dcbbf9a7e784eb75af376e96cd"
LIC_FILES_CHKSUM = "file://COPYING;md5=e639b5c15b4bd709b52c4e7d4e2b09a4"