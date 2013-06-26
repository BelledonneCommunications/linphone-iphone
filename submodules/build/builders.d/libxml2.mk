############################################################################
# libxml2.mk 
# Copyright (C) 2013  Belledonne Communications,Grenoble France
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
libxml2_dir?=externals/libxml2

libxml2_configure_options= \
	--enable-static --disable-shared \
	--disable-rebuild-docs --with-iconv=no --with-python=no --with-zlib=no

$(BUILDER_SRC_DIR)/$(libxml2_dir)/patched.stamp:
	cd $(BUILDER_SRC_DIR)/$(libxml2_dir) \
	&& git apply $(BUILDER_SRC_DIR)/build/builders.d/libxml2.patch \
	&& touch $@

$(BUILDER_SRC_DIR)/$(libxml2_dir)/configure: $(BUILDER_SRC_DIR)/$(libxml2_dir)/patched.stamp
	@echo -e "\033[01;32m Running autogen for libxml2 in $(BUILDER_SRC_DIR)/$(libxml2_dir) \033[0m"
	cd $(BUILDER_SRC_DIR)/$(libxml2_dir) \
	&& PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) NOCONFIGURE=1 \
	$(BUILDER_SRC_DIR)/$(libxml2_dir)/autogen.sh -prefix=$(prefix) --host=$(host) ${library_mode}

$(BUILDER_BUILD_DIR)/$(libxml2_dir)/Makefile: $(BUILDER_SRC_DIR)/$(libxml2_dir)/configure
	@echo -e "\033[01;32m Running configure in $(BUILDER_BUILD_DIR)/$(libxml2_dir) \033[0m"
	mkdir -p $(BUILDER_BUILD_DIR)/$(libxml2_dir)
	cd $(BUILDER_BUILD_DIR)/$(libxml2_dir) \
	&& PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
	$(BUILDER_SRC_DIR)/$(libxml2_dir)/configure -prefix=$(prefix) --host=$(host) ${library_mode} $(libxml2_configure_options)

build-libxml2: $(BUILDER_BUILD_DIR)/$(libxml2_dir)/Makefile
	@echo -e "\033[01;32m building libxml2 \033[0m"
	cd $(BUILDER_BUILD_DIR)/$(libxml2_dir) \
	&& PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
	make && make install

clean-libxml2: 
	-cd  $(BUILDER_BUILD_DIR)/$(libxml2_dir) && make clean

veryclean-libxml2: 
	-cd $(BUILDER_BUILD_DIR)/$(libxml2_dir) && make distclean 
	rm -f $(BUILDER_SRC_DIR)/$(libxml2_dir)/configure
	cd $(BUILDER_SRC_DIR)/$(libxml2_dir) \
	&& git checkout configure.in \
	&& rm -f patched.stamp 

clean-makefile-libxml2: 
	-cd $(BUILDER_BUILD_DIR)/$(libxml2_dir) && rm -f Makefile
