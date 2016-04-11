PR = "r0"
SRC_URI = "file://${HOME}/msilbc-2.0.3.tar.gz"
S = "${WORKDIR}/msilbc-2.0.3"

do_configure_prepend () {
        ./autogen.sh
}

DEFAULT_PREFERENCE="-1"

require msilbc-common.inc
