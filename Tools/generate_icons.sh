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

# Created by Gautier Pelloux-Prayer on 2014/10/22.
# Given a new icon, this script will generate all needed icons for the application
# and replace them in Ressources/.

if [ $# != 1 ]; then
	echo "Usage: $0 logo.png"
	exit 0
elif [ ! -f $1 ]; then
	echo "Icon $1 does not exist"
	exit 1
fi

icon_prefix="linphone_icon"

# quit on first error
set -e

logo=$1
logo_size=$(identify $logo | cut -d' ' -f3)
logo_sizex=$(echo $logo_size | cut -d x -f1)
logo_sizey=$(echo $logo_size | cut -d x -f2)

if [ "$logo_sizex" != "$logo_sizey" ] || [ $logo_sizex -lt 512 ]; then
	echo "Provide a high SQUARED resolution image (512x512 or higher). Current is $logo_size"
	exit 1
fi

resources=$(dirname $0)/../Resources/

for file in $(find $(dirname $0)/.. -name "$icon_prefix*.png" -or -name "iTunesArtwork*" ); do
	dimension=$(identify $file | cut -d' ' -f3 | cut -d x -f1)
	echo "Resizing to dimension ${dimension}x${dimension} for $(basename $file)"
	convert $logo -resize ${dimension}x${dimension} $file
done

echo "**********************
Done.
**********************"
