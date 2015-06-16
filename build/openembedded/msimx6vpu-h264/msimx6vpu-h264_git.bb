DESCRIPTION = "A H264 encoder/decoder plugin for mediastreamer using Freescale's IMX6's VPU"
HOMEPAGE = "http://www.linphone.org/?lang=us"
LICENSE = "GPLv3+"

GIT_SRC_URI = "gitosis@git.linphone.org:msimx6vpu-h264.git"
SRCREV = "master"
LIC_FILES_CHKSUM = "file://COPYING;md5=c46082167a314d785d012a244748d803"

TMP_DIR="/tmp/TMP_MSIMX6VPUH264_${SRCREV}"
SRC_URI = "file://${TMP_DIR}/msimx6vpu-h264.tar.gz"

S = "${WORKDIR}/msimx6vpu-h264"

# note: don't use a ssh key with password, it does not work.
do_fetch_prepend () {
    import os,bb
    os.system("rm -rf ${TMP_DIR}")
    os.system("mkdir -p ${TMP_DIR}")
    os.system("cd ${TMP_DIR}; git clone ${GIT_SRC_URI} msimx6vpu-h264")
    os.system("cd ${TMP_DIR}; tar czf msimx6vpu-h264.tar.gz msimx6vpu-h264")
}

require msimx6vpu-h264.inc
