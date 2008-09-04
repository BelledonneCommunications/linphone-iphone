#!/bin/sh

#AM_VERSION=1.10
#1.9 was the recommended version
if test -n "$AM_VERSION" ; then
	ACLOCAL=aclocal-${AM_VERSION}
	AUTOMAKE=automake-${AM_VERSION}
else
	ACLOCAL=aclocal
	AUTOMAKE=automake
fi

echo "Generating build scripts in this mediastreamer plugin"
set -x
libtoolize --copy --force
$ACLOCAL
$AUTOMAKE --force-missing --add-missing --copy
autoconf
rm -rf config.cache

