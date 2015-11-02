#!/bin/bash

# change_ios_custom_font.sh
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

# Created by Gautier Pelloux-Prayer on 2014/10/07.
# This script can be used to replace default font with a custom one in every xib
# Please note that it changes only .xib files and not hardcoded fonts in .m/.h files.
# It creates a backup file with .backup_font extension.

if [ $# != 0 ]; then
	echo "Usage: $0"
	echo "To avoid reentering input, you can preset values, for intance:"
	echo 'repository=. newfont_family="Movistar Text" newregular_font=MovistarText newbold_font=MovistarText-Bold newitalic_font=MovistarText-Italic' $0
	exit 0
fi

# quit on first error
set -e

#################################### Input values #########################
# read $2 from stdin if it is not already set. Works also for multiple reads, but
# only check that the first read variable is not set
function readv {
	prompt=$1
	shift
	if [ -z "${!1}" ]; then
		echo "$prompt"
		read "$@" #cannot use read -p because unknown on MacOS
	else
		echo "$1 already set ($1=${!1}). Skipping $@"
	fi

	for arg in "$@"; do
		echo "$arg=${!arg}"
	done
}
echo $repository
readv '# Your git repository where we must apply changes like $HOME/code/linphone-iphone' repository
readv '# The font family name like "Helvetica"' newfont_family
readv '# The normal font like "Helvetica"' newregular_font
readv '# The bold font like "Helvetica-Bold"' newbold_font
readv '# The italic font like "Helvetica-Italic"' newitalic_font
readv '# RGB values for new text color like "255 255 255" for white' newred newgreen newblue
newred=$(echo $newred / 255.0 | bc -l)
newgreen=$(echo $newgreen / 255.0 | bc -l)
newblue=$(echo $newblue / 255.0 | bc -l)
################################################################################

# This is default Linphone text color ("gray")
oldred="\"0.35"
oldgreen="\"0.39"
oldblue="\"0.43"

if [ ! -d "$repository" ]; then
	echo "Invalid repository '$repository': it does not exist"
	exit 1
elif [ ! -d "$repository/Classes/" ]; then
	echo "Invalid repository '$repository': expected subfolder Classses/ does not exist"
	exit 2
fi


cd $repository/Classes

fonts=( "system:$newregular_font"
"boldSystem:$newbold_font"
"italicSystem:$newitalic_font" )


for font in "${fonts[@]}"; do
	system_font=${font%:*}
	new_font=${font#*:}
	echo "$system_font -> $new_font"

	find . -name '*.xib' -exec \
		sed -E -i.font_backup \
			-e "s|<fontDescription key=\"fontDescription\" type=\"$system_font\"|<fontDescription key=\"fontDescription\" name=\"$new_font\" family=\"$newfont_family>\"|g"  \
			-e "s|<color key=\"(.*)Color\".*$oldred.*$oldgreen.*$oldblue.*|<color key=\"\\1Color\" red=\"$newred\" green=\"$newgreen\" blue=\"$newblue\" alpha=\"1\" colorSpace=\"deviceRGB\"/>|g" {} \;
done

echo "**********************
Done. Created .backup_font files. If you are OK with the change, you can remove them with:
find $repository -name '*.font_backup' -exec rm {} \;
If you are NOT ok with change, you can put them back.
**********************"
