SRC_URI = "file://${HOME}/linphone-3.5.2.tar.gz"

S = "${WORKDIR}/linphone-3.5.2"

require linphone-common.inc

do_configure_prepend () {
	./autogen.sh
}

#Required to avoid compile errors on May 2011.
EXTRA_OECONF +=" --disable-strict"
