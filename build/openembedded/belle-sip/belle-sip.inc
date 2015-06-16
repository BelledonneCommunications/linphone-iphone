DESCRIPTION = "SIP stack from Belledonne Communications"
LICENSE = "GPL"

DEPENDS_${PN} = "polarssl-dev antlr3c-dev"
DEPENDS = "polarssl-dev antlr3c-dev"
RDEPENDS_${PN} = "polarssl-dev antlr3c-dev"

EXTRA_OECONF += "--disable-strict --with-antlr=${STAGING_DIR_HOST}${layout_exec_prefix}/usr --with-polarssl=${STAGING_DIR_HOST}${layout_exec_prefix}/usr"
INSANE_SKIP_belle-sip += "dev-deps"

inherit autotools pkgconfig

do_autoreconf () {
    ./autogen.sh
}
