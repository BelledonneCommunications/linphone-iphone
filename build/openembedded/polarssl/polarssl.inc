DESCRIPTION = "SSL/TLS library"
LICENSE = "GPL"

RECIPE_TYPES = "machine native"

DEPENDS = ""

inherit c make pkgconfig library

EXTRA_OEMAKE += " SHARED=1"
EXTRA_OEMAKE:>TARGET_OS_darwin = " DARWIN=1"
EXTRA_OEMAKE:>TARGET_OS_mingw32 = " WINDOWS=1"

PACKAGES += "${PN}-utils"

MAKE_DESTDIR = "DESTDIR=${D}/${prefix}"

FILES_${PN} += "${sharedlibdir}/*${SOLIBSDEV}"
FILES_${PN}-dev += "!${sharedlibdir}/*${SOLIBSDEV}"
FILES_${PN}-utils += "${bindir}/polarssl_*"

DEPENDS_${PN} += "${DEPENDS}"
RDEPENDS_${PN} += "${DEPENDS}"
