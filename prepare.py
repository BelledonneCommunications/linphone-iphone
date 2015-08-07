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
from distutils.spawn import find_executable
from subprocess import Popen, PIPE
sys.dont_write_bytecode = True
sys.path.insert(0, 'submodules/cmake-builder')
try:
    import prepare
except:
    print("Could not find prepare module, probably missing submodules/cmake-builder? Try running git submodule update --init --recursive")
    exit(1)


class IOSTarget(prepare.Target):

    def __init__(self, arch):
        prepare.Target.__init__(self, 'ios-' + arch)
        current_path = os.path.dirname(os.path.realpath(__file__))
        self.config_file = 'configs/config-ios-' + arch + '.cmake'
        self.toolchain_file = 'toolchains/toolchain-ios-' + arch + '.cmake'
        self.output = 'liblinphone-sdk/' + arch + '-apple-darwin.ios'
        self.additional_args = [
            '-DLINPHONE_BUILDER_EXTERNAL_SOURCE_PATH=' +
            current_path + '/submodules'
        ]

    def clean(self):
        if os.path.isdir('WORK'):
            shutil.rmtree(
                'WORK', ignore_errors=False, onerror=self.handle_remove_read_only)
        if os.path.isdir('liblinphone-sdk'):
            shutil.rmtree(
                'liblinphone-sdk', ignore_errors=False, onerror=self.handle_remove_read_only)


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


targets = {
    'i386': IOSi386Target(),
    'x86_64': IOSx8664Target(),
    'armv7': IOSarmv7Target(),
    'arm64': IOSarm64Target()
}
archs_device = ['arm64', 'armv7']
archs_simu = ['i386', 'x86_64']
platforms = ['all', 'devices', 'simulators'] + archs_device + archs_simu


class PlatformListAction(argparse.Action):

    def __call__(self, parser, namespace, values, option_string=None):
        if values:
            for value in values:
                if value not in platforms:
                    message = ("invalid platform: {0!r} (choose from {1})".format(
                        value, ', '.join([repr(platform) for platform in platforms])))
                    raise argparse.ArgumentError(self, message)
            setattr(namespace, self.dest, values)


def warning(platforms):
    gpl_third_parties_enabled = False
    regex = re.compile("^ENABLE_GPL_THIRD_PARTIES:BOOL=ON")
    f = open(
        'WORK/ios-{arch}/cmake/CMakeCache.txt'.format(arch=platforms[0]), 'r')
    for line in f:
        if regex.match(line):
            gpl_third_parties_enabled = True
            break
    f.close()

    if gpl_third_parties_enabled:
        print("***************************************************************************\n"
              "***************************************************************************\n"
              "***** CAUTION, this liblinphone SDK is built using 3rd party GPL code *****\n"
              "***** Even if you acquired a proprietary license from Belledonne      *****\n"
              "***** Communications, this SDK is GPL and GPL only.                   *****\n"
              "***** To disable 3rd party gpl code, please use:                      *****\n"
              "***** $ ./prepare.py -DENABLE_GPL_THIRD_PARTIES=NO                    *****\n"
              "***************************************************************************\n"
              "***************************************************************************\n")
    else:
        print("*****************************************************************\n"
              "*****************************************************************\n"
              "***** Linphone SDK without 3rd party GPL software           *****\n"
              "***** If you acquired a proprietary license from Belledonne *****\n"
              "***** Communications, this SDK can be used to create        *****\n"
              "***** a proprietary linphone-based application.             *****\n"
              "*****************************************************************\n"
              "*****************************************************************\n")


def extract_libs_list():
    l = []
    # name = libspeexdsp.a; path = "liblinphone-sdk/apple-darwin/lib/libspeexdsp.a"; sourceTree = "<group>"; };
    regex = re.compile("name = (lib(\S+)\.a); path = \"liblinphone-sdk/apple-darwin/")
    f = open('linphone.xcodeproj/project.pbxproj', 'r')
    lines = f.readlines()
    f.close()
    for line in lines:
        m = regex.search(line)
        if m is not None:
            l += [m.group(1)]
    return list(set(l))


def check_is_installed(binary, prog=None, warn=True):
    if not find_executable(binary):
        if warn:
            print("Could not find {}. Please install {}.".format(binary, prog))
        return False
    return True


