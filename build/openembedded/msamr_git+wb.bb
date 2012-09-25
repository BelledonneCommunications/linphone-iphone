PR = "r1"
SRC_URI = "git://git.linphone.org/msamr.git;protocol=git" 
SRCREV = "HEAD"
S = "${WORKDIR}/git"

OVERRIDES_append = ":wideband"

require msamr-common.inc
