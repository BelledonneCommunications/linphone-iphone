require recipes-images/angstrom/console-image.bb

IMAGE_INSTALL += " \
  kbd \
  kbd-keymaps \
  keymaps \
  linphonec \
  linphone-plugins \
  linphone-rings \
"

rootfs_postprocess() {
  gunzip -c ${IMAGE_ROOTFS}/usr/share/keymaps/i386/dvorak/dvorak.map.gz > ${IMAGE_ROOTFS}/etc/keymap-2.6.map
}

ROOTFS_POSTPROCESS_COMMAND += "rootfs_postprocess; "

export IMAGE_BASENAME = "console-linphone-image"
