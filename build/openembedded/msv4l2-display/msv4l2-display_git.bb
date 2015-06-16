DESCRIPTION = "V4L2 display filter plugin for mediastreamer/linphone"
HOMEPAGE = "http://www.linphone.org/?lang=us"
LICENSE = "GPLv3+"

GIT_SRC_URI = "gitosis@git.linphone.org:msv4l2-display.git"
SRCREV = "master"
LIC_FILES_CHKSUM = "file://COPYING;md5=c46082167a314d785d012a244748d803"

TMP_DIR="/tmp/TMP_MSV4L2Display_${SRCREV}"
SRC_URI = "file://${TMP_DIR}/msv4l2-display.tar.gz"

S = "${WORKDIR}/msv4l2-display"

# note: don't use a ssh key with password, it does not work.
do_fetch_prepend () {
    import os,bb
    os.system("rm -rf ${TMP_DIR}")
    os.system("mkdir -p ${TMP_DIR}")
    os.system("cd ${TMP_DIR}; git clone ${GIT_SRC_URI} msv4l2-display")
    os.system("cd ${TMP_DIR}; tar czf msv4l2-display.tar.gz msv4l2-display")
}

require msv4l2-display-common.inc
