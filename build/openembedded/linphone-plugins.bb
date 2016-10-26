DESCRIPTION = "Plugins for linphone to have additional codecs."
LICENSE = ""
ALLOW_EMPTY_${PN} = "1"
PACKAGES = "${PN}"
DEPENDS_${PN} = "linphone msamr msilbc msx264"
RDEPENDS_${PN} = "linphonec msamr msilbc msx264"

LIC_FILES_CHKSUM = "file://${COREBASE}/LICENSE;md5=3f40d7994397109285ec7b81fdeb3b58 \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
