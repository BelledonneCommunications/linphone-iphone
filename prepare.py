#!/usr/bin/env python

############################################################################
# prepare.py
# Copyright (C) 2015  Belledonne Communications, Grenoble France
#
############################################################################
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
############################################################################

import argparse
import os
import sys
sys.path.insert(0, 'submodules/cmake-builder')
import prepare


platforms = ['all', 'devices', 'simulators', 'armv7', 'arm64', 'i386', 'x86_64']


def main(argv = None):
	if argv is None:
		argv = sys.argv
	argparser = argparse.ArgumentParser(description="Prepare build of Linphone and its dependencies.")
	argparser.add_argument('-c', '--clean', help="Clean a previous build instead of preparing a build.", action='store_true')
	argparser.add_argument('-C', '--veryclean', help="Clean a previous build and its installation directory.", action='store_true')
	argparser.add_argument('-d', '--debug', help="Prepare a debug build.", action='store_true')
	argparser.add_argument('-f', '--force', help="Force preparation, even if working directory already exist.", action='store_true')
	argparser.add_argument('-L', '--list-cmake-variables', help="List non-advanced CMake cache variables.", action='store_true', dest='list_cmake_variables')
	argparser.add_argument('platform', choices=platforms, help="The platform to build for.")
	args, additional_args = argparser.parse_known_args()

	selected_platforms = []
	if args.platform == 'all':
		selected_platforms += ['armv7', 'arm64', 'i386', 'x86_64']
	elif args.platform == 'devices':
		selected_platforms += ['armv7', 'arm64']
	elif args.platform == 'simulators':
		selected_platforms += ['i386', 'x86_64']
	else:
		selected_platforms += [args.platform]

	retcode = 0
	for platform in selected_platforms:
		target = prepare.targets['ios-' + platform]

		if args.veryclean:
			target.veryclean()
		elif args.clean:
			target.clean()
		else:
			retcode = prepare.run(target, args.debug, False, args.list_cmake_variables, args.force, additional_args)
			if retcode != 0:
				return retcode

	return retcode

if __name__ == "__main__":
	sys.exit(main())
