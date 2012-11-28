############################################################################
# x264.mk 
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

#--enable-static is mandatory otherwise the lib is not installed

x264-configure-option= \
			--host=$(host)\
			--enable-static \
			--cross-prefix=$$SDK_BIN_PATH/ \
			--extra-ldflags="-arch $$ARCH -isysroot $$SYSROOT_PATH"



ifneq (,$(findstring i386,$(host)))
       x264-configure-option+=  --extra-cflags="-arch $$ARCH -isysroot $$SYSROOT_PATH"
endif
ifneq (,$(findstring armv6,$(host)))
       x264-configure-option+=  --extra-cflags="-arch $$ARCH -mcpu=arm1176jzf-s  -marm -isysroot $$SYSROOT_PATH"
       x264-configure-option+=  --disable-asm
endif

ifneq (,$(findstring armv7,$(host)))
       x264-configure-option+=  --extra-cflags="-arch $$ARCH -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -isysroot $$SYSROOT_PATH"
endif

x264_dir?=externals/x264
#$(BUILDER_SRC_DIR)/$(x264_dir)/patched :
#	cd $(BUILDER_SRC_DIR)/$(x264_dir) \
#	&& git apply $(BUILDER_SRC_DIR)/build/builders.d/x264.patch \
#	&& touch $(BUILDER_SRC_DIR)/$(x264_dir)/patched

$(BUILDER_BUILD_DIR)/$(x264_dir)/configure: 
	mkdir -p $(BUILDER_BUILD_DIR)/$(x264_dir)
	cd $(BUILDER_BUILD_DIR)/$(x264_dir)/ \
	&& rsync -av --exclude ".git"  $(BUILDER_SRC_DIR)/$(x264_dir)/* . 

$(BUILDER_BUILD_DIR)/$(x264_dir)/config.mak: $(BUILDER_BUILD_DIR)/$(x264_dir)/configure
	cd $(BUILDER_BUILD_DIR)/$(x264_dir)/ \
	&& host_alias=$(host) . $(BUILDER_SRC_DIR)/build/$(config_site) \
	&& ./configure --prefix=$(prefix)  ${x264-configure-option} 

build-x264: $(BUILDER_BUILD_DIR)/$(x264_dir)/config.mak
	cd $(BUILDER_BUILD_DIR)/$(x264_dir)  make && make install

clean-x264:
	cd  $(BUILDER_BUILD_DIR)/$(x264_dir) && make clean

veryclean-x264:
	-cd $(BUILDER_BUILD_DIR)/$(x264_dir) && make distclean
	cd $(BUILDER_SRC_DIR)/$(x264_dir)/ \
	&& git checkout common/arm/asm.S \
	&& rm -f patched
	rm -rf $(BUILDER_BUILD_DIR)/$(x264_dir)

clean-makefile-x264:
	cd $(BUILDER_BUILD_DIR)/$(x264_dir) && rm -f config.mak
