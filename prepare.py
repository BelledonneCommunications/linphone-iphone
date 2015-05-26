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
import shutil
import sys
sys.path.insert(0, 'submodules/cmake-builder')
import prepare


class IOSTarget(prepare.Target):
	def __init__(self, arch):
		prepare.Target.__init__(self, 'ios-' + arch)
		current_path = os.path.dirname(os.path.realpath(__file__))
		self.config_file = 'configs/config-ios-' + arch + '.cmake'
		self.toolchain_file = 'toolchains/toolchain-ios-' + arch + '.cmake'
		self.output = 'liblinphone-sdk/' + arch + '-apple-darwin.ios'
		self.additional_args = [
			'-DLINPHONE_BUILDER_EXTERNAL_SOURCE_PATH=' + current_path + '/submodules'
		]

	def clean(self):
		prepare.Target.clean(self)
		if os.path.isdir('liblinphone-sdk/apple-darwin'):
			shutil.rmtree('liblinphone-sdk/apple-darwin', ignore_errors=False, onerror=self.handle_remove_read_only)


class IOSi386Target(IOSTarget):
	def __init__(self):
		IOSTarget.__init__(self, 'i386')

class IOSx8664Target(IOSTarget):
	def __init__(self):
		IOSTarget.__init__(self, 'x86_64')

class IOSarmv7Target(IOSTarget):
	def __init__(self):
		IOSTarget.__init__(self, 'armv7')

class IOSarm64Target(IOSTarget):
	def __init__(self):
		IOSTarget.__init__(self, 'arm64')


targets = {}
targets['i386'] = IOSi386Target()
targets['x86_64'] = IOSx8664Target()
targets['armv7'] = IOSarmv7Target()
targets['arm64'] = IOSarm64Target()
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
	makefile_platforms = []
	for platform in selected_platforms:
		target = targets[platform]

		if args.veryclean:
			target.veryclean()
		elif args.clean:
			target.clean()
		else:
			retcode = prepare.run(target, args.debug, False, args.list_cmake_variables, args.force, additional_args)
			if retcode != 0:
				return retcode
			makefile_platforms += [platform]

	if makefile_platforms:
		archs_specific = ""
		for arch in makefile_platforms[1:]:
			archs_specific += \
"""		if test -f "$${arch}_path"; then \\
			all_paths=`echo $$all_paths $${arch}_path`; \\
			all_archs="$$all_archs,{arch}" ; \\
		else \\
			echo "WARNING: archive `basename $$archive` exists in {first_arch} tree but does not exists in {arch} tree: $${arch}_path."; \\
		fi; \\
""".format(first_arch=makefile_platforms[0], arch=arch)
		makefile = """
archs={archs}

.PHONY: all

all: multi-arch

build-%:
	$(MAKE) -C WORK/ios-$*/cmake

multi-arch: $(addprefix build-,$(archs))
	archives=`find liblinphone-sdk/{first_arch}-apple-darwin.ios -name *.a` && \\
	mkdir -p liblinphone-sdk/apple-darwin && \\
	cp -rf liblinphone-sdk/{first_arch}-apple-darwin.ios/include liblinphone-sdk/apple-darwin/. && \\
	cp -rf liblinphone-sdk/{first_arch}-apple-darwin.ios/share liblinphone-sdk/apple-darwin/. && \\
	for archive in $$archives ; do \\
		armv7_path=`echo $$archive | sed -e "s/{first_arch}/armv7/"`; \\
		arm64_path=`echo $$archive | sed -e "s/{first_arch}/aarch64/"`; \\
		i386_path=`echo $$archive | sed -e "s/{first_arch}/i386/"`; \\
		x86_64_path=`echo $$archive | sed -e "s/{first_arch}/x86_64/"`; \\
		destpath=`echo $$archive | sed -e "s/-debug//" | sed -e "s/{first_arch}-//" | sed -e "s/\.ios//"`; \\
		all_paths=`echo $$archive`; \\
		all_archs="{first_arch}"; \\
		mkdir -p `dirname $$destpath`; \\
		{archs_specific} \\
		echo "[$$all_archs] Mixing `basename $$archive` in $$destpath"; \\
		lipo -create $$all_paths -output $$destpath; \\
	done && \\
	if ! test -f liblinphone-sdk/apple-darwin/lib/libtunnel.a ; then \\
		cp -f submodules/binaries/libdummy.a liblinphone-sdk/apple-darwin/lib/libtunnel.a ; \\
	fi
""".format(archs=' '.join(makefile_platforms), first_arch=makefile_platforms[0], archs_specific=archs_specific)
		f = open('Makefile', 'w')
		f.write(makefile)
		f.close()
	elif os.path.isfile('Makefile'):
		os.remove('Makefile')

	return retcode

if __name__ == "__main__":
	sys.exit(main())
