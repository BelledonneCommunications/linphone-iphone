DESCRIPTION = "SIP stack from Belledonne Communications"
LICENSE = "GPL"

RECIPE_TYPES = "machine native"

DEPENDS = "polarssl antlr3c libm"

EXTRA_OECONF += "--with-antlr=${BUILD_SYSROOT}${target_prefix} --with-polarssl=${BUILD_SYSROOT}${target_prefix}"

inherit autotools-autoreconf pkgconfig library c

do_autoreconf () {
	./autogen.sh
}

DEPENDS_${PN} += "${DEPENDS}"
RDEPENDS_${PN} += "${DEPENDS}"
RDEPENDS_${PN}-dev += "polarssl-dev antlr3c-dev"
