DESCRIPTION = "Speex is an Open Source/Free Software patent-free audio compression format designed for speech."
SECTION = "libs/multimedia"
LICENSE = "BSD"
HOMEPAGE = "http://www.speex.org"
DEPENDS = "libogg"
PV = "1.1+git"
PR = "r3"
SPEEX_TMP_DIR="/tmp/SPEEX_TMP"
SRC_URI = "file://${SPEEX_TMP_DIR}/speex.tar.gz"
S = "${WORKDIR}/speex"
LIC_FILES_CHKSUM = "file://COPYING;md5=314649d8ba9dd7045dfb6683f298d0a8"

PARALLEL_MAKE = ""

inherit autotools pkgconfig

LEAD_SONAME = "libspeex.so"

#check for TARGET_FPU=soft and inform configure of the result so it can disable some floating points
EXTRA_OECONF += "--enable-fixed-point --enable-armv7neon-asm"

do_fetch_prepend () {
	import os,bb
	bb.note("Hack preparing clone in %s" %"${SPEEX_TMP_DIR}")
	os.system("rm -rf ${SPEEX_TMP_DIR}")
	os.system("mkdir -p ${SPEEX_TMP_DIR}")

	os.system("cd ${SPEEX_TMP_DIR}; git clone --depth 1 git://git.linphone.org/speex")

	os.system("cd ${SPEEX_TMP_DIR}/speex; ./autogen.sh")

	os.system("cd ${SPEEX_TMP_DIR}; tar czf speex.tar.gz --exclude .git speex")
}

do_configure_append() {
	sed -i s/"^OGG_CFLAGS.*$"/"OGG_CFLAGS = "/g Makefile */Makefile */*/Makefile
	sed -i s/"^OGG_LIBS.*$"/"OGG_LIBS = -logg"/g Makefile */Makefile */*/Makefile
	find . -name "Makefile" -exec sed -i s,-I/usr/include,, {} \;
}

PACKAGES =+ "${PN}-utils ${PN}-dsp"
FILES_${PN}-utils = "${bindir}/speex*"
FILES_${PN}-dsp = "${libdir}/libspeexdsp.so.*"
FILES_${PN} = "${libdir}/libspeex.so.*"

