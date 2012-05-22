DESCRIPTION = "High level Session Initiation Protocol (SIP) library"
SECTION = "libs"
PRIORITY = "optional"
LICENSE = "GPL"
DEPENDS = "libosip2"
SRCNAME = "libeXosip2"
LEAD_SONAME = "libeXosip2"

PR = "r1"
SRC_URI = "http://download.savannah.nongnu.org/releases/exosip/${SRCNAME}-${PV}.tar.gz" 
S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit autotools pkgconfig
EXTRA_OECONF = "--disable-josua"
DEFAULT_PREFERENCE = "3"
SRC_URI[md5sum] = "6fef4c110f1305048a8b307f440933d9"
SRC_URI[sha256sum] = "91da1a084c9ab663afe04b493342e075ad59ac54a1af011c7f2ba4543a923564"
