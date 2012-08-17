DESCRIPTION = "OpenCORE Adaptive Multi Rate (AMR) speech codec library implementation"
SECTION = "libs"
PRIORITY = "optional"
LICENSE = "Apache"

PR = "r1"
SRC_URI = "${SOURCEFORGE_MIRROR}/opencore-amr/${PN}-${PV}.tar.gz" 

inherit autotools pkgconfig
SRC_URI[md5sum] = "09d2c5dfb43a9f6e9fec8b1ae678e725"
SRC_URI[sha256sum] = "106bf811c1f36444d7671d8fd2589f8b2e0cca58a2c764da62ffc4a070595385"
LIC_FILES_CHKSUM = "file://COPYING;md5=dd2c2486aca02190153cf399e508c7e7"