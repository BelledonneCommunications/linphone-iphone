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
import re
import shutil
import sys
sys.dont_write_bytecode = True
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
		if os.path.isdir('WORK'):
			shutil.rmtree('WORK', ignore_errors=False, onerror=self.handle_remove_read_only)
		if os.path.isdir('liblinphone-sdk'):
			shutil.rmtree('liblinphone-sdk', ignore_errors=False, onerror=self.handle_remove_read_only)


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

class PlatformListAction(argparse.Action):
	def __call__(self, parser, namespace, values, option_string=None):
		if values:
			for value in values:
				if value not in platforms:
					message = ("invalid platform: {0!r} (choose from {1})".format(value, ', '.join([repr(platform) for platform in platforms])))
					raise argparse.ArgumentError(self, message)
			setattr(namespace, self.dest, values)

def warning(platforms):
	gpl_third_parties_enabled = False
	regex = re.compile("^ENABLE_GPL_THIRD_PARTIES:BOOL=ON")
	f = open('WORK/ios-{arch}/cmake/CMakeCache.txt'.format(arch=platforms[0]), 'r')
	for line in f:
		if regex.match(line):
			gpl_third_parties_enabled = True
			break
	f.close()
	
	if gpl_third_parties_enabled:
		print("""
***************************************************************************
***************************************************************************
***** CAUTION, this liblinphone SDK is built using 3rd party GPL code *****
***** Even if you acquired a proprietary license from Belledonne      *****
***** Communications, this SDK is GPL and GPL only.                   *****
***** To disable 3rd party gpl code, please use:                      *****
***** $ ./prepare.py -DENABLE_GPL_THIRD_PARTIES=NO                    *****
***************************************************************************
***************************************************************************
""")
	else:
		print("""
*****************************************************************
*****************************************************************
***** Linphone SDK without 3rd party GPL software           *****
***** If you acquired a proprietary license from Belledonne *****
***** Communications, this SDK can be used to create        *****
***** a proprietary linphone-based application.             *****
*****************************************************************
*****************************************************************
""")

def main(argv = None):
	if argv is None:
		argv = sys.argv
	argparser = argparse.ArgumentParser(description="Prepare build of Linphone and its dependencies.")
	argparser.add_argument('-c', '-C', '--clean', help="Clean a previous build instead of preparing a build.", action='store_true')
	argparser.add_argument('-d', '--debug', help="Prepare a debug build.", action='store_true')
	argparser.add_argument('-f', '--force', help="Force preparation, even if working directory already exist.", action='store_true')
	argparser.add_argument('-L', '--list-cmake-variables', help="List non-advanced CMake cache variables.", action='store_true', dest='list_cmake_variables')
	argparser.add_argument('platform', nargs='*', action=PlatformListAction, default=['all'], help="The platform to build for (default is all), one of: {0}.".format(', '.join([repr(platform) for platform in platforms])))
	args, additional_args = argparser.parse_known_args()

	selected_platforms = []
	for platform in args.platform:
		if platform == 'all':
			selected_platforms += ['armv7', 'arm64', 'i386', 'x86_64']
		elif platform == 'devices':
			selected_platforms += ['armv7', 'arm64']
		elif platform == 'simulators':
			selected_platforms += ['i386', 'x86_64']
		else:
			selected_platforms += [platform]
	selected_platforms = list(set(selected_platforms))

	retcode = 0
	makefile_platforms = []
	for platform in selected_platforms:
		target = targets[platform]

		if args.clean:
			target.clean()
		else:
			if args.debug:
				additional_args += ["-DENABLE_DEBUG_LOGS=YES"]
			retcode = prepare.run(target, args.debug, False, args.list_cmake_variables, args.force, additional_args)
			if retcode != 0:
				return retcode
			makefile_platforms += [platform]

	if makefile_platforms:
		packages = os.listdir('WORK/ios-' + makefile_platforms[0] + '/Build')
		packages.sort()
		arch_targets = ""
		for arch in makefile_platforms:
			arch_targets += """
{arch}:
	$(MAKE) -C WORK/ios-{arch}/cmake

{arch}-build:
	@for package in $(packages); do \\
		$(MAKE) {arch}-build-$$package; \\
	done

{arch}-clean:
	@for package in $(packages); do \\
		$(MAKE) {arch}-clean-$$package; \\
	done

{arch}-veryclean:
	@for package in $(packages); do \\
		$(MAKE) {arch}-veryclean-$$package; \\
	done

{arch}-build-%:
	$(MAKE) -C WORK/ios-{arch}/cmake EP_$*

{arch}-clean-%:
	$(MAKE) -C WORK/ios-{arch}/Build/$* clean; \\
	rm -f WORK/ios-{arch}/Stamp/EP_$*/EP_$*-build; \\
	rm -f WORK/ios-{arch}/Stamp/EP_$*/EP_$*-install;

{arch}-veryclean-%:
	cat WORK/ios-{arch}/Build/$*/install_manifest.txt | xargs rm; \\
	rm -rf WORK/ios-{arch}/Build/$*/*; \\
	rm -f WORK/ios-{arch}/Stamp/EP_$*/*; \\
	echo "Run 'make {arch}' to rebuild $* correctly.";

{arch}-clean-openh264:
	cd WORK/ios-{arch}/Build/openh264; \\
	make -f ../../../../submodules/externals/openh264/Makefile clean; \\
	rm -f WORK/ios-{arch}/Stamp/EP_openh264/EP_openh264-build; \\
	rm -f WORK/ios-{arch}/Stamp/EP_openh264/EP_openh264-install;
""".format(arch=arch)
		multiarch = ""
		for arch in makefile_platforms[1:]:
			multiarch += \
