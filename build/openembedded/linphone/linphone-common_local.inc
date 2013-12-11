SRC_URI = "file://${HOME}/linphone-3.6.1.tar.gz"

S = "${WORKDIR}/linphone-3.6.1"

require linphone-common.inc

do_configure_prepend () {
        ./autogen.sh
        libtoolize --copy --force
}

#Required to avoid compile errors on May 2011.
EXTRA_OECONF +=" --disable-strict --disable-glx"

LIC_FILES_CHKSUM = "file://COPYING;md5=94d55d512a9ba36caa9b7df079bae19f"