def check_tools():
    reterr = 0

    if " " in os.path.dirname(os.path.realpath(__file__)):
        print("Invalid location: linphone-iphone path should not contain any spaces.")
        reterr = 1

    for prog in ["autoconf", "automake", "pkg-config", "doxygen", "java", "nasm", "cmake", "wget", "yasm", "optipng"]:
        reterr |= not check_is_installed(prog, "it")
    reterr |= not check_is_installed("ginstall", "coreutils")
    reterr |= not check_is_installed("intltoolize", "intltool")
    reterr |= not check_is_installed("convert", "imagemagick")

    if check_is_installed("libtoolize", warn=False):
        if not check_is_installed("glibtoolize", "libtool"):
            glibtoolize_path = find_executable(glibtoolize)
            reterr = 1
            error = "Please do a symbolic link from glibtoolize to libtoolize: 'ln -s {} ${}'."
            print(error.format(glibtoolize_path, glibtoolize_path.replace("glibtoolize", "libtoolize")))

    devnull = open(os.devnull, 'wb')
    # just ensure that JDK is installed - if not, it will automatiaclyl display a popup to user
    p = Popen("java -version".split(" "), stderr=devnull, stdout=devnull)
    p.wait()
    if p.returncode != 0:
        print(p.returncode)
        print("Please install Java JDK (not just JRE).")
        reterr = 1

    # needed by x264
    check_is_installed("gas-preprocessor.pl", """it:
        wget --no-check-certificate https://raw.github.com/yuvi/gas-preprocessor/master/gas-preprocessor.pl
        chmod +x gas-preprocessor.pl
        sudo mv gas-preprocessor.pl /usr/local/bin/""")

    nasm_output = Popen("nasm -f elf32".split(" "), stderr=PIPE, stdout=PIPE).stderr.read()
    if "fatal: unrecognised output format" in nasm_output:
        print(
            "Invalid version of nasm: your version does not support elf32 output format. If you have installed nasm, please check that your PATH env variable is set correctly.")
        reterr = 1

        if not os.path.isdir("submodules/linphone/mediastreamer2") or not os.path.isdir("submodules/linphone/oRTP"):
            print("Missing some git submodules. Did you run 'git submodule update --init --recursive'?")
            reterr = 1
    p = Popen("xcrun --sdk iphoneos --show-sdk-path".split(" "), stdout=devnull, stderr=devnull)
    p.wait()
    if p.returncode != 0:
        print("iOS SDK not found, please install Xcode from AppStore or equivalent.")
        reterr = 1
    else:
        sdk_platform_path = Popen("xcrun --sdk iphonesimulator --show-sdk-platform-path".split(" "), stdout=PIPE, stderr=devnull).stdout.read()[:-1]
        sdk_strings_path = "{}/{}".format(sdk_platform_path, "Developer/usr/bin/strings")
        if not os.path.isfile(sdk_strings_path):
            strings_path = find_executable("strings")
            print("strings binary missing, please run 'sudo ln -s {} {}'.".format(strings_path, sdk_strings_path))
            reterr = 1

    if reterr == 1:
        print("Failed to detect required tools, aborting.")

    return reterr


def install_git_hook():
    git_hook_path = ".git{sep}hooks{sep}pre-commit".format(sep=os.sep)
    if os.path.isdir(".git{sep}hooks".format(sep=os.sep)) and not os.path.isfile(git_hook_path):
        print("Installing Git pre-commit hook")
        shutil.copyfile(".git-pre-commit", git_hook_path)
        os.chmod(git_hook_path, 0755)


