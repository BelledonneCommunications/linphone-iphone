############################################################################
# silk.mk 
# Copyright (C) 2011  Belledonne Communications,Grenoble France
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

SILK_BUILD_DIR?=$(BUILDER_BUILD_DIR)/externals/silk

ifneq (,$(findstring i386,$(host)))
        make_options := TARGET_MTUNE=i386 TARGET_ARCH=i386 
endif
ifneq (,$(findstring armv6,$(host)))
        make_options := TARGET_ARCH="-arch armv6" 
endif
ifneq (,$(findstring armv7,$(host)))
        make_options := USE_NEON=yes TARGET_ARCH="armv7 -mno-thumb" 
endif

$(SILK_BUILD_DIR)/Makefile:
	mkdir -p $(BUILDER_BUILD_DIR)/externals \
	&& cd $(BUILDER_BUILD_DIR)/externals \
	&& rm -rf silk \
	&& wget http://developer.skype.com/silk/SILK_SDK_SRC_v1.0.8.zip \
	&& unzip  SILK_SDK_SRC_v1.0.8.zip \
	&& rm -f SILK_SDK_SRC_v1.0.8.zip \
	&& mv SILK_SDK_SRC_v1.0.8/SILK_SDK_SRC_ARM_v1.0.8 silk  \
	&& rm -rf SILK_SDK_SRC_v1.0.8

build-silk: $(SILK_BUILD_DIR)/Makefile
	cd $(SILK_BUILD_DIR) &&  host_alias=${host} . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& make all TOOLCHAIN_PREFIX=$$SDK_BIN_PATH/ $(make_options)   ADDED_DEFINES+=IPHONE \
	&& mkdir -p $(prefix)/include/silk \
	&& cp -f $(SILK_BUILD_DIR)/interface/*  $(prefix)/include/silk \
	&& cp -f lib*.a  $(prefix)/lib 

clean-silk:
	cd  $(SILK_BUILD_DIR)  && make clean

clean-makefile-silk:
	
veryclean-silk:
	rm -rf $(SILK_BUILD_DIR)

