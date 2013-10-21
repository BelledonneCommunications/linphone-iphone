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
enable_zrtp=no
enable_ffmpeg=no
enable_opus=yes
enable_debug=no

.NOTPARALLEL all: check_options build warning

# check that the selected options are correct
CHECKOPT_MSG := ""

ifeq ($(enable_gpl_third_parties)$(enable_zrtp),noyes)
	CHECKOPT_MSG += "ZRTP is not available in non-gpl build.\n"
endif

ifeq ($(enable_gpl_third_parties)$(enable_ffmpeg),noyes)
# ffmpeg is not compatible with no GPL.
	enable_ffmpeg:=no
endif

ifneq ($(CHECKOPT_MSG),"")
check_options:
	@echo $(CHECKOPT_MSG)
	@exit -1
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
                 enable_debug=$(enable_debug) enable_ffmpeg=$(enable_ffmpeg)

build:
	make -f  builder-iphone-simulator.mk $(LINPHONE_OPTIONS) all  \
	&& make -f  builder-iphone-os.mk  $(LINPHONE_OPTIONS) all  \
	&& make -f  builder-iphone-os.mk  host=armv7s-apple-darwin $(LINPHONE_OPTIONS) all  \
	&& make -f builder-iphone-os.mk delivery-sdk 

ipa: build
	make -f  builder-iphone-simulator.mk  ipa \
	&&  make -f  builder-iphone-os.mk  ipa \
	&&  make -f  builder-iphone-os.mk  host=armv7s-apple-darwin ipa \

clean:
	make -f  builder-iphone-simulator.mk  clean \
	&&  make -f  builder-iphone-os.mk  clean \
	&&  make -f  builder-iphone-os.mk  host=armv7s-apple-darwin clean \

clean-makefile:
	make -f  builder-iphone-simulator.mk  clean-makefile \
	&&  make -f  builder-iphone-os.mk  clean-makefile \
	&&  make -f  builder-iphone-os.mk  host=armv7s-apple-darwin clean-makefile \

veryclean:
	make -f  builder-iphone-simulator.mk  veryclean \
	&&  make -f  builder-iphone-os.mk  veryclean \
	&&  make -f  builder-iphone-os.mk  host=armv7s-apple-darwin veryclean