def generate_makefile(platforms, generator):
    libs_list = extract_libs_list()
    packages = os.listdir('WORK/ios-' + platforms[0] + '/Build')
    packages.sort()
    arch_targets = ""
    for arch in platforms:
        arch_targets += """
{arch}: all-{arch}

{arch}-build:
\t@for package in $(packages); do \\
\t\t$(MAKE) {arch}-build-$$package; \\
\tdone

{arch}-clean:
\t@for package in $(packages); do \\
\t\t$(MAKE) {arch}-clean-$$package; \\
\tdone

{arch}-veryclean:
\t@for package in $(packages); do \\
\t\t$(MAKE) {arch}-veryclean-$$package; \\
\tdone

{arch}-build-%: package-in-list-%
\trm -f WORK/ios-{arch}/Stamp/EP_$*/EP_$*-update; \\
\t{generator} WORK/ios-{arch}/cmake EP_$*

{arch}-clean-%: package-in-list-%
\t{generator} WORK/ios-{arch}/Build/$* clean; \\
\trm -f WORK/ios-{arch}/Stamp/EP_$*/EP_$*-build; \\
\trm -f WORK/ios-{arch}/Stamp/EP_$*/EP_$*-install;

{arch}-veryclean-%: package-in-list-%
\ttest -f WORK/ios-{arch}/Build/$*/install_manifest.txt && \\
\tcat WORK/ios-{arch}/Build/$*/install_manifest.txt | xargs rm; \\
\trm -rf WORK/ios-{arch}/Build/$*/*; \\
\trm -f WORK/ios-{arch}/Stamp/EP_$*/*; \\
\techo "Run 'make {arch}-build-$*' to rebuild $* correctly.";

{arch}-veryclean-ffmpeg:
\t{generator} WORK/ios-{arch}/Build/ffmpeg uninstall; \\
\trm -rf WORK/ios-{arch}/Build/ffmpeg/*; \\
\trm -f WORK/ios-{arch}/Stamp/EP_ffmpeg/*; \\
\techo "Run 'make {arch}-build-ffmpeg' to rebuild ffmpeg correctly.";

{arch}-clean-openh264:
\tcd WORK/ios-{arch}/Build/openh264; \\
\t$(MAKE) -f ../../../../submodules/externals/openh264/Makefile clean; \\
\trm -f WORK/ios-{arch}/Stamp/EP_openh264/EP_openh264-build; \\
\trm -f WORK/ios-{arch}/Stamp/EP_openh264/EP_openh264-install;

{arch}-veryclean-openh264:
\trm -rf liblinphone-sdk/{arch}-apple-darwin.ios/include/wels; \\
\trm -f liblinphone-sdk/{arch}-apple-darwin.ios/lib/libopenh264.*; \\
\trm -rf WORK/ios-{arch}/Build/openh264/*; \\
\trm -f WORK/ios-{arch}/Stamp/EP_openh264/*; \\
\techo "Run 'make {arch}-build-openh264' to rebuild openh264 correctly.";

{arch}-veryclean-vpx:
\trm -rf liblinphone-sdk/{arch}-apple-darwin.ios/include/vpx; \\
\trm -f liblinphone-sdk/{arch}-apple-darwin.ios/lib/libvpx.*; \\
\trm -rf WORK/ios-{arch}/Build/vpx/*; \\
\trm -f WORK/ios-{arch}/Stamp/EP_vpx/*; \\
\techo "Run 'make {arch}-build-vpx' to rebuild vpx correctly.";
""".format(arch=arch, generator=generator)
    multiarch = ""
    for arch in platforms[1:]:
        multiarch += \
