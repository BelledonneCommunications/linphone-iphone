SECTION = "x11/network"
SECTION_liblinphone = "libs/network"
SECTION_libmediastreamer = "libs/network"
SECTION_libortp = "libs/network"
SECTION_linphonec =  "console/network"

SRC_URI_append_igep0020 = " file://alsa_8khz.patch"

DEPENDS = "intltool readline libosip2 libexosip2 speex alsa-lib spandsp"
DEPENDS_append_video = " libxv ffmpeg libv4l"
DEPENDS_append_gtk = " gtk+"
DEPENDS_${PN} = "intltool liblinphone"
DEPENDS_${PN}_append_gsm = " libgsm"
DEPENDS_${PN}_append_gtk = " gtk+"
DEPENDS_${PN}_append_video = " libxv"
DEPENDS_${PN}c = "liblinphone readline"
DEPENDS_liblinphone = "libmediastreamer libortp libosip2 libexosip2"
DEPENDS_libmediastreamer = "speex alsa-lib libortp"
DEPENDS_libmediastreamer_append_video = " ffmpeg libv4l"

PROVIDES = "linphonec liblinphone libmediastreamer libortp"
PROVIDES_gtk = "linphone linphonec liblinphone libmediastreamer libortp"

inherit autotools pkgconfig gettext

export PKG_CONFIG=${STAGING_BINDIR_NATIVE}/pkg-config

LINPHONE_OE_ENABLE_TESTS = "--disable-tests"
LINPHONE_OE_ENABLE_VIDEO = "--disable-video"
LINPHONE_OE_ENABLE_VIDEO_video = "--with-ffmpeg=${STAGING_DIR_HOST}${layout_exec_prefix} --enable-video"
LINPHONE_OE_ENABLE_X11 = "--disable-x11 --disable-xv"
LINPHONE_OE_ENABLE_X11_x11 = ""

LINPHONE_OE_ENABLE_CONSOLE_UI = "no"
LINPHONE_OE_ENABLE_CONSOLE_UI_console = "yes"

LINPHONE_OE_ENABLE_GTK_UI = "no"
LINPHONE_OE_ENABLE_GTK_UI_gtk = "yes"

do_install_append(){
	install -d ${D}${bindir}
}

EXTRA_OECONF = " \
		${LINPHONE_OE_ENABLE_TESTS} \
                ${LINPHONE_OE_ENABLE_VIDEO} \
		${LINPHONE_OE_ENABLE_X11} \
                --enable-alsa \
                --with-osip=${STAGING_DIR_HOST}${layout_exec_prefix} \
                --with-readline=${STAGING_DIR_HOST}${layout_exec_prefix} \
                --with-speex=${STAGING_DIR_HOST}${layout_exec_prefix} \
                --disable-manual --enable-tests=yes \
		--enable-console_ui=${LINPHONE_OE_ENABLE_CONSOLE_UI} \
		--enable-gtk_ui=${LINPHONE_OE_ENABLE_GTK_UI} \
		--with-realprefix=/usr \
		"

EXTRA_OEMAKE = " V=1"

PACKAGES_gtk = "${PN}-dbg ${PN} ${PN}-doc ${PN}-dev ${PN}-locale ${PN}c linphone-rings liblinphone libmediastreamer-bin libmediastreamer libortp ${PN}-utils"
PACKAGES = "${PN}-dbg ${PN}-dev ${PN}c linphone-rings liblinphone libmediastreamer-bin libmediastreamer libortp ${PN}-utils"

FILES_${PN} = " \ 
	    ${bindir}/linphone \
            ${datadir}/linphone \
 	    ${datadir}/pixmaps \
	    ${datadir}/applications \
	    ${datadir}/gnome/apps \
	    ${datadir}/sounds/linphone/hello8000.wav \
	    ${datadir}/sounds/linphone/hello16000.wav \
	    ${datadir}/images/nowebcamCIF.jpg \
	    "
FILES_${PN}c = "${bindir}/linphonec ${bindir}/linphone-daemon ${bindir}/linphone-daemon-pipetest ${bindir}/linphonecsh ${bindir}/sipomatic ${datadir}/sounds/linphone/ringback.wav"
FILES_${PN}-rings = "${datadir}/sounds/linphone/rings"
FILES_liblinphone = "${libdir}/liblinphone.so.*"
FILES_libmediastreamer-bin = "${bindir}/mediastream"
FILES_libmediastreamer = "${libdir}/libmediastreamer.so.*"
FILES_libortp = "${libdir}/libortp.so.*"
FILES_${PN}-dev += "${libdir}/*.a ${libdir}/*.la ${libdir}/pkgconfig ${includedir}"
FILES_${PN}-utils = "${bindir}/test_ecc ${bindir}/test_lsd"
