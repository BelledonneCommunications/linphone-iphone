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
import tempfile
import sys
from logging import error, warning, info, INFO, basicConfig
from distutils.spawn import find_executable
from subprocess import Popen, PIPE
sys.dont_write_bytecode = True
sys.path.insert(0, 'submodules/cmake-builder')
try:
    import prepare
except Exception as e:
    error(
        "Could not find prepare module: {}, probably missing submodules/cmake-builder? Try running:\n"
        "git submodule sync && git submodule update --init --recursive".format(e))
    exit(1)


class IOSTarget(prepare.Target):

    def __init__(self, arch):
        prepare.Target.__init__(self, 'ios-' + arch)
        current_path = os.path.dirname(os.path.realpath(__file__))
        self.config_file = 'configs/config-ios-' + arch + '.cmake'
        self.toolchain_file = 'toolchains/toolchain-ios-' + arch + '.cmake'
        self.output = 'liblinphone-sdk/' + arch + '-apple-darwin.ios'
        self.additional_args = [
            '-DCMAKE_INSTALL_MESSAGE=LAZY',
            '-DLINPHONE_BUILDER_GROUP_EXTERNAL_SOURCE_PATH_BUILDERS=YES',
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


def gpl_disclaimer(platforms):
    cmakecache = 'WORK/ios-{arch}/cmake/CMakeCache.txt'.format(arch=platforms[0])
    gpl_third_parties_enabled = "ENABLE_GPL_THIRD_PARTIES:BOOL=YES" in open(
        cmakecache).read() or "ENABLE_GPL_THIRD_PARTIES:BOOL=ON" in open(cmakecache).read()

    if gpl_third_parties_enabled:
        warning("\n***************************************************************************"
                "\n***************************************************************************"
                "\n***** CAUTION, this liblinphone SDK is built using 3rd party GPL code *****"
                "\n***** Even if you acquired a proprietary license from Belledonne      *****"
                "\n***** Communications, this SDK is GPL and GPL only.                   *****"
                "\n***** To disable 3rd party gpl code, please use:                      *****"
                "\n***** $ ./prepare.py -DENABLE_GPL_THIRD_PARTIES=NO                    *****"
                "\n***************************************************************************"
                "\n***************************************************************************")
    else:
        warning("\n***************************************************************************"
                "\n***************************************************************************"
                "\n***** Linphone SDK without 3rd party GPL software                     *****"
                "\n***** If you acquired a proprietary license from Belledonne           *****"
                "\n***** Communications, this SDK can be used to create                  *****"
                "\n***** a proprietary linphone-based application.                       *****"
                "\n***************************************************************************"
                "\n***************************************************************************")


def extract_from_xcode_project_with_regex(regex):
    l = []
    f = open('linphone.xcodeproj/project.pbxproj', 'r')
    lines = f.readlines()
    f.close()
    for line in lines:
        m = regex.search(line)
        if m is not None:
            l += [m.group(1)]
    return list(set(l))


def extract_deployment_target():
    regex = re.compile("IPHONEOS_DEPLOYMENT_TARGET = (.*);")
    return extract_from_xcode_project_with_regex(regex)[0]


def extract_libs_list():
    # name = libspeexdsp.a; path = "liblinphone-sdk/apple-darwin/lib/libspeexdsp.a"; sourceTree = "<group>"; };
    regex = re.compile("name = \"*(lib\S+)\.a(\")*; path = \"liblinphone-sdk/apple-darwin/")
    return extract_from_xcode_project_with_regex(regex)


missing_dependencies = {}


def check_is_installed(binary, prog=None, warn=True):
    if not find_executable(binary):
        if warn:
            missing_dependencies[binary] = prog
            # error("Could not find {}. Please install {}.".format(binary, prog))
        return False
    return True


def detect_package_manager():
    if find_executable("brew"):
        return "brew"
    elif find_executable("port"):
        return "sudo port"
    else:
        error(
            "No package manager found. Please README or install brew using:\n\truby -e \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)\"")
        return "brew"


def check_tools():
    package_manager_info = {"brew-pkg-config": "pkg-config",
                            "sudo port-pkg-config": "pkgconfig",
                            "brew-binary-path": "/usr/local/bin/",
                            "sudo port-binary-path": "/opt/local/bin/"
                            }
    reterr = 0

    if " " in os.path.dirname(os.path.realpath(__file__)):
        error("Invalid location: linphone-iphone path should not contain any spaces.")
        reterr = 1

    for prog in ["autoconf", "automake", "doxygen", "java", "nasm", "cmake", "wget", "yasm", "optipng"]:
        reterr |= not check_is_installed(prog, prog)

    reterr |= not check_is_installed("pkg-config", package_manager_info[detect_package_manager() + "-pkg-config"])
    reterr |= not check_is_installed("ginstall", "coreutils")
    reterr |= not check_is_installed("intltoolize", "intltool")
    reterr |= not check_is_installed("convert", "imagemagick")

    if find_executable("nasm"):
        nasm_output = Popen("nasm -f elf32".split(" "), stderr=PIPE, stdout=PIPE).stderr.read()
        if "fatal: unrecognised output format" in nasm_output:
            missing_dependencies["nasm"] = "nasm"
            reterr = 1

    if check_is_installed("libtoolize", warn=False):
        if not check_is_installed("glibtoolize", "libtool"):
            glibtoolize_path = find_executable("glibtoolize")
            reterr = 1
            msg = "Please do a symbolic link from glibtoolize to libtoolize:\n\tln -s {} ${}"
            error(msg.format(glibtoolize_path, glibtoolize_path.replace("glibtoolize", "libtoolize")))

    # list all missing packages to install
    if missing_dependencies:
        error("The following binaries are missing: {}. Please install them using:\n\t{} install {}".format(
            " ".join(missing_dependencies.keys()),
            detect_package_manager(),
            " ".join(missing_dependencies.values())))

    devnull = open(os.devnull, 'wb')
    # just ensure that JDK is installed - if not, it will automatically display a popup to user
    p = Popen("java -version".split(" "), stderr=devnull, stdout=devnull)
    p.wait()
    if p.returncode != 0:
        error("Please install Java JDK (not just JRE).")
        reterr = 1

    # needed by x264
    if not find_executable("gas-preprocessor.pl"):
        error("""Could not find gas-preprocessor.pl, please install it:
        wget --no-check-certificate https://raw.githubusercontent.com/FFmpeg/gas-preprocessor/master/gas-preprocessor.pl && \\
        chmod +x gas-preprocessor.pl && \\
        sudo mv gas-preprocessor.pl {}""".format(package_manager_info[detect_package_manager() + "-binary-path"]))
        reterr = 1

    if not os.path.isdir("submodules/linphone/mediastreamer2/src") or not os.path.isdir("submodules/linphone/oRTP/src"):
        error("Missing some git submodules. Did you run:\n\tgit submodule update --init --recursive")
        reterr = 1

    p = Popen("xcrun --sdk iphoneos --show-sdk-path".split(" "), stdout=devnull, stderr=devnull)
    p.wait()
    if p.returncode != 0:
        error("iOS SDK not found, please install Xcode from AppStore or equivalent.")
        reterr = 1
    else:
        xcode_version = int(
            Popen("xcodebuild -version".split(" "), stdout=PIPE).stdout.read().split("\n")[0].split(" ")[1].split(".")[0])
        if xcode_version < 7:
            sdk_platform_path = Popen(
                "xcrun --sdk iphonesimulator --show-sdk-platform-path".split(" "),
                stdout=PIPE, stderr=devnull).stdout.read()[:-1]
            sdk_strings_path = "{}/{}".format(sdk_platform_path, "Developer/usr/bin/strings")
            if not os.path.isfile(sdk_strings_path):
                strings_path = find_executable("strings")
                error("strings binary missing, please run:\n\tsudo ln -s {} {}".format(strings_path, sdk_strings_path))
                reterr = 1
    return reterr


def install_git_hook():
    git_hook_path = ".git{sep}hooks{sep}pre-commit".format(sep=os.sep)
    if os.path.isdir(".git{sep}hooks".format(sep=os.sep)) and not os.path.isfile(git_hook_path):
        info("Installing Git pre-commit hook")
        shutil.copyfile(".git-pre-commit", git_hook_path)
        os.chmod(git_hook_path, 0755)


def generate_makefile(platforms, generator):
    arch_targets = ""
    for arch in platforms:
        arch_targets += """
{arch}: {arch}-build

{arch}-build:
\t{generator} WORK/ios-{arch}/cmake
\t@echo "Done"
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
LINPHONE_IPHONE_VERSION=$(shell git describe --always)

.PHONY: all
.SILENT: sdk
all: build

sdk:
\tarchives=`find liblinphone-sdk/{first_arch}-apple-darwin.ios -name *.a` && \\
\trm -rf liblinphone-sdk/apple-darwin && \\
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
\t\techo "[{archs}] Mixing `basename $$archive` in $$destpath"; \\
\t\tlipo -create $$all_paths -output $$destpath; \\
\tdone; \\
\tif test -s WORK/ios-{first_arch}/Build/dummy_libraries/dummy_libraries.txt; then \\
\t\techo 'NOTE: the following libraries were STUBBED:'; \\
\t\tcat WORK/ios-{first_arch}/Build/dummy_libraries/dummy_libraries.txt; \\
\tfi

build: $(addsuffix -build, $(archs))
\t$(MAKE) sdk

ipa: build
\txcodebuild -configuration Release \\
\t&& xcrun -sdk iphoneos PackageApplication -v build/Release-iphoneos/linphone.app -o $$PWD/linphone-iphone.ipa

zipsdk: sdk
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
\ttx push -s -f --no-interactive

zipres:
\t@tar -czf ios_assets.tar.gz Resources iTunesArtwork

{arch_targets}

help-prepare-options:
\t@echo "prepare.py was previously executed with the following options:"
\t@echo "   {options}"

help: help-prepare-options
\t@echo ""
\t@echo "(please read the README.md file first)"
\t@echo ""
\t@echo "Available architectures: {archs}"
\t@echo ""
\t@echo "Available targets:"
\t@echo ""
\t@echo "   * all or build: builds all architectures and creates the liblinphone SDK"
\t@echo "   * sdk: creates the liblinphone SDK. Use this only after a full build"
\t@echo "   * zipsdk: generates a ZIP archive of liblinphone-sdk/apple-darwin containing the SDK. Use this only after SDK is built."
\t@echo "   * zipres: creates a tar.gz file with all the resources (images)"
\t@echo ""
""".format(archs=' '.join(platforms), arch_opts='|'.join(platforms),
           first_arch=platforms[0], options=' '.join(sys.argv),
           arch_targets=arch_targets,
           multiarch=multiarch, generator=generator)
    f = open('Makefile', 'w')
    f.write(makefile)
    f.close()
    gpl_disclaimer(platforms)


def list_features_with_args(debug, additional_args):
    tmpdir = tempfile.mkdtemp(prefix="linphone-iphone")
    tmptarget = IOSarm64Target()
    tmptarget.abs_cmake_dir = tmpdir

    option_regex = re.compile("ENABLE_(.*):(.*)=(.*)")
    options = {}
    ended = True
    build_type = 'Debug' if debug else 'Release'

    for line in Popen(tmptarget.cmake_command(build_type, False, True, additional_args, verbose=False),
                      cwd=tmpdir, shell=False, stdout=PIPE).stdout.readlines():
        match = option_regex.match(line)
        if match is not None:
            (name, typeof, value) = match.groups()
            options["ENABLE_{}".format(name)] = value
            ended &= (value == 'ON')
    shutil.rmtree(tmpdir)
    return (options, ended)


def list_features(debug, args):
    additional_args = args
    options = {}
    info("Searching for available features...")
    # We have to iterate multiple times to activate ALL options, so that options depending
    # of others are also listed (cmake_dependent_option macro will not output options if
    # prerequisite is not met)
    while True:
        (options, ended) = list_features_with_args(debug, additional_args)
        if ended:
            break
        else:
            additional_args = []
            # Activate ALL available options
            for k in options.keys():
                additional_args.append("-D{}=ON".format(k))

    # Now that we got the list of ALL available options, we must correct default values
    # Step 1: all options are turned off by default
    for x in options.keys():
        options[x] = 'OFF'
    # Step 2: except options enabled when running with default args
    (options_tmp, ended) = list_features_with_args(debug, args)
    final_dict = dict(options.items() + options_tmp.items())

    notice_features = "Here are available features:"
    for k, v in final_dict.items():
        notice_features += "\n\t{}={}".format(k, v)
    info(notice_features)
    info("To enable some feature, please use -DENABLE_SOMEOPTION=ON (example: -DENABLE_OPUS=ON)")
    info("Similarly, to disable some feature, please use -DENABLE_SOMEOPTION=OFF (example: -DENABLE_OPUS=OFF)")


def main(argv=None):
    basicConfig(format="%(levelname)s: %(message)s", level=INFO)

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
        '--disable-gpl-third-parties', help="Disable GPL third parties such as FFMpeg, x264.", action='store_true')
    argparser.add_argument(
        '--enable-non-free-codecs', help="Enable non-free codecs such as OpenH264, MPEG4, etc.. Final application must comply with their respective license (see README.md).", action='store_true')
    argparser.add_argument(
        '--build-all-codecs', help="Build all codecs including non-free. Final application must comply with their respective license (see README.md).", action='store_true')
    argparser.add_argument(
        '-G', '--generator', help="CMake build system generator (default: Unix Makefiles, use cmake -h to get the complete list).", default='Unix Makefiles', dest='generator')
    argparser.add_argument(
        '-lf', '--list-features', help="List optional features and their default values.", action='store_true', dest='list_features')
    argparser.add_argument(
        '-t', '--tunnel', help="Enable Tunnel.", action='store_true')
    argparser.add_argument('platform', nargs='*', action=PlatformListAction, default=[
                           'x86_64', 'devices'], help="The platform to build for (default is 'x86_64 devices'). Space separated architectures in list: {0}.".format(', '.join([repr(platform) for platform in platforms])))
    argparser.add_argument(
        '-L', '--list-cmake-variables', help="(debug) List non-advanced CMake cache variables.", action='store_true', dest='list_cmake_variables')

    args, additional_args2 = argparser.parse_known_args()

    additional_args = []

    additional_args += ["-G", args.generator]

    if check_tools() != 0:
        return 1

    additional_args += ["-DLINPHONE_IOS_DEPLOYMENT_TARGET=" + extract_deployment_target()]
    additional_args += ["-DLINPHONE_BUILDER_DUMMY_LIBRARIES=" + ' '.join(extract_libs_list())]
    if args.debug_verbose is True:
        additional_args += ["-DENABLE_DEBUG_LOGS=YES"]
    if args.enable_non_free_codecs is True:
        additional_args += ["-DENABLE_NON_FREE_CODECS=YES"]
    if args.build_all_codecs is True:
        additional_args += ["-DENABLE_GPL_THIRD_PARTIES=YES"]
        additional_args += ["-DENABLE_NON_FREE_CODECS=YES"]
        additional_args += ["-DENABLE_AMRNB=YES"]
        additional_args += ["-DENABLE_AMRWB=YES"]
        additional_args += ["-DENABLE_G729=YES"]
        additional_args += ["-DENABLE_GSM=YES"]
        additional_args += ["-DENABLE_ILBC=YES"]
        additional_args += ["-DENABLE_ISAC=YES"]
        additional_args += ["-DENABLE_OPUS=YES"]
        additional_args += ["-DENABLE_SILK=YES"]
        additional_args += ["-DENABLE_SPEEX=YES"]
        additional_args += ["-DENABLE_FFMPEG=YES"]
        additional_args += ["-DENABLE_H263=YES"]
        additional_args += ["-DENABLE_H263P=YES"]
        additional_args += ["-DENABLE_MPEG4=YES"]
        additional_args += ["-DENABLE_OPENH264=YES"]
        additional_args += ["-DENABLE_VPX=YES"]
        additional_args += ["-DENABLE_X264=YES"]
    if args.disable_gpl_third_parties is True:
        additional_args += ["-DENABLE_GPL_THIRD_PARTIES=NO"]

    if args.tunnel:
        if not os.path.isdir("submodules/tunnel"):
            info("Tunnel wanted but not found yet, trying to clone it...")
            p = Popen("git clone gitosis@git.linphone.org:tunnel.git submodules/tunnel".split(" "))
            p.wait()
            if p.returncode != 0:
                error("Could not clone tunnel. Please see http://www.belledonne-communications.com/voiptunnel.html")
                return 1
        warning("Tunnel enabled, disabling GPL third parties.")
        additional_args += ["-DENABLE_TUNNEL=ON", "-DENABLE_GPL_THIRD_PARTIES=OFF"]

    # User's options are priority upon all automatic options
    additional_args += additional_args2

    if args.list_features:
        list_features(args.debug, additional_args)
        return 0

    selected_platforms_dup = []
    for platform in args.platform:
        if platform == 'all':
            selected_platforms_dup += archs_device + archs_simu
        elif platform == 'devices':
            selected_platforms_dup += archs_device
        elif platform == 'simulators':
            selected_platforms_dup += archs_simu
        else:
            selected_platforms_dup += [platform]
    # unify platforms but keep provided order
    selected_platforms = []
    for x in selected_platforms_dup:
        if x not in selected_platforms:
            selected_platforms.append(x)

    if os.path.isdir('WORK') and not args.clean and not args.force:
        warning("Working directory WORK already exists. Please remove it (option -C or -c) before re-executing CMake "
                "to avoid conflicts between executions, or force execution (option -f) if you are aware of consequences.")
        if os.path.isfile('Makefile'):
            Popen("make help-prepare-options".split(" "))
        return 0

    for platform in selected_platforms:
        target = targets[platform]

        if args.clean:
            target.clean()
        else:
            retcode = prepare.run(target, args.debug, False, args.list_cmake_variables, args.force, additional_args)
            if retcode != 0:
                return retcode

    if args.clean:
        if os.path.isfile('Makefile'):
            os.remove('Makefile')
    elif selected_platforms:
        install_git_hook()

        # only generated makefile if we are using Ninja or Makefile
        if args.generator == 'Ninja':
            if not check_is_installed("ninja", "it"):
                return 1
            generate_makefile(selected_platforms, 'ninja -C')
        elif args.generator == "Unix Makefiles":
            generate_makefile(selected_platforms, '$(MAKE) -C')
        elif args.generator == "Xcode":
            info("You can now open Xcode project with: open WORK/cmake/Project.xcodeproj")
        else:
            info("Not generating meta-makefile for generator {}.".format(args.generator))

    return 0

if __name__ == "__main__":
    sys.exit(main())
