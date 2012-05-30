
SRCREV = "53c056325acf9bce306ddfd1dfb84e328ab6576c"
L_GIT_SRC_URI = "gitosis@git.linphone.org:linphone-daemon"
PR_append = "+gitr${SRCREV}"

LINPHONE_TMP_DIR="${HOME}/LINPHONE_TMP_${SRCREV}"
SRC_URI = "file://${LINPHONE_TMP_DIR}/linphone.tar.gz"

S = "${WORKDIR}/linphone"

# bitbake git fetcher currently doesn't handle git submodules
# There is also a problem with autogen and AC_SUBST
do_fetch_prepend () {
	import os,bb
	bb.note("Hack preparing clone in %s" %"${LINPHONE_TMP_DIR}")
	os.system("rm -rf ${LINPHONE_TMP_DIR}")
	os.system("mkdir -p ${LINPHONE_TMP_DIR}")

	bb.note("Hack cloning linphone !recursively")
	os.system("cd ${LINPHONE_TMP_DIR}; git clone --depth 1 --recursive ${L_GIT_SRC_URI} linphone; tag=${SRCREV};")

	bb.note("Hack launching autogen.sh manually")
	os.system("cd ${LINPHONE_TMP_DIR}/linphone; ./autogen.sh")

	bb.note("Hack preparing linphone.tar.gz")
	os.system("cd ${LINPHONE_TMP_DIR}; tar czf linphone.tar.gz --exclude .git linphone")
}

require linphone-common.inc


#Required to avoid compile errors on May 2011.
EXTRA_OECONF +=" --disable-strict"
