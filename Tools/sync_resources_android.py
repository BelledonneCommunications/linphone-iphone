#!/usr/bin/env python

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

# Created by Gautier Pelloux-Prayer on 2014/10/23.
# Create the correspondence map between iOS resources name and Android ones.


import os
import sys
import hashlib
import fnmatch


def file_md5(file):
    hasher = hashlib.md5()
    with open(file, 'rb') as afile:
        buf = afile.read()
        hasher.update(buf)
    return hasher.hexdigest()


def list_glob_png(dir):
    matches = []
    for root, dirnames, filenames in os.walk(dir):
        for filename in fnmatch.filter(filenames, '*.png'):
            matches.append(os.path.join(root, filename))
    return matches


def _halt(message, code):
    sys.stderr.write("[ERROR] %s\n" % message)
    sys.exit(0 << code)


def compare_md5(ios_dir, android_dir):
    ios_images = list_glob_png(ios_dir)
    android_images = list_glob_png(android_dir)

    ios_md5 = {}
    for image in ios_images:
        ios_md5[file_md5(image)] = image

    android_md5 = {}
    for image in android_images:
        android_md5[file_md5(image)] = image

    common_list = []
    ios_list = []
    android_list = []

    for key in ios_md5:
        if key in android_md5:
            common_list.append(key)
        else:
            ios_list.append(key)
    for key in android_md5:
        if key not in ios_md5:
            android_list.append(key)

    print("Common:")
    for key in common_list:
        print("{} = {}".format(ios_md5[key], android_md5[key]))
    print("ios only:")
    for key in ios_list:
        print(ios_md5[key])
    print("android only:")
    for key in android_list:
        print(android_md5[key])


if __name__ == '__main__':
    if len(sys.argv) < 3:
        _halt('Usage: {} iOS-repo android-repo'.format(sys.argv[0]), 1)
    ios_dir = sys.argv[1] + "/Resources/"
    android_dir = sys.argv[2] + "/res/"

    if not os.path.exists(ios_dir):
        _halt("The directory '%s' does not exist" %
              ios_dir, 2)
    if not os.path.exists(android_dir):
        _halt("The directory '%s' does not exist" %
              android_dir, 2)

    compare_md5(ios_dir, android_dir)
