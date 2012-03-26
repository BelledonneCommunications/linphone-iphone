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
SRC_URI[md5sum] = "51e85725571870614e448f63c33c8996"
SRC_URI[sha256sum] = "46010e62a6f675df13e5be759d033b6bce1bd5882eebb4acd553f9dd3b461afc"
