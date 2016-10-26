PR = "r1"
SRC_URI = "file://${HOME}/msx264-1.4.2.tar.gz"
S = "${WORKDIR}/msx264-1.4.2"

do_configure_prepend () {
        ./autogen.sh
}

DEFAULT_PREFERENCE="-1"

require msx264-common.inc
