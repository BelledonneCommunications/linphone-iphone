############################################################################
# zrtp.mk
# Copyright (C) 2014  Belledonne Communications,Grenoble France
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
bzrtp_dir?=bzrtp
enable_zrtp?=yes

$(BUILDER_SRC_DIR)/$(bzrtp_dir)/configure:
	@echo -e "\033[01;32m Running autogen for bzrtp in $(BUILDER_SRC_DIR)/$(bzrtp_dir) \033[0m"
	cd $(BUILDER_SRC_DIR)/$(bzrtp_dir) && ./autogen.sh

$(BUILDER_BUILD_DIR)/$(bzrtp_dir)/Makefile: $(BUILDER_SRC_DIR)/$(bzrtp_dir)/configure
	@echo -e "\033[01;32m Running configure in $(BUILDER_BUILD_DIR)/$(bzrtp_dir) \033[0m"
	mkdir -p $(BUILDER_BUILD_DIR)/$(bzrtp_dir)
	cd $(BUILDER_BUILD_DIR)/$(bzrtp_dir)/ \
		&& PKG_CONFIG_LIBDIR=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
		$(BUILDER_SRC_DIR)/$(bzrtp_dir)/configure -prefix=$(prefix) --host=$(host) ${library_mode} \
		--enable-static

ifeq ($(enable_zrtp),yes)

build-bzrtp: $(BUILDER_BUILD_DIR)/$(bzrtp_dir)/Makefile
	@echo -e "\033[01;32m building bzrtp \033[0m"
	cd $(BUILDER_BUILD_DIR)/$(bzrtp_dir) \
		&& PKG_CONFIG_LIBDIR=$(prefix)/lib/pkgconfig \
		CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
		make -j1 && make install

else
build-bzrtp:
	@echo "ZRTP is disabled"

endif

clean-bzrtp:
	-cd  $(BUILDER_BUILD_DIR)/$(bzrtp_dir) && make clean

veryclean-bzrtp:
	-cd $(BUILDER_BUILD_DIR)/$(bzrtp_dir) && make distclean
	rm -f $(BUILDER_SRC_DIR)/$(bzrtp_dir)/configure

clean-makefile-bzrtp:
	-cd $(BUILDER_BUILD_DIR)/$(bzrtp_dir) && rm -f Makefile
