cunit_dir?=cunit
SRC_CUNIT=$(BUILDER_SRC_DIR)/${cunit_dir}
BUILD_CUNIT=$(BUILDER_BUILD_DIR)/${cunit_dir}

${BUILD_CUNIT}/CMakeCache.txt:
	mkdir -p ${BUILD_CUNIT} && \
	cd ${BUILD_CUNIT} && \
	cmake -DENABLE_STATIC=YES -DCMAKE_TOOLCHAIN_FILE=$(BUILDER_SRC_DIR)/build/toolchain.cmake \
			-DIOS_ARCH=${ARCH} -DCMAKE_INSTALL_PREFIX=$(prefix) ${SRC_CUNIT}

build-cunit: ${BUILD_CUNIT}/CMakeCache.txt
	cd ${BUILD_CUNIT} && \
	make && \
	make install

clean-cunit:
	cd $(BUILD_CUNIT)/$(cunit_dir) && \
	make clean

veryclean-cunit:
	rm -rf $(BUILD_CUNIT)/$(cunit_dir)

clean-makefile-cunit: veryclean-cunit