"""\tif test -f "$${arch}_path"; then \\
\t\tall_paths=`echo $$all_paths $${arch}_path`; \\
\t\tall_archs="$$all_archs,{arch}" ; \\
\telse \\
\t\techo "WARNING: archive `basename $$archive` exists in {first_arch} tree but does not exists in {arch} tree: $${arch}_path."; \\
\tfi; \\
""".format(first_arch=platforms[0], arch=arch)
    makefile = """
archs={archs}
packages={packages}
libs_list={libs_list}
LINPHONE_IPHONE_VERSION=$(shell git describe --always)

.PHONY: all
.SILENT: lipo

all: build

{arch_targets}
all-%:
\t@for package in $(packages); do \\
\t\trm -f WORK/ios-$*/Stamp/EP_$$package/EP_$$package-update; \\
\tdone
\t{generator} WORK/ios-$*/cmake

package-in-list-%:
\tif ! grep -q " $* " <<< " $(packages) "; then \\
\t\techo "$* not in list of available packages: $(packages)"; \\
\t\texit 3; \\
\tfi

build-%: package-in-list-%
\t@for arch in $(archs); do \\
\t\techo "==== starting build of $* for arch $$arch ===="; \\
\t\t$(MAKE) $$arch-build-$*; \\
\tdone

clean-%: package-in-list-%
\t@for arch in $(archs); do \\
\t\techo "==== starting clean of $* for arch $$arch ===="; \\
\t\t$(MAKE) $$arch-clean-$*; \\
\tdone

veryclean-%: package-in-list-%
\t@for arch in $(archs); do \\
\t\techo "==== starting veryclean of $* for arch $$arch ===="; \\
\t\t$(MAKE) $$arch-veryclean-$*; \\
\tdone; \\
\techo "Run 'make build-$*' to rebuild $* correctly."

build: libs

clean: $(addprefix clean-,$(packages))

veryclean: $(addprefix veryclean-,$(packages))

lipo:
\tarchives=`find liblinphone-sdk/{first_arch}-apple-darwin.ios -name *.a` && \\
\tmkdir -p liblinphone-sdk/apple-darwin && \\
\tcp -rf liblinphone-sdk/{first_arch}-apple-darwin.ios/include liblinphone-sdk/apple-darwin/. && \\
\tcp -rf liblinphone-sdk/{first_arch}-apple-darwin.ios/share liblinphone-sdk/apple-darwin/. && \\
\tfor archive in $$archives ; do \\
\t\tarmv7_path=`echo $$archive | sed -e "s/{first_arch}/armv7/"`; \\
\t\tarm64_path=`echo $$archive | sed -e "s/{first_arch}/arm64/"`; \\
\t\ti386_path=`echo $$archive | sed -e "s/{first_arch}/i386/"`; \\
\t\tx86_64_path=`echo $$archive | sed -e "s/{first_arch}/x86_64/"`; \\
\t\tdestpath=`echo $$archive | sed -e "s/-debug//" | sed -e "s/{first_arch}-//" | sed -e "s/\.ios//"`; \\
\t\tall_paths=`echo $$archive`; \\
\t\tall_archs="{first_arch}"; \\
\t\tmkdir -p `dirname $$destpath`; \\
\t\t{multiarch} \\
\t\techo "[$$all_archs] Mixing `basename $$archive` in $$destpath"; \\
\t\tlipo -create $$all_paths -output $$destpath; \\
\tdone && \\
\tfor lib in {libs_list} ; do \\
\t\tif [ $${{lib:0:5}} = "libms" ] ; then \\
\t\t\tlibrary_path=liblinphone-sdk/apple-darwin/lib/mediastreamer/plugins/$$lib ; \\
\t\telse \\
\t\t\tlibrary_path=liblinphone-sdk/apple-darwin/lib/$$lib ; \\
\t\tfi ; \\
\t\tif ! test -f $$library_path ; then \\
\t\t\techo "[$$all_archs] Generating dummy $$lib static library." ; \\
\t\t\tcp -f submodules/binaries/libdummy.a $$library_path ; \\
\t\tfi \\
\tdone

libs: $(addprefix all-,$(archs))
\t$(MAKE) lipo

ipa: build
\txcodebuild -configuration Release \\
\t&& xcrun -sdk iphoneos PackageApplication -v build/Release-iphoneos/linphone.app -o linphone-iphone.ipa

sdk: libs
\techo "Generating SDK zip file for version $(LINPHONE_IPHONE_VERSION)"
\tzip -r liblinphone-iphone-sdk-$(LINPHONE_IPHONE_VERSION).zip \\
\tliblinphone-sdk/apple-darwin \\
\tliblinphone-tutorials \\
\t-x liblinphone-tutorials/hello-world/build\* \\
\t-x liblinphone-tutorials/hello-world/hello-world.xcodeproj/*.pbxuser \\
\t-x liblinphone-tutorials/hello-world/hello-world.xcodeproj/*.mode1v3

pull-transifex:
\ttx pull -af

push-transifex:
\t./Tools/i18n_generate_strings_files.sh && \\
\ttx push -s -t -f --no-interactive

zipres:
\t@tar -czf ios_assets.tar.gz Resources iTunesArtwork

help-prepare-options:
\t@echo "prepare.py was previously executed with the following options:"
\t@echo "   {options}"

help: help-prepare-options
\t@echo ""
\t@echo "(please read the README.md file first)"
\t@echo ""
\t@echo "Available architectures: {archs}"
\t@echo "Available packages: {packages}"
\t@echo ""
\t@echo "Available targets:"
\t@echo ""
\t@echo "   * all       : builds all architectures and creates the liblinphone sdk"
\t@echo "   * zipres    : creates a tar.gz file with all the resources (images)"
\t@echo ""
\t@echo "=== Advanced usage ==="
\t@echo ""
\t@echo "   *            build-[package] : builds the package for all architectures"
\t@echo "   *            clean-[package] : clean the package for all architectures"
\t@echo ""
\t@echo "   *     [{arch_opts}]-build-[package] : builds a package for the selected architecture"
\t@echo "   *     [{arch_opts}]-clean-[package] : clean the package for the selected architecture"
\t@echo ""
\t@echo "   * sdk  : re-add all generated libraries to the SDK. Use this only after a full build."
\t@echo "   * libs : after a rebuild of a subpackage, will mix the new libs in liblinphone-sdk/apple-darwin directory"
""".format(archs=' '.join(platforms), arch_opts='|'.join(platforms),
           first_arch=platforms[0], options=' '.join(sys.argv),
           arch_targets=arch_targets, packages=' '.join(packages),
           libs_list=' '.join(libs_list), multiarch=multiarch,
           generator=generator)
    f = open('Makefile', 'w')
    f.write(makefile)
    f.close()
    warning(platforms)


