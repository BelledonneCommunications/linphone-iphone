DESCRIPTION = "Linphone version of antlr3"
LICENSE = "GPL"

PROVIDES = "antlr3c antlr3c-dev"
ALLOW_EMPTY_${PN} = "1"

S = "${WORKDIR}/git/runtime/C"

inherit autotools pkgconfig lib_package

do_fetch_append() {
    import os
    os.system("autogen.sh")
}

