#I coudn't manage to crosscompile using only -D arguments to cmake
#Thus the use of a toolchain file.
TC = -DCMAKE_TOOLCHAIN_FILE=$(BUILDER_SRC_DIR)build/iphone-toolchain.cmake$(tc_proc)
$(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)/Makefile:
	cp $(BUILDER_SRC_DIR)/build/builders.d/zrtpcpp.CMakeLists.txt $(BUILDER_SRC_DIR)/$(zrtpcpp_dir)/CMakeLists.txt
	mkdir -p $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)
	cd $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)/\
        && host_alias=$(host) . $(BUILDER_SRC_DIR)/build/$(config_site) \
        && cmake $(BUILDER_SRC_DIR)/$(zrtpcpp_dir) -Denable-ccrtp=false $(TC) -LH -Wdev -DCMAKE_INSTALL_PREFIX=$(prefix) -DCMAKE_FIND_ROOT_PATH="$(prefix)"
# Used toolchain: $(TC)

ifeq ($(enable_zrtp),yes)

build-zrtpcpp: $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)/Makefile
	echo "Build ZRTP - prefix $(prefix)"
	cd $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir) && make && make install

else
build-zrtpcpp:
	echo "Build of zrtpcpp disabled"
endif

clean-zrtpcpp:
	-cd $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir) && make clean

clean-makefile-zrtpcpp: clean-zrtpcpp
	-rm -f $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)/Makefile
	-rm -f $(BUILDER_BUILD_DIR)/$(zrtpcpp_dir)/CMakeCache.txt




