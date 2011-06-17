############################################################################
# msamr.mk 
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
msamr_dir?=msamr
$(BUILDER_SRC_DIR)/$(msamr_dir)/configure:
	cd $(BUILDER_SRC_DIR)/$(msamr_dir) && ./autogen.sh

$(BUILDER_BUILD_DIR)/$(msamr_dir)/Makefile: $(BUILDER_SRC_DIR)/$(msamr_dir)/configure
	mkdir -p $(BUILDER_BUILD_DIR)/$(msamr_dir)
	cd $(BUILDER_BUILD_DIR)/$(msamr_dir)/ \
	&& PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
	$(BUILDER_SRC_DIR)/$(msamr_dir)/configure -prefix=$(prefix) --host=$(host) ${library_mode}  

build-msamr: build-opencore-amr $(BUILDER_BUILD_DIR)/$(msamr_dir)/Makefile
	cd $(BUILDER_BUILD_DIR)/$(msamr_dir) && PKG_CONFIG_PATH=$(prefix)/lib/pkgconfig CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site)  make && make install

clean-msamr: clean-opencore-amr
	cd  $(BUILDER_BUILD_DIR)/$(msamr_dir) && make clean

veryclean-msamr: veryclean-opencore-amr
	-cd $(BUILDER_BUILD_DIR)/$(msamr_dir) && make distclean 
	rm -f $(BUILDER_SRC_DIR)/$(msamr_dir)/configure

clean-makefile-msamr: clean-makefile-opencore-amr
	cd $(BUILDER_BUILD_DIR)/$(msamr_dir) && rm -f Makefile
