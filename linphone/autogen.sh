#!/bin/sh

AM_VERSION="1.9"
if ! type aclocal-$AM_VERSION 1>/dev/null 2>&1; then
	# automake-1.9 (recommended) is not available on Fedora 8
	AUTOMAKE=automake
	ACLOCAL=aclocal
else
	ACLOCAL=aclocal-${AM_VERSION}
	AUTOMAKE=automake-${AM_VERSION}
fi

echo "Generating build scripts in linphone..."
set -x
libtoolize --copy --force
autoheader
$ACLOCAL -I m4
$AUTOMAKE --force-missing --add-missing --copy
autoconf
rm -rf config.cache

echo "Generating build scripts in oRTP..."
cd oRTP && ./autogen.sh && cd -

echo "Generating build scripts in mediastreamer2..."
cd mediastreamer2 && ./autogen.sh && cd -
