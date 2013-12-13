DESCRIPTION = "SSL/TLS library"
LICENSE = "GPL"


DEPENDS = ""

inherit pkgconfig

EXTRA_OEMAKE += " SHARED=1"

PACKAGES += "${PN}-utils"

MAKE_DESTDIR = "DESTDIR=${D}/${prefix}"

FILES_${PN} += "${sharedlibdir}/*${SOLIBSDEV}"
FILES_${PN}-dev += "!${sharedlibdir}/*${SOLIBSDEV}"
FILES_${PN}-utils += "${bindir}/polarssl_*"

DEPENDS_${PN} += "${DEPENDS}"
