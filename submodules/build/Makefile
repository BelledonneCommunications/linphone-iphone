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
enable_gpl=yes

.NOTPARALLEL all: build warning
ifeq ($(enable_gpl),yes)
warning:
	@echo 
	@echo "*****************************************************************"
	@echo "*****************************************************************"
	@echo "*****CAUTION, this liblinphone SDK is built using GPL code ******"
	@echo "*****To disable gpl code, use make enable_gpl=no    *************"
	@echo "*****************************************************************"
	@echo "*****************************************************************"
else
warning:
	@echo 
	@echo "*****************************************************************"
	@echo "*****************************************************************"
	@echo "*****linphone SDK without GPL code                         ******"
	@echo "*****************************************************************"
	@echo "*****************************************************************"
endif

build:
	make -f  builder-iphone-os.mk  all enable_gpl=$(enable_gpl) \
	&& make -f  builder-iphone-simulator.mk all enable_gpl=$(enable_gpl)\
	&& make -f  builder-iphone-os.mk   host=armv7-apple-darwin all enable_gpl=$(enable_gpl)\
	&& make -f builder-iphone-os.mk delivery-sdk 
clean:
	make -f  builder-iphone-simulator.mk  clean \
	&&  make -f  builder-iphone-os.mk  clean \
	&&  make -f  builder-iphone-os.mk   host=armv7-apple-darwin clean
clean-makefile:
	make -f  builder-iphone-simulator.mk  clean-makefile \
	&&  make -f  builder-iphone-os.mk  clean-makefile \
	&&  make -f  builder-iphone-os.mk   host=armv7-apple-darwin clean-makefile

veryclean:
	make -f  builder-iphone-simulator.mk  veryclean \
	&&  make -f  builder-iphone-os.mk  veryclean \
	&&  make -f  builder-iphone-os.mk  host=armv7-apple-darwin veryclean 
