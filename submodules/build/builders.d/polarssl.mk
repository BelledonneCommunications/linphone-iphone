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

polarssl_project:=polarssl
polarssl_project_dir:=externals
polarssl_cmake_specific_options:=-DUSE_STATIC_POLARSSL_LIBRARY=YES -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF -DENABLE_APIDOC=OFF

polarssl_src_dir=$(BUILDER_SRC_DIR)/${polarssl_project_dir}/${polarssl_project}
polarssl_build_dir=$(BUILDER_BUILD_DIR)/${polarssl_project_dir}/${polarssl_project}

${polarssl_build_dir}/Makefile:
	rm -rf ${polarssl_build_dir}/CMakeCache.txt ${polarssl_build_dir}/CMakeFiles/ && \
	mkdir -p ${polarssl_build_dir} && \
	cd ${polarssl_build_dir} && \
	cmake -DCMAKE_TOOLCHAIN_FILE=$(BUILDER_SRC_DIR)/build/toolchain.cmake -DIOS_ARCH=${ARCH} \
		-DCMAKE_INSTALL_PREFIX=$(prefix) -DCMAKE_PREFIX_PATH=$(prefix) -DCMAKE_MODULE_PATH=$(prefix)/share/cmake/Modules/ \
		${polarssl_cmake_specific_options} ${polarssl_src_dir}

build-${polarssl_project}: ${polarssl_build_dir}/Makefile
	cd ${polarssl_build_dir} && \
	make && \
	make install

clean-${polarssl_project}:
	cd $(polarssl_build_dir) && \
	make clean

veryclean-${polarssl_project}:
	if [ -d $(polarssl_build_dir) ]; then grep -v $(prefix) $(polarssl_build_dir)/install_manifest.txt | xargs rm; fi && \
	rm -rf $(polarssl_build_dir)

clean-makefile-${polarssl_project}: veryclean-${polarssl_project}

