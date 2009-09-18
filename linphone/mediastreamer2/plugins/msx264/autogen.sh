#!/bin/sh

AM_VERSION=1.10
if ! type aclocal-$AM_VERSION 1>/dev/null 2>&1; then
	ACLOCAL=aclocal
        AUTOMAKE=automake
else
	ACLOCAL=aclocal-${AM_VERSION}
	AUTOMAKE=automake-${AM_VERSION}
fi

echo "Generating build scripts in this mediastreamer plugin"
set -x
libtoolize --copy --force
$ACLOCAL
$AUTOMAKE --force-missing --add-missing --copy
autoconf


