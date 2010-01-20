#!/bin/sh

AM_VERSION="1.10"
if ! type aclocal-$AM_VERSION 1>/dev/null 2>&1; then
	# automake-1.9 (recommended) is not available on Fedora 8
	AUTOMAKE=automake
	ACLOCAL=aclocal
else
	ACLOCAL=aclocal-${AM_VERSION}
	AUTOMAKE=automake-${AM_VERSION}
fi

echo "Generating build scripts in buddylookup..."
set -x
libtoolize --copy --force
autoheader
$ACLOCAL 
$AUTOMAKE --force-missing --add-missing --copy
autoconf
