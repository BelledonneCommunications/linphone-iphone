DESCRIPTION = "Audio/video SIP-based IP phone (console edition)"
HOMEPAGE = "http://www.linphone.org/?lang=us"
LICENSE = "GPLv2"

SRCREV = "855f3aa1b83fd979bb8ff4c6373522bc72fe77ec"
L_GIT_SRC_URI = "gitosis@git.linphone.org:linphone-daemon"

LINPHONE_TMP_DIR="/tmp/LINPHONE_TMP_${SRCREV}"
SRC_URI = "file://${LINPHONE_TMP_DIR}/linphone.tar.gz"

S = "${WORKDIR}/linphone"

# bitbake git fetcher currently doesn't handle git submodules
# There is also a problem with autogen and AC_SUBST
# note: don't use a ssh key with password, it does not work.
do_fetch_prepend () {
    import os,bb
    bb.note("Hack preparing clone in %s" %"${LINPHONE_TMP_DIR}")
    os.system("rm -rf ${LINPHONE_TMP_DIR}")
    os.system("mkdir -p ${LINPHONE_TMP_DIR}")

    bb.note("Hack cloning linphone !recursively")
    os.system("cd ${LINPHONE_TMP_DIR}; git clone ${L_GIT_SRC_URI} linphone; cd linphone; git checkout ${SRCREV}; git submodule update --recursive --init")

    bb.note("Hack launching autogen.sh manually")
    os.system("cd ${LINPHONE_TMP_DIR}/linphone; ./autogen.sh")

    bb.note("Hack preparing linphone.tar.gz")
    # we need to keep the .git since the versioning in linphone is done through `git describe`
    os.system("cd ${LINPHONE_TMP_DIR}; tar czf linphone.tar.gz linphone")
}

require linphone-common.inc


#Required to avoid compile errors on May 2011.
EXTRA_OECONF +=" --disable-strict"

LIC_FILES_CHKSUM = "file://COPYING;md5=9f9938e31db89d55a796e86808c96848"
