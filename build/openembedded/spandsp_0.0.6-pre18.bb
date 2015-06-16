PR = "r0"

SRC_URI = "http://www.soft-switch.org/downloads/spandsp/${PN}-0.0.6pre18.tgz"

S = "${WORKDIR}/spandsp-0.0.6"

# *cough*
do_configure_append() {
      rm config.log
}

DESCRIPTION = "A library of many DSP functions for telephony."
HOMEPAGE = "http://www.soft-switch.org"
SECTION = "libs"
LICENSE = "LGPL"
DEPENDS_${PN} = "tiff libxml2"

inherit autotools

#PARALLEL_MAKE = ""

SRC_URI[md5sum] = "98330bc00a581ed8d71ebe34afabbcf9"
SRC_URI[sha256sum] = "835cd886105e4e39791f0e8cfe004c39b069f2e6dcb0795a68a6c79b5d14af2c"
LIC_FILES_CHKSUM = "file://COPYING;md5=8791c23ddf418deb5be264cffb5fa6bc"
