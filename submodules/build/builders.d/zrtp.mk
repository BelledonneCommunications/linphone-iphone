############################################################################
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

bzrtp_project:=bzrtp
bzrtp_project_dir:=.
bzrtp_cmake_specific_options:=-DENABLE_STATIC=YES

bzrtp_src_dir=$(BUILDER_SRC_DIR)/${bzrtp_project_dir}/${bzrtp_project}
bzrtp_build_dir=$(BUILDER_BUILD_DIR)/${bzrtp_project_dir}/${bzrtp_project}

${bzrtp_build_dir}/Makefile:
	rm -rf ${bzrtp_build_dir}/CMakeCache.txt ${bzrtp_build_dir}/CMakeFiles/ && \
	mkdir -p ${bzrtp_build_dir} && \
	cd ${bzrtp_build_dir} && \
	cmake -DCMAKE_TOOLCHAIN_FILE=$(BUILDER_SRC_DIR)/build/toolchain.cmake -DIOS_ARCH=${ARCH} \
		-DCMAKE_INSTALL_PREFIX=$(prefix) -DCMAKE_PREFIX_PATH=$(prefix) -DCMAKE_MODULE_PATH=$(prefix)/share/cmake/Modules/ \
		${bzrtp_cmake_specific_options} ${bzrtp_src_dir}

build-${bzrtp_project}: ${bzrtp_build_dir}/Makefile
	cd ${bzrtp_build_dir} && \
	make && \
	make install

clean-${bzrtp_project}:
	cd $(bzrtp_build_dir) && \
	make clean

veryclean-${bzrtp_project}:
	if [ -d $(bzrtp_build_dir) ]; then grep -v $(prefix) $(bzrtp_build_dir)/install_manifest.txt | xargs rm; fi && \
	rm -rf $(bzrtp_build_dir)

clean-makefile-${bzrtp_project}: veryclean-${bzrtp_project}

