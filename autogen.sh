#!/bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

THEDIR=`pwd`
cd $srcdir

#AM_VERSION="1.10"
if ! type aclocal-$AM_VERSION 1>/dev/null 2>&1; then
	# automake-1.10 (recommended) is not available on Fedora 8
	AUTOMAKE=automake
	ACLOCAL=aclocal
else
	ACLOCAL=aclocal-${AM_VERSION}
	AUTOMAKE=automake-${AM_VERSION}
fi

LIBTOOLIZE="libtoolize"
for lt in glibtoolize libtoolize15 libtoolize14 libtoolize13 ; do
        if test -x /usr/bin/$lt ; then
                LIBTOOLIZE=$lt ; break
        fi
        if test -x /usr/local/bin/$lt ; then
                LIBTOOLIZE=$lt ; break
        fi
        if test -x /opt/local/bin/$lt ; then
                LIBTOOLIZE=$lt ; break
        fi
done

if test -d /opt/local/share/aclocal ; then
		ACLOCAL_ARGS="-I /opt/local/share/aclocal"
fi

if test -d /share/aclocal ; then
		ACLOCAL_ARGS="$ACLOCAL_ARGS -I /share/aclocal"
fi

INTLTOOLIZE=$(which intltoolize)

#workaround for mingw bug in intltoolize script.
if test "$INTLTOOLIZE" = "/bin/intltoolize" ; then
	INTLTOOLIZE=/usr/bin/intltoolize
fi

echo "Generating build scripts in linphone..."
set -x
$LIBTOOLIZE --copy --force

$INTLTOOLIZE -c --force --automake
$ACLOCAL -I m4 $ACLOCAL_ARGS
autoheader
$AUTOMAKE --force-missing --add-missing --copy
autoconf

set +x

#install git pre-commit hooks if possible
if [ -d .git/hooks ] && [ ! -f .git/hooks/pre-commit ]; then
		cp .git-pre-commit .git/hooks/pre-commit
		chmod +x .git/hooks/pre-commit
fi

if [ "$srcdir" = "." ]; then
	if [ -x oRTP/autogen.sh ]; then
		echo "Generating build scripts in oRTP..."
		( cd oRTP && ./autogen.sh )
	fi

	if [ -x mediastreamer2/autogen.sh ]; then
		echo "Generating build scripts in mediastreamer2..."
		( cd mediastreamer2 && ./autogen.sh )
	fi
fi

cd $THEDIR
