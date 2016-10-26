PR = "r0"
SRC_URI = "file://${HOME}/mswebrtc-1.0.tar.gz"
S = "${WORKDIR}/mswebrtc-1.0"

do_configure_prepend () {
        ./autogen.sh
}

DEFAULT_PREFERENCE="-1"

require mswebrtc-common.inc
