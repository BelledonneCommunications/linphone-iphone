PR = "r1"
SRC_URI = "file://${HOME}/msamr-0.0.2.tar.gz"
S = "${WORKDIR}/msamr-0.0.2"

do_configure_prepend () {
        ./autogen.sh
}

DEFAULT_PREFERENCE="-1"

require msamr-common.inc
