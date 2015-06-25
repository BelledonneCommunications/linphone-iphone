#!/usr/bin/env python3

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
# Update non-English translation by regenerating .strings files and reapplying
# existing translations after that

import codecs
import re
import sys
import shutil

kvpattern = re.compile('^(.*) = (.*);$')


def find_english_for_key(file, key):
    with codecs.open(file, 'r', 'utf-16') as fp:
        for line in fp:
            match = kvpattern.match(line)

            if match is None:
                continue

            if key == match.groups()[0]:
                return match.groups()[1]

    return None


def update_messages_for_file(old_file, new_file):
    translations = {}
    with codecs.open(old_file, 'r', 'utf-16') as fp:
        for line in fp:
            match = kvpattern.match(line)

            if match is None:
                continue

            english_value = find_english_for_key(new_file, match.groups()[0])
            foreign_value = match.groups()[1]

            translations[english_value] = foreign_value
    with codecs.open(new_file, 'r', 'utf-16') as f:
        lines = f.read()
    for english_value, foreign_value in translations.items():
        print("replace {} with {}".format(english_value, foreign_value))
        lines = lines.replace("{};".format(english_value), "{};".format(foreign_value))
    with codecs.open(new_file, 'w', 'utf-16') as f1:
        f1.write(lines)
        return True

    return False

if len(sys.argv) != 3:
    print("Usage: {} English.strings CustomLanguage.strings".format(sys.argv[0]))
    print("CustomLanguage.strings will be modified to take all English strings with its own translations")
else:
    new_file = sys.argv[2] + ".new"
    shutil.copyfile(sys.argv[1], new_file)
    if (update_messages_for_file(sys.argv[2], new_file)):
        shutil.move(new_file, sys.argv[2])
