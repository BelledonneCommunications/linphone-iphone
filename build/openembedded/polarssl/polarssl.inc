DESCRIPTION = "SSL/TLS library"
LICENSE = "GPL"

inherit pkgconfig

EXTRA_OEMAKE += " SHARED=1"

PROVIDES = "polarssl polarssl-dev"
ALLOW_EMPTY_${PN} = "1"

PACKAGES += "${PN}-utils"

MAKE_DESTDIR = "DESTDIR=${D}/${prefix}"

FILES_${PN} += "${sharedlibdir}/*${SOLIBSDEV}"
FILES_${PN}-dev += "!${sharedlibdir}/*${SOLIBSDEV}"
FILES_${PN}-utils += "${bindir}/polarssl_*"
