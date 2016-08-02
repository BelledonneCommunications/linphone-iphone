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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
############################################################################

import os
import re
import shutil
import sys
from distutils.spawn import find_executable
from logging import error, warning, info
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
	self.external_source_path = os.path.join(current_path, 'submodules')


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



ios_targets = {
    'i386': IOSi386Target(),
    'x86_64': IOSx8664Target(),
    'armv7': IOSarmv7Target(),
    'arm64': IOSarm64Target()
}

ios_virtual_targets = {
    'devices': ['armv7', 'arm64'],
    'simulators': ['i386', 'x86_64'],
    'all': ['i386', 'x86_64', 'armv7', 'arm64']
}

class IOSPreparator(prepare.Preparator):

    def __init__(self, targets=ios_targets, virtual_targets=ios_virtual_targets):
        prepare.Preparator.__init__(self, targets, default_targets=['armv7', 'arm64', 'x86_64'], virtual_targets=virtual_targets)
        self.veryclean = True
        self.show_gpl_disclaimer = True
        self.argparser.add_argument('-ac', '--all-codecs', help="Enable all codecs, including the non-free ones. Final application must comply with their respective license (see README.md).", action='store_true')

    def parse_args(self):
        prepare.Preparator.parse_args(self)

        self.additional_args += ["-DLINPHONE_IOS_DEPLOYMENT_TARGET=" + self.extract_deployment_target()]
        self.additional_args += ["-DLINPHONE_BUILDER_DUMMY_LIBRARIES=" + ' '.join(self.extract_libs_list())]
        if self.args.all_codecs:
            self.additional_args += ["-DENABLE_GPL_THIRD_PARTIES=ON"]
            self.additional_args += ["-DENABLE_NON_FREE_CODECS=ON"]
            self.additional_args += ["-DENABLE_AMRNB=ON"]
            self.additional_args += ["-DENABLE_AMRWB=ON"]
            self.additional_args += ["-DENABLE_BV16=ON"]
            self.additional_args += ["-DENABLE_G729=ON"]
            self.additional_args += ["-DENABLE_GSM=ON"]
            self.additional_args += ["-DENABLE_ILBC=ON"]
            self.additional_args += ["-DENABLE_ISAC=ON"]
            self.additional_args += ["-DENABLE_OPUS=ON"]
            self.additional_args += ["-DENABLE_SILK=ON"]
            self.additional_args += ["-DENABLE_SPEEX=ON"]
            self.additional_args += ["-DENABLE_FFMPEG=ON"]
            self.additional_args += ["-DENABLE_H263=ON"]
            self.additional_args += ["-DENABLE_H263P=ON"]
            self.additional_args += ["-DENABLE_MPEG4=ON"]
            self.additional_args += ["-DENABLE_OPENH264=ON"]
            self.additional_args += ["-DENABLE_VPX=ON"]
            self.additional_args += ["-DENABLE_X264=ON"]
            self.additional_args += ["-DENABLE_CODEC2=ON"]

    def clean(self):
        prepare.Preparator.clean(self)
        if os.path.isfile('Makefile'):
            os.remove('Makefile')
        if os.path.isdir('WORK') and not os.listdir('WORK'):
            os.rmdir('WORK')
        if os.path.isdir('liblinphone-sdk'):
            l = os.listdir('liblinphone-sdk')
            if len(l) == 1 and l[0] == 'apple-darwin':
                shutil.rmtree('liblinphone-sdk', ignore_errors=False)

    def extract_from_xcode_project_with_regex(self, regex):
        l = []
        f = open('linphone.xcodeproj/project.pbxproj', 'r')
        lines = f.readlines()
        f.close()
        for line in lines:
            m = regex.search(line)
            if m is not None:
                l += [m.group(1)]
        return list(set(l))

    def extract_deployment_target(self):
        regex = re.compile("IPHONEOS_DEPLOYMENT_TARGET = (.*);")
        return self.extract_from_xcode_project_with_regex(regex)[0]

    def extract_libs_list(self):
        # name = libspeexdsp.a; path = "liblinphone-sdk/apple-darwin/lib/libspeexdsp.a"; sourceTree = "<group>"; };
        regex = re.compile("name = \"*(lib\S+)\.a(\")*; path = \"liblinphone-sdk/apple-darwin/")
        return self.extract_from_xcode_project_with_regex(regex)

    def detect_package_manager(self):
        if find_executable("brew"):
            return "brew"
        elif find_executable("port"):
            return "sudo port"
        else:
            error("No package manager found. Please README or install brew using:\n\truby -e \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)\"")
            return "brew"

    def check_environment(self):
        reterr = 0
        reterr |= prepare.Preparator.check_environment(self)
        package_manager_info = {"brew-pkg-config": "pkg-config",
                                "sudo port-pkg-config": "pkgconfig",
                                "brew-binary-path": "/usr/local/bin/",
                                "sudo port-binary-path": "/opt/local/bin/"
                                }

        for prog in ["autoconf", "automake", "doxygen", "java", "nasm", "cmake", "wget", "yasm", "optipng"]:
            reterr |= not self.check_is_installed(prog, prog)

        reterr |= not self.check_is_installed("pkg-config", package_manager_info[self.detect_package_manager() + "-pkg-config"])
        reterr |= not self.check_is_installed("ginstall", "coreutils")
        reterr |= not self.check_is_installed("intltoolize", "intltool")
        reterr |= not self.check_is_installed("convert", "imagemagick")

        if find_executable("nasm"):
            nasm_output = Popen("nasm -f elf32".split(" "), stderr=PIPE, stdout=PIPE).stderr.read()
            if "fatal: unrecognised output format" in nasm_output:
                error("Invalid version of nasm detected. Please make sure that you are NOT using Apple's binary here")
                self.missing_dependencies["nasm"] = "nasm"
                reterr = 1

        if self.check_is_installed("libtoolize", "libtoolize", warn=False):
            if not self.check_is_installed("glibtoolize", "libtool"):
                reterr = 1
                glibtoolize_path = find_executable("glibtoolize")
                if glibtoolize_path is not None:
                    msg = "Please do a symbolic link from glibtoolize to libtoolize:\n\tln -s {} ${}"
                    error(msg.format(glibtoolize_path, glibtoolize_path.replace("glibtoolize", "libtoolize")))

        devnull = open(os.devnull, 'wb')
        # just ensure that JDK is installed - if not, it will automatically display a popup to user
        p = Popen("java -version".split(" "), stderr=devnull, stdout=devnull)
        p.wait()
        if p.returncode != 0:
            error("Please install Java JDK (not just JRE).")
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
                if not find_executable("strings"):
                    sdk_strings_path = Popen("xcrun --find strings".split(" "), stdout=PIPE).stdout.read().split("\n")[0]
                    error("strings binary missing, please run:\n\tsudo ln -s {} {}".format(sdk_strings_path, package_manager_info[detect_package_manager() + "-binary-path"]))
                    reterr = 1
        return reterr

    def show_missing_dependencies(self):
        if self.missing_dependencies:
            error("The following binaries are missing: {}. Please install them using:\n\t{} install {}".format(
                " ".join(self.missing_dependencies.keys()),
                self.detect_package_manager(),
                " ".join(self.missing_dependencies.values())))

    def install_git_hook(self):
        git_hook_path = ".git{sep}hooks{sep}pre-commit".format(sep=os.sep)
        if os.path.isdir(".git{sep}hooks".format(sep=os.sep)) and not os.path.isfile(git_hook_path):
            info("Installing Git pre-commit hook")
            shutil.copyfile(".git-pre-commit", git_hook_path)
            os.chmod(git_hook_path, 0755)

    def generate_makefile(self, generator, project_file=''):
        platforms = self.args.target
        arch_targets = ""
        for arch in platforms:
            arch_targets += """
{arch}: {arch}-build

{arch}-build:
\t{generator} WORK/ios-{arch}/cmake/{project_file}
\t@echo "Done"
""".format(arch=arch, generator=generator, project_file=project_file)
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
\tarchives=`find liblinphone-sdk/{first_arch}-apple-darwin.ios -name '*.a'` && \\
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



def main():
    preparator = IOSPreparator()
    preparator.parse_args()
    if preparator.check_environment() != 0:
        preparator.show_environment_errors()
        return 1
    preparator.install_git_hook()
    return preparator.run()

if __name__ == "__main__":
    sys.exit(main())

