DESCRIPTION = "GSM Audio Library"
SECTION = "libs"
PRIORITY = "optional"
LICENSE = "libgsm"

INC_PR = "r2"

SRC_URI = "http://www.quut.com/gsm/gsm-${PV}.tar.gz \
	   file://01_makefile.patch \
           file://02_cplusplus.patch \
           file://03_config.patch \
           file://04_includes.patch \
           file://05_compiler_warnings.patch \
	  "

CFLAGS += "-c -g -fPIC -Wall -D_GNU_SOURCE -D_REENTRANT -DNeedFunctionPrototypes=1 -DWAV49 -I./inc"

PARALLEL_MAKE = ""

do_compile() {
	unset LD
	oe_runmake CCFLAGS="${CFLAGS}"
}

do_install() {
	oe_libinstall -a -C lib libgsm ${D}${libdir}
        oe_libinstall -so -C lib libgsm ${D}${libdir}
        install -d ${D}${includedir}/gsm
        install -m 0644 ${S}/inc/gsm.h ${D}${includedir}/gsm/
	cd ${D}${includedir}
	ln -s gsm/gsm.h gsm.h
}

LIC_FILES_CHKSUM = "file://COPYRIGHT;md5=fc1372895b173aaf543a122db37e04f5"