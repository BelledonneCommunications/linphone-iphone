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

TUNNEL_AVAILABLE=$(shell git submodule status ../tunnel)

ifneq ($(TUNNEL_AVAILABLE),)
	enable_tunnel=yes
	enable_gpl_third_parties=no
else
	enable_tunnel=no
endif


.NOTPARALLEL all: check_options build warning

# check that the selected options are correct
CHECKOPT_MSG := ""

ifeq ($(enable_gpl_third_parties)$(enable_ffmpeg),noyes)
# ffmpeg is not compatible with no GPL.
	enable_ffmpeg:=no
endif

ifneq ($(CHECKOPT_MSG),"")
check_options:
	@echo $(CHECKOPT_MSG)
else
check_options:
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

build-% clean-% veryclean-%: 
	make -f builder-iphone-simulator.mk $(LINPHONE_OPTIONS) $@ \
	&& make -f builder-iphone-os.mk $(LINPHONE_OPTIONS) $@ \
	&& make -f builder-iphone-os.mk host=armv7s-apple-darwin $(LINPHONE_OPTIONS) $@

# sends the target after 'broadcast_' to all sub-architectures (armv7, armv7s, simulator)
broadcast_%:
	@echo "Broadcasting target '$*' to all sub-architectures"
	make -f builder-iphone-simulator.mk $(LINPHONE_OPTIONS) $* \
	&& make -f builder-iphone-os.mk $(LINPHONE_OPTIONS) $* \
	&& make -f builder-iphone-os.mk host=armv7s-apple-darwin $(LINPHONE_OPTIONS) $*

sdk: 
	make -f builder-iphone-os.mk delivery-sdk 

build: broadcast_all sdk
	make -f builder-iphone-os.mk delivery-sdk 

ipa: build broadcast_ipa

clean: broadcast_clean

veryclean: broadcast_veryclean


zipres:
	@tar  -C ../.. -czf ../../ios_assets.tar.gz Resources iTunesArtwork 
	@echo Archive 'ios_assets.tar.gz' placed in root directory

