############################################################################
# opencore-amr.mk 
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
ifneq (,$(findstring arm,$(host)))
	opencore-amr-configure-option=--enable-gcc-armv5	
endif

opencore-amr_dir?=externals/opencore-amr

$(BUILDER_SRC_DIR)/$(opencore-amr_dir)/configure: $(BUILDER_SRC_DIR)/$(opencore-amr_dir)/configure.ac
	cd $(BUILDER_SRC_DIR)/$(opencore-amr_dir) && autoreconf

$(BUILDER_BUILD_DIR)/$(opencore-amr_dir)/Makefile: $(BUILDER_SRC_DIR)/$(opencore-amr_dir)/configure
	mkdir -p $(BUILDER_BUILD_DIR)/$(opencore-amr_dir)
	cd $(BUILDER_BUILD_DIR)/$(opencore-amr_dir)/ \
	&& CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site_gcc) \
	$(BUILDER_SRC_DIR)/$(opencore-amr_dir)/configure -prefix=$(prefix) --host=$(host) ${library_mode} ${opencore-amr-configure-option} 

build-opencore-amr: $(BUILDER_BUILD_DIR)/$(opencore-amr_dir)/Makefile
	cd $(BUILDER_BUILD_DIR)/$(opencore-amr_dir) && PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site_gcc)  make && make install

clean-opencore-amr:
	cd  $(BUILDER_BUILD_DIR)/$(opencore-amr_dir) && make clean

veryclean-opencore-amr:
	-cd $(BUILDER_BUILD_DIR)/$(opencore-amr_dir) && make distclean
	-rm -rf $(BUILDER_BUILD_DIR)/$(opencore-amr_dir) 

clean-makefile-opencore-amr:
	cd $(BUILDER_BUILD_DIR)/$(opencore-amr_dir) && rm -f Makefile
