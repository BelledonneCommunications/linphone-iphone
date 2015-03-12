############################################################################
# Makefile
# Copyright (C) 2009  Belledonne Communications,Grenoble France
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
enable_gpl_third_parties=yes
enable_zrtp=yes
enable_ffmpeg=yes
enable_opus=yes
enable_debug=no
enable_i386=no


TUNNEL_AVAILABLE=$(shell git submodule status ../tunnel 2>/dev/null 1>/dev/null && echo yes)
ifneq ($(TUNNEL_AVAILABLE),)
	enable_tunnel=yes
	enable_gpl_third_parties=no
else
	enable_tunnel=no
endif


.NOTPARALLEL all: checks build warning

# check that the selected options are correct
CHECKOPT_MSG := ""

ifeq ($(enable_gpl_third_parties)$(enable_ffmpeg),noyes)
# ffmpeg is not compatible with no GPL.
	enable_ffmpeg:=no
endif

checks: check_options check_progs

ifneq ($(CHECKOPT_MSG),"")
check_options:
	@echo $(CHECKOPT_MSG)
else
check_options:
endif

# you can skip the tool check by export BYPASS_TOOLCHECK=1
ifneq ($(BYPASS_TOOLCHECK),1)

# Checks
CHECK_MSG=$(shell ../../Tools/check_tools.sh)

check_progs:
    ifneq ($(CHECK_MSG),)
	$(error Some tools are missing.)
    else
	$(info All tools are present.)
    endif

else
check_progs:
	$(info Skipping tool checks)
endif


# setup footer

ifeq ($(enable_gpl_third_parties),yes)
warning:
	@echo
	@echo "***************************************************************************"
	@echo "***************************************************************************"
	@echo "*****CAUTION, this liblinphone SDK is built using 3rd party GPL code ******"
	@echo "*****Even if you acquired a proprietary license from Belledonne      ******"
	@echo "*****Communications, this SDK is GPL and GPL only.                   ******"
	@echo "*****To disable 3rd party gpl code, please use:                      ******"
	@echo "*****$ make enable_gpl_third_parties=no                              ******"
	@echo "***************************************************************************"
	@echo "***************************************************************************"
else
warning:
	@echo
	@echo "*****************************************************************"
	@echo "*****************************************************************"
	@echo "*****Linphone SDK without 3rd party GPL software           ******"
	@echo "*****If you acquired a proprietary license from Belledonne ******"
	@echo "*****Communications, this SDK can be used to create        ******"
	@echo "*****a proprietary linphone-based application.             ******"
	@echo "*****************************************************************"
	@echo "*****************************************************************"
endif

LINPHONE_OPTIONS=enable_gpl_third_parties=$(enable_gpl_third_parties) \
                 enable_zrtp=$(enable_zrtp)   enable_opus=$(enable_opus) \
                 enable_debug=$(enable_debug) enable_ffmpeg=$(enable_ffmpeg) enable_tunnel=$(enable_tunnel)

armv7_host=armv7-apple-darwin.ios
arm64_host=aarch64-apple-darwin.ios
i386_host=i386-apple-darwin.ios
x86_64_host=x86_64-apple-darwin.ios

all_archs=armv7 aarch64 i386 x86_64
active_arch=$(filter-out i386,$(all_archs))
ifeq ($(enable_i386),yes)
active_arch+=i386
endif

all_hosts=$(addsuffix -apple-darwin.ios,$(active_arch))

armv7-%:
	make -f builder-iphone-os.mk host=$(armv7_host) $(LINPHONE_OPTIONS) $*

arm64-%:
	make -f builder-iphone-os.mk host=$(arm64_host) $(LINPHONE_OPTIONS) $*

i386-%:
	make -f builder-iphone-os.mk host=$(i386_host) $(LINPHONE_OPTIONS) $*

x86_64-%:
	make -f builder-iphone-os.mk host=$(x86_64_host) $(LINPHONE_OPTIONS) $*


# sends the target after 'broadcast_' to all sub-architectures 
broadcast_%:
	@echo "Broadcasting target '$*' to $(all_hosts)"
	@for h in $(all_hosts); do \
		echo "==== starting build for host $$h ===="; \
		make -f builder-iphone-os.mk host=$$h $(LINPHONE_OPTIONS) $* || exit 1 ; \
	done

build-% clean-% veryclean-%:
	@for h in $(all_hosts); do \
		echo "==== starting build of $@ for host $$h ===="; \
		make -f builder-iphone-os.mk host=$$h $(LINPHONE_OPTIONS) $@ || exit 1 ; \
	done

sdk:
	make -f builder-iphone-os.mk delivery-sdk

libs:
	make -f builder-iphone-os.mk multi-arch

download-sdk:
	make -f builder-iphone-os.mk download-sdk

build: broadcast_all libs

ipa: build broadcast_ipa

clean: broadcast_clean

veryclean: broadcast_veryclean

list-packages:
	@make -f builder-iphone-os.mk list-packages

pull-transifex:
	cd ../../ && tx pull -af

push-transifex:
	cd ../../ && ./Tools/generate_strings_files.sh && tx push -s -t -f --no-interactive

zipres:
	@tar  -C ../.. -czf ../../ios_assets.tar.gz Resources iTunesArtwork
	@echo Archive 'ios_assets.tar.gz' placed in root directory

empty:=
space:= $(empty) $(empty)
arch_opt=$(subst $(space),|,$(all_archs))

help:
	@echo "(please read the README.md file first)"
	@echo "Available architectures: $(all_archs)"
	@echo "Activated architectures: $(active_arch)"
	@echo ""
	@echo "Invoke with 'enable_i386=yes' to add i386 build"
	@echo ""
	@echo "Available targets:"
	@echo ""
	@echo "   * all       : builds all architectures and creates the liblinphone sdk"
	@echo "   * ipa       : builds all, creates the SDK, then builds an IPA of the Linphone-iphone application"
	@echo "   * clean     : clean all packages for all architectures"
	@echo "   * veryclean : completely clean all packages for all architectures"
	@echo "   * zipres    : creates a tar.gz file with all the resources (images)"
	@echo ""
	@echo "=== Advanced usage ==="
	@echo ""
	@echo "   *              list-packages : lists the packages that you can build"
	@echo ""
	@echo "   *            build-[package] : builds the package for all architectures"
	@echo "   *            clean-[package] : clean the package for all architectures"
	@echo "   *        veryclean-[package] : completely clean the package for all architectures"
	@echo ""
	@echo "   *     [$(arch_opt)]-build-[package] : builds a package for the selected architecture"
	@echo "   *     [$(arch_opt)]-clean-[package] : clean the package for the selected architecture"
	@echo "   * [$(arch_opt)]-veryclean-[package] : completely clean the package for the selected architecture"
	@echo ""
	@echo "   * sdk  : re-add all generated libraries to the SDK. Use this only after a full build."
	@echo "   * libs : after a rebuild of a subpackage, will mix the new libs in liblinphone-sdk/apple-darwin directory"
