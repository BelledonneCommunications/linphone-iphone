############################################################################
# opus.mk 
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
opus_dir?=externals/opus
enable_opus?=yes

libopus_configure_options=--disable-extra-programs --disable-doc 
ifneq (,$(findstring armv7,$(host)))
        libopus_configure_options+= --enable-fixed-point --disable-asm
endif
ifneq (,$(findstring armv7s,$(host)))
        libopus_configure_options+= --enable-fixed-point --disable-asm
endif

$(BUILDER_SRC_DIR)/$(opus_dir)/configure:
	@echo -e "\033[01;32m Running autogen for msopus in $(BUILDER_SRC_DIR)/$(opus_dir) \033[0m"
	cd $(BUILDER_SRC_DIR)/$(opus_dir) && ./autogen.sh

$(BUILDER_BUILD_DIR)/$(opus_dir)/Makefile: $(BUILDER_SRC_DIR)/$(opus_dir)/configure
	@echo -e "\033[01;32m Running configure in $(BUILDER_BUILD_DIR)/$(opus_dir) \033[0m"
	mkdir -p $(BUILDER_BUILD_DIR)/$(opus_dir)
	cd $(BUILDER_BUILD_DIR)/$(opus_dir)/ \
	&& PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
	$(BUILDER_SRC_DIR)/$(opus_dir)/configure -prefix=$(prefix) --host=$(host) ${library_mode} \
	${libopus_configure_options}

ifeq ($(enable_opus),yes)

build-opus: $(BUILDER_BUILD_DIR)/$(opus_dir)/Makefile
	@echo -e "\033[01;32m building opus \033[0m"
	cd $(BUILDER_BUILD_DIR)/$(opus_dir) \
		&& PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig \
		CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
		make && make install


else
build-opus:
	@echo "opus is disabled"

endif

clean-opus: 
	-cd  $(BUILDER_BUILD_DIR)/$(opus_dir) && make clean

veryclean-opus: 
	-cd $(BUILDER_BUILD_DIR)/$(opus_dir) && make distclean 
	rm -f $(BUILDER_SRC_DIR)/$(opus_dir)/configure

clean-makefile-opus: 
	-cd $(BUILDER_BUILD_DIR)/$(opus_dir) && rm -f Makefile
