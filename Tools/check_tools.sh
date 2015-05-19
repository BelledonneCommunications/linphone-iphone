#!/bin/bash

error_on_quit=0

echo_err() {
	 echo "$@" >&2
	 error_on_quit=1
}

check_installed() {
	if [ -z "$(which $1)" ]; then
		echo_err "Could not find $1. Please install $2."
		return 1
	fi
	return 0
}

cd $(dirname $0)/..

if grep -q ' ' <<< $PWD; then
	echo_err "Invalid location: your location should not contain spaces"
fi

for prog in autoconf automake pkg-config doxygen java nasm gettext wget yasm optipng; do
	check_installed "$prog" "it"
done

check_installed "ginstall" "coreutils"
check_installed "intltoolize" "intltool"
check_installed "convert" "imagemagick"

if [ -z "$(which libtoolize)" ]; then
	glibtoolize=$(which glibtoolize)
	if [ ! -z "$glibtoolize" ]; then
		echo_err "Please do a symbolic link from glibtoolize to libtoolize: 'ln -s $glibtoolize ${glibtoolize/glibtoolize/libtoolize}'"
	else
		echo_err "Could not find libtoolize. Please install libtool."
	fi
fi

# just ensure that JDK is installed - if not, it will display user a popup
if ! java -version &>/dev/null; then
	echo_err "Please install Java JDK (not just JRE)"
fi

# needed by x264
check_installed "gas-preprocessor.pl" "it following the README.md"

if nasm -f elf32 2>&1 | grep -q "fatal: unrecognised output format"; then
	echo_err "Invalid version of nasm: your version does not support elf32 output format. If you have installed nasm, please check that your PATH env variable is set correctly."
fi

if ! (find submodules/linphone/mediastreamer2 -mindepth 1 2>/dev/null | grep -q . \
	|| find submodules/linphone/oRTP -mindepth 1 2>/dev/null | grep -q .); then
	echo_err "Missing some git submodules. Did you run 'git submodule update --init --recursive'?"
fi

if ! xcrun --sdk iphoneos --show-sdk-path &>/dev/null; then
	echo_err "iOS SDK not found, please install Xcode from AppStore or equivalent"
elif [ ! -f $(xcrun --sdk iphonesimulator --show-sdk-platform-path)/Developer/usr/bin/strings ]; then
	echo_err "strings binary missing, please run 'sudo ln -s $(which strings) $(xcrun --sdk iphonesimulator --show-sdk-platform-path)/Developer/usr/bin/strings'"
fi

if [ $error_on_quit != 0 ]; then
	echo "Failed to detect required tools, aborting. Please run 'make very-clean' before rerunning 'make'"
fi

exit $error_on_quit
