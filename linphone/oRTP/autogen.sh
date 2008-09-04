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

set -x
rm -rf config.cache autom4te.cache
$ACLOCAL
autoheader
$AUTOMAKE --add-missing --copy
libtoolize --copy --force
autoconf
