#!/bin/bash -x

function die {
	echo "$@"
	exit 1
}

if [ $# != 1 ]; then
	die "error: please provide liblinphone SDK version to download (for instance 3.13.9)"
fi

root_path="$(dirname $0)/.."
# only download SDK if it has not yet been built
if [ ! -d "$root_path"/liblinphone-sdk ]; then
	sdk_version=liblinphone-iphone-sdk-$1
	sdk_path="$root_path/$sdk_version"
	if [ -L "$root_path/liblinphone-sdk" ]; then
		rm "$root_path/liblinphone-sdk"
	fi

	if [ ! -d "$sdk_path" ]; then
		if which wget &>/dev/null; then
			wget https://www.linphone.org/snapshots/ios/$sdk_version.zip -O "$sdk_path".zip
		elif which curl &>/dev/null; then
			curl -# https://www.linphone.org/snapshots/ios/$sdk_version.zip > "$sdk_path".zip
		else
			return 1
		fi || die "error: cannot download liblinphone SDK from linphone.org. Please check the README.md"

		echo "info: liblinphone SDK successfully downloaded."
	fi

	unzip -x $sdk_path.zip
	mv liblinphone-sdk $sdk_path
	ln -s $sdk_path liblinphone-sdk
fi
