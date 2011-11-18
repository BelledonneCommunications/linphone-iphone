zrtpcpp_dir=externals/zrtpcpp
$(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)/Makefile: 
	mkdir -p $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)
	cd $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)/\
        && host_alias=$(host) . $(BUILDER_SRC_DIR)/build/$(config_site) \
        && cmake $(BUILDER_SRC_DIR)/$(zrtpcpp_dir) -Denable-ccrtp=false -DCMAKE_TOOLCHAIN_FILE=$(BUILDER_SRC_DIR)build/iphone-toolchain.cmake  \
	-LH -Wdev -DCMAKE_C_COMPILER=$$SDK_BIN_PATH/gcc -DCMAKE_CXX_COMPILER=$$SDK_BIN_PATH/g++ \
	-DCMAKE_SYSTEM_PROCESSOR=$$ARCH -DCMAKE_C_FLAGS="$$COMMON_FLAGS" -DCMAKE_CXX_FLAGS="$$COMMON_FLAGS" \
	-DCMAKE_INSTALL_PREFIX=$(prefix) -DCMAKE_FIND_ROOT_PATH="$(prefix) -DBUILD_STATIC=ON "
# Used toolchain: $(TC)


build-zrtpcpp: $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)/Makefile
	@echo "Build ZRTP - prefix $(prefix)"
	cd $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir) && make VERBOSE=1 && make install

clean-zrtpcpp:
	-cd $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir) && make clean

clean-makefile-zrtpcpp: clean-zrtpcpp
	-rm -f $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)/Makefile

veryclean-zrtpcpp:
	-rm -rf $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)


