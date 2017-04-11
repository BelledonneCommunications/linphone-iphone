#!/bin/bash -x

function die {
	echo "$@"
	exit 1
}

if [ $# != 2 ]; then
	die "error: please provide what kind of SDK (for instance release and liblinphone SDK version to download (for instance 3.16.1)"
fi

root_path="$(dirname $0)/.."
sdk_version=liblinphone-iphone-sdk-$2
filename=$(find . -name liblinphone-iphone-sdk-*.zip)
version=$(echo $filename| cut -d'-' -f 4)
version=$(echo "${version:0:${#version}-4}")
if [ -z "$version" ]; then
	version="0";
fi 

if [ $(expr ${version} \< ${2}) -eq 1 ]; then
	for f in ./liblinphone-iphone-sdk*
	do
		rm -r -f "$f"
	done
	
	echo "Downloading SDK"
	sdk_dir=$1
	sdk_path="$root_path/$sdk_version"
	if [ -L "$root_path/liblinphone-sdk" ]; then
		rm "$root_path/liblinphone-sdk"
	fi

	if [ ! -d "$sdk_path" ]; then
		if which wget &>/dev/null; then
			wget https://www.linphone.org/$sdk_dir/ios/$sdk_version.zip -O "$sdk_path".zip
		elif which curl &>/dev/null; then
			curl -# https://www.linphone.org/$sdk_dir/ios/$sdk_version.zip > "$sdk_path".zip
		else
			return 1
		fi || die "error: cannot download liblinphone SDK from linphone.org. Please check the README.md"

		echo "info: liblinphone SDK successfully downloaded."
	fi

	unzip -x $sdk_path.zip
	mv liblinphone-sdk $sdk_path
	ln -s $sdk_path liblinphone-sdk
fi
