#!/bin/sh

# Copyright (C) 2012  Belledonne Comunications, Grenoble, France
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# Created by Gautier Pelloux-Prayer on 2014.
# Generate English translation files so that they can be pushed to Transifex
# for translation

root_directory=$(cd "$(dirname $0)" && pwd)/../

set -e

if [ $# != 0 ]; then
	echo "No argument needed. This script will (re)generate .strings file from .xib files and register them in the transifex config file located in .tx/config."
	exit 0
fi

function generate_transifex_config {
	res_name=$1
	file_filter=$2
	source_file=$(test -f ${file_filter/<lang>/en}&&echo ${file_filter/<lang>/en}||echo ${file_filter/<lang>/Base})
	if ! grep -q $res_name $root_directory/.tx/config; then
		echo "not found in .tx/config, adding it"
		echo "
[linphone-ios.$res_name]
source_lang = en
file_filter = $file_filter" >> $root_directory/.tx/config
		if [ ! -z "$source_file" ]; then
			echo "source_file = $source_file" >> $root_directory/.tx/config
		fi
	fi
}

##### 1. Generate Localizable.strings from source files (.m)
function generate_localizable_from_sources {
	#WARNING: in case of sed issue "extra characters at the end of g command", it means that
	# we are trying to modify an UTF-16 file which is not supported..

	localizable_en=$root_directory/Resources/en.lproj/Localizable.strings
	# The 2 only specific cases of the application: since we are length limited for push
	# notifications, the ID is not matching the English translation... so we must keep
	# the translations!
	iconv -f utf-16 -t utf-8 $localizable_en > $localizable_en.tmp
	IC_MSG_EN=$(sed -nE 's/"IC_MSG" = "(.*)";/\1/p' $localizable_en.tmp)
	IM_MSG_EN=$(sed -nE 's/"IM_MSG" = "(.*)";/\1/p' $localizable_en.tmp)
	IM_FULLMSG_EN=$(sed -nE 's/"IM_FULLMSG" = "(.*)";/\1/p' $localizable_en.tmp)
	rm -f $localizable_en $localizable_en.tmp

	find $root_directory/Classes -name '*.m' | xargs genstrings -u -a -o $(dirname $localizable_en)
	iconv -f utf-16LE -t utf-8 $localizable_en > $localizable_en.tmp
	sed -i.bak "s/= \"IC_MSG\";/= \"$IC_MSG_EN\";/" $localizable_en.tmp
	sed -i.bak "s/= \"IM_MSG\";/= \"$IM_MSG_EN\";/" $localizable_en.tmp
	sed -i.bak "s/= \"IM_FULLMSG\";/= \"$IM_FULLMSG_EN\";/" $localizable_en.tmp
	iconv -f utf-8 -t utf-16LE $localizable_en.tmp > $localizable_en
	rm $localizable_en.tmp.bak $localizable_en.tmp

	generate_transifex_config localizablestrings "Resources/<lang>.lproj/Localizable.strings"
}

##### 2. Generate .strings for all XIB files
function generate_strings_from_xib {
	to_utf8_file=$(mktemp -t linphone)
	find $root_directory/Classes -not -path "$root_directory/Classes/KIF/*" -name Base.lproj -exec find {} -name '*.xib' \; | while read -r xibfile; do
		stringsfile="${xibfile/.xib/.strings}"

		ibtool --generate-strings-file "$stringsfile" "$xibfile"

		# remove if empty
		iconv -f utf-16 -t utf-8 "$stringsfile" > "$to_utf8_file"
		if [ $(stat -f '%z' $to_utf8_file) -le 1 ]; then
			echo "$(basename "$stringsfile") is empty, removing"
			rm "$stringsfile"
		else
			echo "$(basename "$xibfile")->$(basename "$stringsfile")"

			res_name=$(basename "$stringsfile" | tr -d '_.~-' | tr '[:upper:]' '[:lower:]')
			dir_name=$(echo $(dirname "$stringsfile") | sed -E "s|$root_directory/||")


			generate_transifex_config $res_name $(echo $dir_name| sed 's/Base.lproj/<lang>.lproj/')/$(basename "$stringsfile")
		fi
	done
	rm $to_utf8_file
}

##### 3. Generate .strings for all InAppSettings PLIST files
function generate_strings_from_inappsettings_plist {
	tmp_file=$(mktemp -t linphone)
	find $root_directory/Settings/InAppSettings.bundle -name '*.plist' | while read -r plistfile; do
		echo $plistfile
		plistfilestrings="$(basename ${plistfile/.plist/.strings})"
		printf '' > $tmp_file
		while read title; do
			echo "\"$title\" = \"$title\";" >> $tmp_file
		done <<< "$(grep -e '>Title<' $plistfile -A 1 | sed -nE 's|.*<string>(.*)</string>.*|\1|p')"

		mv $tmp_file $root_directory/Settings/InAppSettings.bundle/en.lproj/$plistfilestrings

		res_name=inappsettings$(echo "$plistfilestrings" | tr -d '_.~-' | tr '[:upper:]' '[:lower:]')
		generate_transifex_config $res_name "Settings/InAppSettings.bundle/<lang>.lproj/$plistfilestrings"
	done
}

generate_localizable_from_sources
generate_strings_from_xib
generate_strings_from_inappsettings_plist
