require polarssl.inc

SRCREV="cecb44e4f13f42f793dde34b42793e1ebcce91a5"

S = "${WORKDIR}/git"

do_fetch_prepend () {
    import bb
    bb.note("Will checkout in %s" % "${S}" )
}

SRC_URI = "git://git.linphone.org/polarssl.git"
LIC_FILES_CHKSUM = "file://LICENSE;md5=751419260aa954499f7abaabaa882bbe"

