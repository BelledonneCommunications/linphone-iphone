############################################################################
# openh264.mk 
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

ifneq (,$(findstring i386-,$(host)))
	ARCH=i386
endif
ifneq (,$(findstring armv7-,$(host)))
	ARCH=armv7
endif
ifneq (,$(findstring aarch64-,$(host)))
	ARCH=arm64
endif
ifneq (,$(findstring x86_64-,$(host)))
	ARCH=x86_64
endif

ifeq (,$(ARCH))
$(error Undefined arch for openh264)
endif

openh264_dir?=externals/openh264

$(BUILDER_SRC_DIR)/$(openh264_dir)/openh264-permissive.patch.stamp:
	cd $(BUILDER_SRC_DIR)/$(openh264_dir) \
	&& patch -p1 < $(BUILDER_SRC_DIR)/build/builders.d/openh264-permissive.patch
	touch $(BUILDER_SRC_DIR)/$(openh264_dir)/openh264-permissive.patch.stamp

patch-openh264: $(BUILDER_SRC_DIR)/$(openh264_dir)/openh264-permissive.patch.stamp 

#update 03/2015; with openh264 v1.4.0 patches seems no longer useful.

update-openh264:
	mkdir -p $(BUILDER_BUILD_DIR)/$(openh264_dir) \
	&& cd $(BUILDER_BUILD_DIR)/$(openh264_dir)/ \
	&& rsync -rvLpgoc --exclude ".git"  $(BUILDER_SRC_DIR)/$(openh264_dir)/* .

make-target-%:
	cd $(BUILDER_BUILD_DIR)/$(openh264_dir) \
	&& echo ===== OpenH264: make $* ===== \
	&& make CC="xcrun clang" CXX="xcrun clang++" AR="xcrun ar" LD="xcrun clang" RANLIB="xcrun ranlib" OS=ios ARCH=$(ARCH) PREFIX=$(prefix) $*

build-openh264: update-openh264 make-target-libraries make-target-install

clean-openh264:
	cd $(BUILDER_BUILD_DIR)/$(openh264_dir) \
	&& make clean OS=ios ARCH=$(ARCH)

veryclean-openh264:
	cd $(BUILDER_SRC_DIR)/$(openh264_dir)/ \
	&& git clean -f && git reset --hard \
	&& rm -rf $(BUILDER_BUILD_DIR)/$(openh264_dir)

