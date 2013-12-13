DESCRIPTION = "Linphone version of antlr3"
LICENSE = "GPL"


DEPENDS = ""

S = "${WORKDIR}/git/runtime/C"

inherit autotools pkgconfig lib_package

do_fetch_append() {
    import os
    os.system("autogen.sh")
}

