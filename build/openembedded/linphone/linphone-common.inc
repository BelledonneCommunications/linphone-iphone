SECTION = "x11/network"
SECTION_liblinphone = "libs/network"
SECTION_libmediastreamer = "libs/network"
SECTION_libortp = "libs/network"
SECTION_linphonec =  "console/network"

SRC_URI_append_igep0020 = " file://alsa_8khz.patch"

DEPENDS_${PN} = "intltool-native speex alsa-lib spandsp belle-sip liblinphone libxv ffmpeg libv4l libgsm"
DEPENDS_liblinphone = "libmediastreamer libortp"
DEPENDS_libmediastreamer = "speex alsa-lib libortp"

PROVIDES = "linphone linphonec liblinphone libmediastreamer libortp"

inherit autotools pkgconfig gettext

INSANE_SKIP_linphone += "dev-deps"
INSANE_SKIP_liblinphone += "dev-deps"

do_install_append(){
  install -d ${D}${bindir}
}

EXTRA_OECONF = " \
                --disable-tests \
                --with-ffmpeg=${STAGING_DIR_HOST}${layout_exec_prefix} --enable-video --disable-vp8 \
                --disable-glx \
                --enable-alsa --disable-pulseaudio \
                --without-readline \
                --with-speex=${STAGING_DIR_HOST}${layout_exec_prefix} \
                --disable-manual --enable-tests=yes \
                --enable-console_ui=no \
                --enable-gtk_ui=no \
                --with-realprefix=/usr \
                "

EXTRA_OEMAKE = " V=1"

PACKAGES = "${PN}-dbg ${PN}-dev ${PN}-doc ${PN}c ${PN}-common linphone-rings liblinphone libmediastreamer-bin libmediastreamer libortp ${PN}-utils ${PN}-tests"

FILES_${PN}-common = "\
            ${bindir}/lp-gen-wrappers \
            ${datadir}/pixmaps \
            ${datadir}/applications \
            ${datadir}/gnome \
            ${datadir}/tutorials \
            ${datadir}/linphone \
            ${datadir}/sounds/linphone/hello8000.wav \
            ${datadir}/sounds/linphone/hello16000.wav \
            ${datadir}/sounds/linphone/incoming_chat.wav \
            ${datadir}/sounds/linphone/ringback.wav \
            ${datadir}/images/nowebcamCIF.jpg \
            ${datadir}/appdata/linphone.appdata.xml \
            ${datadir}/icons \ 
            "
FILES_${PN}-tests = "${bindir}/xml2lpc_test ${bindir}/lpc2xml_test"
FILES_${PN} = "${bindir}/linphone"
FILES_${PN}c = "${bindir}/linphonec ${bindir}/linphone-daemon ${bindir}/linphone-daemon-pipetest ${bindir}/linphonecsh ${bindir}/sipomatic ${bindir}/auto_answer"
FILES_${PN}-rings = "${datadir}/sounds/linphone/rings"
FILES_liblinphone = "${libdir}/liblinphone.so.*"
FILES_libmediastreamer-bin = "${bindir}/mediastream ${bindir}/msaudiocmp"
FILES_libmediastreamer = "${libdir}/libmediastreamer_base.so.* ${libdir}/libmediastreamer_voip.so.* ${libdir}/mediastreamer/ ${libdir}/mediastreamer/plugins"
FILES_libortp = "${libdir}/libortp.so.*"
FILES_${PN}-dev += "${libdir}/*.a ${libdir}/*.la ${libdir}/pkgconfig ${includedir}"
FILES_${PN}-utils = "${bindir}/test_ecc ${bindir}/test_lsd"
FILES_${PN}-doc = "${docdir}/ortp-0.24.1 ${docdir}/mediastreamer-2.11.1 ${docdir}/linphone-3.8.1-linphone-daemon ${mandir}"
