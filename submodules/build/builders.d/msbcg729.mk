############################################################################
# msbcg729.mk 
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
msbcg729_dir?=bcg729
enable_bcg729?=yes

$(BUILDER_SRC_DIR)/$(msbcg729_dir)/configure:
	@echo -e "\033[01;32m Running autogen for msbcg729 in $(BUILDER_SRC_DIR)/$(msbcg729_dir) \033[0m"
	cd $(BUILDER_SRC_DIR)/$(msbcg729_dir) && ./autogen.sh

$(BUILDER_BUILD_DIR)/$(msbcg729_dir)/Makefile: $(BUILDER_SRC_DIR)/$(msbcg729_dir)/configure
	@echo -e "\033[01;32m Running configure in $(BUILDER_BUILD_DIR)/$(msbcg729_dir) \033[0m"
	mkdir -p $(BUILDER_BUILD_DIR)/$(msbcg729_dir)
	cd $(BUILDER_BUILD_DIR)/$(msbcg729_dir)/ \
		&& PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
		$(BUILDER_SRC_DIR)/$(msbcg729_dir)/configure -prefix=$(prefix) --host=$(host) ${library_mode} \
		--enable-static

ifeq ($(enable_bcg729),yes)

build-msbcg729: $(BUILDER_BUILD_DIR)/$(msbcg729_dir)/Makefile
	@echo -e "\033[01;32m building bcg729 \033[0m"
	cd $(BUILDER_BUILD_DIR)/$(msbcg729_dir) \
		&& PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig \
		CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
		make -j1 && make install


else
build-msbcg729:
	@echo "G729 is disabled"

endif

clean-msbcg729: 
	-cd  $(BUILDER_BUILD_DIR)/$(msbcg729_dir) && make clean

veryclean-msbcg729: 
	-cd $(BUILDER_BUILD_DIR)/$(msbcg729_dir) && make distclean 
	rm -f $(BUILDER_SRC_DIR)/$(msbcg729_dir)/configure

clean-makefile-msbcg729: 
	-cd $(BUILDER_BUILD_DIR)/$(msbcg729_dir) && rm -f Makefile
