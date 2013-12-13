DESCRIPTION = "SIP stack from Belledonne Communications"
LICENSE = "GPL"

DEPENDS = "polarssl antlr3c"
RDEPENDS = "antlr3c"

EXTRA_OECONF += "--with-antlr=${STAGING_DIR_HOST}${layout_exec_prefix}/usr --with-polarssl=${STAGING_DIR_HOST}${layout_exec_prefix}/usr"
INSANE_SKIP_belle-sip += "dev-deps"

inherit autotools pkgconfig

do_autoreconf () {
    ./autogen.sh
}

DEPENDS_${PN} += "${DEPENDS}"
RDEPENDS_${PN} += "${RDEPENDS}"

