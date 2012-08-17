PR = "r1"
SRC_URI = "git://git.linphone.org/msamr.git;protocol=git" 
SRCREV = "HEAD"
S = "${WORKDIR}/git"

OVERRIDES_append = ":wideband"

LIC_FILES_CHKSUM = "file://COPYING;md5=d32239bcb673463ab874e80d47fae504"

require msamr-common.inc