def main(argv=None):
    if argv is None:
        argv = sys.argv
    argparser = argparse.ArgumentParser(
        description="Prepare build of Linphone and its dependencies.")
    argparser.add_argument(
        '-c', '-C', '--clean', help="Clean a previous build instead of preparing a build.", action='store_true')
    argparser.add_argument(
        '-d', '--debug', help="Prepare a debug build, eg. add debug symbols and use no optimizations.", action='store_true')
    argparser.add_argument(
        '-dv', '--debug-verbose', help="Activate ms_debug logs.", action='store_true')
    argparser.add_argument(
        '-f', '--force', help="Force preparation, even if working directory already exist.", action='store_true')
    argparser.add_argument(
        '-G' '--generator', help="CMake build system generator (default: Unix Makefiles).", default='Unix Makefiles', choices=['Unix Makefiles', 'Ninja'])
    argparser.add_argument(
        '-L', '--list-cmake-variables', help="List non-advanced CMake cache variables.", action='store_true', dest='list_cmake_variables')
    argparser.add_argument(
        '-t', '--tunnel', help="Enable Tunnel.", action='store_true')
    argparser.add_argument('platform', nargs='*', action=PlatformListAction, default=[
                           'x86_64', 'devices'], help="The platform to build for (default is 'x86_64 devices'). Space separated architectures in list: {0}.".format(', '.join([repr(platform) for platform in platforms])))

    args, additional_args = argparser.parse_known_args()

    if args.debug_verbose:
        additional_args += ["-DENABLE_DEBUG_LOGS=YES"]

    if check_tools() != 0:
        return 1

    install_git_hook()

    additional_args += ["-G", args.G__generator]
    if args.G__generator == 'Ninja':
        if not check_is_installed("ninja", "it"):
            return 1
        generator = 'ninja -C'
    else:
        generator = '$(MAKE) -C'

    if args.tunnel:
        if not os.path.isdir("submodules/tunnel"):
            print("Tunnel enabled but not found, trying to clone it...")
            if check_is_installed("git", "it", True):
                Popen("git clone gitosis@git.linphone.org:tunnel.git submodules/tunnel".split(" ")).wait()
            else:
                return 1
        additional_args += ["-DENABLE_TUNNEL=YES"]

    selected_platforms = []
    for platform in args.platform:
        if platform == 'all':
            selected_platforms += archs_device + archs_simu
        elif platform == 'devices':
            selected_platforms += archs_device
        elif platform == 'simulators':
            selected_platforms += archs_simu
        else:
            selected_platforms += [platform]
    selected_platforms = list(set(selected_platforms))

    for platform in selected_platforms:
        target = targets[platform]

        if args.clean:
            target.clean()
        else:
            retcode = prepare.run(target, args.debug, False, args.list_cmake_variables, args.force, additional_args)
            if retcode != 0:
                if retcode == 51:
                    Popen("make help-prepare-options".split(" "))
                    retcode = 0
                return retcode

    if args.clean:
        if os.path.isfile('Makefile'):
            os.remove('Makefile')
    elif selected_platforms:
        generate_makefile(selected_platforms, generator)

    return 0

if __name__ == "__main__":
    sys.exit(main())