"""		if test -f "$${arch}_path"; then \\
			all_paths=`echo $$all_paths $${arch}_path`; \\
			all_archs="$$all_archs,{arch}" ; \\
		else \\
			echo "WARNING: archive `basename $$archive` exists in {first_arch} tree but does not exists in {arch} tree: $${arch}_path."; \\
		fi; \\
""".format(first_arch=makefile_platforms[0], arch=arch)
		makefile = """
archs={archs}
packages={packages}
LINPHONE_IPHONE_VERSION=$(shell git describe --always)

.PHONY: all

all: build

{arch_targets}
all-%:
	$(MAKE) -C WORK/ios-$*/cmake

build-%:
	@for arch in $(archs); do \\
		echo "==== starting build of $* for arch $$arch ===="; \\
		$(MAKE) $$arch-build-$*; \\
	done

clean-%:
	@for arch in $(archs); do \\
		echo "==== starting clean of $* for arch $$arch ===="; \\
		$(MAKE) $$arch-clean-$*; \\
	done

veryclean-%:
	@for arch in $(archs); do \\
		echo "==== starting veryclean of $* for arch $$arch ===="; \\
		$(MAKE) $$arch-veryclean-$*; \\
	done; \\
	echo "Run 'make' to rebuild $* correctly."

build: libs sdk

clean: $(addprefix clean-,$(packages))

veryclean: $(addprefix veryclean-,$(packages))

libs: $(addprefix all-,$(archs))
	archives=`find liblinphone-sdk/{first_arch}-apple-darwin.ios -name *.a` && \\
	mkdir -p liblinphone-sdk/apple-darwin && \\
	cp -rf liblinphone-sdk/{first_arch}-apple-darwin.ios/include liblinphone-sdk/apple-darwin/. && \\
	cp -rf liblinphone-sdk/{first_arch}-apple-darwin.ios/share liblinphone-sdk/apple-darwin/. && \\
	for archive in $$archives ; do \\
		armv7_path=`echo $$archive | sed -e "s/{first_arch}/armv7/"`; \\
		arm64_path=`echo $$archive | sed -e "s/{first_arch}/arm64/"`; \\
		i386_path=`echo $$archive | sed -e "s/{first_arch}/i386/"`; \\
		x86_64_path=`echo $$archive | sed -e "s/{first_arch}/x86_64/"`; \\
		destpath=`echo $$archive | sed -e "s/-debug//" | sed -e "s/{first_arch}-//" | sed -e "s/\.ios//"`; \\
		all_paths=`echo $$archive`; \\
		all_archs="{first_arch}"; \\
		mkdir -p `dirname $$destpath`; \\
		{multiarch} \\
		echo "[$$all_archs] Mixing `basename $$archive` in $$destpath"; \\
		lipo -create $$all_paths -output $$destpath; \\
	done && \\
	if ! test -f liblinphone-sdk/apple-darwin/lib/libtunnel.a ; then \\
		cp -f submodules/binaries/libdummy.a liblinphone-sdk/apple-darwin/lib/libtunnel.a ; \\
	fi

ipa: build
	xcodebuild -configuration Release \\
	&& xcrun -sdk iphoneos PackageApplication -v build/Release-iphoneos/linphone.app -o linphone-iphone.ipa

sdk: libs
	echo "Generating SDK zip file for version $(LINPHONE_IPHONE_VERSION)"
	zip -r liblinphone-iphone-sdk-$(LINPHONE_IPHONE_VERSION).zip \\
	liblinphone-sdk/apple-darwin \\
	liblinphone-tutorials \\
	-x liblinphone-tutorials/hello-world/build\* \\
	-x liblinphone-tutorials/hello-world/hello-world.xcodeproj/*.pbxuser \\
	-x liblinphone-tutorials/hello-world/hello-world.xcodeproj/*.mode1v3

pull-transifex:
	tx pull -af

push-transifex:
	&& ./Tools/generate_strings_files.sh && tx push -s -t -f --no-interactive

zipres:
	@tar -czf ios_assets.tar.gz Resources iTunesArtwork

help:
	@echo "(please read the README.md file first)"
	@echo ""
	@echo "Available architectures: {archs}"
	@echo "Available packages: {packages}"
	@echo ""
	@echo "Available targets:"
	@echo ""
	@echo "   * all       : builds all architectures and creates the liblinphone sdk"
	@echo "   * zipres    : creates a tar.gz file with all the resources (images)"
	@echo ""
	@echo "=== Advanced usage ==="
	@echo ""
	@echo "   *            build-[package] : builds the package for all architectures"
	@echo "   *            clean-[package] : clean the package for all architectures"
	@echo ""
	@echo "   *     [{arch_opts}]-build-[package] : builds a package for the selected architecture"
	@echo "   *     [{arch_opts}]-clean-[package] : clean the package for the selected architecture"
	@echo ""
	@echo "   * sdk  : re-add all generated libraries to the SDK. Use this only after a full build."
	@echo "   * libs : after a rebuild of a subpackage, will mix the new libs in liblinphone-sdk/apple-darwin directory"
""".format(archs=' '.join(makefile_platforms), arch_opts='|'.join(makefile_platforms), first_arch=makefile_platforms[0], arch_targets=arch_targets, packages=' '.join(packages), multiarch=multiarch)
		f = open('Makefile', 'w')
		f.write(makefile)
		f.close()
		warning(makefile_platforms)
	elif os.path.isfile('Makefile'):
		os.remove('Makefile')

	return retcode

if __name__ == "__main__":
	sys.exit(main())
