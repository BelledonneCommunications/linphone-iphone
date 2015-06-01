#!/bin/sh

# tag_missing_resources.sh
#
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

# Created by Gautier Pelloux-Prayer on 2014/10/09.
# This script will search for resources contained in Resources/ folder but not
# present in xcode build project project.pbxproj. Since we do not use folder
# references (because of http://vocaro.com/trevor/blog/2012/10/21/xcode-groups-vs-folder-references/),
# it helps keeping resources synced.
# Basically, the script will set a red tag to every files not added in the xcode project. User will then
# have to drag&drop these into xcode project. Red tags are automatically reset by the script on execution


if [ $# != 0 ]; then
	echo "Usage: $0"
	exit 0
fi

if ! which tag &>/dev/null; then
	echo "Please install tag: port install tag or brew install tag"
	exit 1
fi

# quit on first error
set -e

# go where the script is
cd $(dirname $0)

already_sync=$(mktemp -t tag_missing_resources)
to_sync=$(mktemp -t tag_missing_resources)

grep -oE '([^ /"])*.png' ../linphone.xcodeproj/project.pbxproj | sort -u > $already_sync
find ../Resources/ -not -path '*/Images.xcassets/*' -name '*.png' -exec basename {} \; | sort -u > $to_sync

# clean red tags
for file in $to_sync $already_sync; do
	find ../Resources -name $file -exec tag -r red {} \;
done

# 'comm' command output files contained in second file but not in first nor in common
non_synced_files=$(comm -13 $already_sync $to_sync)

for file in $non_synced_files; do
	find ../Resources -name $file -exec tag -a red {} \;
done

rm $already_sync $to_sync

echo "**********************
Done. Created red tags for non-synced files:
${non_synced_files}.

Drag and drop these files into your xcode project. You can go to the folder using:
	open $PWD/../Resources
Then use search feature and type 'red', select the red tag and go!
**********************"
